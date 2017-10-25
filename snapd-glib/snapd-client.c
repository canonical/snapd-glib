/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "snapd-client.h"
#include "snapd-alias.h"
#include "snapd-app.h"
#include "snapd-assertion.h"
#include "snapd-channel.h"
#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-login.h"
#include "snapd-plug.h"
#include "snapd-screenshot.h"
#include "snapd-slot.h"
#include "snapd-task.h"

/**
 * SECTION:snapd-client
 * @short_description: Client connection to snapd
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdClient is the means of talking to snapd.
 *
 * To communicate with snapd create a client with snapd_client_new() then
 * connect with snapd_client_connect_sync().
 *
 * Some requests require authorization which can be set with
 * snapd_client_set_auth_data().
 */

/**
 * SnapdClient:
 *
 * #SnapdClient contains connection state with snapd.
 *
 * Since: 1.0
 */

/**
 * SnapdClientClass:
 *
 * Class structure for #SnapdClient.
 */

/**
 * SECTION:snapd-version
 * @short_description: Library version information
 * @include: snapd-glib/snapd-glib.h
 *
 * Programs can check if snapd-glib feature is enabled by checking for the
 * existance of a define called SNAPD_GLIB_VERSION_<version>, i.e.
 *
 * |[<!-- language="C" -->
 * #ifdef SNAPD_GLIB_VERSION_1_14
 * confinement = snapd_system_information_get_confinement (info);
 * #endif
 * ]|
 */

typedef struct
{
    /* Socket path to connect to */
    gchar *socket_path;

    /* Socket to communicate with snapd */
    GSocket *snapd_socket;

    /* User agent to send to snapd */
    gchar *user_agent;

    /* Authentication data to send with requests to snapd */
    SnapdAuthData *auth_data;

    /* Outstanding requests */
    GMutex requests_mutex;
    GList *requests;

    /* Whether to send the X-Allow-Interaction request header */
    gboolean allow_interaction;

    /* Data received from snapd */
    GMutex buffer_mutex;
    GByteArray *buffer;
    gsize n_read;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

/* snapd API documentation is at https://github.com/snapcore/snapd/wiki/REST-API */

/* Default socket to connect to */
#define SNAPD_SOCKET "/run/snapd.socket"

/* Number of milliseconds to poll for status in asynchronous operations */
#define ASYNC_POLL_TIME 100

typedef enum
{
    SNAPD_REQUEST_GET_CHANGE,
    SNAPD_REQUEST_ABORT_CHANGE,
    SNAPD_REQUEST_GET_SYSTEM_INFORMATION,
    SNAPD_REQUEST_LOGIN,
    SNAPD_REQUEST_GET_ICON,
    SNAPD_REQUEST_LIST,
    SNAPD_REQUEST_LIST_ONE,
    SNAPD_REQUEST_GET_ASSERTIONS,
    SNAPD_REQUEST_ADD_ASSERTIONS,
    SNAPD_REQUEST_GET_INTERFACES,
    SNAPD_REQUEST_CONNECT_INTERFACE,
    SNAPD_REQUEST_DISCONNECT_INTERFACE,
    SNAPD_REQUEST_FIND,
    SNAPD_REQUEST_FIND_REFRESHABLE,
    SNAPD_REQUEST_SIDELOAD_SNAP, // FIXME
    SNAPD_REQUEST_CHECK_BUY,
    SNAPD_REQUEST_BUY,
    SNAPD_REQUEST_INSTALL,
    SNAPD_REQUEST_INSTALL_STREAM,
    SNAPD_REQUEST_TRY,
    SNAPD_REQUEST_REFRESH,
    SNAPD_REQUEST_REFRESH_ALL,
    SNAPD_REQUEST_REMOVE,
    SNAPD_REQUEST_ENABLE,
    SNAPD_REQUEST_DISABLE,
    SNAPD_REQUEST_CREATE_USER,
    SNAPD_REQUEST_CREATE_USERS,
    SNAPD_REQUEST_GET_SECTIONS,
    SNAPD_REQUEST_GET_ALIASES,
    SNAPD_REQUEST_ENABLE_ALIASES,
    SNAPD_REQUEST_DISABLE_ALIASES,
    SNAPD_REQUEST_RESET_ALIASES,
    SNAPD_REQUEST_RUN_SNAPCTL
} RequestType;

G_DECLARE_FINAL_TYPE (SnapdRequest, snapd_request, SNAPD, REQUEST, GObject)

struct _SnapdRequest
{
    GObject parent_instance;

    GMainContext *context;

    SnapdClient *client;

    SoupMessage *message;

    GSource *read_source;

    RequestType request_type;

    SnapdRequest *parent;

    GCancellable *cancellable;
    gulong cancelled_id;
    GAsyncReadyCallback ready_callback;
    gpointer ready_callback_data;
    gboolean sent_cancel;

    SnapdProgressCallback progress_callback;
    gpointer progress_callback_data;

    gchar *change_id;
    GSource *poll_source;
    SnapdChange *change;

    SnapdInstallFlags install_flags;
    GInputStream *snap_stream;
    GByteArray *snap_contents;

    GError *error;
    gboolean result;
    SnapdSystemInformation *system_information;
    GPtrArray *snaps;
    gchar *suggested_currency;
    SnapdSnap *snap;
    SnapdIcon *icon;
    SnapdAuthData *received_auth_data;
    gchar **assertions;
    GPtrArray *plugs;
    GPtrArray *slots;
    SnapdUserInformation *user_information;
    GPtrArray *users_information;
    gchar **sections;
    GPtrArray *aliases;
    gchar *stdout_output;
    gchar *stderr_output;
    gboolean responded;
    JsonNode *async_data;
};

static SnapdRequest *
make_request (SnapdClient *client, RequestType request_type,
              const gchar *method, const gchar *path,
              gboolean authorize,
              SnapdProgressCallback progress_callback, gpointer progress_callback_data,
              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);

static gboolean
respond_cb (gpointer user_data)
{
    SnapdRequest *request = user_data;

    if (request->ready_callback != NULL)
        request->ready_callback (G_OBJECT (request->client), G_ASYNC_RESULT (request), request->ready_callback_data);

    return G_SOURCE_REMOVE;
}

static void
snapd_request_respond (SnapdRequest *request, GError *error)
{
    g_autoptr(GSource) source = NULL;

    if (request->responded)
        return;
    request->responded = TRUE;
    request->error = error;

    source = g_idle_source_new ();
    g_source_set_callback (source, respond_cb, g_object_ref (request), g_object_unref);
    g_source_attach (source, request->context);
}

static void
snapd_request_complete (SnapdRequest *request, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (request->client);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    if (request->read_source != NULL)
        g_source_destroy (request->read_source);
    g_clear_pointer (&request->read_source, g_source_unref);

    snapd_request_respond (request, error);
    priv->requests = g_list_remove (priv->requests, request);
    g_object_unref (request);
}

static void
complete_all_requests (SnapdClient *client, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GList *link;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (link = priv->requests; link; link = link->next) {
        SnapdRequest *request = link->data;
        snapd_request_respond (request, g_error_copy (error));
    }
    g_error_free (error);

    g_list_free_full (priv->requests, g_object_unref);
    priv->requests = NULL;
}

static gboolean
snapd_request_set_error (SnapdRequest *request, GError **error)
{
    if (request->error != NULL) {
        g_propagate_error (error, request->error);
        request->error = NULL;
        return TRUE;
    }

    /* If no error provided from snapd, use a generic cancelled error */
    if (g_cancellable_set_error_if_cancelled (request->cancellable, error))
        return TRUE;

    return FALSE;
}

static GObject *
snapd_request_get_source_object (GAsyncResult *result)
{
    return g_object_ref (SNAPD_REQUEST (result)->client);
}

static void
snapd_request_async_result_init (GAsyncResultIface *iface)
{
    iface->get_source_object = snapd_request_get_source_object;
}

G_DEFINE_TYPE_WITH_CODE (SnapdRequest, snapd_request, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_RESULT, snapd_request_async_result_init))

static void
snapd_request_finalize (GObject *object)
{
    SnapdRequest *request = SNAPD_REQUEST (object);

    g_clear_object (&request->message);
    if (request->read_source != NULL)
        g_source_destroy (request->read_source);
    g_clear_pointer (&request->read_source, g_source_unref);
    g_clear_object (&request->parent);
    g_cancellable_disconnect (request->cancellable, request->cancelled_id);
    g_clear_object (&request->cancellable);
    g_free (request->change_id);
    g_clear_pointer (&request->poll_source, g_source_destroy);
    g_clear_object (&request->change);
    g_clear_object (&request->snap_stream);
    g_clear_pointer (&request->snap_contents, g_byte_array_unref);
    g_clear_pointer (&request->error, g_error_free);
    g_clear_object (&request->system_information);
    g_clear_pointer (&request->snaps, g_ptr_array_unref);
    g_free (request->suggested_currency);
    g_clear_object (&request->snap);
    g_clear_object (&request->icon);
    g_clear_object (&request->received_auth_data);
    g_clear_pointer (&request->assertions, g_strfreev);
    g_clear_pointer (&request->plugs, g_ptr_array_unref);
    g_clear_pointer (&request->slots, g_ptr_array_unref);
    g_clear_object (&request->user_information);
    g_clear_pointer (&request->users_information, g_ptr_array_unref);
    g_clear_pointer (&request->sections, g_strfreev);
    g_clear_pointer (&request->aliases, g_ptr_array_unref);
    g_clear_pointer (&request->stdout_output, g_free);
    g_clear_pointer (&request->stderr_output, g_free);
    g_clear_pointer (&request->async_data, json_node_unref);
    g_clear_pointer (&request->context, g_main_context_unref);
}

static void
snapd_request_class_init (SnapdRequestClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = snapd_request_finalize;
}

static void
snapd_request_init (SnapdRequest *request)
{
}

static void
append_string (GByteArray *array, const gchar *value)
{
    g_byte_array_append (array, (const guint8 *) value, strlen (value));
}

