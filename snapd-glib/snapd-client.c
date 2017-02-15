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
#include "snapd-error.h"
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
 * #SnapdClient is an opaque data structure and can only be accessed
 * using the provided functions.
 */

/**
 * SnapdClientClass:
 *
 * Class structure for #SnapdClient.
 */

typedef struct
{
    GSocket *snapd_socket;
    SnapdAuthData *auth_data;
    GList *requests;
    GSource *read_source;
    GByteArray *buffer;
    gsize n_read;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

/* snapd API documentation is at https://github.com/snapcore/snapd/wiki/REST-API */

/* Default socket to connect to */
#define SNAPD_SOCKET "/run/snapd.socket"

/* Number of milliseconds to poll for status in asynchronous operations */
#define ASYNC_POLL_TIME 100
#define ASYNC_POLL_TIMEOUT 1000

// FIXME: Make multiple async requests work at the same time

typedef enum
{
    SNAPD_REQUEST_GET_SYSTEM_INFORMATION,
    SNAPD_REQUEST_LOGIN,
    SNAPD_REQUEST_GET_ICON,
    SNAPD_REQUEST_LIST,
    SNAPD_REQUEST_LIST_ONE,
    SNAPD_REQUEST_GET_INTERFACES,
    SNAPD_REQUEST_CONNECT_INTERFACE,
    SNAPD_REQUEST_DISCONNECT_INTERFACE,
    SNAPD_REQUEST_FIND,
    SNAPD_REQUEST_FIND_REFRESHABLE,  
    SNAPD_REQUEST_SIDELOAD_SNAP, // FIXME
    SNAPD_REQUEST_CHECK_BUY,
    SNAPD_REQUEST_BUY,  
    SNAPD_REQUEST_INSTALL,
    SNAPD_REQUEST_REFRESH,
    SNAPD_REQUEST_REFRESH_ALL,  
    SNAPD_REQUEST_REMOVE,
    SNAPD_REQUEST_ENABLE,
    SNAPD_REQUEST_DISABLE,
    SNAPD_REQUEST_CREATE_USER,
    SNAPD_REQUEST_CREATE_USERS,
    SNAPD_REQUEST_GET_SECTIONS,
    SNAPD_REQUEST_GET_ALIASES,
    SNAPD_REQUEST_CHANGE_ALIASES
} RequestType;

G_DECLARE_FINAL_TYPE (SnapdRequest, snapd_request, SNAPD, REQUEST, GObject)

struct _SnapdRequest
{
    GObject parent_instance;

    SnapdClient *client;
    SnapdAuthData *auth_data;

    RequestType request_type;

    GCancellable *cancellable;
    GAsyncReadyCallback ready_callback;
    gpointer ready_callback_data;

    SnapdProgressCallback progress_callback;
    gpointer progress_callback_data;

    gchar *change_id;
    guint poll_timer;
    guint timeout_timer;
    SnapdChange *change;

    gboolean completed;
    GError *error;
    gboolean result;
    SnapdSystemInformation *system_information;
    GPtrArray *snaps;
    gchar *suggested_currency;
    SnapdSnap *snap;
    SnapdIcon *icon;
    SnapdAuthData *received_auth_data;
    GPtrArray *plugs;
    GPtrArray *slots;
    SnapdUserInformation *user_information;
    GPtrArray *users_information;
    gchar **sections;
    GPtrArray *aliases;
    guint complete_handle;
    JsonNode *async_data;
};

static gboolean
complete_cb (gpointer user_data)
{
    g_autoptr(SnapdRequest) request = user_data;
    SnapdClientPrivate *priv = snapd_client_get_instance_private (request->client);

    priv->requests = g_list_remove (priv->requests, request);

    if (request->ready_callback != NULL)
        request->ready_callback (G_OBJECT (request->client), G_ASYNC_RESULT (request), request->ready_callback_data);

    return G_SOURCE_REMOVE;
}

static void
snapd_request_complete (SnapdRequest *request, GError *error)
{
    g_return_if_fail (!request->completed);

    request->completed = TRUE;
    if (error != NULL)
        g_propagate_error (&request->error, error);

    /* Do in main loop so user can't block our reading callback */
    request->complete_handle = g_idle_add (complete_cb, request);
}

static void
snapd_request_wait (SnapdRequest *request)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (request->client);
    GMainContext *context;

    if (priv->read_source == NULL)
        return;

    context = g_source_get_context (priv->read_source);
    while (!request->completed)
        g_main_context_iteration (context, TRUE);
}

static gboolean
snapd_request_set_error (SnapdRequest *request, GError **error)
{
    if (g_cancellable_set_error_if_cancelled (request->cancellable, error))
        return TRUE;

    if (request->error != NULL) {
        g_propagate_error (error, request->error);
        return TRUE;
    }

    return FALSE;
}

static GObject *
snapd_request_get_source_object (GAsyncResult *result)
{
    return G_OBJECT (SNAPD_REQUEST (result)->client);
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

    g_clear_object (&request->auth_data);
    g_clear_object (&request->cancellable);
    g_free (request->change_id);
    if (request->poll_timer != 0)
        g_source_remove (request->poll_timer);
    if (request->timeout_timer != 0)
        g_source_remove (request->timeout_timer);
    g_clear_object (&request->change);
    g_clear_object (&request->system_information);
    g_clear_pointer (&request->snaps, g_ptr_array_unref);
    g_free (request->suggested_currency);
    g_clear_object (&request->snap);
    g_clear_object (&request->icon);
    g_clear_object (&request->received_auth_data);
    g_clear_pointer (&request->plugs, g_ptr_array_unref);
    g_clear_pointer (&request->slots, g_ptr_array_unref);
    g_clear_object (&request->user_information);
    g_clear_pointer (&request->users_information, g_ptr_array_unref);
    g_clear_pointer (&request->sections, g_strfreev);
    g_clear_pointer (&request->aliases, g_ptr_array_unref);
    g_clear_pointer (&request->async_data, json_node_unref);
    if (request->complete_handle != 0)
        g_source_remove (request->complete_handle);
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

static gchar *
builder_to_string (JsonBuilder *builder)
{
    g_autoptr(JsonNode) json_root = NULL;
    g_autoptr(JsonGenerator) json_generator = NULL;

    json_root = json_builder_get_root (builder);
    json_generator = json_generator_new ();
    json_generator_set_pretty (json_generator, TRUE);
    json_generator_set_root (json_generator, json_root);
    return json_generator_to_data (json_generator, NULL);
}

static void
send_request (SnapdRequest *request, gboolean authorize, const gchar *method, const gchar *path, const gchar *content_type, const gchar *content)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (request->client);
    g_autoptr(SoupMessageHeaders) headers = NULL;
    g_autoptr(GString) request_data = NULL;
    SoupMessageHeadersIter iter;
    const char *name, *value;
    gssize n_written;
    g_autoptr(GError) local_error = NULL;

    // NOTE: Would love to use libsoup but it doesn't support unix sockets
    // https://bugzilla.gnome.org/show_bug.cgi?id=727563

    headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_REQUEST);
    soup_message_headers_append (headers, "Host", "");
    soup_message_headers_append (headers, "Connection", "keep-alive");
    if (content_type)
        soup_message_headers_set_content_type (headers, content_type, NULL);
    if (content)
        soup_message_headers_set_content_length (headers, strlen (content));
    if (authorize && request->auth_data != NULL) {
        g_autoptr(GString) authorization = NULL;
        gchar **discharges;
        gsize i;

        authorization = g_string_new ("");
        g_string_append_printf (authorization, "Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (request->auth_data));
        discharges = snapd_auth_data_get_discharges (request->auth_data);
        if (discharges != NULL)
            for (i = 0; discharges[i] != NULL; i++)
                g_string_append_printf (authorization, ",discharge=\"%s\"", discharges[i]);
        soup_message_headers_append (headers, "Authorization", authorization->str);
    }

    request_data = g_string_new ("");
    g_string_append_printf (request_data, "%s %s HTTP/1.1\r\n", method, path);
    soup_message_headers_iter_init (&iter, headers);
    while (soup_message_headers_iter_next (&iter, &name, &value))
        g_string_append_printf (request_data, "%s: %s\r\n", name, value);
    g_string_append (request_data, "\r\n");
    if (content)
        g_string_append (request_data, content);

    if (priv->snapd_socket == NULL) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_CONNECTION_FAILED,
                                     "Not connected to snapd");
        snapd_request_complete (request, error);
        return;
    }

    /* send HTTP request */
    // FIXME: Check for short writes
    n_written = g_socket_send (priv->snapd_socket, request_data->str, request_data->len, request->cancellable, &local_error);
    if (n_written < 0) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_WRITE_FAILED,
                                     "Failed to write to snapd: %s",
                                     local_error->message);
        snapd_request_complete (request, error);
    }
}

