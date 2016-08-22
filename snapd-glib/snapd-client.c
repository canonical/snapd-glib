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
#include "snapd-app.h"
#include "snapd-plug.h"
#include "snapd-slot.h"

typedef struct
{
    GSocket *snapd_socket;
    GList *tasks;
    GSource *read_source;
    GByteArray *buffer;
    gsize n_read;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (snapd-client-error-quark, snapd_client_error)

/* snapd API documentation is at https://github.com/snapcore/snapd/blob/master/docs/rest.md */

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
    SNAPD_REQUEST_SIDELOAD_SNAP, // FIXME
    SNAPD_REQUEST_GET_PAYMENT_METHODS,
    SNAPD_REQUEST_BUY,
    SNAPD_REQUEST_INSTALL,
    SNAPD_REQUEST_REFRESH,  
    SNAPD_REQUEST_REMOVE,
    SNAPD_REQUEST_ENABLE,
    SNAPD_REQUEST_DISABLE
} RequestType;

typedef struct 
{
    RequestType request_type;
    SnapdProgressCallback progress_callback;
    gpointer progress_callback_data;
    gchar *change_id;
    guint poll_timer;
    guint timeout_timer;
} RequestData;

typedef struct 
{
    GPtrArray *plugs;
    GPtrArray *slots;  
} GetInterfacesResult;

typedef struct 
{
    gboolean allows_automatic_payment;
    GPtrArray *methods;
} GetPaymentMethodsResult;

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
send_request (GTask *task, SnapdAuthData *auth_data, const gchar *method, const gchar *path, const gchar *content_type, const gchar *content)
{
    SnapdClient *client = g_task_get_source_object (task);
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    g_autoptr(SoupMessageHeaders) headers = NULL;
    g_autoptr(GString) request = NULL;
    SoupMessageHeadersIter iter;
    const char *name, *value;
    gssize n_written;
    g_autoptr(GError) error = NULL;

    // NOTE: Would love to use libsoup but it doesn't support unix sockets
    // https://bugzilla.gnome.org/show_bug.cgi?id=727563

    headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_REQUEST);
    soup_message_headers_append (headers, "Host", "");
    soup_message_headers_append (headers, "Connection", "keep-alive");
    if (content_type)
        soup_message_headers_set_content_type (headers, content_type, NULL);
    if (content)
        soup_message_headers_set_content_length (headers, strlen (content));
    if (auth_data != NULL) {
        g_autoptr(GString) authorization;
        gchar **discharges;
        gsize i;

        authorization = g_string_new ("");
        g_string_append_printf (authorization, "Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (auth_data));
        discharges = snapd_auth_data_get_discharges (auth_data);
        for (i = 0; discharges[i] != NULL; i++)
            g_string_append_printf (authorization, ",discharge=\"%s\"", discharges[i]);
        soup_message_headers_append (headers, "Authorization", authorization->str);
    }

    request = g_string_new ("");
    g_string_append_printf (request, "%s %s HTTP/1.1\r\n", method, path);
    soup_message_headers_iter_init (&iter, headers);
    while (soup_message_headers_iter_next (&iter, &name, &value))
        g_string_append_printf (request, "%s: %s\r\n", name, value);
    g_string_append (request, "\r\n");
    if (content)
        g_string_append (request, content);

    /* send HTTP request */
    n_written = g_socket_send (priv->snapd_socket, request->str, request->len, g_task_get_cancellable (task), &error);
    if (n_written < 0)
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_WRITE_ERROR,
                                 "Failed to write to snapd: %s",
                                 error->message);
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
parse_result (const gchar *content_type, const gchar *content, gsize content_length, JsonObject **response, gchar **change_id, GError **error)
{
    g_autoptr(JsonParser) parser = NULL;
    g_autoptr(GError) error_local = NULL;
    JsonObject *root;
    const gchar *type;

    if (content_type == NULL) {
        g_set_error_literal (error,
                             SNAPD_CLIENT_ERROR,
                             SNAPD_CLIENT_ERROR_PARSE_ERROR,
                             "snapd returned no content type");
        return FALSE;
    }
    if (g_strcmp0 (content_type, "application/json") != 0) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_PARSE_ERROR,
                     "snapd returned unexpected content type %s", content_type);
        return FALSE;
    }

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, content, content_length, &error_local)) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_PARSE_ERROR,
                     "Unable to parse snapd response: %s",
                     error_local->message);
        return FALSE;
    }

    if (!JSON_NODE_HOLDS_OBJECT (json_parser_get_root (parser))) {
        g_set_error_literal (error,
                             SNAPD_CLIENT_ERROR,
                             SNAPD_CLIENT_ERROR_PARSE_ERROR,
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
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_LOGIN_REQUIRED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "invalid-auth-data") == 0) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_INVALID_AUTH_DATA,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "two-factor-required") == 0) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_TWO_FACTOR_REQUIRED,
                                 message);
            return FALSE;
        }
        else if (g_strcmp0 (kind, "two-factor-failed") == 0) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_TWO_FACTOR_FAILED,
                                 message);
            return FALSE;
        }
        else if (status_code == SOUP_STATUS_BAD_REQUEST) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_BAD_REQUEST,
                                 message);
            return FALSE;
        }
        else {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_GENERAL_ERROR,
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

static gboolean
parse_get_system_information_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(SnapdSystemInformation) system_information = NULL;
    JsonObject *os_release;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "No result returned");
        return TRUE;
    }

    os_release = get_object (result, "os-release");
    system_information = g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                                       "on-classic", get_bool (result, "on-classic", FALSE),
                                       "os-id", os_release != NULL ? get_string (os_release, "id", NULL) : NULL,
                                       "os-version", os_release != NULL ? get_string (os_release, "version-id", NULL) : NULL,
                                       "series", get_string (result, "series", NULL),
                                       "version", get_string (result, "version", NULL),
                                       NULL);
    g_task_return_pointer (task, g_steal_pointer (&system_information), g_object_unref);

    return TRUE;
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
    g_autoptr(JsonArray) prices = NULL;  
    g_autoptr(GPtrArray) apps_array = NULL;
    g_autoptr(GPtrArray) prices_array = NULL;  
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
        g_autoptr(SnapdApp) app = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "Unexpected app type");
            return NULL;
        }

        a = json_node_get_object (node);
        app = g_object_new (SNAPD_TYPE_APP,
                            "name", get_string (a, "name", NULL),
                            NULL);
        g_ptr_array_add (apps_array, g_steal_pointer (&app));
    }

    prices = get_array (object, "prices");
    prices_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (prices); i++) {
        JsonNode *node = json_array_get_element (apps, i);
        JsonObject *p;
        g_autoptr(SnapdPrice) price = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error_literal (error,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "Unexpected price type");
            return NULL;
        }

        p = json_node_get_object (node);      
        price = g_object_new (SNAPD_TYPE_PRICE,
                              "amount", get_string (p, "price", NULL),
                              "currency", get_string (p, "currency", NULL),                              
                              NULL);
        g_ptr_array_add (prices_array, g_steal_pointer (&price));
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
                         "install-date", get_string (object, "install-date", NULL),
                         "installed-size", get_int (object, "installed-size", 0),
                         "name", get_string (object, "name", NULL),
                         "prices", prices_array,
                         "private", get_bool (object, "private", FALSE),
                         "revision", get_string (object, "revision", NULL),
                         "snap-type", snap_type,
                         "status", snap_status,
                         "summary", get_string (object, "summary", NULL),
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
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
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

