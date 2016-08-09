#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "snapd-client.h"
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

// snapd API documentation is at https://github.com/snapcore/snapd/blob/master/docs/rest.md

#define SNAPD_SOCKET "/run/snapd.socket"

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
} SnapdRequestType;

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
    g_autoptr (GString) request = NULL;
    gssize n_written;
    g_autoptr(GError) error = NULL;

    // NOTE: Would love to use libsoup but it doesn't support unix sockets
    // https://bugzilla.gnome.org/show_bug.cgi?id=727563

    request = g_string_new ("");
    g_string_append_printf (request, "%s %s HTTP/1.1\r\n", method, path);
    g_string_append (request, "Host:\r\n");
    g_string_append (request, "Connection: keep-alive\r\n");
    if (auth_data) {
        gsize i;
        g_string_append_printf (request, "Authorization: Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (auth_data));
        for (i = 0; i < snapd_auth_data_get_discharge_count (auth_data); i++)
            g_string_append_printf (request, ",discharge=\"%s\"", snapd_auth_data_get_discharge (auth_data, i));
        g_string_append (request, "\r\n");
    }
    if (content_type)
        g_string_append_printf (request, "Content-Type: %s\r\n", content_type);
    if (content)
        g_string_append_printf (request, "Content-Length: %zi\r\n", strlen (content));
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
    if (json_object_has_member (object, name))
        return json_object_get_boolean_member (object, name); // FIXME: Check is a bool
    else
        return default_value;
}

static gint64
get_int (JsonObject *object, const gchar *name, gint64 default_value)
{
    if (json_object_has_member (object, name))
        return json_object_get_int_member (object, name); // FIXME: Check is an int
    else
        return default_value;
}

static const gchar *
get_string (JsonObject *object, const gchar *name, const gchar *default_value)
{
    if (json_object_has_member (object, name))
        return json_object_get_string_member (object, name); // FIXME: Check is a string
    else
        return default_value;
}

static JsonArray *
get_array (JsonObject *object, const gchar *name)
{
    if (json_object_has_member (object, name))
        return json_object_get_array_member (object, name); // FIXME: Check is an array
    else
        return json_array_new ();
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
        JsonObject *result;

        result = json_object_get_object_member (root, "result"); // FIXME: Check is an object
        kind = get_string (result, "kind", NULL);
        message = get_string (result, "message", NULL);

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

    result = json_object_get_object_member (response, "result"); // FIXME: Check is an object
    os_release = json_object_get_object_member (result, "os-release"); // FIXME: Check is an object
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
parse_snap (JsonObject *object)
{
    const gchar *confinement_string;
    SnapdConfinement confinement = SNAPD_CONFINEMENT_UNKNOWN;
    const gchar *snap_type_string;
    SnapdConfinement snap_type = SNAPD_SNAP_TYPE_UNKNOWN;
    const gchar *snap_status_string;
    SnapdSnapStatus snap_status = SNAPD_SNAP_STATUS_UNKNOWN;

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

    return g_object_new (SNAPD_TYPE_SNAP,
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
parse_snap_array (JsonArray *array)
{
    GPtrArray *snaps;
    guint i;

    snaps = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonObject *object = json_array_get_object_element (array, i); // FIXME: Check is an object
        g_ptr_array_add (snaps, parse_snap (object));
    }

    return snaps;
}

static gboolean
parse_get_icon_response (GTask *task, guint code, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(SnapdIcon) icon = NULL;

    if (code != SOUP_STATUS_OK) {
        g_task_return_new_error (task,
                                 SNAPD_CLIENT_ERROR,
                                 SNAPD_CLIENT_ERROR_READ_ERROR,
                                 "Got response %u retrieving icon", code);
    }

    icon = g_object_new (SNAPD_TYPE_ICON,
                         "mime-type", soup_message_headers_get_content_type (headers, NULL),
                         NULL);
    _snapd_icon_set_data (icon, (const guint8 *) content, content_length);

    g_task_return_pointer (task, g_steal_pointer (&icon), g_object_unref);

    return TRUE;
}

static gboolean
parse_list_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    GPtrArray *snaps;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    snaps = parse_snap_array (json_object_get_array_member (response, "result")); // FIXME: Check is an array
    g_task_return_pointer (task, snaps, (GDestroyNotify) g_ptr_array_unref);

    return TRUE;
}

static gboolean
parse_list_one_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    snap = parse_snap (json_object_get_object_member (response, "result")); // FIXME: Check is an object
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

static gboolean
parse_get_interfaces_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    JsonObject *result;
    JsonArray *plugs, *slots;
    guint i, j;
    GetInterfacesResult *r;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = json_object_get_object_member (response, "result"); // FIXME: Check is an object
    plugs = json_object_get_array_member (result, "plugs"); // FIXME: Check is an array
    plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (plugs); i++) {
        JsonObject *object = json_array_get_object_element (plugs, i); // FIXME: Check is an object
        g_autoptr(SnapdPlug) plug = NULL;
        JsonArray *connections;

        plug = g_object_new (SNAPD_TYPE_PLUG,
                             "name", get_string (object, "plug", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
                             // FIXME: apps
                             // FIXME: attrs
                             NULL);

        connections = get_array (object, "connections");
        for (j = 0; j < json_array_get_length (connections); j++) {
            JsonObject *c = json_array_get_object_element (connections, j); // FIXME: Check is an object
            SnapdConnection *connection;

            connection = g_object_new (SNAPD_TYPE_CONNECTION,
                                       "name", get_string (c, "slot", NULL),
                                       "snap", get_string (c, "snap", NULL),
                                       NULL);
            g_ptr_array_add (snapd_plug_get_connections (plug), connection);
        }

        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = json_object_get_array_member (result, "slots"); // FIXME: Check is an array
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonObject *object = json_array_get_object_element (slots, i); // FIXME: Check is an object
        g_autoptr(SnapdSlot) slot = NULL;
        JsonArray *connections;

        slot = g_object_new (SNAPD_TYPE_SLOT,
                             "name", get_string (object, "slot", NULL),
                             "snap", get_string (object, "snap", NULL),
                             "interface", get_string (object, "interface", NULL),
                             "label", get_string (object, "label", NULL),
                             // FIXME: apps
                             // FIXME: attrs
                             NULL);

        connections = get_array (object, "connections");      
        for (j = 0; j < json_array_get_length (connections); j++) {
            JsonObject *c = json_array_get_object_element (connections, j); // FIXME: Check is an object
            SnapdConnection *connection;

            connection = g_object_new (SNAPD_TYPE_CONNECTION,
                                       "name", get_string (c, "plug", NULL),
                                       "snap", get_string (c, "snap", NULL),
                                       NULL);
            g_ptr_array_add (snapd_slot_get_connections (slot), connection);
        }

        g_ptr_array_add (slot_array, g_steal_pointer (&slot));
    }

    r = g_slice_new (GetInterfacesResult);
    r->plugs = g_steal_pointer (&plug_array);
    r->slots = g_steal_pointer (&slot_array);
    g_task_return_pointer (task, r, (GDestroyNotify) free_get_interfaces_result);

    return TRUE;
}