static gboolean
get_bool (JsonObject *object, const gchar *name, gboolean default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_BOOLEAN)
        return json_node_get_boolean (node);
    else
        return default_value;
}

static gint64
get_int (JsonObject *object, const gchar *name, gint64 default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_INT64)
        return json_node_get_int (node);
    else
        return default_value;
}

static const gchar *
get_string (JsonObject *object, const gchar *name, const gchar *default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_STRING)
        return json_node_get_string (node);
    else
        return default_value;
}

static JsonArray *
get_array (JsonObject *object, const gchar *name)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == JSON_TYPE_ARRAY)
        return json_array_ref (json_node_get_array (node));
    else
        return json_array_new ();
}

static JsonObject *
get_object (JsonObject *object, const gchar *name)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == JSON_TYPE_OBJECT)
        return json_node_get_object (node);
    else
        return NULL;
}

static gboolean
parse_date (const gchar *date_string, gint *year, gint *month, gint *day)
{
    /* Example: 2016-05-17 */
    if (strchr (date_string, '-') != NULL) {
        g_auto(GStrv) tokens = NULL;

        tokens = g_strsplit (date_string, "-", -1);
        if (g_strv_length (tokens) != 3)
            return FALSE;

        *year = atoi (tokens[0]);
        *month = atoi (tokens[1]);
        *day = atoi (tokens[2]);

        return TRUE;
    }
    /* Example: 20160517 */
    else if (strlen (date_string) == 8) {
        // FIXME: Implement
        return FALSE;
    }
    else
        return FALSE;
}

static gboolean
parse_time (const gchar *time_string, gint *hour, gint *minute, gdouble *seconds)
{
    /* Example: 09:36:53.682 or 09:36:53 or 09:36 */
    if (strchr (time_string, ':') != NULL) {
        g_auto(GStrv) tokens = NULL;

        tokens = g_strsplit (time_string, ":", 3);
        *hour = atoi (tokens[0]);
        if (tokens[1] == NULL)
            return FALSE;
        *minute = atoi (tokens[1]);
        if (tokens[2] != NULL)
            *seconds = g_ascii_strtod (tokens[2], NULL);
        else
            *seconds = 0.0;

        return TRUE;
    }
    /* Example: 093653.682 or 093653 or 0936 */
    else {
        // FIXME: Implement
        return FALSE;
    }
}

static gboolean
is_timezone_prefix (gchar c)
{
    return c == '+' || c == '-' || c == 'Z';
}

static GDateTime *
get_date_time (JsonObject *object, const gchar *name)
{
    const gchar *value;
    g_auto(GStrv) tokens = NULL;
    g_autoptr(GTimeZone) timezone = NULL;
    gint year = 0, month = 0, day = 0, hour = 0, minute = 0;
    gdouble seconds = 0.0;

    value = get_string (object, name, NULL);
    if (value == NULL)
        return NULL;

    /* Example: 2016-05-17T09:36:53+12:00 */
    tokens = g_strsplit (value, "T", 2);
    if (!parse_date (tokens[0], &year, &month, &day))
        return NULL;
    if (tokens[1] != NULL) {
        gchar *timezone_start;

        /* Timezone is either Z (UTC) +hh:mm or -hh:mm */
        timezone_start = tokens[1];
        while (*timezone_start != '\0' && !is_timezone_prefix (*timezone_start))
            timezone_start++;
        if (*timezone_start != '\0')
            timezone = g_time_zone_new (timezone_start);

        /* Strip off timezone */
        *timezone_start = '\0';

        if (!parse_time (tokens[1], &hour, &minute, &seconds))
            return NULL;
    }

    if (timezone == NULL)
        timezone = g_time_zone_new_local ();

    return g_date_time_new (timezone, year, month, day, hour, minute, seconds);
}

static gboolean
parse_result (const gchar *content_type, const gchar *content, gsize content_length, JsonObject **response, gchar **change_id, GError **error)
{
    g_autoptr(JsonParser) parser = NULL;
    g_autoptr(GError) error_local = NULL;
    JsonObject *root;
    const gchar *type;

    if (content_type == NULL) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_BAD_RESPONSE,
                             "snapd returned no content type");
        return FALSE;
    }
    if (g_strcmp0 (content_type, "application/json") != 0) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_BAD_RESPONSE,
                     "snapd returned unexpected content type %s", content_type);
        return FALSE;
    }

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, content, content_length, &error_local)) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_BAD_RESPONSE,
                     "Unable to parse snapd response: %s",
                     error_local->message);
        return FALSE;
    }

    if (!JSON_NODE_HOLDS_OBJECT (json_parser_get_root (parser))) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_BAD_RESPONSE,
                             "snapd response does is not a valid JSON object");
        return FALSE;
    }
    root = json_node_get_object (json_parser_get_root (parser));

    type = get_string (root, "type", NULL);
    if (g_strcmp0 (type, "error") == 0) {
        const gchar *kind, *message;
        gint64 status_code;
        JsonObject *result;

        result = get_object (root, "result");
        status_code = get_int (root, "status-code", 0);
        kind = result != NULL ? get_string (result, "kind", NULL) : NULL;
        message = result != NULL ? get_string (result, "message", NULL) : NULL;

        if (g_strcmp0 (kind, "login-required") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_AUTH_DATA_REQUIRED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "invalid-auth-data") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_AUTH_DATA_INVALID,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "two-factor-required") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_TWO_FACTOR_REQUIRED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "two-factor-failed") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_TWO_FACTOR_INVALID,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "terms-not-accepted") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_TERMS_NOT_ACCEPTED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "no-payment-methods") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_PAYMENT_NOT_SETUP,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "payment-declined") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_PAYMENT_DECLINED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "snap-already-installed") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_ALREADY_INSTALLED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "snap-not-installed") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_NOT_INSTALLED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "snap-no-update-available") == 0) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_NO_UPDATE_AVAILABLE,
                                 message);
            return FALSE;
        }
        else if (status_code == SOUP_STATUS_BAD_REQUEST) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_BAD_REQUEST,
                                 message);
            return FALSE;
        }
        else {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_FAILED,
                                 message);
            return FALSE;
        }
    }
    else if (g_strcmp0 (type, "async") == 0) {
        if (change_id)
            *change_id = g_strdup (get_string (root, "change", NULL));
    }
    else if (g_strcmp0 (type, "sync") == 0) {
    }

    if (response != NULL)
        *response = json_object_ref (root);

    return TRUE;
}