static gboolean
parse_get_icon_response (GTask *task, guint code, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    const gchar *content_type;
    g_autoptr(SnapdIcon) icon = NULL;
    g_autoptr(GBytes) data = NULL;

    content_type = soup_message_headers_get_content_type (headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        GError *error = NULL;

        if (!parse_result (content_type, content, content_length, NULL, NULL, &error)) {
            g_task_return_error (task, error);
            return TRUE;
        }
    }

    if (code != SOUP_STATUS_OK) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "Got response %u retrieving icon", code);
    }

    data = g_bytes_new (content, content_length);
    icon = g_object_new (SNAPD_TYPE_ICON,
                         "mime-type", content_type,
                         "data", data,
                         NULL);

    g_task_return_pointer (task, g_steal_pointer (&icon), g_object_unref);

    return TRUE;
}

static gboolean
parse_list_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;
    GPtrArray *snaps;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    array = get_array (response, "result");
    snaps = parse_snap_array (array, &error);
    if (snaps == NULL) {
        g_task_return_error (task, error);
        return TRUE;
    }

    g_task_return_pointer (task, g_steal_pointer (&snaps), (GDestroyNotify) g_ptr_array_unref);

    return TRUE;
}

static gboolean
parse_list_one_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(SnapdSnap) snap = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "No result returned");
        return TRUE;
    }

    snap = parse_snap (result, &error);
    if (snap == NULL) {
        g_task_return_error (task, error);
        return TRUE;
    }

    g_task_return_pointer (task, g_steal_pointer (&snap), g_object_unref);

    return TRUE;
}

static void
free_get_interfaces_result (GetInterfacesResult *result)
{
    g_ptr_array_unref (result->plugs);
    g_ptr_array_unref (result->slots);  
    g_slice_free (GetInterfacesResult, result);
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
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
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

static gboolean
parse_get_interfaces_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    JsonObject *result;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;  
    guint i;
    GetInterfacesResult *r;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "No result returned");
        return TRUE;
    }

    plugs = get_array (result, "plugs");
    plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);
        JsonObject *object;
        g_autoptr(GPtrArray) connections = NULL;
        g_autoptr(SnapdPlug) plug = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "Unexpected plug type");
            return TRUE;
        }
        object = json_node_get_object (node);

        connections = get_connections (object, "slot", &error);
        if (connections == NULL) {
            g_task_return_error (task, error);
            return TRUE;
        }

        plug = g_object_new (SNAPD_TYPE_PLUG,
                             "name", get_string (object, "plug", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
                             "connections", g_steal_pointer (&connections),
                             // FIXME: apps
                             // FIXME: attrs
                             NULL);
        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = get_array (result, "slots");
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);
        JsonObject *object;
        g_autoptr(GPtrArray) connections = NULL;
        g_autoptr(SnapdSlot) slot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "Unexpected slot type");
            return TRUE;
        }
        object = json_node_get_object (node);

        connections = get_connections (object, "plug", &error);
        if (connections == NULL) {
            g_task_return_error (task, error);
            return TRUE;
        }

        slot = g_object_new (SNAPD_TYPE_SLOT,
                             "name", get_string (object, "slot", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
                             "connections", g_steal_pointer (&connections),
                             // FIXME: apps
                             // FIXME: attrs
                             NULL);
        g_ptr_array_add (slot_array, g_steal_pointer (&slot));
    }

    r = g_slice_new (GetInterfacesResult);
    r->plugs = g_steal_pointer (&plug_array);
    r->slots = g_steal_pointer (&slot_array);
    g_task_return_pointer (task, r, (GDestroyNotify) free_get_interfaces_result);

    return TRUE;
}