static void
send_request (SnapdRequest *request)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (request->client);
    g_autoptr(GByteArray) request_data = NULL;
    SoupURI *uri;
    SoupMessageHeadersIter iter;
    const char *name, *value;
    g_autoptr(SoupBuffer) buffer = NULL;
    gssize n_written;
    g_autoptr(GError) local_error = NULL;

    // NOTE: Would love to use libsoup but it doesn't support unix sockets
    // https://bugzilla.gnome.org/show_bug.cgi?id=727563

    request_data = g_byte_array_new ();
    append_string (request_data, request->message->method);
    append_string (request_data, " ");
    uri = soup_message_get_uri (request->message);
    append_string (request_data, uri->path);
    if (uri->query != NULL) {
        append_string (request_data, "?");
        append_string (request_data, uri->query);
    }
    append_string (request_data, " HTTP/1.1\r\n");
    soup_message_headers_iter_init (&iter, request->message->request_headers);
    while (soup_message_headers_iter_next (&iter, &name, &value)) {
        append_string (request_data, name);
        append_string (request_data, ": ");
        append_string (request_data, value);
        append_string (request_data, "\r\n");
    }
    append_string (request_data, "\r\n");

    buffer = soup_message_body_flatten (request->message->request_body);
    g_byte_array_append (request_data, (const guint8 *) buffer->data, buffer->length);

    if (priv->snapd_socket == NULL) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_CONNECTION_FAILED,
                                     "Not connected to snapd");
        snapd_request_complete (request, error);
        return;
    }

    /* send HTTP request */
    // FIXME: Check for short writes
    n_written = g_socket_send (priv->snapd_socket, (const gchar *) request_data->data, request_data->len, request->cancellable, &local_error);
    if (n_written < 0) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_WRITE_FAILED,
                                     "Failed to write to snapd: %s",
                                     local_error->message);
        snapd_request_complete (request, error);
    }
}

/* Converts a language in POSIX format and to be RFC2616 compliant */
static gchar *
posix_lang_to_rfc2616 (const gchar *language)
{
    /* Don't include charset variants, etc */
    if (strchr (language, '.') || strchr (language, '@'))
        return NULL;

    /* Ignore "C" locale, which g_get_language_names() always includes as a fallback. */
    if (strcmp (language, "C") == 0)
        return NULL;

    return g_strdelimit (g_ascii_strdown (language, -1), "_", '-');
}

/* Converts @quality from 0-100 to 0.0-1.0 and appends to @str */
static gchar *
add_quality_value (const gchar *str, int quality)
{
    g_return_val_if_fail (str != NULL, NULL);

    if (quality >= 0 && quality < 100) {
        /* We don't use %.02g because of "." vs "," locale issues */
        if (quality % 10)
            return g_strdup_printf ("%s;q=0.%02d", str, quality);
        else
            return g_strdup_printf ("%s;q=0.%d", str, quality / 10);
    } else
        return g_strdup (str);
}

/* Returns a RFC2616 compliant languages list from system locales */
/* Copied from libsoup */
static gchar *
get_accept_languages (void)
{
    const char * const * lang_names;
    g_autoptr(GPtrArray) langs = NULL;
    int delta;
    guint i;

    lang_names = g_get_language_names ();
    g_return_val_if_fail (lang_names != NULL, NULL);

    /* Build the array of languages */
    langs = g_ptr_array_new_with_free_func (g_free);
    for (i = 0; lang_names[i] != NULL; i++) {
        gchar *lang = posix_lang_to_rfc2616 (lang_names[i]);
        if (lang != NULL)
            g_ptr_array_add (langs, lang);
    }

    /* Add quality values */
    if (langs->len < 10)
        delta = 10;
    else if (langs->len < 20)
        delta = 5;
    else
        delta = 1;
    for (i = 0; i < langs->len; i++) {
        gchar *lang = langs->pdata[i];
        langs->pdata[i] = add_quality_value (lang, 100 - i * delta);
        g_free (lang);
    }

    /* Fallback to "en" if list is empty */
    if (langs->len == 0)
        return g_strdup ("en");

    g_ptr_array_add (langs, NULL);
    return g_strjoinv (", ", (char **)langs->pdata);
}

static void
set_json_body (SnapdRequest *request, JsonBuilder *builder)
{
    g_autoptr(JsonNode) json_root = NULL;
    g_autoptr(JsonGenerator) json_generator = NULL;
    g_autofree gchar *data = NULL;
    gsize data_length;

    json_root = json_builder_get_root (builder);
    json_generator = json_generator_new ();
    json_generator_set_pretty (json_generator, TRUE);
    json_generator_set_root (json_generator, json_root);
    data = json_generator_to_data (json_generator, &data_length);

    soup_message_headers_set_content_type (request->message->request_headers, "application/json", NULL);
    soup_message_body_append_take (request->message->request_body, g_steal_pointer (&data), data_length);
    soup_message_headers_set_content_length (request->message->request_headers, request->message->request_body->length);
}

static void
set_multipart_body (SnapdRequest *request, SoupMultipart *multipart)
{
    soup_multipart_to_message (multipart, request->message->request_headers, request->message->request_body);
    soup_message_headers_set_content_length (request->message->request_headers, request->message->request_body->length);
}

static void
parse_get_system_information_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdSystemInformation) system_information = NULL;
    const gchar *confinement_string;
    SnapdSystemConfinement confinement = SNAPD_SYSTEM_CONFINEMENT_UNKNOWN;
    JsonObject *os_release, *locations;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    confinement_string = snapd_json_get_string (result, "confinement", "");
    if (strcmp (confinement_string, "strict") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_STRICT;
    else if (strcmp (confinement_string, "partial") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_PARTIAL;
    os_release = snapd_json_get_object (result, "os-release");
    locations  = snapd_json_get_object (result, "locations");
    system_information = g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                                       "binaries-directory", locations != NULL ? snapd_json_get_string (locations, "snap-bin-dir", NULL) : NULL,
                                       "confinement", confinement,
                                       "kernel-version", snapd_json_get_string (result, "kernel-version", NULL),
                                       "managed", snapd_json_get_bool (result, "managed", FALSE),
                                       "mount-directory", locations != NULL ? snapd_json_get_string (locations, "snap-mount-dir", NULL) : NULL,
                                       "on-classic", snapd_json_get_bool (result, "on-classic", FALSE),
                                       "os-id", os_release != NULL ? snapd_json_get_string (os_release, "id", NULL) : NULL,
                                       "os-version", os_release != NULL ? snapd_json_get_string (os_release, "version-id", NULL) : NULL,
                                       "series", snapd_json_get_string (result, "series", NULL),
                                       "store", snapd_json_get_string (result, "store", NULL),
                                       "version", snapd_json_get_string (result, "version", NULL),
                                       NULL);
    request->system_information = g_steal_pointer (&system_information);
    snapd_request_complete (request, NULL);
}

static void
parse_get_icon_response (SnapdRequest *request)
{
    const gchar *content_type;
    g_autoptr(SoupBuffer) buffer = NULL;
    g_autoptr(GBytes) data = NULL;
    g_autoptr(SnapdIcon) icon = NULL;

    content_type = soup_message_headers_get_content_type (request->message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = NULL;
        g_autoptr(JsonObject) result = NULL;
        GError *error = NULL;

        response = snapd_json_parse_response (request->message, &error);
        if (response == NULL) {
            snapd_request_complete (request, error);
            return;
        }
        result = snapd_json_get_sync_result_o (response, &error);
        if (result == NULL) {
            snapd_request_complete (request, error);
            return;
        }

        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unknown response");
        snapd_request_complete (request, error);
        return;
    }

    if (request->message->status_code != SOUP_STATUS_OK) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Got response %u retrieving icon", request->message->status_code);
        snapd_request_complete (request, error);
    }

    buffer = soup_message_body_flatten (request->message->response_body);
    data = soup_buffer_get_as_bytes (buffer);
    icon = g_object_new (SNAPD_TYPE_ICON,
                         "mime-type", content_type,
                         "data", data,
                         NULL);

    request->icon = g_steal_pointer (&icon);
    snapd_request_complete (request, NULL);
}

static void
parse_list_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    GPtrArray *snaps;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_a (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    snaps = snapd_json_parse_snap_array (result, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_list_one_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    snap = snapd_json_parse_snap (result, &error);
    if (snap == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->snap = g_steal_pointer (&snap);
    snapd_request_complete (request, NULL);
}

static GPtrArray *
get_connections (JsonObject *object, const gchar *name, GError **error)
{
    g_autoptr(JsonArray) array = NULL;
    GPtrArray *connections;
    guint i;

    connections = g_ptr_array_new_with_free_func (g_object_unref);
    array = snapd_json_get_array (object, "connections");
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        JsonObject *object;
        SnapdConnection *connection;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected connection type");
            return NULL;
        }

        object = json_node_get_object (node);
        connection = g_object_new (SNAPD_TYPE_CONNECTION,
                                   "name", snapd_json_get_string (object, name, NULL),
                                   "snap", snapd_json_get_string (object, "snap", NULL),
                                   NULL);
        g_ptr_array_add (connections, connection);
    }

    return connections;
}

static GVariant *node_to_variant (JsonNode *node);

static GVariant *
object_to_variant (JsonObject *object)
{
    JsonObjectIter iter;
    GType container_type = G_TYPE_INVALID;
    const gchar *name;
    JsonNode *node;
    GVariantBuilder builder;

    /* If has a consistent type, make an array of that type */
    json_object_iter_init (&iter, object);
    while (json_object_iter_next (&iter, &name, &node)) {
        GType type;
        type = json_node_get_value_type (node);
        if (container_type == G_TYPE_INVALID || type == container_type)
            container_type = type;
        else {
            container_type = G_TYPE_INVALID;
            break;
        }
    }

    switch (container_type)
    {
    case G_TYPE_BOOLEAN:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sb}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sb}", name, json_node_get_boolean (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_INT64:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sx}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sx}", name, json_node_get_int (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_DOUBLE:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sd}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sd}", name, json_node_get_double (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_STRING:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{ss}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{ss}", name, json_node_get_string (node));
        return g_variant_builder_end (&builder);
    default:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sv}", name, node_to_variant (node));
        return g_variant_builder_end (&builder);
    }
}

static GVariant *
array_to_variant (JsonArray *array)
{
    guint i, length;
    GType container_type = G_TYPE_INVALID;
    GVariantBuilder builder;

    /* If has a consistent type, make an array of that type */
    length = json_array_get_length (array);
    for (i = 0; i < length; i++) {
        GType type;
        type = json_node_get_value_type (json_array_get_element (array, i));
        if (container_type == G_TYPE_INVALID || type == container_type)
            container_type = type;
        else {
            container_type = G_TYPE_INVALID;
            break;
        }
    }

    switch (container_type)
    {
    case G_TYPE_BOOLEAN:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ab"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "b", json_array_get_boolean_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_INT64:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ax"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "x", json_array_get_int_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_DOUBLE:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ad"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "d", json_array_get_double_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_STRING:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "s", json_array_get_string_element (array, i));
        return g_variant_builder_end (&builder);
    default:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "v", node_to_variant (json_array_get_element (array, i)));
        return g_variant_builder_end (&builder);
    }
}