static void
parse_get_system_information_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(SnapdSystemInformation) system_information = NULL;
    JsonObject *os_release;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        snapd_request_complete (request, error);
        return;
    }

    os_release = get_object (result, "os-release");
    system_information = g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                                       "managed", get_bool (result, "managed", FALSE),
                                       "on-classic", get_bool (result, "on-classic", FALSE),
                                       "os-id", os_release != NULL ? get_string (os_release, "id", NULL) : NULL,
                                       "os-version", os_release != NULL ? get_string (os_release, "version-id", NULL) : NULL,
                                       "series", get_string (result, "series", NULL),
                                       "store", get_string (result, "store", NULL),
                                       "version", get_string (result, "version", NULL),
                                       NULL);
    request->system_information = g_steal_pointer (&system_information);
    snapd_request_complete (request, NULL);
}

static SnapdSnap *
parse_snap (JsonObject *object, GError **error)
{
    const gchar *confinement_string;
    SnapdConfinement confinement = SNAPD_CONFINEMENT_UNKNOWN;
    const gchar *snap_type_string;
    SnapdConfinement snap_type = SNAPD_SNAP_TYPE_UNKNOWN;
    const gchar *snap_status_string;
    SnapdSnapStatus snap_status = SNAPD_SNAP_STATUS_UNKNOWN;
    g_autoptr(JsonArray) apps = NULL;
    g_autoptr(GDateTime) install_date = NULL;
    JsonObject *prices;
    g_autoptr(GPtrArray) apps_array = NULL;
    g_autoptr(GPtrArray) prices_array = NULL;
    g_autoptr(JsonArray) screenshots = NULL;
    g_autoptr(GPtrArray) screenshots_array = NULL;
    guint i;

    confinement_string = get_string (object, "confinement", "");
    if (strcmp (confinement_string, "strict") == 0)
        confinement = SNAPD_CONFINEMENT_STRICT;
    else if (strcmp (confinement_string, "devmode") == 0)
        confinement = SNAPD_CONFINEMENT_DEVMODE;

    snap_type_string = get_string (object, "type", "");
    if (strcmp (snap_type_string, "app") == 0)
        snap_type = SNAPD_SNAP_TYPE_APP;
    else if (strcmp (snap_type_string, "kernel") == 0)
        snap_type = SNAPD_SNAP_TYPE_KERNEL;
    else if (strcmp (snap_type_string, "gadget") == 0)
        snap_type = SNAPD_SNAP_TYPE_GADGET;
    else if (strcmp (snap_type_string, "os") == 0)
        snap_type = SNAPD_SNAP_TYPE_OS;

    snap_status_string = get_string (object, "status", "");
    if (strcmp (snap_status_string, "available") == 0)
        snap_status = SNAPD_SNAP_STATUS_AVAILABLE;
    else if (strcmp (snap_status_string, "priced") == 0)
        snap_status = SNAPD_SNAP_STATUS_PRICED;
    else if (strcmp (snap_status_string, "installed") == 0)
        snap_status = SNAPD_SNAP_STATUS_INSTALLED;
    else if (strcmp (snap_status_string, "active") == 0)
        snap_status = SNAPD_SNAP_STATUS_ACTIVE;

    apps = get_array (object, "apps");
    apps_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (apps); i++) {
        JsonNode *node = json_array_get_element (apps, i);
        JsonObject *a;
        g_autoptr(JsonArray) aliases = NULL;
        g_autoptr(GPtrArray) aliases_array = NULL;
        g_autoptr(SnapdApp) app = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected app type");
            return NULL;
        }

        a = json_node_get_object (node);

        aliases = get_array (a, "aliases");
        aliases_array = g_ptr_array_new ();
        for (i = 0; i < json_array_get_length (aliases); i++) {
            JsonNode *node = json_array_get_element (aliases, i);

            if (json_node_get_value_type (node) != G_TYPE_STRING) {
                g_set_error_literal (error,
                                     SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Unexpected alias type");
                return NULL;
            }

            g_ptr_array_add (aliases_array, (gpointer) json_node_get_string (node));
        }
        g_ptr_array_add (aliases_array, NULL);

        app = g_object_new (SNAPD_TYPE_APP,
                            "name", get_string (a, "name", NULL),
                            "aliases", (gchar **) aliases_array->pdata,
                            NULL);
        g_ptr_array_add (apps_array, g_steal_pointer (&app));
    }

    install_date = get_date_time (object, "install-date");

    prices = get_object (object, "prices");
    prices_array = g_ptr_array_new_with_free_func (g_object_unref);
    if (prices != NULL) {
        JsonObjectIter iter;
        const gchar *currency;
        JsonNode *amount_node;

        json_object_iter_init (&iter, prices);
        while (json_object_iter_next (&iter, &currency, &amount_node)) {
            g_autoptr(SnapdPrice) price = NULL;

            if (json_node_get_value_type (amount_node) != G_TYPE_DOUBLE) {
                g_set_error_literal (error,
                                     SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Unexpected price type");
                return NULL;
            }

            price = g_object_new (SNAPD_TYPE_PRICE,
                                  "amount", json_node_get_double (amount_node),
                                  "currency", currency,
                                  NULL);
            g_ptr_array_add (prices_array, g_steal_pointer (&price));
        }
    }

    screenshots = get_array (object, "screenshots");
    screenshots_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (screenshots); i++) {
        JsonNode *node = json_array_get_element (screenshots, i);
        JsonObject *s;
        g_autoptr(SnapdScreenshot) screenshot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected screenshot type");
            return NULL;
        }

        s = json_node_get_object (node);
        screenshot = g_object_new (SNAPD_TYPE_SCREENSHOT,
                                   "url", get_string (s, "url", NULL),
                                   "width", (guint) get_int (s, "width", 0),
                                   "height", (guint) get_int (s, "height", 0),                              
                                   NULL);
        g_ptr_array_add (screenshots_array, g_steal_pointer (&screenshot));
    }

    return g_object_new (SNAPD_TYPE_SNAP,
                         "apps", apps_array,
                         "channel", get_string (object, "channel", NULL),
                         "confinement", confinement,
                         "description", get_string (object, "description", NULL),
                         "developer", get_string (object, "developer", NULL),
                         "devmode", get_bool (object, "devmode", FALSE),
                         "download-size", get_int (object, "download-size", 0),
                         "icon", get_string (object, "icon", NULL),
                         "id", get_string (object, "id", NULL),
                         "install-date", install_date,
                         "installed-size", get_int (object, "installed-size", 0),
                         "name", get_string (object, "name", NULL),
                         "prices", prices_array,
                         "private", get_bool (object, "private", FALSE),
                         "revision", get_string (object, "revision", NULL),
                         "screenshots", screenshots_array,
                         "snap-type", snap_type,
                         "status", snap_status,
                         "summary", get_string (object, "summary", NULL),
                         "tracking-channel", get_string (object, "tracking-channel", NULL),
                         "trymode", get_bool (object, "trymode", FALSE),
                         "version", get_string (object, "version", NULL),
                         NULL);
}

static GPtrArray *
parse_snap_array (JsonArray *array, GError **error)
{
    g_autoptr(GPtrArray) snaps = NULL;
    guint i;

    snaps = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        SnapdSnap *snap;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected snap type");
            return NULL;
        }

        snap = parse_snap (json_node_get_object (node), error);
        if (snap == NULL)
            return NULL;
        g_ptr_array_add (snaps, snap);
    }

    return g_steal_pointer (&snaps);
}