static gboolean
async_poll_cb (gpointer data)
{
    GTask *task = data;
    RequestData *request_data;
    g_autofree gchar *path = NULL;

    request_data = g_task_get_task_data (task);
    path = g_strdup_printf ("/v2/changes/%s", request_data->change_id);
    send_request (task, NULL, "GET", path, NULL, NULL);

    request_data->poll_timer = 0;
    return G_SOURCE_REMOVE;
}

static gboolean
async_timeout_cb (gpointer data)
{
    GTask *task = data;
    RequestData *request_data;

    request_data = g_task_get_task_data (task);  
    request_data->timeout_timer = 0;

    g_task_return_new_error (task,
                             SNAPD_CLIENT_ERROR,
                             SNAPD_CLIENT_ERROR_READ_ERROR,
                             "Timeout waiting for snapd");

    return G_SOURCE_REMOVE;
}

static gboolean
parse_async_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autofree gchar *change_id = NULL;
    RequestData *request_data;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, &change_id, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    request_data = g_task_get_task_data (task);

    /* Cancel any pending timeout */
    if (request_data->timeout_timer != 0)
        g_source_remove (request_data->timeout_timer);
    request_data->timeout_timer = 0;

    /* First run we expect the change ID, following runs are updates */
    if (request_data->change_id == NULL) {
         if (change_id == NULL) {
             g_task_return_new_error (task,
                                      SNAPD_CLIENT_ERROR,
                                      SNAPD_CLIENT_ERROR_READ_ERROR,
                                      "No async response received");
             return TRUE;
         }

         request_data->change_id = g_strdup (change_id);
    }
    else {
        gboolean ready;
        JsonObject *result;

        if (change_id != NULL) {
             g_task_return_new_error (task,
                                      SNAPD_CLIENT_ERROR,
                                      SNAPD_CLIENT_ERROR_READ_ERROR,
                                      "Duplicate async response received");
             return TRUE;
        }
      
        result = get_object (response, "result");
        if (result == NULL) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "No async result returned");
            return TRUE;
        }

        if (g_strcmp0 (request_data->change_id, get_string (result, "id", NULL)) != 0) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "Unexpected change ID returned");
            return TRUE;
        }

        /* Update caller with progress */
        if (request_data->progress_callback != NULL) {
            g_autoptr(JsonArray) array = NULL;
            guint i;
            g_autoptr(GPtrArray) tasks = NULL;
            g_autoptr(SnapdTask) main_task = NULL;

            array = get_array (result, "tasks");
            tasks = g_ptr_array_new_with_free_func (g_object_unref);
            for (i = 0; i < json_array_get_length (array); i++) {
                JsonNode *node = json_array_get_element (array, i);
                JsonObject *object, *progress;
                g_autoptr(SnapdTask) t = NULL;

                if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
                    g_task_return_new_error (task,
                                             SNAPD_CLIENT_ERROR,
                                             SNAPD_CLIENT_ERROR_READ_ERROR,
                                             "Unexpected task type");
                    return TRUE;
                }
                object = json_node_get_object (node);
                progress = get_object (object, "progress");

                t = g_object_new (SNAPD_TYPE_TASK,
                                  "id", get_string (object, "id", NULL),
                                  "kind", get_string (object, "kind", NULL),
                                  "summary", get_string (object, "summary", NULL),
                                  "status", get_string (object, "status", NULL),
                                  "ready", get_bool (object, "ready", FALSE),
                                  "progress-done", progress != NULL ? get_int (progress, "done", 0) : 0,
                                  "progress-total", progress != NULL ? get_int (progress, "total", 0) : 0,
                                  "spawn-time", get_string (object, "spawn-time", NULL),
                                  "ready-time", get_string (object, "ready-time", NULL),
                                  NULL);
                g_ptr_array_add (tasks, g_steal_pointer (&t));
            }

            main_task = g_object_new (SNAPD_TYPE_TASK,
                                      "id", get_string (result, "id", NULL),
                                      "kind", get_string (result, "kind", NULL),
                                      "summary", get_string (result, "summary", NULL),
                                      "status", get_string (result, "status", NULL),
                                      "ready", get_bool (result, "ready", FALSE),
                                      "spawn-time", get_string (result, "spawn-time", NULL),
                                      "ready-time", get_string (result, "ready-time", NULL),                                     
                                      NULL);

            request_data->progress_callback (g_task_get_source_object (task), main_task, tasks, request_data->progress_callback_data);
        }

        ready = get_bool (result, "ready", FALSE);
        if (ready) {
            g_task_return_boolean (task, TRUE);
            return TRUE;
        }
    }

    /* Poll for updates */
    if (request_data->poll_timer != 0)
        g_source_remove (request_data->poll_timer);
    request_data->poll_timer = g_timeout_add (ASYNC_POLL_TIME, async_poll_cb, task);
    request_data->timeout_timer = g_timeout_add (ASYNC_POLL_TIMEOUT, async_timeout_cb, task);  
    return FALSE;
}