static GVariant *
node_to_variant (JsonNode *node)
{
    switch (json_node_get_node_type (node))
    {
    case JSON_NODE_OBJECT:
        return object_to_variant (json_node_get_object (node));
    case JSON_NODE_ARRAY:
        return array_to_variant (json_node_get_array (node));
    case JSON_NODE_VALUE:
        switch (json_node_get_value_type (node))
        {
        case G_TYPE_BOOLEAN:
            return g_variant_new_boolean (json_node_get_boolean (node));
        case G_TYPE_INT64:
            return g_variant_new_int64 (json_node_get_int (node));
        case G_TYPE_DOUBLE:
            return g_variant_new_double (json_node_get_double (node));
        case G_TYPE_STRING:
            return g_variant_new_string (json_node_get_string (node));
        default:
            /* Should never occur - as the above are all the valid types */
            return g_variant_new ("mv", NULL);
        }
    default:
        return g_variant_new ("mv", NULL);
    }
}

static GHashTable *
get_attributes (JsonObject *object, const gchar *name, GError **error)
{
    JsonObject *attrs;
    JsonObjectIter iter;
    GHashTable *attributes;
    const gchar *attribute_name;
    JsonNode *node;

    attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    attrs = snapd_json_get_object (object, "attrs");
    if (attrs == NULL)
        return attributes;

    json_object_iter_init (&iter, attrs);
    while (json_object_iter_next (&iter, &attribute_name, &node))
        g_hash_table_insert (attributes, g_strdup (attribute_name), node_to_variant (node));

    return attributes;
}

static void
parse_get_assertions_response (SnapdRequest *request)
{
    const gchar *content_type;
    g_autoptr(GPtrArray) assertions = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;
    gsize offset = 0;

    content_type = soup_message_headers_get_content_type (request->message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = NULL;
        g_autoptr(JsonObject) result = NULL;
        GError *error = NULL;

        response = snapd_json_parse_response (request->message, &error);
        if (response == NULL) {
            snapd_request_complete (request, error);
            return;
        }
        result = snapd_json_get_sync_result_o (response, &error);
        if (result == NULL) {
            snapd_request_complete (request, error);
            return;
        }

        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unknown response");
        snapd_request_complete (request, error);
        return;
    }

    if (request->message->status_code != SOUP_STATUS_OK) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Got response %u retrieving assertions", request->message->status_code);
        snapd_request_complete (request, error);
        return;
    }

    if (g_strcmp0 (content_type, "application/x.ubuntu.assertion") != 0) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Got unknown content type '%s' retrieving assertions", content_type);
        snapd_request_complete (request, error);
        return;
    }

    assertions = g_ptr_array_new ();
    buffer = soup_message_body_flatten (request->message->response_body);
    while (offset < buffer->length) {
        gsize assertion_start, assertion_end, body_length = 0;
        g_autofree gchar *body_length_header = NULL;
        SnapdAssertion *assertion;

        /* Headers terminated by double newline */
        assertion_start = offset;
        while (offset < buffer->length && !g_str_has_prefix (buffer->data + offset, "\n\n"))
            offset++;
        offset += 2;

        /* Make a temporary assertion object to decode body length header */
        assertion = snapd_assertion_new (g_strndup (buffer->data + assertion_start, offset - assertion_start));
        body_length_header = snapd_assertion_get_header (assertion, "body-length");
        g_object_unref (assertion);

        /* Skip over body */
        body_length = body_length_header != NULL ? strtoul (body_length_header, NULL, 10) : 0;
        if (body_length > 0)
            offset += body_length + 2;

        /* Find end of signature */
        while (offset < buffer->length && !g_str_has_prefix (buffer->data + offset, "\n\n"))
            offset++;
        assertion_end = offset;
        offset += 2;

        g_ptr_array_add (assertions, g_strndup (buffer->data + assertion_start, assertion_end - assertion_start));
    }
    g_ptr_array_add (assertions, NULL);

    request->assertions = g_steal_pointer (&assertions->pdata);
    snapd_request_complete (request, NULL);
}

static void
parse_add_assertions_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static void
parse_get_interfaces_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;
    guint i;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    plugs = snapd_json_get_array (result, "plugs");
    plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);
        JsonObject *object;
        g_autoptr(GPtrArray) connections = NULL;
        g_autoptr(GHashTable) attributes = NULL;
        g_autoptr(SnapdPlug) plug = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected plug type");
            snapd_request_complete (request, error);
            return;
        }
        object = json_node_get_object (node);

        connections = get_connections (object, "slot", &error);
        if (connections == NULL) {
            snapd_request_complete (request, error);
            return;
        }
        attributes = get_attributes (object, "slot", &error);

        plug = g_object_new (SNAPD_TYPE_PLUG,
                             "name", snapd_json_get_string (object, "plug", NULL),
                             "snap", snapd_json_get_string (object, "snap", NULL),
                             "interface", snapd_json_get_string (object, "interface", NULL),
                             "label", snapd_json_get_string (object, "label", NULL),
                             "connections", connections,
                             "attributes", attributes,
                             // FIXME: apps
                             NULL);
        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = snapd_json_get_array (result, "slots");
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);
        JsonObject *object;
        g_autoptr(GPtrArray) connections = NULL;
        g_autoptr(GHashTable) attributes = NULL;
        g_autoptr(SnapdSlot) slot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected slot type");
            snapd_request_complete (request, error);
            return;
        }
        object = json_node_get_object (node);

        connections = get_connections (object, "plug", &error);
        if (connections == NULL) {
            snapd_request_complete (request, error);
            return;
        }
        attributes = get_attributes (object, "plug", &error);

        slot = g_object_new (SNAPD_TYPE_SLOT,
                             "name", snapd_json_get_string (object, "slot", NULL),
                             "snap", snapd_json_get_string (object, "snap", NULL),
                             "interface", snapd_json_get_string (object, "interface", NULL),
                             "label", snapd_json_get_string (object, "label", NULL),
                             "connections", connections,
                             "attributes", attributes,
                             // FIXME: apps
                             NULL);
        g_ptr_array_add (slot_array, g_steal_pointer (&slot));
    }

    request->plugs = g_steal_pointer (&plug_array);
    request->slots = g_steal_pointer (&slot_array);
    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static gboolean
async_poll_cb (gpointer data)
{
    SnapdRequest *request = data;
    g_autofree gchar *path = NULL;
    SnapdRequest *change_request;

    path = g_strdup_printf ("/v2/changes/%s", request->change_id);
    change_request = make_request (request->client, SNAPD_REQUEST_GET_CHANGE, "GET", path, TRUE, NULL, NULL, NULL, NULL, NULL);
    change_request->parent = g_object_ref (request);
    send_request (change_request);

    request->poll_source = NULL;
    return G_SOURCE_REMOVE;
}

static gboolean
times_equal (GDateTime *time1, GDateTime *time2)
{
    if (time1 == NULL || time2 == NULL)
        return time1 == time2;
    return g_date_time_equal (time1, time2);
}

static gboolean
tasks_equal (SnapdTask *task1, SnapdTask *task2)
{
    return g_strcmp0 (snapd_task_get_id (task1), snapd_task_get_id (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_kind (task1), snapd_task_get_kind (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_summary (task1), snapd_task_get_summary (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_status (task1), snapd_task_get_status (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_progress_label (task1), snapd_task_get_progress_label (task2)) == 0 &&
           snapd_task_get_progress_done (task1) == snapd_task_get_progress_done (task2) &&
           snapd_task_get_progress_total (task1) == snapd_task_get_progress_total (task2) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2)) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2));
}

static gboolean
changes_equal (SnapdChange *change1, SnapdChange *change2)
{
    GPtrArray *tasks1, *tasks2;

    if (change1 == NULL || change2 == NULL)
        return change1 == change2;

    tasks1 = snapd_change_get_tasks (change1);
    tasks2 = snapd_change_get_tasks (change2);
    if (tasks1 == NULL || tasks2 == NULL) {
        if (tasks1 != tasks2)
            return FALSE;
    }
    else {
        int i;

        if (tasks1->len != tasks2->len)
            return FALSE;
        for (i = 0; i < tasks1->len; i++) {
            SnapdTask *t1 = tasks1->pdata[i], *t2 = tasks2->pdata[i];
            if (!tasks_equal (t1, t2))
                return FALSE;
        }
    }

    return g_strcmp0 (snapd_change_get_id (change1), snapd_change_get_id (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_kind (change1), snapd_change_get_kind (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_summary (change1), snapd_change_get_summary (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_status (change1), snapd_change_get_status (change2)) == 0 &&
           !!snapd_change_get_ready (change1) == !!snapd_change_get_ready (change2) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2)) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2));

    return TRUE;
}

static void
send_cancel (SnapdRequest *request)
{
    g_autofree gchar *path = NULL;
    SnapdRequest *change_request;
    g_autoptr(JsonBuilder) builder = NULL;

    if (request->sent_cancel)
        return;
    request->sent_cancel = TRUE;

    path = g_strdup_printf ("/v2/changes/%s", request->change_id);
    change_request = make_request (request->client, SNAPD_REQUEST_ABORT_CHANGE, "POST", path, TRUE, NULL, NULL, NULL, NULL, NULL);
    change_request->parent = g_object_ref (request);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "abort");
    json_builder_end_object (builder);
    set_json_body (change_request, builder);

    send_request (change_request);
}

static void
parse_async_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    gchar *change_id = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    change_id = snapd_json_get_async_result (response, &error);
    if (change_id == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->change_id = g_strdup (change_id);

    /* Immediately cancel if requested */
    if (g_cancellable_is_cancelled (request->cancellable)) {
        send_cancel (request);
        return;
    }

    /* Poll for updates */
    if (request->poll_source != NULL)
        g_source_destroy (request->poll_source);

    request->poll_source = g_timeout_source_new (ASYNC_POLL_TIME);
    g_source_set_callback (request->poll_source, async_poll_cb, request, NULL);
    g_source_attach (request->poll_source, request->context);
}