static gboolean
parse_async_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    g_autofree gchar *change_id = NULL;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, &change_id, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    if (change_id == NULL) {      
        gboolean ready;
        JsonObject *result;

        result = json_object_get_object_member (response, "result"); // FIXME: Check is an object

        ready = get_bool (result, "ready", FALSE);
        g_printerr ("%d\n", ready);
        if (ready) {
            g_task_return_boolean (task, TRUE);
            return TRUE;
        }

        change_id = g_strdup (get_string (result, "id", NULL));
    }

    /* Poll for changes */
    // FIXME: Should wait for 100ms or so
    if (change_id != NULL) {
        g_autofree gchar *path = NULL;

        path = g_strdup_printf ("/v2/changes/%s", change_id);
        send_request (task, NULL, "GET", path, NULL, NULL);
        return FALSE;
    }

    // FIXME: Throw some sort of error
    return TRUE;
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
    JsonArray *discharges;
    guint i;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    result = json_object_get_object_member (response, "result"); // FIXME: Check is an object
    auth_data = snapd_auth_data_new ();
    snapd_auth_data_set_macaroon (auth_data, get_string (result, "macaroon", NULL));
    discharges = json_object_get_array_member (result, "discharges"); // FIXME: Check is an array
    for (i = 0; i < json_array_get_length (discharges); i++)
        snapd_auth_data_add_discharge (auth_data, json_array_get_string_element (discharges, i));
    g_task_return_pointer (task, g_steal_pointer (&auth_data), g_object_unref);

    return TRUE;
}

static gboolean
parse_find_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) response = NULL;
    GPtrArray *snaps;
    GError *error = NULL;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    snaps = parse_snap_array (json_object_get_array_member (response, "result")); // FIXME: Check is an array
    g_task_return_pointer (task, snaps, (GDestroyNotify) g_ptr_array_unref);

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
    JsonObject *list_object;
    JsonArray *methods;
    guint i;
    GetPaymentMethodsResult *r;

    if (!parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &response, NULL, &error)) {
        g_task_return_error (task, error);
        return TRUE;
    }

    list_object = json_object_get_object_member (response, "result"); // FIXME: Check is an object
    allows_automatic_payment = get_bool (list_object, "allows-automatic-payment", FALSE);
    payment_methods = g_ptr_array_new_with_free_func (g_object_unref);
    methods = json_object_get_array_member (list_object, "methods"); // FIXME: Check is an array
    for (i = 0; i < json_array_get_length (methods); i++) {
        JsonObject *object = json_array_get_object_element (methods, i); // FIXME: Check is an object
        SnapdPaymentMethod *payment_method;

        payment_method = g_object_new (SNAPD_TYPE_PAYMENT_METHOD,
                                       "backend-id", get_string (object, "backend-id", NULL),
                                       // FIXME: currencies
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
    SnapdRequestType request_type;
    gboolean completed;

    /* Get which request this response is for */
    if (priv->tasks == NULL) {
        g_warning ("Ignoring unexpected response");
        return;
    }
    task = g_list_nth_data (priv->tasks, 0);

    g_printerr ("PARSE %.*s\n", (int) content_length, content);

    request_type = GPOINTER_TO_INT (g_task_get_task_data (task));
    switch (request_type)
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
        priv->tasks = g_list_remove (priv->tasks, g_list_first (priv->tasks));
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
    g_autoptr (SoupMessageHeaders) headers = NULL;
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
        memmove (priv->buffer, priv->buffer + header_length + content_length, priv->n_read - (header_length + content_length));
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

static GTask *
make_get_system_information_task (SnapdClient *client,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_GET_SYSTEM_INFORMATION), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    send_request (task, NULL, "GET", "/v2/system-info", NULL, NULL);

    return task;
}

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

void
snapd_client_get_system_information_async (SnapdClient *client,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_system_information_task (client, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_LIST_ONE), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, NULL, "GET", path, NULL, NULL);

    return task;
}

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