static void
parse_get_icon_response (SnapdRequest *request, guint code, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    const gchar *content_type;
    g_autoptr(SnapdIcon) icon = NULL;
    g_autoptr(GBytes) data = NULL;

    content_type = soup_message_headers_get_content_type (headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        GError *error = NULL;

        if (!parse_result (content_type, content, content_length, NULL, NULL, &error)) {
            snapd_request_complete (request, error);
            return;
        }
    }

    if (code != SOUP_STATUS_OK) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Got response %u retrieving icon", code);
        snapd_request_complete (request, error);
    }

    data = g_bytes_new (content, content_length);
    icon = g_object_new (SNAPD_TYPE_ICON,
                         "mime-type", content_type,
                         "data", data,
                         NULL);

    request->icon = g_steal_pointer (&icon);
    snapd_request_complete (request, NULL);
}

static void
parse_list_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;
    GPtrArray *snaps;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    array = get_array (response, "result");
    snaps = parse_snap_array (array, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_list_one_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(SnapdSnap) snap = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        snapd_request_complete (request, error);
        return;
    }

    snap = parse_snap (result, &error);
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
    array = get_array (object, "connections");
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
                                   "name", get_string (object, name, NULL),
                                   "snap", get_string (object, "snap", NULL),
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
    g_autoptr(JsonObject) attrs = NULL;
    JsonObjectIter iter;
    GHashTable *attributes;
    const gchar *attribute_name;
    JsonNode *node;

    attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    attrs = get_object (object, "attrs");
    if (attrs == NULL)
        return attributes;

    json_object_iter_init (&iter, attrs);
    while (json_object_iter_next (&iter, &attribute_name, &node))
        g_hash_table_insert (attributes, g_strdup (attribute_name), node_to_variant (node));

    return attributes;
}

static void
parse_get_interfaces_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    JsonObject *result;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        snapd_request_complete (request, error);
        return;
    }

    plugs = get_array (result, "plugs");
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
                             "name", get_string (object, "plug", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
                             "connections", connections,
                             "attributes", attributes,
                             // FIXME: apps
                             NULL);
        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = get_array (result, "slots");
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
                             "name", get_string (object, "slot", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
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

    path = g_strdup_printf ("/v2/changes/%s", request->change_id);
    send_request (request, TRUE, "GET", path, NULL, NULL);

    request->poll_timer = 0;
    return G_SOURCE_REMOVE;
}

static gboolean
async_timeout_cb (gpointer data)
{
    SnapdRequest *request = data;
    GError *error;

    request->timeout_timer = 0;

    error = g_error_new (SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Timeout waiting for snapd");
    snapd_request_complete (request, error);

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
parse_async_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autofree gchar *change_id = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, &change_id, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    /* Cancel any pending timeout */
    if (request->timeout_timer != 0)
        g_source_remove (request->timeout_timer);
    request->timeout_timer = 0;

    /* First run we expect the change ID, following runs are updates */
    if (request->change_id == NULL) {
         if (change_id == NULL) {
             g_error_new (SNAPD_ERROR,
                          SNAPD_ERROR_READ_FAILED,
                          "No async response received");
             snapd_request_complete (request, error);
             return;
         }

         request->change_id = g_strdup (change_id);
    }
    else {
        gboolean ready;
        JsonObject *result;

        if (change_id != NULL) {
             error = g_error_new (SNAPD_ERROR,
                                  SNAPD_ERROR_READ_FAILED,
                                  "Duplicate async response received");
             snapd_request_complete (request, error);
             return;
        }

        result = get_object (response, "result");
        if (result == NULL) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "No async result returned");
            snapd_request_complete (request, error);
            return;
        }

        if (g_strcmp0 (request->change_id, get_string (result, "id", NULL)) != 0) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected change ID returned");
            snapd_request_complete (request, error);
            return;
        }

        /* Update caller with progress */
        if (request->progress_callback != NULL) {
            g_autoptr(JsonArray) array = NULL;
            guint i;
            g_autoptr(GPtrArray) tasks = NULL;
            g_autoptr(SnapdChange) change = NULL;
            g_autoptr(GDateTime) main_spawn_time = NULL;
            g_autoptr(GDateTime) main_ready_time = NULL;

            array = get_array (result, "tasks");
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
                    snapd_request_complete (request, error);
                    return;
                }
                object = json_node_get_object (node);
                progress = get_object (object, "progress");
                spawn_time = get_date_time (object, "spawn-time");
                ready_time = get_date_time (object, "ready-time");

                t = g_object_new (SNAPD_TYPE_TASK,
                                  "id", get_string (object, "id", NULL),
                                  "kind", get_string (object, "kind", NULL),
                                  "summary", get_string (object, "summary", NULL),
                                  "status", get_string (object, "status", NULL),
                                  "progress-label", progress != NULL ? get_string (progress, "label", NULL) : NULL,
                                  "progress-done", progress != NULL ? get_int (progress, "done", 0) : 0,
                                  "progress-total", progress != NULL ? get_int (progress, "total", 0) : 0,
                                  "spawn-time", spawn_time,
                                  "ready-time", ready_time,
                                  NULL);
                g_ptr_array_add (tasks, g_steal_pointer (&t));
            }

            main_spawn_time = get_date_time (result, "spawn-time");
            main_ready_time = get_date_time (result, "ready-time");
            change = g_object_new (SNAPD_TYPE_CHANGE,
                                   "id", get_string (result, "id", NULL),
                                   "kind", get_string (result, "kind", NULL),
                                   "summary", get_string (result, "summary", NULL),
                                   "status", get_string (result, "status", NULL),
                                   "tasks", tasks,
                                   "ready", get_bool (result, "ready", FALSE),
                                   "spawn-time", main_spawn_time,
                                   "ready-time", main_ready_time,
                                   NULL);

            if (!changes_equal (request->change, change)) {
                g_clear_object (&request->change);
                request->change = g_steal_pointer (&change);
                // NOTE: tasks is passed for ABI compatibility - this field is
                // deprecated and can be accessed with snapd_change_get_tasks ()
                request->progress_callback (request->client, request->change, tasks, request->progress_callback_data);
            }
        }

        ready = get_bool (result, "ready", FALSE);
        if (ready) {
            request->result = TRUE;
            if (json_object_has_member (result, "data"))
                request->async_data = json_node_ref (json_object_get_member (result, "data"));
            snapd_request_complete (request, NULL);
            return;
        }
    }

    /* Poll for updates */
    if (request->poll_timer != 0)
        g_source_remove (request->poll_timer);
    request->poll_timer = g_timeout_add (ASYNC_POLL_TIME, async_poll_cb, request);
    request->timeout_timer = g_timeout_add (ASYNC_POLL_TIMEOUT, async_timeout_cb, request);
}

static void
parse_connect_interface_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_disconnect_interface_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_login_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(JsonArray) discharges = NULL;
    g_autoptr(GPtrArray) discharge_array = NULL;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        snapd_request_complete (request, error);
        return;
    }

    discharges = get_array (result, "discharges");
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
    request->received_auth_data = snapd_auth_data_new (get_string (result, "macaroon", NULL), (gchar **) discharge_array->pdata);
    snapd_request_complete (request, NULL);
}

static void
parse_find_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    array = get_array (response, "result");
    snaps = parse_snap_array (array, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->suggested_currency = g_strdup (get_string (response, "suggested-currency", NULL));

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_find_refreshable_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    array = get_array (response, "result");
    snaps = parse_snap_array (array, &error);
    if (snaps == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->snaps = g_steal_pointer (&snaps);
    snapd_request_complete (request, NULL);
}

static void
parse_check_buy_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, NULL, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static void
parse_buy_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, NULL, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static void
parse_install_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_refresh_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_refresh_all_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_remove_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_enable_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static void
parse_disable_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    parse_async_response (request, headers, content, content_length);
}