static void
parse_change_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    gboolean ready;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request->parent, error);
        snapd_request_complete (request, NULL);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request->parent, error);
        snapd_request_complete (request, NULL);
        return;
    }

    if (g_strcmp0 (request->parent->change_id, snapd_json_get_string (result, "id", NULL)) != 0) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected change ID returned");
        snapd_request_complete (request->parent, error);
        snapd_request_complete (request, NULL);
        return;
    }

    /* Update caller with progress */
    if (request->parent->progress_callback != NULL) {
        g_autoptr(JsonArray) array = NULL;
        guint i;
        g_autoptr(GPtrArray) tasks = NULL;
        g_autoptr(SnapdChange) change = NULL;
        g_autoptr(GDateTime) main_spawn_time = NULL;
        g_autoptr(GDateTime) main_ready_time = NULL;

        array = snapd_json_get_array (result, "tasks");
        tasks = g_ptr_array_new_with_free_func (g_object_unref);
        for (i = 0; i < json_array_get_length (array); i++) {
            JsonNode *node = json_array_get_element (array, i);
            JsonObject *object, *progress;
            g_autoptr(GDateTime) spawn_time = NULL;
            g_autoptr(GDateTime) ready_time = NULL;
            g_autoptr(SnapdTask) t = NULL;

            if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
                error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Unexpected task type");
                snapd_request_complete (request->parent, error);
                snapd_request_complete (request, NULL);
                return;
            }
            object = json_node_get_object (node);
            progress = snapd_json_get_object (object, "progress");
            spawn_time = snapd_json_get_date_time (object, "spawn-time");
            ready_time = snapd_json_get_date_time (object, "ready-time");

            t = g_object_new (SNAPD_TYPE_TASK,
                              "id", snapd_json_get_string (object, "id", NULL),
                              "kind", snapd_json_get_string (object, "kind", NULL),
                              "summary", snapd_json_get_string (object, "summary", NULL),
                              "status", snapd_json_get_string (object, "status", NULL),
                              "progress-label", progress != NULL ? snapd_json_get_string (progress, "label", NULL) : NULL,
                              "progress-done", progress != NULL ? snapd_json_get_int (progress, "done", 0) : 0,
                              "progress-total", progress != NULL ? snapd_json_get_int (progress, "total", 0) : 0,
                              "spawn-time", spawn_time,
                              "ready-time", ready_time,
                              NULL);
            g_ptr_array_add (tasks, g_steal_pointer (&t));
        }

        main_spawn_time = snapd_json_get_date_time (result, "spawn-time");
        main_ready_time = snapd_json_get_date_time (result, "ready-time");
        change = g_object_new (SNAPD_TYPE_CHANGE,
                               "id", snapd_json_get_string (result, "id", NULL),
                               "kind", snapd_json_get_string (result, "kind", NULL),
                               "summary", snapd_json_get_string (result, "summary", NULL),
                               "status", snapd_json_get_string (result, "status", NULL),
                               "tasks", tasks,
                               "ready", snapd_json_get_bool (result, "ready", FALSE),
                               "spawn-time", main_spawn_time,
                               "ready-time", main_ready_time,
                               NULL);

        if (!changes_equal (request->parent->change, change)) {
            g_clear_object (&request->parent->change);
            request->parent->change = g_steal_pointer (&change);
            // NOTE: tasks is passed for ABI compatibility - this field is
            // deprecated and can be accessed with snapd_change_get_tasks ()
            request->parent->progress_callback (request->parent->client, request->parent->change, tasks, request->parent->progress_callback_data);
        }
    }

    ready = snapd_json_get_bool (result, "ready", FALSE);
    if (ready) {
        GError *error = NULL;

        request->parent->result = TRUE;
        if (json_object_has_member (result, "data"))
            request->parent->async_data = json_node_ref (json_object_get_member (result, "data"));
        if (!g_cancellable_set_error_if_cancelled (request->parent->cancellable, &error) &&
            json_object_has_member (result, "err"))
            error = g_error_new_literal (SNAPD_ERROR,
                                         SNAPD_ERROR_FAILED,
                                         snapd_json_get_string (result, "err", "Unknown error"));
        snapd_request_complete (request->parent, error);
        snapd_request_complete (request, NULL);
        return;
    }

    /* Poll for updates */
    if (request->parent->poll_source != NULL)
        g_source_destroy (request->parent->poll_source);

    request->parent->poll_source = g_timeout_source_new (ASYNC_POLL_TIME);
    g_source_set_callback (request->parent->poll_source, async_poll_cb, request->parent, NULL);
    g_source_attach (request->parent->poll_source, request->parent->context);

    snapd_request_complete (request, NULL);
}

static void
parse_login_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(JsonArray) discharges = NULL;
    g_autoptr(GPtrArray) discharge_array = NULL;
    guint i;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    discharges = snapd_json_get_array (result, "discharges");
    discharge_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (discharges); i++) {
        JsonNode *node = json_array_get_element (discharges, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected discharge type");
            snapd_request_complete (request, error);
            return;
        }

        g_ptr_array_add (discharge_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (discharge_array, NULL);
    request->received_auth_data = snapd_auth_data_new (snapd_json_get_string (result, "macaroon", NULL), (gchar **) discharge_array->pdata);
    snapd_request_complete (request, NULL);
}

static void
parse_find_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_a (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    snaps = snapd_json_parse_snap_array (result, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->suggested_currency = g_strdup (snapd_json_get_string (response, "suggested-currency", NULL));

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_find_refreshable_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_a (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    snaps = snapd_json_parse_snap_array (result, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_check_buy_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static void
parse_buy_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static void
parse_create_user_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    user_information = snapd_json_parse_user_information (result, &error);
    if (user_information == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->user_information = g_steal_pointer (&user_information);
    snapd_request_complete (request, NULL);
}

static void
parse_create_users_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) users_information = NULL;
    guint i;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_a (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    users_information = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        SnapdUserInformation *user_information;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected user information type");
            snapd_request_complete (request, error);
            return;
        }

        user_information = snapd_json_parse_user_information (json_node_get_object (node), &error);
        if (user_information == NULL)
        {
            snapd_request_complete (request, error);
            return;
        }
        g_ptr_array_add (users_information, user_information);
    }

    request->users_information = g_steal_pointer (&users_information);
    snapd_request_complete (request, NULL);
}

static void
parse_get_sections_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) sections = NULL;
    guint i;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_a (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    sections = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected snap name type");
            snapd_request_complete (request, error);
            return;
        }

        g_ptr_array_add (sections, g_strdup (json_node_get_string (node)));
    }
    g_ptr_array_add (sections, NULL);

    request->sections = g_steal_pointer (&sections->pdata);

    snapd_request_complete (request, NULL);
}

static void
parse_get_aliases_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(GPtrArray) aliases = NULL;
    JsonObjectIter snap_iter;
    const gchar *snap;
    JsonNode *snap_node;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    aliases = g_ptr_array_new_with_free_func (g_object_unref);
    json_object_iter_init (&snap_iter, result);
    while (json_object_iter_next (&snap_iter, &snap, &snap_node)) {
        JsonObjectIter alias_iter;
        const gchar *name;
        JsonNode *alias_node;

        if (json_node_get_value_type (snap_node) != JSON_TYPE_OBJECT) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected alias type");
            snapd_request_complete (request, error);
        }

        json_object_iter_init (&alias_iter, json_node_get_object (snap_node));
        while (json_object_iter_next (&alias_iter, &name, &alias_node)) {
            JsonObject *o;
            SnapdAliasStatus status = SNAPD_ALIAS_STATUS_UNKNOWN;
            const gchar *status_string;
            g_autoptr(SnapdAlias) alias = NULL;

            if (json_node_get_value_type (alias_node) != JSON_TYPE_OBJECT) {
                error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Unexpected alias type");
                snapd_request_complete (request, error);
            }

            o = json_node_get_object (alias_node);
            status_string = snapd_json_get_string (o, "status", NULL);
            if (status_string == NULL)
                status = SNAPD_ALIAS_STATUS_DEFAULT;
            else if (strcmp (status_string, "enabled") == 0)
                status = SNAPD_ALIAS_STATUS_ENABLED;
            else if (strcmp (status_string, "disabled") == 0)
                status = SNAPD_ALIAS_STATUS_DISABLED;
            else if (strcmp (status_string, "auto") == 0)
                status = SNAPD_ALIAS_STATUS_AUTO;
            else
                status = SNAPD_ALIAS_STATUS_UNKNOWN;

            alias = g_object_new (SNAPD_TYPE_ALIAS,
                                  "snap", snap,
                                  "app", snapd_json_get_string (o, "app", NULL),
                                  "name", name,
                                  "status", status,
                                  NULL);
            g_ptr_array_add (aliases, g_steal_pointer (&alias));
        }
    }

    request->aliases = g_steal_pointer (&aliases);

    snapd_request_complete (request, NULL);
}

static void
parse_run_snapctl_response (SnapdRequest *request)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    GError *error = NULL;

    response = snapd_json_parse_response (request->message, &error);
    if (response == NULL) {
        snapd_request_complete (request, error);
        return;
    }
    result = snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->stdout_output = g_strdup (snapd_json_get_string (result, "stdout", NULL));
    request->stderr_output = g_strdup (snapd_json_get_string (result, "stderr", NULL));

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static SnapdRequest *
get_first_request (SnapdClient *client)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GList *link;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (link = priv->requests; link; link = link->next) {
        SnapdRequest *request = link->data;

        /* Return first non-async request */
        if (request->change_id == NULL)
            return request;
    }

    return NULL;
}

static void
parse_response (SnapdRequest *request)
{
    GError *error = NULL;

    switch (request->request_type)
    {
    case SNAPD_REQUEST_GET_CHANGE:
    case SNAPD_REQUEST_ABORT_CHANGE:
        parse_change_response (request);
        break;
    case SNAPD_REQUEST_GET_SYSTEM_INFORMATION:
        parse_get_system_information_response (request);
        break;
    case SNAPD_REQUEST_GET_ICON:
        parse_get_icon_response (request);
        break;
    case SNAPD_REQUEST_LIST:
        parse_list_response (request);
        break;
    case SNAPD_REQUEST_LIST_ONE:
        parse_list_one_response (request);
        break;
    case SNAPD_REQUEST_GET_ASSERTIONS:
        parse_get_assertions_response (request);
        break;
    case SNAPD_REQUEST_ADD_ASSERTIONS:
        parse_add_assertions_response (request);
        break;
    case SNAPD_REQUEST_GET_INTERFACES:
        parse_get_interfaces_response (request);
        break;
    case SNAPD_REQUEST_LOGIN:
        parse_login_response (request);
        break;
    case SNAPD_REQUEST_FIND:
        parse_find_response (request);
        break;
    case SNAPD_REQUEST_FIND_REFRESHABLE:
        parse_find_refreshable_response (request);
        break;
    case SNAPD_REQUEST_CHECK_BUY:
        parse_check_buy_response (request);
        break;
    case SNAPD_REQUEST_BUY:
        parse_buy_response (request);
        break;
    case SNAPD_REQUEST_CREATE_USER:
        parse_create_user_response (request);
        break;
    case SNAPD_REQUEST_CREATE_USERS:
        parse_create_users_response (request);
        break;
    case SNAPD_REQUEST_GET_SECTIONS:
        parse_get_sections_response (request);
        break;
    case SNAPD_REQUEST_GET_ALIASES:
        parse_get_aliases_response (request);
        break;
    case SNAPD_REQUEST_RUN_SNAPCTL:
        parse_run_snapctl_response (request);
        break;
    case SNAPD_REQUEST_CONNECT_INTERFACE:
    case SNAPD_REQUEST_DISCONNECT_INTERFACE:
    case SNAPD_REQUEST_INSTALL:
    case SNAPD_REQUEST_INSTALL_STREAM:
    case SNAPD_REQUEST_TRY:
    case SNAPD_REQUEST_REFRESH:
    case SNAPD_REQUEST_REFRESH_ALL:
    case SNAPD_REQUEST_REMOVE:
    case SNAPD_REQUEST_ENABLE:
    case SNAPD_REQUEST_DISABLE:
    case SNAPD_REQUEST_ENABLE_ALIASES:
    case SNAPD_REQUEST_DISABLE_ALIASES:
    case SNAPD_REQUEST_RESET_ALIASES:
        parse_async_response (request);
        break;
    default:
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_FAILED,
                             "Unknown request");
        snapd_request_complete (request, error);
        break;
    }
}