void
snapd_client_list_one_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_one_task (client, name, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_GET_ICON), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/icons/%s/icon", escaped);
    send_request (task, NULL, "GET", path, NULL, NULL);

    return task;
}

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

void
snapd_client_get_icon_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_icon_task (client, name, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_LIST), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    send_request (task, NULL, "GET", "/v2/snaps", NULL, NULL);

    return task;
}

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

void
snapd_client_list_async (SnapdClient *client,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_list_task (client, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_GET_INTERFACES), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    send_request (task, NULL, "GET", "/v2/interfaces", NULL, NULL);

    return task;
}

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

void
snapd_client_get_interfaces_async (SnapdClient *client,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_interfaces_task (client, cancellable, callback, user_data);
}

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
                     SnapdRequestType request,
                     const gchar *action,
                     const gchar *plug_snap, const gchar *plug_name,
                     const gchar *slot_snap, const gchar *slot_name,
                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (request), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

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
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_task (client, auth_data, SNAPD_REQUEST_CONNECT_INTERFACE, "connect", plug_snap, plug_name, slot_snap, slot_name, cancellable, callback, user_data);
}

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

    task = g_object_ref (make_connect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_connect_interface_finish (client, G_ASYNC_RESULT (task), error);
}

void
snapd_client_connect_interface_async (SnapdClient *client,
                                      SnapdAuthData *auth_data,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_connect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, cancellable, callback, user_data);  
}

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
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return make_interface_task (client, auth_data, SNAPD_REQUEST_DISCONNECT_INTERFACE, "disconnect", plug_snap, plug_name, slot_snap, slot_name, cancellable, callback, user_data);
}

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

    task = g_object_ref (make_disconnect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_disconnect_interface_finish (client, G_ASYNC_RESULT (task), error);
}

void
snapd_client_disconnect_interface_async (SnapdClient *client,
                                         SnapdAuthData *auth_data,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_disconnect_interface_task (client, auth_data, plug_snap, plug_name, slot_snap, slot_name, cancellable, callback, user_data);  
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_LOGIN), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

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

void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_login_task (client, username, password, otp, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autoptr (GString) path = NULL;
    g_autofree gchar *escaped = NULL;  

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_FIND), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_INSTALL), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

    data = make_action_data ("install", channel);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_REFRESH), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

    data = make_action_data ("refresh", channel);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_REMOVE), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

    data = make_action_data ("remove", NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_ENABLE), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

    data = make_action_data ("enable", NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autofree gchar *data = NULL;
    g_autofree gchar *escaped = NULL, *path = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_DISABLE), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

    data = make_action_data ("disable", NULL);

    escaped = soup_uri_encode (name, NULL);
    path = g_strdup_printf ("/v2/snaps/%s", escaped);
    send_request (task, auth_data, "POST", path, "application/json", data);

    return task;
}

gboolean
snapd_client_disable_sync (SnapdClient *client,
                           SnapdAuthData *auth_data, const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    task = g_object_ref (make_disable_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_disable_finish (client, G_ASYNC_RESULT (task), error);
}

void
snapd_client_disable_async (SnapdClient *client,
                            SnapdAuthData *auth_data, const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    make_disable_task (client, auth_data, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_GET_PAYMENT_METHODS), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    send_request (task, auth_data, "GET", "/v2/buy/methods", NULL, NULL);

    return task;
}

GPtrArray *
snapd_client_get_payment_methods_sync (SnapdClient *client,
                                       SnapdAuthData *auth_data,
                                       gboolean *allows_automatic_payments,
                                       GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    task = g_object_ref (make_get_payment_methods_task (client, auth_data, cancellable, NULL, NULL));
    wait_for_task (task);
    return snapd_client_get_payment_methods_finish (client, G_ASYNC_RESULT (task), allows_automatic_payments, error);
}

void
snapd_client_get_payment_methods_async (SnapdClient *client,
                                        SnapdAuthData *auth_data,
                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    make_get_payment_methods_task (client, auth_data, cancellable, callback, user_data);
}

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
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *data = NULL;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_BUY), NULL);
    priv->tasks = g_list_append (priv->tasks, task);

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

gboolean
snapd_client_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), FALSE);
    return g_task_propagate_boolean (G_TASK (result), error);
}

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