static gboolean
parse_connect_interface_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_disconnect_interface_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_login_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    JsonObject *result;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(JsonArray) discharges = NULL;
    g_autoptr(GPtrArray) discharge_array = NULL;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "No result returned");
        return TRUE;
    }

    discharges = get_array (result, "discharges");
    discharge_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (discharges); i++) {
        JsonNode *node = json_array_get_element (discharges, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "Unexpected discharge type");
            return TRUE;
        }

        g_ptr_array_add (discharge_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (discharge_array, NULL);
    auth_data = snapd_auth_data_new (get_string (result, "macaroon", NULL), (gchar **) discharge_array->pdata);
    g_task_return_pointer (task, g_steal_pointer (&auth_data), g_object_unref);

    return TRUE;
}

static gboolean
parse_find_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) array = NULL;  
    g_autoptr(GPtrArray) snaps = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    array = get_array (response, "result");
    snaps = parse_snap_array (array, &error);
    if (snaps == NULL) {
        g_task_return_error (task, error);
        return TRUE;
    }

    g_task_return_pointer (task, g_steal_pointer (&snaps), (GDestroyNotify) g_ptr_array_unref);

    return TRUE;
}

static void
free_get_payment_methods_result (GetPaymentMethodsResult *result)
{
    g_ptr_array_unref (result->methods);
    g_slice_free (GetPaymentMethodsResult, result);
}

static gboolean
parse_get_payment_methods_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    gboolean allows_automatic_payment;
    GPtrArray *payment_methods;
    GError *error = NULL;
    JsonObject *result;
    g_autoptr(JsonArray) methods = NULL;
    guint i;
    GetPaymentMethodsResult *r;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = get_object (response, "result");
    if (result == NULL) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "No result returned");
        return TRUE;
    }

    allows_automatic_payment = get_bool (result, "allows-automatic-payment", FALSE);
    payment_methods = g_ptr_array_new_with_free_func (g_object_unref);
    methods = get_array (result, "methods");
    for (i = 0; i < json_array_get_length (methods); i++) {
        JsonNode *node = json_array_get_element (methods, i);
        JsonObject *object;
        g_autoptr(JsonArray) currencies = NULL;
        g_autoptr(GPtrArray) currencies_array = NULL;
        guint j;
        SnapdPaymentMethod *payment_method;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_task_return_new_error (task,
                                     SNAPD_CLIENT_ERROR,
                                     SNAPD_CLIENT_ERROR_READ_ERROR,
                                     "Unexpected method type");
            return TRUE;
        }
        object = json_node_get_object (node);

        currencies = get_array (object, "currencies");
        currencies_array = g_ptr_array_new ();
        for (j = 0; j < json_array_get_length (currencies); j++) {
            JsonNode *node = json_array_get_element (currencies, j);

            if (json_node_get_value_type (node) != G_TYPE_STRING) {
                g_task_return_new_error (task,
                                         SNAPD_CLIENT_ERROR,
                                         SNAPD_CLIENT_ERROR_READ_ERROR,
                                         "Unexpected currency type");
                return TRUE;
            }

            g_ptr_array_add (currencies_array, (gchar *) json_node_get_string (node));
        }
        g_ptr_array_add (currencies_array, NULL);
        payment_method = g_object_new (SNAPD_TYPE_PAYMENT_METHOD,
                                       "backend-id", get_string (object, "backend-id", NULL),
                                       "currencies", (gchar **) currencies_array->pdata,
                                       "description", get_string (object, "description", NULL),
                                       "id", get_int (object, "id", 0),
                                       "preferred", get_bool (object, "preferred", FALSE),
                                       "requires-interaction", get_bool (object, "requires-interaction", FALSE),
                                       NULL);
        g_ptr_array_add (payment_methods, payment_method);
    }

    r = g_slice_new (GetPaymentMethodsResult);
    r->allows_automatic_payment = allows_automatic_payment;
    r->methods = g_steal_pointer (&payment_methods);
    g_task_return_pointer (task, r, (GDestroyNotify) free_get_payment_methods_result);

    return TRUE;
}

static gboolean
parse_buy_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, NULL, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    g_task_return_boolean (task, TRUE);

    return TRUE;
}

static gboolean
parse_install_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_refresh_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_remove_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_enable_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static gboolean
parse_disable_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    return parse_async_response (task, headers, content, content_length);
}