static gboolean
read_data (SnapdClient *client,
           gsize size,
           GCancellable *cancellable)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    gssize n_read;
    g_autoptr(GError) error_local = NULL;

    if (priv->n_read + size > priv->buffer->len)
        g_byte_array_set_size (priv->buffer, priv->n_read + size);
    n_read = g_socket_receive (priv->snapd_socket,
                               (gchar *) (priv->buffer->data + priv->n_read),
                               size,
                               cancellable,
                               &error_local);

    if (n_read == 0) {
        GError *error;

        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "snapd connection closed");
        complete_all_requests (client, error);
        return FALSE;
    }

    if (n_read < 0) {
        GError *error;

        if (g_error_matches (error_local, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
            return TRUE;

        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Failed to read from snapd: %s",
                             error_local->message);
        complete_all_requests (client, error);
        return FALSE;
    }

    priv->n_read += n_read;

    return TRUE;
}

/* Check if we have all HTTP chunks */
static gboolean
have_chunked_body (const gchar *body, gsize body_length)
{
    while (TRUE) {
        const gchar *chunk_start;
        gsize chunk_header_length, chunk_length;

        /* Read chunk header, stopping on zero length chunk */
        chunk_start = g_strstr_len (body, body_length, "\r\n");
        if (chunk_start == NULL)
            return FALSE;
        chunk_header_length = chunk_start - body + 2;
        chunk_length = strtoul (body, NULL, 16);
        if (chunk_length == 0)
            return TRUE;

        /* Check enough space for chunk body */
        if (chunk_header_length + chunk_length + strlen ("\r\n") > body_length)
            return FALSE;
        // FIXME: Validate that \r\n is on the end of a chunk?
        body += chunk_header_length + chunk_length;
        body_length -= chunk_header_length + chunk_length;
    }
}

/* If more than one HTTP chunk, re-order buffer to contain one chunk.
 * Assumes body is a valid chunked data block (as checked with have_chunked_body()) */
static void
compress_chunks (gchar *body, gsize body_length, gchar **combined_start, gsize *combined_length, gsize *total_length)
{
    gchar *chunk_start;

    /* Use first chunk as output */
    *combined_length = strtoul (body, NULL, 16);
    *combined_start = strstr (body, "\r\n") + 2;

    /* Copy any remaining chunks beside the first one */
    chunk_start = *combined_start + *combined_length + 2;
    while (TRUE) {
        gsize chunk_length;

        chunk_length = strtoul (chunk_start, NULL, 16);
        chunk_start = strstr (chunk_start, "\r\n") + 2;
        if (chunk_length == 0)
            break;

        /* Move this chunk on the end of the last one */
        memmove (*combined_start + *combined_length, chunk_start, chunk_length);
        *combined_length += chunk_length;

        chunk_start += chunk_length + 2;
    }

    *total_length = chunk_start - body;
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, SnapdRequest *r)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (r->client);
    gchar *body;
    gsize header_length;
    g_autoptr(SoupMessageHeaders) headers = NULL;
    gchar *combined_start;
    gsize content_length, combined_length;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->buffer_mutex);

    if (!read_data (r->client, 1024, NULL)) {
        r->read_source = NULL;
        return G_SOURCE_REMOVE;
    }

    while (TRUE) {
        SnapdRequest *request;
        GError *error;

        /* Look for header divider */
        body = g_strstr_len ((gchar *) priv->buffer->data, priv->n_read, "\r\n\r\n");
        if (body == NULL)
            return G_SOURCE_CONTINUE;
        body += 4;
        header_length = body - (gchar *) priv->buffer->data;

        /* Match this response to the next uncompleted request */
        request = get_first_request (r->client);
        if (request == NULL) {
            g_warning ("Ignoring unexpected response");
            return G_SOURCE_REMOVE;
        }

        /* Parse headers */
        if (!soup_headers_parse_response ((gchar *) priv->buffer->data, header_length, request->message->response_headers,
                                          NULL, &request->message->status_code, &request->message->reason_phrase)) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Failed to parse headers from snapd");
            complete_all_requests (request->client, error);
            request->read_source = NULL;
            return G_SOURCE_REMOVE;
        }

        /* Read content and process content */
        switch (soup_message_headers_get_encoding (request->message->response_headers)) {
        case SOUP_ENCODING_EOF:
            if (!g_socket_is_closed (priv->snapd_socket))
                return G_SOURCE_CONTINUE;

            content_length = priv->n_read - header_length;
            soup_message_body_append (request->message->response_body, SOUP_MEMORY_COPY, body, content_length);
            parse_response (request);
            break;

        case SOUP_ENCODING_CHUNKED:
            // FIXME: Find a way to abort on error
            if (!have_chunked_body (body, priv->n_read - header_length))
                return G_SOURCE_CONTINUE;

            compress_chunks (body, priv->n_read - header_length, &combined_start, &combined_length, &content_length);
            soup_message_body_append (request->message->response_body, SOUP_MEMORY_COPY, combined_start, combined_length);
            parse_response (request);
            break;

        case SOUP_ENCODING_CONTENT_LENGTH:
            content_length = soup_message_headers_get_content_length (request->message->response_headers);
            if (priv->n_read < header_length + content_length)
                return G_SOURCE_CONTINUE;

            soup_message_body_append (request->message->response_body, SOUP_MEMORY_COPY, body, content_length);
            parse_response (request);
            break;

        default:
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unable to determine header encoding");
            complete_all_requests (request->client, error);
            request->read_source = NULL;
            return G_SOURCE_REMOVE;
        }

        /* Move remaining data to the start of the buffer */
        g_byte_array_remove_range (priv->buffer, 0, header_length + content_length);
        priv->n_read -= header_length + content_length;
    }
}

/**
 * snapd_client_connect_async:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously connect to snapd.
 * See snapd_client_connect_sync () for more information.
 *
 * Since: 1.3
 */
void
snapd_client_connect_async (SnapdClient *client,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv;
    g_autoptr(GTask) task = NULL;
    g_autoptr(GSocketAddress) address = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);

    task = g_task_new (client, cancellable, callback, user_data);

    if (priv->snapd_socket == NULL) {
        priv->snapd_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                           G_SOCKET_TYPE_STREAM,
                                           G_SOCKET_PROTOCOL_DEFAULT,
                                           &error_local);
        if (priv->snapd_socket == NULL) {
            g_task_return_new_error (task,
                                     SNAPD_ERROR,
                                     SNAPD_ERROR_CONNECTION_FAILED,
                                     "Unable to open snapd socket: %s",
                                     error_local->message);
            return;
        }
        g_socket_set_blocking (priv->snapd_socket, FALSE);
        address = g_unix_socket_address_new (priv->socket_path);
        if (!g_socket_connect (priv->snapd_socket, address, cancellable, &error_local)) {
            g_clear_object (&priv->snapd_socket);
            g_task_return_new_error (task,
                                     SNAPD_ERROR,
                                     SNAPD_ERROR_CONNECTION_FAILED,
                                     "Unable to connect snapd socket: %s",
                                     error_local->message);
            g_clear_object (&priv->snapd_socket);
            return;
        }
    }

    g_task_return_boolean (task, TRUE);
}

/**
 * snapd_client_connect_finish:
 * @client: a #SnapdClient
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_connect_async().
 * See snapd_client_connect_sync() for more information.
 *
 * Returns: %TRUE if successfully connected to snapd.
 *
 * Since: 1.3
 */
gboolean
snapd_client_connect_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (G_IS_TASK (result), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_set_socket_path:
 * @client: a #SnapdClient
 * @socket_path: (allow-none): a socket path or %NULL to reset to the default.
 *
 * Set the Unix socket path to connect to snapd with.
 * Defaults to the system socket.
 *
 * Since: 1.24
 */
void
snapd_client_set_socket_path (SnapdClient *client, const gchar *socket_path)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_free (priv->socket_path);
    if (priv->socket_path != NULL)
        priv->socket_path = g_strdup (socket_path);
    else
        priv->socket_path = g_strdup (SNAPD_SOCKET);
}

/**
 * snapd_client_get_socket_path:
 * @client: a #SnapdClient
 *
 * Get the unix socket path to connect to snapd with.
 *
 * Returns: socket path.
 *
 * Since: 1.24
 */
const gchar *
snapd_client_get_socket_path (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);
    return priv->socket_path;
}

/**
 * snapd_client_set_user_agent:
 * @client: a #SnapdClient
 * @user_agent: (allow-none): a user agent or %NULL.
 *
 * Set the HTTP user-agent that is sent with each request to snapd.
 * Defaults to "snapd-glib/VERSION".
 *
 * Since: 1.16
 */
void
snapd_client_set_user_agent (SnapdClient *client, const gchar *user_agent)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_free (priv->user_agent);
    priv->user_agent = g_strdup (user_agent);
}

/**
 * snapd_client_get_user_agent:
 * @client: a #SnapdClient
 *
 * Get the HTTP user-agent that is sent with each request to snapd.
 *
 * Returns: user agent or %NULL if none set.
 *
 * Since: 1.16
 */
const gchar *
snapd_client_get_user_agent (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);
    return priv->user_agent;
}

/**
 * snapd_client_set_allow_interaction:
 * @client: a #SnapdClient
 * @allow_interaction: whether to allow interaction.
 *
 * Set whether snapd operations are allowed to interact with the user.
 * This affects operations that use polkit authorisation.
 * Defaults to TRUE.
 *
 * Since: 1.19
 */
void
snapd_client_set_allow_interaction (SnapdClient *client, gboolean allow_interaction)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    priv->allow_interaction = allow_interaction;
}

