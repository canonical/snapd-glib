#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "snapd-client.h"

typedef struct
{
    GSocket *snapd_socket;
    GList *tasks;
    GSource *read_source;
    gchar buffer[1024]; // FIXME: Make expandable
    gsize n_read;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (snapd-client-error-quark, snapd_client_error)

// snapd API documentation is at https://github.com/snapcore/snapd/blob/master/docs/rest.md

#define SNAPD_SOCKET "/run/snapd.socket"

typedef enum
{
    SNAPD_REQUEST_SYSTEM_INFORMATION,
    SNAPD_REQUEST_LOGIN
} SnapdRequestType;

static void
send_request (GTask *task, const gchar *method, const gchar *path, const gchar *content_type, const gchar *content)
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
parse_result (const gchar *response_type, const gchar *response, gsize response_length, JsonObject **result, GError **error)
{
    g_autoptr(JsonParser) parser = NULL;
    g_autoptr(GError) error_local = NULL;
    JsonObject *root;

    if (response_type == NULL) {
        g_set_error_literal (error,
                             SNAPD_CLIENT_ERROR,
                             SNAPD_CLIENT_ERROR_PARSE_ERROR,
                             "snapd returned no content type");
        return FALSE;
    }
    if (g_strcmp0 (response_type, "application/json") != 0) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_PARSE_ERROR,
                     "snapd returned unexpected content type %s", response_type);
        return FALSE;
    }

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, response, response_length, &error_local)) {
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
    if (!json_object_has_member (root, "result")) {
        g_set_error_literal (error,
                             SNAPD_CLIENT_ERROR,
                             SNAPD_CLIENT_ERROR_PARSE_ERROR,
                             "snapd response does not contain a \"result\" field");
        return FALSE;
    }
    if (result != NULL)
        *result = json_object_ref (json_object_get_object_member (root, "result"));

    return TRUE;
}

static void
parse_system_information_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdSystemInformation) system_information = NULL;
    JsonObject *os_release;

    parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &result, NULL); // FIXME: error
    os_release = json_object_get_object_member (result, "os-release");
    system_information = g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                                       "on-classic", json_object_get_boolean_member (result, "on-classic"),
                                       "os-id", os_release != NULL ? json_object_get_string_member (os_release, "id") : NULL,
                                       "os-version", os_release != NULL ? json_object_get_string_member (os_release, "version-id") : NULL,
                                       "series", json_object_get_string_member (result, "series"),
                                       "version", json_object_get_string_member (result, "version"),
                                       NULL);
    g_task_return_pointer (task, g_steal_pointer (&system_information), g_object_unref); 
}

static void
parse_login_response (GTask *task, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    JsonArray *discharges;
    guint i;

    parse_result (soup_message_headers_get_content_type (headers, NULL), content, content_length, &result, NULL); // FIXME: error
    auth_data = snapd_auth_data_new ();
    snapd_auth_data_set_macaroon (auth_data, json_object_get_string_member (result, "macaroon"));
    discharges = json_object_get_array_member (result, "discharges");
    for (i = 0; i < json_array_get_length (discharges); i++)
        snapd_auth_data_add_discharge (auth_data, json_array_get_string_element (discharges, i));
    g_task_return_pointer (task, g_steal_pointer (&auth_data), g_object_unref); 
}

static void
parse_response (SnapdClient *client, SoupMessageHeaders *headers, const gchar *content, gsize content_length)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    g_autoptr(GTask) task = NULL;
    SnapdRequestType request_type;

    /* Get which request this response is for */
    if (priv->tasks == NULL) {
        g_warning ("Ignoring unexpected response");
        return;
    }
    task = g_list_nth_data (priv->tasks, 0);
    priv->tasks = g_list_remove (priv->tasks, g_list_first (priv->tasks));

    g_printerr ("PARSE %.*s\n", content_length, content);

    request_type = GPOINTER_TO_INT (g_task_get_task_data (task));
    switch (request_type)
    {
    case SNAPD_REQUEST_SYSTEM_INFORMATION:
        parse_system_information_response (task, headers, content, content_length);
        break;
    case SNAPD_REQUEST_LOGIN:
        parse_login_response (task, headers, content, content_length);      
        break;
    }
}