static void
parse_response (SnapdClient *client, guint code, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    RequestData *request_data;
    gboolean completed;

    /* Get which request this response is for */
    if (priv->tasks == NULL) {
        g_warning ("Ignoring unexpected response");
        return;
    }
    task = g_list_nth_data (priv->tasks, 0);

    request_data = g_task_get_task_data (task);
    switch (request_data->request_type)
    {
    case SNAPD_REQUEST_GET_SYSTEM_INFORMATION:
        completed = parse_get_system_information_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_ICON:
        completed = parse_get_icon_response (task, code, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LIST:
        completed = parse_list_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LIST_ONE:
        completed = parse_list_one_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_INTERFACES:
        completed = parse_get_interfaces_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_CONNECT_INTERFACE:
        completed = parse_connect_interface_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_DISCONNECT_INTERFACE:
        completed = parse_disconnect_interface_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LOGIN:
        completed = parse_login_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_FIND:
        completed = parse_find_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_GET_PAYMENT_METHODS:
        completed = parse_get_payment_methods_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_BUY:
        completed = parse_buy_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_INSTALL:
        completed = parse_install_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_REFRESH:
        completed = parse_refresh_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_REMOVE:
        completed = parse_remove_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_ENABLE:
        completed = parse_enable_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_DISABLE:
        completed = parse_disable_response (task, headers, content, content_length);
        break;
    default:
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_GENERAL_ERROR,
                                 "Unknown task");
        completed = TRUE;
        break;
    }

    if (completed)
    {
        priv->tasks = g_list_remove_link (priv->tasks, g_list_first (priv->tasks));
        g_object_unref (task);
    }
}

static gssize
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
        // FIXME: Cancel all tasks
        //g_set_error (error,
        //             SNAPD_CLIENT_ERROR,
        //             SNAPD_CLIENT_ERROR_READ_ERROR,
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
            // FIXME: Cancel all tasks
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
    return read_from_snapd (client, NULL, FALSE); // FIXME: Use Cancellable from first task?
}

static void
wait_for_task (GTask *task)
{
    while (!g_task_get_completed (task))
        g_main_context_iteration (g_task_get_context (task), TRUE);
}

/**
 * snapd_client_connect_sync:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @error: a #GError or %NULL
 *
 * Connect to snapd.
 *
 * Returns: %TRUE if successfully connected to snapd.
 */
gboolean
snapd_client_connect_sync (SnapdClient *client,
                           GCancellable *cancellable, GError **error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    g_autoptr(GSocketAddress) address = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (priv->snapd_socket == NULL, FALSE);

    priv->snapd_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                       G_SOCKET_TYPE_STREAM,
                                       G_SOCKET_PROTOCOL_DEFAULT,
                                       &error_local);
    if (priv->snapd_socket == NULL) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
                     "Unable to open snapd socket: %s",
                     error_local->message);
        return FALSE;
    }
    address = g_unix_socket_address_new (SNAPD_SOCKET);
    if (!g_socket_connect (priv->snapd_socket, address, cancellable, &error_local)) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
                     "Unable to connect snapd socket: %s",
                     error_local->message);
        g_clear_object (&priv->snapd_socket);
        return FALSE;
    }

    priv->read_source = g_socket_create_source (priv->snapd_socket, G_IO_IN, NULL);
    g_source_set_callback (priv->read_source, (GSourceFunc) read_cb, client, NULL);
    g_source_attach (priv->read_source, NULL);

    return TRUE;
}

static void
free_request_data (gpointer data)
{
    RequestData *d = data;
    g_free (d->change_id);
    if (d->poll_timer != 0)
        g_source_remove (d->poll_timer);
    if (d->timeout_timer != 0)
        g_source_remove (d->timeout_timer);
    g_slice_free (RequestData, d);
}

static GTask *
make_task (SnapdClient *client, RequestType request_type,
           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    RequestData *data;

    task = g_task_new (client, cancellable, callback, user_data);
    data = g_slice_new0 (RequestData);
    data->request_type = request_type;
    data->progress_callback = progress_callback;
    data->progress_callback_data = progress_callback_data;
    g_task_set_task_data (task, data, free_request_data);
    priv->tasks = g_list_append (priv->tasks, task);

    return task;
}

static GTask *
make_get_system_information_task (SnapdClient *client,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    task = make_task (client, SNAPD_REQUEST_GET_SYSTEM_INFORMATION, NULL, NULL, cancellable, callback, user_data);
    send_request (task, NULL, "GET", "/v2/system-info", NULL, NULL);

    return task;
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
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_get_system_information_task (client, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_get_system_information_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_get_system_information_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Request system information asynchronously from snapd.
 */
void
snapd_client_get_system_information_async (SnapdClient *client,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_system_information_task (client, cancellable, callback, user_data);
}

/**
 * snapd_client_get_system_information_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdSystemInformation or %NULL on error.
 */
SnapdSystemInformation *
snapd_client_get_system_information_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_list_one_task (SnapdClient *client,
                    const gchar *name,
                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_LIST_ONE, NULL, NULL, cancellable, callback, user_data);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, NULL, "GET", path, NULL, NULL);

    return task;
}