/**
 * snapd_client_get_allow_interaction:
 * @client: a #SnapdClient
 *
 * Get whether snapd operations are allowed to interact with the user.
 *
 * Returns: %TRUE if interaction is allowed.
 *
 * Since: 1.19
 */
gboolean
snapd_client_get_allow_interaction (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    priv = snapd_client_get_instance_private (client);
    return priv->allow_interaction;
}

static gboolean
request_is_async (SnapdRequest *request)
{
    switch (request->request_type)
    {
    case SNAPD_REQUEST_CONNECT_INTERFACE:
    case SNAPD_REQUEST_DISCONNECT_INTERFACE:
    case SNAPD_REQUEST_INSTALL:
    case SNAPD_REQUEST_INSTALL_STREAM:
    case SNAPD_REQUEST_TRY:
    case SNAPD_REQUEST_REFRESH:
    case SNAPD_REQUEST_REFRESH_ALL:
    case SNAPD_REQUEST_REMOVE:
    case SNAPD_REQUEST_ENABLE:
    case SNAPD_REQUEST_DISABLE:
    case SNAPD_REQUEST_ENABLE_ALIASES:
    case SNAPD_REQUEST_DISABLE_ALIASES:
    case SNAPD_REQUEST_RESET_ALIASES:
        return TRUE;
    default:
        return FALSE;
    }
}

static void
request_cancelled_cb (GCancellable *cancellable, SnapdRequest *request)
{
    /* Asynchronous requests require asking snapd to stop them */
    if (request_is_async (request))
    {
        /* Cancel if we have got a response from snapd */
        if (request->change_id != NULL)
            send_cancel (request);
    }
    else
    {
        GError *error = NULL;
        g_cancellable_set_error_if_cancelled (request->cancellable, &error);
        snapd_request_respond (request, error);
    }
}

static SnapdRequest *
make_request (SnapdClient *client, RequestType request_type,
              const gchar *method, const gchar *path,
              gboolean authorize,
              SnapdProgressCallback progress_callback, gpointer progress_callback_data,
              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    SnapdRequest *request;
    g_autofree gchar *uri = NULL;
    g_autofree gchar *accept_languages = NULL;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    request = g_object_new (snapd_request_get_type (), NULL);
    request->context = g_main_context_ref_thread_default ();
    request->client = client;
    uri = g_strdup_printf ("http://snapd%s", path);
    request->message = soup_message_new (method, uri);
    soup_message_headers_append (request->message->request_headers, "Host", "");
    soup_message_headers_append (request->message->request_headers, "Connection", "keep-alive");
    if (priv->user_agent != NULL)
        soup_message_headers_append (request->message->request_headers, "User-Agent", priv->user_agent);
    if (priv->allow_interaction)
        soup_message_headers_append (request->message->request_headers, "X-Allow-Interaction", "true");

    accept_languages = get_accept_languages ();
    soup_message_headers_append (request->message->request_headers, "Accept-Language", accept_languages);

    if (authorize && priv->auth_data != NULL) {
        g_autoptr(GString) authorization = NULL;
        gchar **discharges;
        gsize i;

        authorization = g_string_new ("");
        g_string_append_printf (authorization, "Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (priv->auth_data));
        discharges = snapd_auth_data_get_discharges (priv->auth_data);
        if (discharges != NULL)
            for (i = 0; discharges[i] != NULL; i++)
                g_string_append_printf (authorization, ",discharge=\"%s\"", discharges[i]);
        soup_message_headers_append (request->message->request_headers, "Authorization", authorization->str);
    }
    request->request_type = request_type;
    request->ready_callback = callback;
    request->ready_callback_data = user_data;
    request->progress_callback = progress_callback;
    request->progress_callback_data = progress_callback_data;
    priv->requests = g_list_append (priv->requests, request);
    if (cancellable != NULL) {
        request->cancellable = g_object_ref (cancellable);
        request->cancelled_id = g_cancellable_connect (cancellable, G_CALLBACK (request_cancelled_cb), request, NULL);
    }
    request->read_source = g_socket_create_source (priv->snapd_socket, G_IO_IN, NULL);
    g_source_set_name (request->read_source, "snapd-glib-read-source");
    g_source_set_callback (request->read_source, (GSourceFunc) read_cb, request, NULL);
    g_source_attach (request->read_source, request->context);

    return request;
}

/**
 * snapd_client_login_async:
 * @client: a #SnapdClient.
 * @username: usename to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get authorization to install/remove snaps.
 * See snapd_client_login_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_LOGIN, "POST", "/v2/login", FALSE, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, username);
    json_builder_set_member_name (builder, "password");
    json_builder_add_string_value (builder, password);
    if (otp != NULL) {
        json_builder_set_member_name (builder, "otp");
        json_builder_add_string_value (builder, otp);
    }
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_login_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_login_async().
 * See snapd_client_login_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 *
 * Since: 1.0
 */
SnapdAuthData *
snapd_client_login_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_LOGIN, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;

    return g_steal_pointer (&request->received_auth_data);
}

/**
 * snapd_client_set_auth_data:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): a #SnapdAuthData or %NULL.
 *
 * Set the authorization data to use for requests. Authorization data can be
 * obtained by:
 *
 * - Logging into snapd using snapd_login_sync() or snapd_client_login_sync()
 *   (requires root access)
 *
 * - Using an existing authorization with snapd_auth_data_new().
 *
 * Since: 1.0
 */
void
snapd_client_set_auth_data (SnapdClient *client, SnapdAuthData *auth_data)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_clear_object (&priv->auth_data);
    if (auth_data != NULL)
        priv->auth_data = g_object_ref (auth_data);
}

/**
 * snapd_client_get_auth_data:
 * @client: a #SnapdClient.
 *
 * Get the authorization data that is used for requests.
 *
 * Returns: (transfer none) (allow-none): a #SnapdAuthData or %NULL.
 *
 * Since: 1.0
 */
SnapdAuthData *
snapd_client_get_auth_data (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);

    return priv->auth_data;
}

/**
 * snapd_client_get_system_information_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Request system information asynchronously from snapd.
 * See snapd_client_get_system_information_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_system_information_async (SnapdClient *client,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_GET_SYSTEM_INFORMATION, "GET", "/v2/system-info", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_system_information_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_system_information_async().
 * See snapd_client_get_system_information_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdSystemInformation or %NULL on error.
 *
 * Since: 1.0
 */
SnapdSystemInformation *
snapd_client_get_system_information_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_SYSTEM_INFORMATION, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return request->system_information != NULL ? g_object_ref (request->system_information) : NULL;
}

/**
 * snapd_client_list_one_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information of a single installed snap.
 * See snapd_client_list_one_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_list_one_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_LIST_ONE, "GET", path, TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_list_one_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_list_one_async().
 * See snapd_client_list_one_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
SnapdSnap *
snapd_client_list_one_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_LIST_ONE, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return request->snap != NULL ? g_object_ref (request->snap) : NULL;
}

/**
 * snapd_client_get_icon_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the icon for an installed snap.
 * See snapd_client_get_icon_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_icon_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/icons/%s/icon", escaped);
    request = make_request (client, SNAPD_REQUEST_GET_ICON, "GET", path, TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_icon_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_icon_async().
 * See snapd_client_get_icon_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 *
 * Since: 1.0
 */
SnapdIcon *
snapd_client_get_icon_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_ICON, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return request->icon != NULL ? g_object_ref (request->icon) : NULL;
}

/**
 * snapd_client_list_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on all installed snaps.
 * See snapd_client_list_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_list_async (SnapdClient *client,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_LIST, "GET", "/v2/snaps", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_list_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_list_async().
 * See snapd_client_list_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_client_list_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_LIST, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return g_steal_pointer (&request->snaps);
}

/**
 * snapd_client_get_assertions_async:
 * @client: a #SnapdClient.
 * @type: assertion type to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get assertions.
 * See snapd_client_get_assertions_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_get_assertions_async (SnapdClient *client,
                                   const gchar *type,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    escaped = soup_uri_encode (type, NULL);
    path = g_strdup_printf ("/v2/assertions/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_GET_ASSERTIONS, "GET", path, TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_assertions_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_assertions_async().
 * See snapd_client_get_assertions_sync() for more information.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of assertions or %NULL on error.
 *
 * Since: 1.8
 */
gchar **
snapd_client_get_assertions_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_ASSERTIONS, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return g_steal_pointer (&request->assertions);
}

/**
 * snapd_client_add_assertions_async:
 * @client: a #SnapdClient.
 * @assertions: assertions to add.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously add an assertion.
 * See snapd_client_add_assertions_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_add_assertions_async (SnapdClient *client,
                                   gchar **assertions,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    int i;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (assertions != NULL);

    request = make_request (client, SNAPD_REQUEST_ADD_ASSERTIONS, "POST", "/v2/assertions", TRUE, NULL, NULL, cancellable, callback, user_data);

    soup_message_headers_set_content_type (request->message->request_headers, "application/x.ubuntu.assertion", NULL); //FIXME
    for (i = 0; assertions[i]; i++) {
        if (i != 0)
            soup_message_body_append (request->message->request_body, SOUP_MEMORY_TEMPORARY, "\n\n", 2);
        soup_message_body_append (request->message->request_body, SOUP_MEMORY_TEMPORARY, assertions[i], strlen (assertions[i]));
    }
    soup_message_headers_set_content_length (request->message->request_headers, request->message->request_body->length);
    send_request (request);
}

/**
 * snapd_client_add_assertions_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_add_assertions_async().
 * See snapd_client_add_assertions_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_add_assertions_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_ADD_ASSERTIONS, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_get_interfaces_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the installed snap interfaces.
 * See snapd_client_get_interfaces_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_interfaces_async (SnapdClient *client,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_GET_INTERFACES, "GET", "/v2/interfaces", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_interfaces_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_interfaces_async().
 * See snapd_client_get_interfaces_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_get_interfaces_finish (SnapdClient *client, GAsyncResult *result,
                                    GPtrArray **plugs, GPtrArray **slots,
                                    GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_INTERFACES, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    if (plugs)
       *plugs = request->plugs != NULL ? g_ptr_array_ref (request->plugs) : NULL;
    if (slots)
       *slots = request->slots != NULL ? g_ptr_array_ref (request->slots) : NULL;
    return request->result;
}

static void
send_interface_request (SnapdClient *client,
                        RequestType request_type,
                        const gchar *action,
                        const gchar *plug_snap, const gchar *plug_name,
                        const gchar *slot_snap, const gchar *slot_name,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    request = make_request (client, request_type, "POST", "/v2/interfaces", TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, action);
    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, plug_snap);
    json_builder_set_member_name (builder, "plug");
    json_builder_add_string_value (builder, plug_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "slots");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, slot_snap);
    json_builder_set_member_name (builder, "slot");
    json_builder_add_string_value (builder, slot_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_connect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously connect two interfaces together.
 * See snapd_client_connect_interface_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_connect_interface_async (SnapdClient *client,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));

    send_interface_request (client, SNAPD_REQUEST_CONNECT_INTERFACE,
                            "connect",
                            plug_snap, plug_name,
                            slot_snap, slot_name,
                            progress_callback, progress_callback_data,
                            cancellable, callback, user_data);
}

/**
 * snapd_client_connect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_connect_interface_async().
 * See snapd_client_connect_interface_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_connect_interface_finish (SnapdClient *client,
                                       GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_CONNECT_INTERFACE, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_disconnect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously disconnect two interfaces.
 * See snapd_client_disconnect_interface_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_disconnect_interface_async (SnapdClient *client,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));

    send_interface_request (client, SNAPD_REQUEST_DISCONNECT_INTERFACE,
                            "disconnect",
                            plug_snap, plug_name,
                            slot_snap, slot_name,
                            progress_callback, progress_callback_data,
                            cancellable, callback, user_data);
}

/**
 * snapd_client_disconnect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disconnect_interface_async().
 * See snapd_client_disconnect_interface_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disconnect_interface_finish (SnapdClient *client,
                                          GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_DISCONNECT_INTERFACE, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_find_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: query string to send.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_find_async (SnapdClient *client,
                         SnapdFindFlags flags, const gchar *query,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (query != NULL);
    snapd_client_find_section_async (client, flags, NULL, query, cancellable, callback, user_data);
}

/**
 * snapd_client_find_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_client_find_finish (SnapdClient *client, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    return snapd_client_find_section_finish (client, result, suggested_currency, error);
}

/**
 * snapd_client_find_section_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @section: (allow-none): store section to search in or %NULL to search in all sections.
 * @query: (allow-none): query string to send or %NULL to get all snaps from the given section.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_section_sync() for more information.
 *
 * Since: 1.7
 */