static gssize
read_data (SnapdClient *client, GCancellable *cancellable)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    gssize n_read;
    g_autoptr(GError) error_local = NULL;

    if (priv->n_read >= sizeof (priv->buffer)) {
        // FIXME: Cancel all tasks
        //g_set_error (error,
        //             SNAPD_CLIENT_ERROR,
        //             SNAPD_CLIENT_ERROR_FAILED,
        //             "Not enough space for snapd response, "
        //             "require %zi octets, have %zi",
        //             n_required, max_data_length);
        return FALSE;
    }

    n_read = g_socket_receive (priv->snapd_socket,
                               priv->buffer + priv->n_read,
                               sizeof (priv->buffer) - priv->n_read,
                               cancellable,
                               &error_local);
    if (n_read < 0)
    {
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
    gsize chunk_length;
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
read_from_snapd (SnapdClient *client, GCancellable *cancellable, gboolean blocking)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    gchar *body;
    gsize header_length;
    g_autoptr (SoupMessageHeaders) headers = NULL;
    guint code;
    g_autofree gchar *reason_phrase = NULL;
    goffset content_length;
    gchar *combined_start;
    gsize combined_length, total_length;

    if (!read_data (client, cancellable))
        return G_SOURCE_REMOVE;

    while (TRUE) {
        /* Look for header divider */
        body = g_strstr_len (priv->buffer, priv->n_read, "\r\n\r\n");
        if (body == NULL)
            return G_SOURCE_CONTINUE;
        body += 4;
        header_length = body - priv->buffer;

        /* Parse headers */
        headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_RESPONSE);
        if (!soup_headers_parse_response (priv->buffer, header_length, headers,
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
                if (!read_data (client, cancellable))
                    return G_SOURCE_REMOVE;
            }

            parse_response (client, headers, body, priv->n_read - header_length);
            break;

        case SOUP_ENCODING_CHUNKED:
            // FIXME: Find a way to abort on error
            while (!have_chunked_body (body, priv->n_read - header_length)) {
                if (!blocking)
                    return G_SOURCE_CONTINUE;
                if (!read_data (client, cancellable))
                    return G_SOURCE_REMOVE;
            }

            compress_chunks (body, priv->n_read - header_length, &combined_start, &combined_length, &content_length);
            parse_response (client, headers, combined_start, combined_length);
            break;

        case SOUP_ENCODING_CONTENT_LENGTH:
            content_length = soup_message_headers_get_content_length (headers);
            while (priv->n_read < header_length + content_length) {
                if (!blocking)
                    return G_SOURCE_CONTINUE;
                if (!read_data (client, cancellable))
                    return G_SOURCE_REMOVE;
            }

            parse_response (client, headers, body, content_length);
            break;
        default:
            // FIXME
            return G_SOURCE_REMOVE;
        }

        /* Move remaining data to the start of the buffer */
        memmove (priv->buffer, priv->buffer + header_length + total_length, priv->n_read - (header_length + content_length));
        priv->n_read -= header_length + content_length;
    }
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, SnapdClient *client)
{
    return read_from_snapd (client, NULL, FALSE); // FIXME: Use Cancellable from first task?
}

gboolean
snapd_client_connect_sync (SnapdClient *client, GCancellable *cancellable, GError **error)
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
make_system_information_task (SnapdClient *client, GCancellable *cancellable,
                              GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_set_task_data (task, GINT_TO_POINTER (SNAPD_REQUEST_SYSTEM_INFORMATION), NULL);
    priv->tasks = g_list_append (priv->tasks, task);
    send_request (task, "GET", "/v2/system-info", NULL, NULL);

    return task;
}

SnapdSystemInformation *
snapd_client_get_system_information_sync (SnapdClient *client, GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    task = g_object_ref (make_system_information_task (client, cancellable, NULL, NULL));
    while (!g_task_get_completed (task))
        g_main_context_iteration (g_task_get_context (task), TRUE);
    return snapd_client_get_system_information_finish (client, G_ASYNC_RESULT (task), error);
}

void
snapd_client_get_system_information_async (SnapdClient *client, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
    make_system_information_task (client, cancellable, callback, user_data);
}

SnapdSystemInformation *
snapd_client_get_system_information_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);
}

static GTask *
make_login_task (SnapdClient *client,
                 const gchar *username, const gchar *password, const gchar *otp,
                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GTask *task;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autoptr(JsonNode) json_root = NULL;
    g_autoptr(JsonGenerator) json_generator = NULL;
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
    json_root = json_builder_get_root (builder);
    json_generator = json_generator_new ();
    json_generator_set_pretty (json_generator, TRUE);
    json_generator_set_root (json_generator, json_root);
    data = json_generator_to_data (json_generator, NULL);

    send_request (task, "POST", "/v2/login", "application/json", data);

    return task;
}

SnapdAuthData *
snapd_client_login_sync (SnapdClient *client,
                         const gchar *username, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_autoptr(GTask) task = NULL;

    task = g_object_ref (make_login_task (client, username, password, otp, cancellable, NULL, NULL));
    while (!g_task_get_completed (task))
        g_main_context_iteration (g_task_get_context (task), TRUE);
    return snapd_client_login_finish (client, G_ASYNC_RESULT (task), error);  
}

void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    make_login_task (client, username, password, otp, cancellable, callback, user_data);
}

SnapdAuthData *
snapd_client_login_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (g_task_is_valid (G_TASK (result), client), NULL);
    return g_task_propagate_pointer (G_TASK (result), error);  
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
}