/**
 * snapd_client_list_one_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 */
SnapdSnap *
snapd_client_list_one_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_list_one_task (client, name, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_list_one_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_list_one_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_list_one_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_one_task (client, name, cancellable, callback, user_data);
}

/**
 * snapd_client_list_one_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 */
SnapdSnap *
snapd_client_list_one_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_get_icon_task (SnapdClient *client,
                    const gchar *name,
                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_GET_ICON, NULL, NULL, cancellable, callback, user_data);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/icons/%s/icon", escaped);
    send_request (task, NULL, "GET", path, NULL, NULL);

    return task;
}

/**
 * snapd_client_get_icon_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 */
SnapdIcon *
snapd_client_get_icon_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_get_icon_task (client, name, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_get_icon_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_get_icon_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_get_icon_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_icon_task (client, name, cancellable, callback, user_data);
}

/**
 * snapd_client_get_icon_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 */
SnapdIcon *
snapd_client_get_icon_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_list_task (SnapdClient *client,
                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    task = make_task (client, SNAPD_REQUEST_LIST, NULL, NULL, cancellable, callback, user_data);
    send_request (task, NULL, "GET", "/v2/snaps", NULL, NULL);

    return task;
}

/**
 * snapd_client_list_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_list_sync (SnapdClient *client,
                        GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_list_task (client, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_list_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_list_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_list_async (SnapdClient *client,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_task (client, cancellable, callback, user_data);
}

/**
 * snapd_client_list_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_list_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_get_interfaces_task (SnapdClient *client,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    task = make_task (client, SNAPD_REQUEST_GET_INTERFACES, NULL, NULL, cancellable, callback, user_data);
    send_request (task, NULL, "GET", "/v2/interfaces", NULL, NULL);

    return task;
}

/**
 * snapd_client_get_interfaces_sync:
 * @client: a #SnapdClient.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the plug array or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the slot array or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_get_interfaces_sync (SnapdClient *client,
                                  GPtrArray **plugs, GPtrArray **slots,
                                  GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    task = g_object_ref (make_get_interfaces_task (client, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_get_interfaces_finish (client, G_ASYNC_RESULT (task), plugs, slots, error);
}

/**
 * snapd_client_get_interfaces_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_get_interfaces_async (SnapdClient *client,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_interfaces_task (client, cancellable, callback, user_data);
}

/**
 * snapd_client_get_interfaces_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the plug array or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the slot array or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_get_interfaces_finish (SnapdClient *client, GAsyncResult *result,
                                    GPtrArray **plugs, GPtrArray **slots,
                                    GError **error)
{
    GetInterfacesResult *r;

    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);

    r = g_task_propagate_pointer (G_TASK (result), error);
    if (r == NULL)
        return FALSE;

    if (plugs)
       *plugs = g_ptr_array_ref (r->plugs);
    if (slots)
       *slots = g_ptr_array_ref (r->slots);
    free_get_interfaces_result (r);

    return TRUE;
}

static GTask *
make_interface_task (SnapdClient *client,
                     SnapdAuthData *auth_data,
                     RequestType request,
                     const gchar *action,
                     const gchar *plug_snap, const gchar *plug_name,
                     const gchar *slot_snap, const gchar *slot_name,
                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = make_task (client, request, progress_callback, progress_callback_data, cancellable, callback, user_data);

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

    send_request (task, auth_data, "POST", "/v2/interfaces", "application/json", data);

    return task;
}

static GTask *
make_connect_interface_task (SnapdClient *client,
                             SnapdAuthData *auth_data,
                             const gchar *plug_snap, const gchar *plug_name,
                             const gchar *slot_snap, const gchar *slot_name,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_task (client, auth_data, SNAPD_REQUEST_CONNECT_INTERFACE,
                                "connect",
                                plug_snap, plug_name,
                                slot_snap, slot_name,
                                progress_callback, progress_callback_data,
                                cancellable, callback, user_data);
}

/**
 * snapd_client_connect_interface_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 */