static SnapdUserInformation *
parse_user_information (JsonObject *object, GError **error)
{
    g_autoptr(JsonArray) ssh_keys = NULL;
    g_autoptr(GPtrArray) ssh_key_array = NULL;
    guint i;

    ssh_keys = get_array (object, "ssh-keys");
    ssh_key_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (ssh_keys); i++) {
        JsonNode *node = json_array_get_element (ssh_keys, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected SSH key type");
            return NULL;
        }

        g_ptr_array_add (ssh_key_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (ssh_key_array, NULL);

    return g_object_new (SNAPD_TYPE_USER_INFORMATION,
                         "username", get_string (object, "username", NULL),
                         "ssh-keys", (gchar **) ssh_key_array->pdata,
                         NULL);
}

static void
parse_create_user_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    user_information = parse_user_information (result, &error);
    if (user_information == NULL) {
        snapd_request_complete (request, error);
        return;
    }

    request->user_information = g_steal_pointer (&user_information);
    snapd_request_complete (request, NULL);
}

static void
parse_create_users_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;
    g_autoptr(GPtrArray) users_information = NULL;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    array = get_array (response, "result");
    users_information = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        SnapdUserInformation *user_information;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected user information type");
            snapd_request_complete (request, error);
            return;
        }

        user_information = parse_user_information (json_node_get_object (node), &error);
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
parse_get_sections_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) sections = NULL;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_array (response, "result");
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
parse_get_aliases_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(GPtrArray) aliases = NULL;
    JsonObjectIter snap_iter;
    const gchar *snap;
    JsonNode *snap_node;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
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
            status_string = get_string (o, "status", NULL);
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
                                  "app", get_string (o, "app", NULL),
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
parse_change_aliases_response (SnapdRequest *request, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, NULL, NULL, &error)) {
        snapd_request_complete (request, error);
        return;
    }

    request->result = TRUE;
    snapd_request_complete (request, NULL);
}

static SnapdRequest *
get_next_request (SnapdClient *client)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GList *link;

    for (link = priv->requests; link != NULL; link = link->next) {
        SnapdRequest *request = link->data;
        if (!request->completed)
            return request;
    }

    return NULL;
}

static void
parse_response (SnapdClient *client, guint code, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    SnapdRequest *request;
    GError *error = NULL;

    /* Match this response to the next uncompleted request */
    request = get_next_request (client);
    if (request == NULL) {
        g_warning ("Ignoring unexpected response");
        return;
    }

    switch (request->request_type)
    {
    case SNAPD_REQUEST_GET_SYSTEM_INFORMATION:
        parse_get_system_information_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_ICON:
        parse_get_icon_response (request, code, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LIST:
        parse_list_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LIST_ONE:
        parse_list_one_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_INTERFACES:
        parse_get_interfaces_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CONNECT_INTERFACE:
        parse_connect_interface_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_DISCONNECT_INTERFACE:
        parse_disconnect_interface_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LOGIN:
        parse_login_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_FIND:
        parse_find_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_FIND_REFRESHABLE:
        parse_find_refreshable_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CHECK_BUY:
        parse_check_buy_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_BUY:
        parse_buy_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_INSTALL:
        parse_install_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_REFRESH:
        parse_refresh_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_REFRESH_ALL:
        parse_refresh_all_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_REMOVE:
        parse_remove_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_ENABLE:
        parse_enable_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_DISABLE:
        parse_disable_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CREATE_USER:
        parse_create_user_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CREATE_USERS:
        parse_create_users_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_SECTIONS:
        parse_get_sections_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_ALIASES:
        parse_get_aliases_response (request, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CHANGE_ALIASES:
        parse_change_aliases_response (request, headers, content, content_length);
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
    if (n_read < 0)
    {
        g_printerr ("read error\n");
        // FIXME: Cancel all requests
        //g_set_error (error,
        //             SNAPD_ERROR,
        //             SNAPD_ERROR_READ_FAILED,
        //             "Failed to read from snapd: %s",
        //             error_local->message);
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
read_from_snapd (SnapdClient *client,
                 GCancellable *cancellable, gboolean blocking)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    gchar *body;
    gsize header_length;
    g_autoptr(SoupMessageHeaders) headers = NULL;
    guint code;
    g_autofree gchar *reason_phrase = NULL;
    gchar *combined_start;
    gsize content_length, combined_length;

    if (!read_data (client, 1024, cancellable))
        return G_SOURCE_REMOVE;

    while (TRUE) {
        /* Look for header divider */
        body = g_strstr_len ((gchar *) priv->buffer->data, priv->n_read, "\r\n\r\n");
        if (body == NULL)
            return G_SOURCE_CONTINUE;
        body += 4;
        header_length = body - (gchar *) priv->buffer->data;

        /* Parse headers */
        headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_RESPONSE);
        if (!soup_headers_parse_response ((gchar *) priv->buffer->data, header_length, headers,
                                          NULL, &code, &reason_phrase)) {
            // FIXME: Cancel all requests
            return G_SOURCE_REMOVE;
        }

        /* Read content and process content */
        switch (soup_message_headers_get_encoding (headers)) {
        case SOUP_ENCODING_EOF:
            while (!g_socket_is_closed (priv->snapd_socket)) {
                if (!blocking)
                    return G_SOURCE_CONTINUE;
                if (!read_data (client, 1024, cancellable))
                    return G_SOURCE_REMOVE;
            }

            content_length = priv->n_read - header_length;
            parse_response (client, code, headers, body, content_length);
            break;

        case SOUP_ENCODING_CHUNKED:
            // FIXME: Find a way to abort on error
            while (!have_chunked_body (body, priv->n_read - header_length)) {
                if (!blocking)
                    return G_SOURCE_CONTINUE;
                if (!read_data (client, 1024, cancellable))
                    return G_SOURCE_REMOVE;
            }

            compress_chunks (body, priv->n_read - header_length, &combined_start, &combined_length, &content_length);
            parse_response (client, code, headers, combined_start, combined_length);
            break;

        case SOUP_ENCODING_CONTENT_LENGTH:
            content_length = soup_message_headers_get_content_length (headers);
            while (priv->n_read < header_length + content_length) {
                if (!blocking)
                    return G_SOURCE_CONTINUE;
                if (!read_data (client, 1024, cancellable))
                    return G_SOURCE_REMOVE;
            }

            parse_response (client, code, headers, body, content_length);
            break;

        default:
            // FIXME
            return G_SOURCE_REMOVE;
        }

        /* Move remaining data to the start of the buffer */
        g_byte_array_remove_range (priv->buffer, 0, header_length + content_length);
        priv->n_read -= header_length + content_length;
    }
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, SnapdClient *client)
{
    return read_from_snapd (client, NULL, FALSE); // FIXME: Use Cancellable from first request?
}

/**
 * snapd_client_connect_sync:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Connect to snapd.
 *
 * Returns: %TRUE if successfully connected to snapd.
 */
gboolean
snapd_client_connect_sync (SnapdClient *client,
                           GCancellable *cancellable, GError **error)
{
    SnapdClientPrivate *priv;
    g_autoptr(GSocketAddress) address = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    priv = snapd_client_get_instance_private (client);

    if (priv->snapd_socket == NULL) {
        priv->snapd_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                           G_SOCKET_TYPE_STREAM,
                                           G_SOCKET_PROTOCOL_DEFAULT,
                                           &error_local);
        if (priv->snapd_socket == NULL) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_CONNECTION_FAILED,
                         "Unable to open snapd socket: %s",
                         error_local->message);
            return FALSE;
        }
        address = g_unix_socket_address_new (SNAPD_SOCKET);
        if (!g_socket_connect (priv->snapd_socket, address, cancellable, &error_local)) {
            g_clear_object (&priv->snapd_socket);
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_CONNECTION_FAILED,
                         "Unable to connect snapd socket: %s",
                         error_local->message);
            g_clear_object (&priv->snapd_socket);
            return FALSE;
        }      
    }

    if (priv->read_source == NULL) {
        priv->read_source = g_socket_create_source (priv->snapd_socket, G_IO_IN, NULL);
        g_source_set_callback (priv->read_source, (GSourceFunc) read_cb, client, NULL);
        g_source_attach (priv->read_source, NULL);
    }
      
    return TRUE;
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
 */