void
snapd_client_find_section_async (SnapdClient *client,
                                 SnapdFindFlags flags, const gchar *section, const gchar *query,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(GPtrArray) query_attributes = NULL;
    g_autoptr(GString) path = NULL;
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (section != NULL || query != NULL);

    query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (query != NULL) {
        g_autofree gchar *escaped = soup_uri_encode (query, NULL);
        if ((flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
            g_ptr_array_add (query_attributes, g_strdup_printf ("name=%s", escaped));
        else
            g_ptr_array_add (query_attributes, g_strdup_printf ("q=%s", escaped));
    }

    if ((flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=private"));
    else if ((flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=refresh"));

    if (section != NULL) {
        g_autofree gchar *escaped = soup_uri_encode (section, NULL);
        g_ptr_array_add (query_attributes, g_strdup_printf ("section=%s", escaped));
    }

    path = g_string_new ("/v2/find");
    if (query_attributes->len > 0) {
        guint i;

        g_string_append_c (path, '?');
        for (i = 0; i < query_attributes->len; i++) {
            if (i != 0)
                g_string_append_c (path, '&');
            g_string_append (path, (gchar *) query_attributes->pdata[i]);
        }
    }

    request = make_request (client, SNAPD_REQUEST_FIND, "GET", path->str, TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_find_section_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.7
 */
GPtrArray *
snapd_client_find_section_finish (SnapdClient *client, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_FIND, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;

    if (suggested_currency != NULL)
        *suggested_currency = g_steal_pointer (&request->suggested_currency);
    return g_steal_pointer (&request->snaps);
}

/**
 * snapd_client_find_refreshable_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in store that are newer revisions than locally installed versions.
 * See snapd_client_find_refreshable_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_find_refreshable_async (SnapdClient *client,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_FIND_REFRESHABLE, "GET", "/v2/find?select=refresh", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_find_refreshable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_refreshable_async().
 * See snapd_client_find_refreshable_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.5
 */
GPtrArray *
snapd_client_find_refreshable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_FIND_REFRESHABLE, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;

    return g_steal_pointer (&request->snaps);
}

/**
 * snapd_client_install_async:
 * @client: a #SnapdClient.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap from the store.
 * See snapd_client_install_sync() for more information.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use snapd_client_install2_async()
 */
void
snapd_client_install_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, name, channel, NULL, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_install_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install_async().
 * See snapd_client_install_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use snapd_client_install2_finish()
 */
gboolean
snapd_client_install_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    return snapd_client_install2_finish (client, result, error);
}

/**
 * snapd_client_install2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @revision: (allow-none): revision to install or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap from the store.
 * See snapd_client_install2_sync() for more information.
 *
 * Since: 1.12
 */
void
snapd_client_install2_async (SnapdClient *client,
                             SnapdInstallFlags flags,
                             const gchar *name, const gchar *channel, const gchar *revision,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_INSTALL, "POST", path, TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "install");
    if (channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, channel);
    }
    if (revision != NULL) {
        json_builder_set_member_name (builder, "revision");
        json_builder_add_string_value (builder, revision);
    }
    if ((flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0) {
        json_builder_set_member_name (builder, "classic");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0) {
        json_builder_set_member_name (builder, "dangerous");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0) {
        json_builder_set_member_name (builder, "devmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0) {
        json_builder_set_member_name (builder, "jailmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_install2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install2_async().
 * See snapd_client_install2_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.12
 */
gboolean
snapd_client_install2_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_INSTALL, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

static void
append_multipart_value (SoupMultipart *multipart, const gchar *name, const gchar *value)
{
    g_autoptr(SoupMessageHeaders) headers = NULL;
    g_autoptr(GHashTable) params = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;

    headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_MULTIPART);
    params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup (name));
    soup_message_headers_set_content_disposition (headers, "form-data", params);
    buffer = soup_buffer_new_take ((guchar *) g_strdup (value), strlen (value));
    soup_multipart_append_part (multipart, headers, buffer);
}

static void
send_install_stream_request (SnapdRequest *request)
{
    g_autoptr(GHashTable) params = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;
    g_autoptr(SoupMultipart) multipart = NULL;

    multipart = soup_multipart_new ("multipart/form-data");
    if ((request->install_flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0)
        append_multipart_value (multipart, "classic", "true");
    if ((request->install_flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0)
        append_multipart_value (multipart, "dangerous", "true");
    if ((request->install_flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0)
        append_multipart_value (multipart, "devmode", "true");
    if ((request->install_flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0)
        append_multipart_value (multipart, "jailmode", "true");

    params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup ("snap"));
    g_hash_table_insert (params, g_strdup ("filename"), g_strdup ("x"));
    soup_message_headers_set_content_disposition (request->message->request_headers, "form-data", params);
    soup_message_headers_set_content_type (request->message->request_headers, "application/vnd.snap", NULL);
    buffer = soup_buffer_new (SOUP_MEMORY_TEMPORARY, request->snap_contents->data, request->snap_contents->len);
    soup_multipart_append_part (multipart, request->message->request_headers, buffer);
    set_multipart_body (request, multipart);

    send_request (request);
}

static void
stream_read_cb (GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    GInputStream *stream = G_INPUT_STREAM (source_object);
    SnapdRequest *request = user_data;
    g_autoptr(GBytes) data = NULL;
    g_autoptr(GError) error = NULL;

    data = g_input_stream_read_bytes_finish (stream, result, &error);
    if (snapd_request_set_error (request, &error))
        return;

    if (g_bytes_get_size (data) == 0)
        send_install_stream_request (request);
    else {
        g_byte_array_append (request->snap_contents, g_bytes_get_data (data, NULL), g_bytes_get_size (data));
        g_input_stream_read_bytes_async (stream, 65535, G_PRIORITY_DEFAULT, request->cancellable, stream_read_cb, request);
    }
}

/**
 * snapd_client_install_stream_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @stream: a #GInputStream containing the snap file contents to install.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap.
 * See snapd_client_install_stream_sync() for more information.
 *
 * Since: 1.9
 */
void
snapd_client_install_stream_async (SnapdClient *client,
                                   SnapdInstallFlags flags,
                                   GInputStream *stream,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (G_IS_INPUT_STREAM (stream));

    request = make_request (client, SNAPD_REQUEST_INSTALL_STREAM, "POST", "/v2/snaps", TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    request->install_flags = flags;
    request->snap_stream = g_object_ref (stream);
    request->snap_contents = g_byte_array_new ();
    g_input_stream_read_bytes_async (stream, 65535, G_PRIORITY_DEFAULT, cancellable, stream_read_cb, request);
}

/**
 * snapd_client_install_stream_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install_stream_async().
 * See snapd_client_install_stream_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_install_stream_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_INSTALL_STREAM, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_try_async:
 * @client: a #SnapdClient.
 * @path: path to snap directory to try.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously try a snap.
 * See snapd_client_try_sync() for more information.
 *
 * Since: 1.9
 */
void
snapd_client_try_async (SnapdClient *client,
                        const gchar *path,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(SoupMultipart) multipart = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (path != NULL);

    request = make_request (client, SNAPD_REQUEST_TRY, "POST", "/v2/snaps", TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    multipart = soup_multipart_new ("multipart/form-data");
    append_multipart_value (multipart, "action", "try");
    append_multipart_value (multipart, "snap-path", path);
    set_multipart_body (request, multipart);

    send_request (request);
}

/**
 * snapd_client_try_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_try_async().
 * See snapd_client_try_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_try_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_TRY, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_refresh_async:
 * @client: a #SnapdClient.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ensure an installed snap is at the latest version.
 * See snapd_client_refresh_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_refresh_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_REFRESH, "POST", path, TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "refresh");
    if (channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, channel);
    }
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_refresh_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_refresh_async().
 * See snapd_client_refresh_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_refresh_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_REFRESH, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_refresh_all_async:
 * @client: a #SnapdClient.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ensure all snaps are updated to their latest versions.
 * See snapd_client_refresh_all_sync() for more information.
 *
 * Since: 1.5
 */
void
snapd_client_refresh_all_async (SnapdClient *client,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_REFRESH_ALL, "POST", "/v2/snaps", TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "refresh");
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_refresh_all_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_refresh_all_async().
 * See snapd_client_refresh_all_sync() for more information.
 *
 * Returns: (transfer full): a %NULL-terminated array of the snap names refreshed or %NULL on error.
 *
 * Since: 1.5
 */
gchar **
snapd_client_refresh_all_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;
    g_autoptr(GPtrArray) snap_names = NULL;
    JsonObject *o;
    g_autoptr(JsonArray) a = NULL;
    guint i;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_REFRESH_ALL, FALSE);

    if (snapd_request_set_error (request, error))
        return NULL;

    if (request->async_data == NULL || json_node_get_value_type (request->async_data) != JSON_TYPE_OBJECT) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected result type");
        return NULL;
    }
    o = json_node_get_object (request->async_data);
    if (o == NULL) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        return NULL;
    }
    a = snapd_json_get_array (o, "snap-names");
    snap_names = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (a); i++) {
        JsonNode *node = json_array_get_element (a, i);
        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected snap name type");
            return NULL;
        }

        g_ptr_array_add (snap_names, g_strdup (json_node_get_string (node)));
    }
    g_ptr_array_add (snap_names, NULL);

    return (gchar **) g_steal_pointer (&snap_names->pdata);
}

/**
 * snapd_client_remove_async:
 * @client: a #SnapdClient.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously uninstall a snap.
 * See snapd_client_remove_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_remove_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_REMOVE, "POST", path, TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "remove");
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_remove_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_remove_async().
 * See snapd_client_remove_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_remove_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_REMOVE, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_enable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously enable an installed snap.
 * See snapd_client_enable_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_enable_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_ENABLE, "POST", path, TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "enable");
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_enable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_enable_async().
 * See snapd_client_enable_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_enable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_ENABLE, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_disable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously disable an installed snap.
 * See snapd_client_disable_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_disable_async (SnapdClient *client,
                            const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autofree gchar *escaped = NULL, *path = NULL;
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    request = make_request (client, SNAPD_REQUEST_DISABLE, "POST", path, TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, "disable");
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_disable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disable_async().
 * See snapd_client_disable_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_DISABLE, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_check_buy_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously check if able to buy snaps.
 * See snapd_client_check_buy_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_check_buy_async (SnapdClient *client,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_CHECK_BUY, "GET", "/v2/buy/ready", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_check_buy_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_check_buy_async().
 * See snapd_client_check_buy_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_check_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_CHECK_BUY, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_buy_async:
 * @client: a #SnapdClient.
 * @id: id of snap to buy.
 * @amount: amount of currency to spend, e.g. 0.99.
 * @currency: the currency to buy with as an ISO 4217 currency code, e.g. "NZD".
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously buy a snap from the store.
 * See snapd_client_buy_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_buy_async (SnapdClient *client,
                        const gchar *id, gdouble amount, const gchar *currency,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (id != NULL);
    g_return_if_fail (currency != NULL);

    request = make_request (client, SNAPD_REQUEST_BUY, "POST", "/v2/buy", TRUE, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-id");
    json_builder_add_string_value (builder, id);
    json_builder_set_member_name (builder, "price");
    json_builder_add_double_value (builder, amount);
    json_builder_set_member_name (builder, "currency");
    json_builder_add_string_value (builder, currency);
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_buy_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_buy_async().
 * See snapd_client_buy_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_BUY, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_create_user_async:
 * @client: a #SnapdClient.
 * @email: the email of the user to create.
 * @flags: a set of #SnapdCreateUserFlags to control how the user account is created.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_create_user_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_create_user_async (SnapdClient *client,
                                const gchar *email, SnapdCreateUserFlags flags,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (email != NULL);

    request = make_request (client, SNAPD_REQUEST_CREATE_USER, "POST", "/v2/create-user", TRUE, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "email");
    json_builder_add_string_value (builder, email);
    if ((flags & SNAPD_CREATE_USER_FLAGS_SUDO) != 0) {
        json_builder_set_member_name (builder, "sudoer");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((flags & SNAPD_CREATE_USER_FLAGS_KNOWN) != 0) {
        json_builder_set_member_name (builder, "known");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_create_user_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_create_user_async().
 * See snapd_client_create_user_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
SnapdUserInformation *
snapd_client_create_user_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_CREATE_USER, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return request->user_information;
}

/**
 * snapd_client_create_users_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create local user accounts using the system-user assertions that are valid for this device.
 * See snapd_client_create_users_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_create_users_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_CREATE_USER, "POST", "/v2/create-user", TRUE, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "known");
    json_builder_add_boolean_value (builder, TRUE);
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_create_users_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_create_users_async().
 * See snapd_client_create_users_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
GPtrArray *
snapd_client_create_users_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_CREATE_USER, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return request->users_information;
}

/**
 * snapd_client_get_sections_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_get_sections_sync() for more information.
 *
 * Since: 1.7
 */
void
snapd_client_get_sections_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_GET_SECTIONS, "GET", "/v2/sections", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_sections_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_sections_async().
 * See snapd_client_get_sections_sync() for more information.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of section names or %NULL on error.
 *
 * Since: 1.7
 */
gchar **
snapd_client_get_sections_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_SECTIONS, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return g_steal_pointer (&request->sections);
}

/**
 * snapd_client_get_aliases_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the available aliases.
 * See snapd_client_get_aliases_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_get_aliases_async (SnapdClient *client,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = make_request (client, SNAPD_REQUEST_GET_ALIASES, "GET", "/v2/aliases", TRUE, NULL, NULL, cancellable, callback, user_data);
    send_request (request);
}

/**
 * snapd_client_get_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_aliases_async().
 * See snapd_client_get_aliases_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdAlias): an array of #SnapdAlias or %NULL on error.
 *
 * Since: 1.8
 */
GPtrArray *
snapd_client_get_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), NULL);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_GET_ALIASES, NULL);

    if (snapd_request_set_error (request, error))
        return NULL;
    return g_steal_pointer (&request->aliases);
}

static void
send_change_aliases_request (SnapdClient *client,
                             RequestType request_type,
                             const gchar *action, const gchar *snap, gchar **aliases,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    int i;

    request = make_request (client, request_type, "POST", "/v2/aliases", TRUE, progress_callback, progress_callback_data, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, action);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, snap);
    json_builder_set_member_name (builder, "aliases");
    json_builder_begin_array (builder);
    for (i = 0; aliases[i] != NULL; i++)
        json_builder_add_string_value (builder, aliases[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_enable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_enable_aliases_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_enable_aliases_async (SnapdClient *client,
                                   const gchar *snap, gchar **aliases,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (aliases != NULL);
    send_change_aliases_request (client, SNAPD_REQUEST_ENABLE_ALIASES, "alias", snap, aliases, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_enable_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_enable_aliases_async().
 * See snapd_client_enable_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_enable_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_ENABLE_ALIASES, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_disable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_disable_aliases_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_disable_aliases_async (SnapdClient *client,
                                    const gchar *snap, gchar **aliases,
                                    SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (aliases != NULL);
    send_change_aliases_request (client, SNAPD_REQUEST_DISABLE_ALIASES, "unalias", snap, aliases, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_disable_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disable_aliases_async().
 * See snapd_client_disable_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_disable_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_DISABLE_ALIASES, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_reset_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_reset_aliases_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_reset_aliases_async (SnapdClient *client,
                                  const gchar *snap, gchar **aliases,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (aliases != NULL);
    send_change_aliases_request (client, SNAPD_REQUEST_RESET_ALIASES, "reset", snap, aliases, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_reset_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_reset_aliases_async().
 * See snapd_client_reset_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_reset_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_RESET_ALIASES, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_run_snapctl_async:
 * @client: a #SnapdClient.
 * @context_id: context for this call.
 * @args: the arguments to pass to snapctl.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously run a snapctl command.
 * See snapd_client_run_snapctl_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_run_snapctl_async (SnapdClient *client,
                                const gchar *context_id, gchar **args,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    int i;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (context_id != NULL);
    g_return_if_fail (args != NULL);

    request = make_request (client, SNAPD_REQUEST_RUN_SNAPCTL, "POST", "/v2/snapctl", TRUE, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "context-id");
    json_builder_add_string_value (builder, context_id);
    json_builder_set_member_name (builder, "args");
    json_builder_begin_array (builder);
    for (i = 0; args[i] != NULL; i++)
        json_builder_add_string_value (builder, args[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    set_json_body (request, builder);

    send_request (request);
}

/**
 * snapd_client_run_snapctl_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @stdout_output: (out) (allow-none): the location to write the stdout from the command or %NULL.
 * @stderr_output: (out) (allow-none): the location to write the stderr from the command or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_run_snapctl_async().
 * See snapd_client_run_snapctl_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_run_snapctl_finish (SnapdClient *client, GAsyncResult *result,
                                 gchar **stdout_output, gchar **stderr_output,
                                 GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_RUN_SNAPCTL, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;

    if (stdout_output)
        *stdout_output = g_steal_pointer (&request->stdout_output);
    if (stderr_output)
        *stderr_output = g_steal_pointer (&request->stderr_output);

    return request->result;
}

/**
 * snapd_client_new:
 *
 * Create a new client to talk to snapd.
 *
 * Returns: a new #SnapdClient
 *
 * Since: 1.0
 **/
SnapdClient *
snapd_client_new (void)
{
    return g_object_new (SNAPD_TYPE_CLIENT, NULL);
}

/**
 * snapd_client_new_from_socket:
 * @socket: A #GSocket that is connected to snapd.
 *
 * Create a new client to talk on an existing socket.
 *
 * Returns: a new #SnapdClient
 *
 * Since: 1.5
 **/
SnapdClient *
snapd_client_new_from_socket (GSocket *socket)
{
    SnapdClient *client;
    SnapdClientPrivate *priv;

    client = snapd_client_new ();
    priv = snapd_client_get_instance_private (SNAPD_CLIENT (client));
    priv->snapd_socket = g_object_ref (socket);
    g_socket_set_blocking (priv->snapd_socket, FALSE);

    return client;
}

static void
snapd_client_finalize (GObject *object)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (SNAPD_CLIENT (object));

    g_mutex_clear (&priv->requests_mutex);
    g_mutex_clear (&priv->buffer_mutex);
    g_clear_pointer (&priv->socket_path, g_free);
    g_clear_pointer (&priv->user_agent, g_free);
    g_clear_object (&priv->auth_data);
    g_list_free_full (priv->requests, g_object_unref);
    priv->requests = NULL;
    if (priv->snapd_socket != NULL)
        g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);
    g_clear_pointer (&priv->buffer, g_byte_array_unref);
}

static void
snapd_client_class_init (SnapdClientClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = snapd_client_finalize;
}

static void
snapd_client_init (SnapdClient *client)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);

    priv->socket_path = g_strdup (SNAPD_SOCKET);
    priv->user_agent = g_strdup ("snapd-glib/" VERSION);
    priv->allow_interaction = TRUE;
    priv->buffer = g_byte_array_new ();
    g_mutex_init (&priv->requests_mutex);
    g_mutex_init (&priv->buffer_mutex);
}