gboolean
snapd_client_connect_interface_sync (SnapdClient *client,
                                     SnapdAuthData *auth_data,
                                     const gchar *plug_snap, const gchar *plug_name,
                                     const gchar *slot_snap, const gchar *slot_name,
                                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                     GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    task = g_object_ref (make_connect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_connect_interface_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_connect_interface_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_connect_interface_async (SnapdClient *client,
                                      SnapdAuthData *auth_data,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_connect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_connect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_connect_interface_finish (SnapdClient *client,
                                       GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_disconnect_interface_task (SnapdClient *client,
                                SnapdAuthData *auth_data,
                                const gchar *plug_snap, const gchar *plug_name,
                                const gchar *slot_snap, const gchar *slot_name,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_task (client, auth_data, SNAPD_REQUEST_DISCONNECT_INTERFACE,
                                "disconnect",
                                plug_snap, plug_name,
                                slot_snap, slot_name,
                                progress_callback, progress_callback_data,
                                cancellable, callback, user_data);
}

/**
 * snapd_client_disconnect_interface_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disconnect_interface_sync (SnapdClient *client,
                                        SnapdAuthData *auth_data,
                                        const gchar *plug_snap, const gchar *plug_name,
                                        const gchar *slot_snap, const gchar *slot_name,
                                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                        GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    task = g_object_ref (make_disconnect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_disconnect_interface_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_disconnect_interface_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_disconnect_interface_async (SnapdClient *client,
                                         SnapdAuthData *auth_data,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_disconnect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_disconnect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disconnect_interface_finish (SnapdClient *client,
                                          GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_login_task (SnapdClient *client,
                 const gchar *username, const gchar *password, const gchar *otp,
                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = make_task (client, SNAPD_REQUEST_LOGIN, NULL, NULL, cancellable, callback, user_data);

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

    send_request (task, NULL, "POST", "/v2/login", "application/json", data);

    return task;
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
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 */
SnapdAuthData *
snapd_client_login_sync (SnapdClient *client,
                         const gchar *username, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (username != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    task = g_object_ref (make_login_task (client, username, password, otp, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_login_finish (client, G_ASYNC_RESULT (task), error);
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
 */
void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_login_task (client, username, password, otp, cancellable, callback, user_data);
}

/**
 * snapd_client_login_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 */
SnapdAuthData *
snapd_client_login_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_find_task (SnapdClient *client,
                SnapdAuthData *auth_data,
                SnapdFindFlags flags, const gchar *query,
                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autoptr(GString) path = NULL;
    g_autofree gchar *escaped = NULL;  

    task = make_task (client, SNAPD_REQUEST_FIND, NULL, NULL, cancellable, callback, user_data);
    path = g_string_new ("/v2/find");
    escaped = soup_uri_encode (query, NULL);

    if ((flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
        g_string_append_printf (path, "?name=%s", escaped);
    else
        g_string_append_printf (path, "?q=%s", escaped);

    if ((flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        g_string_append_printf (path, "&select=private");
    else if ((flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        g_string_append_printf (path, "&select=refresh");

    send_request (task, auth_data, "GET", path->str, NULL, NULL);

    return task;
}

/**
 * snapd_client_find_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: query string to send.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_find_sync (SnapdClient *client,
                        SnapdAuthData *auth_data,
                        SnapdFindFlags flags, const gchar *query,
                        GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (query != NULL, NULL);

    task = g_object_ref (make_find_task (client, auth_data, flags, query, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_find_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_find_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: query string to send.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_find_async (SnapdClient *client,
                         SnapdAuthData *auth_data,                         
                         SnapdFindFlags flags, const gchar *query,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (query != NULL);
    make_find_task (client, auth_data, flags, query, cancellable, callback, user_data);
}

/**
 * snapd_client_find_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 */
GPtrArray *
snapd_client_find_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
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

static GTask *
make_install_task (SnapdClient *client,
                   SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_INSTALL, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("install", channel);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

/**
 * snapd_client_install_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_install_sync (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_install_task (client, auth_data, name, channel, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_install_finish (client, G_ASYNC_RESULT (task), error);
}

/**
 * snapd_client_install_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_install_async (SnapdClient *client,
                            SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_install_task (client, auth_data, name, channel, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_install_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_install_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_refresh_task (SnapdClient *client,
                   SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_REFRESH, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("refresh", channel);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

/**
 * snapd_client_refresh_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_refresh_sync (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_refresh_task (client, auth_data, name, channel, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_refresh_finish (client, G_ASYNC_RESULT (task), error);
}


/**
 * snapd_client_refresh_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_refresh_async (SnapdClient *client,
                            SnapdAuthData *auth_data, const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);
    make_refresh_task (client, auth_data, name, channel, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_refresh_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_refresh_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_remove_task (SnapdClient *client,
                  SnapdAuthData *auth_data, const gchar *name,
                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_REMOVE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("remove", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

/**
 * snapd_client_remove_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_remove_sync (SnapdClient *client,
                          SnapdAuthData *auth_data, const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_remove_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_remove_finish (client, G_ASYNC_RESULT (task), error);
}


/**
 * snapd_client_remove_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_remove_async (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    make_remove_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_remove_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_remove_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_enable_task (SnapdClient *client,
                  SnapdAuthData *auth_data, const gchar *name,
                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_ENABLE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("enable", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

/**
 * snapd_client_enable_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_enable_sync (SnapdClient *client,
                          SnapdAuthData *auth_data, const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_enable_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_enable_finish (client, G_ASYNC_RESULT (task), error);
}


/**
 * snapd_client_enable_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_enable_async (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    make_enable_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_enable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_enable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_disable_task (SnapdClient *client,
                   SnapdAuthData *auth_data, const gchar *name,
                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = make_task (client, SNAPD_REQUEST_DISABLE, progress_callback, progress_callback_data, cancellable, callback, user_data);
    data = make_action_data ("disable", NULL);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

/**
 * snapd_client_disable_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disable_sync (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_disable_task (client, auth_data, name,
                                            progress_callback, progress_callback_data,
                                            cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_disable_finish (client, G_ASYNC_RESULT (task), error);
}


/**
 * snapd_client_disable_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_disable_async (SnapdClient *client,
                            SnapdAuthData *auth_data, const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    make_disable_task (client, auth_data, name,
                       progress_callback, progress_callback_data,
                       cancellable, callback, user_data);
}

/**
 * snapd_client_disable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_disable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

static GTask *
make_get_payment_methods_task (SnapdClient *client,
                               SnapdAuthData *auth_data,
                               GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;

    task = make_task (client, SNAPD_REQUEST_GET_PAYMENT_METHODS, NULL, NULL, cancellable, callback, user_data);
    send_request (task, auth_data, "GET", "/v2/buy/methods", NULL, NULL);

    return task;
}

/**
 * snapd_client_get_payment_methods_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @allows_automatic_payment: (allow-none): the location to store if automatic payments are allowed or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdPaymentMethod): an array of #SnapdPaymentMethod or %NULL on error.
 */
GPtrArray *
snapd_client_get_payment_methods_sync (SnapdClient *client,
                                       SnapdAuthData *auth_data,
                                       gboolean *allows_automatic_payment,
                                       GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_get_payment_methods_task (client, auth_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_get_payment_methods_finish (client, G_ASYNC_RESULT (task), allows_automatic_payment, error);
}


/**
 * snapd_client_get_payment_methods_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_get_payment_methods_async (SnapdClient *client,
                                        SnapdAuthData *auth_data,
                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_payment_methods_task (client, auth_data, cancellable, callback, user_data);
}

/**
 * snapd_client_get_payment_methods_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @allows_automatic_payment: (allow-none):
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: (transfer container) (element-type SnapdPaymentMethod): an array of #SnapdPaymentMethod or %NULL on error.
 */
GPtrArray *
snapd_client_get_payment_methods_finish (SnapdClient *client, GAsyncResult *result, gboolean *allows_automatic_payment, GError **error)
{
    GetPaymentMethodsResult *r;
    GPtrArray *methods;

    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);

    r = g_task_propagate_pointer (G_TASK (result), error);
    if (r == NULL)
        return NULL;

    if (allows_automatic_payment)
        *allows_automatic_payment = r->allows_automatic_payment;
    methods = g_ptr_array_ref (r->methods);
    free_get_payment_methods_result (r);

    return methods;
}

static GTask *
make_buy_task (SnapdClient *client,
               SnapdAuthData *auth_data, SnapdSnap *snap, SnapdPrice *price, SnapdPaymentMethod *payment_method,
               GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = make_task (client, SNAPD_REQUEST_BUY, NULL, NULL, cancellable, callback, user_data);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-id");
    json_builder_add_string_value (builder, snapd_snap_get_id (snap));
    json_builder_set_member_name (builder, "snap-name");
    json_builder_add_string_value (builder, snapd_snap_get_name (snap));
    json_builder_set_member_name (builder, "price");
    json_builder_add_string_value (builder, snapd_price_get_amount (price));
    json_builder_set_member_name (builder, "currency");
    json_builder_add_string_value (builder, snapd_price_get_currency (price));
    if (payment_method != NULL) {
        json_builder_set_member_name (builder, "backend-id");
        json_builder_add_string_value (builder, snapd_payment_method_get_backend_id (payment_method));
        json_builder_set_member_name (builder, "method-id");
        json_builder_add_int_value (builder, snapd_payment_method_get_id (payment_method));
    }
    json_builder_end_object (builder);
    data = builder_to_string (builder);

    send_request (task, auth_data, "GET", "/v2/buy/methods", "application/json", data);

    return task;
}

/**
 * snapd_client_buy_sync:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @snap: snap to buy.
 * @price: price to pay.
 * @payment_method: payment method to use.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_buy_sync (SnapdClient *client,
                       SnapdAuthData *auth_data, SnapdSnap *snap, SnapdPrice *price, SnapdPaymentMethod *payment_method,
                       GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), FALSE);
    g_return_val_if_fail (SNAPD_IS_PRICE (price), FALSE);

    task = g_object_ref (make_buy_task (client, auth_data, snap, price, payment_method, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_buy_finish (client, G_ASYNC_RESULT (task), error);
}


/**
 * snapd_client_buy_async:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): authentication data to use.
 * @snap: snap to buy.
 * @price: price to pay.
 * @payment_method: payment method to use.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 */
void
snapd_client_buy_async (SnapdClient *client,
                        SnapdAuthData *auth_data, SnapdSnap *snap, SnapdPrice *price, SnapdPaymentMethod *payment_method,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (SNAPD_IS_SNAP (snap));
    g_return_if_fail (SNAPD_IS_PRICE (price));

    make_buy_task (client, auth_data, snap, price, payment_method, cancellable, callback, user_data);
}

/**
 * snapd_client_buy_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Returns: %TRUE on success or %FALSE on error.
 */
gboolean
snapd_client_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
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

static void
snapd_client_finalize (GObject *object)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (SNAPD_CLIENT (object));

    g_clear_object (&priv->snapd_socket);
    g_list_free_full (priv->tasks, g_object_unref);
    priv->tasks = NULL;
    g_clear_pointer (&priv->read_source, g_source_unref);
    g_byte_array_unref (priv->buffer);
    priv->buffer = NULL;
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