void
snapd_client_connect_async (SnapdClient *client,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autoptr(GError) error = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    if (snapd_client_connect_sync (client, cancellable, &error))
        g_task_return_boolean (task, TRUE);
    else
        g_task_return_error (task, error);
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
 */
gboolean
snapd_client_connect_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    return g_task_propagate_boolean (G_TASK (result), error);
}

static SnapdRequest *
make_request (SnapdClient *client, RequestType request_type,
              SnapdProgressCallback progress_callback, gpointer progress_callback_data,
              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    SnapdRequest *request;

    request = g_object_new (snapd_request_get_type (), NULL);
    if (priv->auth_data != NULL)
        request->auth_data = g_object_ref (priv->auth_data);
    request->client = client;
    request->request_type = request_type;
    if (cancellable != NULL)
        request->cancellable = g_object_ref (cancellable);
    request->ready_callback = callback;
    request->ready_callback_data = user_data;
    request->progress_callback = progress_callback;
    request->progress_callback_data = progress_callback_data;
    priv->requests = g_list_append (priv->requests, request);

    return request;
}

static SnapdRequest *
make_login_request (SnapdClient *client,
                    const gchar *username, const gchar *password, const gchar *otp,
                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_LOGIN, NULL, NULL, cancellable, callback, user_data);

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
    data = builder_to_string (builder);

    send_request (request, FALSE, "POST", "/v2/login", "application/json", data);

    return request;
}

/**
 * snapd_client_login_sync:
 * @client: a #SnapdClient.
 * @username: usename to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Log in to snapd and get authorization to install/remove snaps.
 * This call requires root access; use snapd_login_sync() if you are non-root.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 */
SnapdAuthData *
snapd_client_login_sync (SnapdClient *client,
                         const gchar *username, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (username != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    request = g_object_ref (make_login_request (client, username, password, otp, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_login_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_login_request (client, username, password, otp, cancellable, callback, user_data);
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
 */
SnapdAuthData *
snapd_client_get_auth_data (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);

    return priv->auth_data;
}

static SnapdRequest *
make_get_system_information_request (SnapdClient *client,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_GET_SYSTEM_INFORMATION, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/system-info", NULL, NULL);

    return request;
}

/**
 * snapd_client_get_system_information_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Request system information from snapd.
 * While this blocks, snapd is expected to return the information quickly.
 *
 * Returns: (transfer full): a #SnapdSystemInformation or %NULL on error.
 */
SnapdSystemInformation *
snapd_client_get_system_information_sync (SnapdClient *client,
                                          GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_get_system_information_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_get_system_information_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_get_system_information_async (SnapdClient *client,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_system_information_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_list_one_request (SnapdClient *client,
                       const gchar *name,
                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_LIST_ONE, NULL, NULL, cancellable, callback, user_data);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "GET", path, NULL, NULL);

    return request;
}

/**
 * snapd_client_list_one_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information of a single installed snap.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 */
SnapdSnap *
snapd_client_list_one_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_list_one_request (client, name, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_list_one_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_list_one_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_one_request (client, name, cancellable, callback, user_data);
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

static SnapdRequest *
make_get_icon_request (SnapdClient *client,
                       const gchar *name,
                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_GET_ICON, NULL, NULL, cancellable, callback, user_data);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/icons/%s/icon", escaped);
    send_request (request, TRUE, "GET", path, NULL, NULL);

    return request;
}

/**
 * snapd_client_get_icon_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the icon for an installed snap.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 */
SnapdIcon *
snapd_client_get_icon_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_get_icon_request (client, name, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_get_icon_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_get_icon_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_icon_request (client, name, cancellable, callback, user_data);
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

static SnapdRequest *
make_list_request (SnapdClient *client,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_LIST, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/snaps", NULL, NULL);

    return request;
}

/**
 * snapd_client_list_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on all installed snaps.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_list_sync (SnapdClient *client,
                        GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_list_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_list_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_list_async (SnapdClient *client,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_get_interfaces_request (SnapdClient *client,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_GET_INTERFACES, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/interfaces", NULL, NULL);

    return request;
}

/**
 * snapd_client_get_interfaces_sync:
 * @client: a #SnapdClient.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the installed snap interfaces.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_get_interfaces_sync (SnapdClient *client,
                                  GPtrArray **plugs, GPtrArray **slots,
                                  GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    request = g_object_ref (make_get_interfaces_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_get_interfaces_finish (client, G_ASYNC_RESULT (request), plugs, slots, error);
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
 */
void
snapd_client_get_interfaces_async (SnapdClient *client,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_interfaces_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_interface_request (SnapdClient *client,
                        RequestType request_type,
                        const gchar *action,
                        const gchar *plug_snap, const gchar *plug_name,
                        const gchar *slot_snap, const gchar *slot_name,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    request = make_request (client, request_type, progress_callback, progress_callback_data, cancellable, callback, user_data);

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
    data = builder_to_string (builder);

    send_request (request, TRUE, "POST", "/v2/interfaces", "application/json", data);

    return request;
}

static SnapdRequest *
make_connect_interface_request (SnapdClient *client,
                                const gchar *plug_snap, const gchar *plug_name,
                                const gchar *slot_snap, const gchar *slot_name,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_request (client, SNAPD_REQUEST_CONNECT_INTERFACE,
                                   "connect",
                                   plug_snap, plug_name,
                                   slot_snap, slot_name,
                                   progress_callback, progress_callback_data,
                                   cancellable, callback, user_data);
}

/**
 * snapd_client_connect_interface_sync:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Connect two interfaces together.
 * An asynchronous version of this function is snapd_client_connect_interface_async().
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_connect_interface_sync (SnapdClient *client,
                                     const gchar *plug_snap, const gchar *plug_name,
                                     const gchar *slot_snap, const gchar *slot_name,
                                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                     GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    request = g_object_ref (make_connect_interface_request (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_connect_interface_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_connect_interface_async (SnapdClient *client,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_connect_interface_request (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
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

static SnapdRequest *
make_disconnect_interface_request (SnapdClient *client,
                                   const gchar *plug_snap, const gchar *plug_name,
                                   const gchar *slot_snap, const gchar *slot_name,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_request (client, SNAPD_REQUEST_DISCONNECT_INTERFACE,
                                   "disconnect",
                                   plug_snap, plug_name,
                                   slot_snap, slot_name,
                                   progress_callback, progress_callback_data,
                                   cancellable, callback, user_data);
}

/**
 * snapd_client_disconnect_interface_sync:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Disconnect two interfaces.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disconnect_interface_sync (SnapdClient *client,
                                        const gchar *plug_snap, const gchar *plug_name,
                                        const gchar *slot_snap, const gchar *slot_name,
                                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                        GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    request = g_object_ref (make_disconnect_interface_request (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_disconnect_interface_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_disconnect_interface_async (SnapdClient *client,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_disconnect_interface_request (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
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

static SnapdRequest *
make_find_request (SnapdClient *client,
                   SnapdFindFlags flags, const gchar *section, const gchar *query,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(GPtrArray) query_attributes = NULL;
    g_autoptr(GString) path = NULL;

    request = make_request (client, SNAPD_REQUEST_FIND, NULL, NULL, cancellable, callback, user_data);
    path = g_string_new ("/v2/find");

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

    if (query_attributes->len > 0) {
        guint i;

        g_string_append_c (path, '?');
        for (i = 0; i < query_attributes->len; i++) {
            if (i != 0)
                g_string_append_c (path, '&');
            g_string_append (path, (gchar *) query_attributes->pdata[i]);
        }
    }

    send_request (request, TRUE, "GET", path->str, NULL, NULL);

    return request;
}

/**
 * snapd_client_find_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: (allow-none): query string to send or %NULL to find all.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in the store.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_find_sync (SnapdClient *client,
                        SnapdFindFlags flags, const gchar *query,
                        gchar **suggested_currency,
                        GCancellable *cancellable, GError **error)
{
    return snapd_client_find_section_sync (client, flags, NULL, query, suggested_currency, cancellable, error);
}

/**
 * snapd_client_find_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: (allow-none): query string to send or %NULL to find all.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_sync() for more information.
 */
void
snapd_client_find_async (SnapdClient *client,
                         SnapdFindFlags flags, const gchar *query,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
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
 */
GPtrArray *
snapd_client_find_finish (SnapdClient *client, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    return snapd_client_find_section_finish (client, result, suggested_currency, error);
}

/**
 * snapd_client_find_section_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @section: (allow-none): store section to search in or %NULL to search in all sections.
 * @query: (allow-none): query string to send or %NULL to find all.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in the store.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_find_section_sync (SnapdClient *client,
                                SnapdFindFlags flags, const gchar *section, const gchar *query,
                                gchar **suggested_currency,
                                GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_find_request (client, flags, section, query, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_find_section_finish (client, G_ASYNC_RESULT (request), suggested_currency, error);
}

/**
 * snapd_client_find_section_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @section: (allow-none): store section to search in or %NULL to search in all sections.
 * @query: (allow-none): query string to send or %NULL to find all.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_section_sync() for more information.
 */
void
snapd_client_find_section_async (SnapdClient *client,
                                 SnapdFindFlags flags, const gchar *section, const gchar *query,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_find_request (client, flags, section, query, cancellable, callback, user_data);
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

static SnapdRequest *
make_find_refreshable_request (SnapdClient *client,
                               GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_FIND_REFRESHABLE, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/find?select=refresh", NULL, NULL);

    return request;
}

/**
 * snapd_client_find_refreshable_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in store that are newer revisions than locally installed versions.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_find_refreshable_sync (SnapdClient *client,
                                    GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_find_refreshable_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_find_refreshable_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_find_refreshable_async (SnapdClient *client,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_find_refreshable_request (client, cancellable, callback, user_data);
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

static gchar *
make_action_data (const gchar *action, const gchar *channel)
{
    g_autoptr(JsonBuilder) builder = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, action);
    if (channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, channel);
    }
    json_builder_end_object (builder);

    return builder_to_string (builder);
}

static SnapdRequest *
make_install_request (SnapdClient *client,
                      const gchar *name, const gchar *channel,
                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_INSTALL, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("install", channel);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "POST", path, "application/json", data);

    return request;
}

/**
 * snapd_client_install_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Install a snap from the store.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_install_sync (SnapdClient *client,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    request = g_object_ref (make_install_request (client, name, channel, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_install_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_install_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_install_request (client, name, channel, progress_callback, progress_callback_data, cancellable, callback, user_data);
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
 */
gboolean
snapd_client_install_finish (SnapdClient *client, GAsyncResult *result, GError **error)
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

static SnapdRequest *
make_refresh_request (SnapdClient *client,
                      const gchar *name, const gchar *channel,
                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_REFRESH, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("refresh", channel);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "POST", path, "application/json", data);

    return request;
}

/**
 * snapd_client_refresh_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Ensure an installed snap is at the latest version.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_refresh_sync (SnapdClient *client,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    request = g_object_ref (make_refresh_request (client, name, channel, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_refresh_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_refresh_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    make_refresh_request (client, name, channel, progress_callback, progress_callback_data, cancellable, callback, user_data);
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

static SnapdRequest *
make_refresh_all_request (SnapdClient *client,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_REFRESH_ALL, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("refresh", NULL);
    send_request (request, TRUE, "POST", "/v2/snaps", "application/json", data);

    return request;
}

/**
 * snapd_client_refresh_all_sync:
 * @client: a #SnapdClient.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Update all installed snaps to their latest version.
 *
 * Returns: (transfer full): a %NULL-terminated array of the snap names refreshed or %NULL on error.
 */
gchar **
snapd_client_refresh_all_sync (SnapdClient *client,
                               SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                               GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_refresh_all_request (client, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_refresh_all_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_refresh_all_async (SnapdClient *client,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_refresh_all_request (client, progress_callback, progress_callback_data, cancellable, callback, user_data);
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
    a = get_array (o, "snap-names");
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

static SnapdRequest *
make_remove_request (SnapdClient *client,
                     const gchar *name,
                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_REMOVE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("remove", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "POST", path, "application/json", data);

    return request;
}

/**
 * snapd_client_remove_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Uninstall a snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_remove_sync (SnapdClient *client,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    request = g_object_ref (make_remove_request (client, name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_remove_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_remove_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_remove_request (client, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
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

static SnapdRequest *
make_enable_request (SnapdClient *client,
                     const gchar *name,
                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_ENABLE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("enable", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "POST", path, "application/json", data);

    return request;
}

/**
 * snapd_client_enable_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Enable an installed snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_enable_sync (SnapdClient *client,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    request = g_object_ref (make_enable_request (client, name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_enable_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_enable_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_enable_request (client, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
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

static SnapdRequest *
make_disable_request (SnapdClient *client,
                      const gchar *name,
                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    request = make_request (client, SNAPD_REQUEST_DISABLE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("disable", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (request, TRUE, "POST", path, "application/json", data);

    return request;
}

/**
 * snapd_client_disable_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Disable an installed snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disable_sync (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    request = g_object_ref (make_disable_request (client, name,
                                                  progress_callback, progress_callback_data,
                                                  cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_disable_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_disable_async (SnapdClient *client,
                            const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_disable_request (client, name,
                          progress_callback, progress_callback_data,
                          cancellable, callback, user_data);
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

static SnapdRequest *
make_check_buy_request (SnapdClient *client,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_CHECK_BUY, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/buy/ready", NULL, NULL);

    return request;
}

/**
 * snapd_client_check_buy_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Check if able to buy snaps.
 *
 * Returns: %TRUE if able to buy snaps or %FALSE on error.
 */
gboolean
snapd_client_check_buy_sync (SnapdClient *client,
                             GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    request = g_object_ref (make_check_buy_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_check_buy_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_check_buy_async (SnapdClient *client,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_check_buy_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_buy_request (SnapdClient *client,
                  const gchar *id, gdouble amount, const gchar *currency,
                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_BUY, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-id");
    json_builder_add_string_value (builder, id);
    json_builder_set_member_name (builder, "price");
    json_builder_add_double_value (builder, amount);
    json_builder_set_member_name (builder, "currency");
    json_builder_add_string_value (builder, currency);
    json_builder_end_object (builder);
    data = builder_to_string (builder);

    send_request (request, TRUE, "POST", "/v2/buy", "application/json", data);

    return request;
}

/**
 * snapd_client_buy_sync:
 * @client: a #SnapdClient.
 * @id: id of snap to buy.
 * @amount: amount of currency to spend, e.g. 0.99.
 * @currency: the currency to buy with as an ISO 4217 currency code, e.g. "NZD".
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Buy a snap from the store. Once purchased, this snap can be installed with
 * snapd_client_install_sync().
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_buy_sync (SnapdClient *client,
                       const gchar *id, gdouble amount, const gchar *currency,
                       GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (id != NULL, FALSE);
    g_return_val_if_fail (currency != NULL, FALSE);

    request = g_object_ref (make_buy_request (client, id, amount, currency, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_buy_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_buy_async (SnapdClient *client,
                        const gchar *id, gdouble amount, const gchar *currency,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (id != NULL);
    g_return_if_fail (currency != NULL);
    make_buy_request (client, id, amount, currency, cancellable, callback, user_data);
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

static SnapdRequest *
make_create_user_request (SnapdClient *client,
                          const gchar *email, SnapdCreateUserFlags flags,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_CREATE_USER, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "email");
    json_builder_add_string_value (builder, email);
    if ((flags & SNAPD_CREATE_USER_FLAGS_SUDO) != 0) {
        json_builder_set_member_name (builder, "sudoers");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((flags & SNAPD_CREATE_USER_FLAGS_KNOWN) != 0) {  
        json_builder_set_member_name (builder, "known");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    data = builder_to_string (builder);

    send_request (request, TRUE, "POST", "/v2/create-user", "application/json", data);

    return request;
}

/**
 * snapd_client_create_user_sync:
 * @client: a #SnapdClient.
 * @email: the email of the user to create.
 * @flags: a set of #SnapdCreateUserFlags to control how the user account is created.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Create a local user account for the given user.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 */
SnapdUserInformation *
snapd_client_create_user_sync (SnapdClient *client,
                               const gchar *email, SnapdCreateUserFlags flags,
                               GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (email != NULL, NULL);

    request = g_object_ref (make_create_user_request (client, email, flags, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_create_user_finish (client, G_ASYNC_RESULT (request), error);
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
 */
void
snapd_client_create_user_async (SnapdClient *client,
                                const gchar *email, SnapdCreateUserFlags flags,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (email != NULL);
    make_create_user_request (client, email, flags, cancellable, callback, user_data);
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

static SnapdRequest *
make_create_users_request (SnapdClient *client,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_CREATE_USER, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "known");
    json_builder_add_boolean_value (builder, TRUE);
    json_builder_end_object (builder);
    data = builder_to_string (builder);

    send_request (request, TRUE, "POST", "/v2/create-user", "application/json", data);

    return request;
}

/**
 * snapd_client_create_users_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Create local user accounts using the system-user assertions that are valid for this device.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 */
GPtrArray *
snapd_client_create_users_sync (SnapdClient *client,
                                GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_create_users_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_create_users_finish (client, G_ASYNC_RESULT (request), error);
}

/**
 * snapd_client_create_users_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_create_user_sync() for more information.
 */
void
snapd_client_create_users_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_create_users_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_get_sections_request (SnapdClient *client,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_GET_SECTIONS, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/sections", NULL, NULL);

    return request;
}

/**
 * snapd_client_get_sections_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get the store sections.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of section names or %NULL on error.
 */
gchar **
snapd_client_get_sections_sync (SnapdClient *client,
                                GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_get_sections_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_get_sections_finish (client, G_ASYNC_RESULT (request), error);
}

/**
 * snapd_client_get_sections_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_create_user_sync() for more information.
 */
void
snapd_client_get_sections_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_sections_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_get_aliases_request (SnapdClient *client,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;

    request = make_request (client, SNAPD_REQUEST_GET_ALIASES, NULL, NULL, cancellable, callback, user_data);
    send_request (request, TRUE, "GET", "/v2/aliases", NULL, NULL);

    return request;
}

/**
 * snapd_client_get_aliases_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get the available aliases.
 *
 * Returns: (transfer container) (element-type SnapdAlias): an array of #SnapdAlias or %NULL on error.
 */
GPtrArray *
snapd_client_get_aliases_sync (SnapdClient *client,
                               GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    request = g_object_ref (make_get_aliases_request (client, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_get_aliases_finish (client, G_ASYNC_RESULT (request), error);
}

/**
 * snapd_client_get_aliases_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the available aliases.
 * See snapd_client_create_user_sync() for more information.
 */
void
snapd_client_get_aliases_async (SnapdClient *client,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_aliases_request (client, cancellable, callback, user_data);
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

static SnapdRequest *
make_change_aliases_request (SnapdClient *client,
                             SnapdAliasAction action, const gchar *snap, gchar **aliases,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdRequest *request;
    g_autoptr(JsonBuilder) builder = NULL;
    int i;
    g_autofree gchar *data = NULL;

    request = make_request (client, SNAPD_REQUEST_CHANGE_ALIASES, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    switch (action) {
    case SNAPD_ALIAS_ACTION_ALIAS:
        json_builder_add_string_value (builder, "alias");
        break;
    case SNAPD_ALIAS_ACTION_UNALIAS:
        json_builder_add_string_value (builder, "unalias");
        break;
    default:
    case SNAPD_ALIAS_ACTION_RESET:
        json_builder_add_string_value (builder, "reset");
        break;
    }
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, snap);
    json_builder_set_member_name (builder, "aliases");
    json_builder_begin_array (builder);
    for (i = 0; aliases[i] != NULL; i++)
        json_builder_add_string_value (builder, aliases[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    data = builder_to_string (builder);

    send_request (request, TRUE, "POST", "/v2/aliases", "application/json", data);

    return request;
}

/**
 * snapd_client_change_aliases_sync:
 * @client: a #SnapdClient.
 * @action: the action to perform.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Change the state of aliases.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_change_aliases_sync (SnapdClient *client,
                                  SnapdAliasAction action, const gchar *snap, gchar **aliases,
                                  GCancellable *cancellable, GError **error)
{
    g_autoptr(SnapdRequest) request = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (snap != NULL, FALSE);
    g_return_val_if_fail (aliases != NULL, FALSE);

    request = g_object_ref (make_change_aliases_request (client, action, snap, aliases, cancellable, NULL, NULL));
    snapd_request_wait (request);
    return snapd_client_change_aliases_finish (client, G_ASYNC_RESULT (request), error);
}

/**
 * snapd_client_change_aliases_async:
 * @client: a #SnapdClient.
 * @action: the action to perform.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_create_user_sync() for more information.
 */
void
snapd_client_change_aliases_async (SnapdClient *client,
                                   SnapdAliasAction action, const gchar *snap, gchar **aliases,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (aliases != NULL);
    make_change_aliases_request (client, action, snap, aliases, cancellable, callback, user_data);
}

/**
 * snapd_client_change_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_change_aliases_async().
 * See snapd_client_change_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_change_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_REQUEST (result), FALSE);

    request = SNAPD_REQUEST (result);
    g_return_val_if_fail (request->request_type == SNAPD_REQUEST_CHANGE_ALIASES, FALSE);

    if (snapd_request_set_error (request, error))
        return FALSE;
    return request->result;
}

/**
 * snapd_client_new:
 *
 * Create a new client to talk to snapd.
 *
 * Returns: a new #SnapdClient
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
 **/
SnapdClient *
snapd_client_new_from_socket (GSocket *socket)
{
    SnapdClient *client;
    SnapdClientPrivate *priv;

    client = snapd_client_new ();
    priv = snapd_client_get_instance_private (SNAPD_CLIENT (client));
    priv->snapd_socket = g_object_ref (socket);

    return client;
}

static void
snapd_client_finalize (GObject *object)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (SNAPD_CLIENT (object));

    g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);
    g_clear_object (&priv->auth_data);
    g_list_free_full (priv->requests, g_object_unref);
    priv->requests = NULL;
    if (priv->read_source != NULL)
        g_source_destroy (priv->read_source);
    g_clear_pointer (&priv->read_source, g_source_unref);
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

    priv->buffer = g_byte_array_new ();
}
