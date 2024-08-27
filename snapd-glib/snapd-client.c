/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdlib.h>
#include <string.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>

#include "snapd-client.h"

#include "snapd-error.h"
#include "requests/snapd-get-aliases.h"
#include "requests/snapd-get-apps.h"
#include "requests/snapd-get-assertions.h"
#include "requests/snapd-get-buy-ready.h"
#include "requests/snapd-get-categories.h"
#include "requests/snapd-get-change.h"
#include "requests/snapd-get-changes.h"
#include "requests/snapd-get-connections.h"
#include "requests/snapd-get-find.h"
#include "requests/snapd-get-icon.h"
#include "requests/snapd-get-interfaces.h"
#include "requests/snapd-get-interfaces-legacy.h"
#include "requests/snapd-get-logs.h"
#include "requests/snapd-get-notices.h"
#include "requests/snapd-get-sections.h"
#include "requests/snapd-get-snap.h"
#include "requests/snapd-get-snap-conf.h"
#include "requests/snapd-get-snaps.h"
#include "requests/snapd-get-system-info.h"
#include "requests/snapd-get-themes.h"
#include "requests/snapd-get-users.h"
#include "requests/snapd-post-aliases.h"
#include "requests/snapd-post-assertions.h"
#include "requests/snapd-post-buy.h"
#include "requests/snapd-post-change.h"
#include "requests/snapd-post-create-user.h"
#include "requests/snapd-post-create-users.h"
#include "requests/snapd-post-download.h"
#include "requests/snapd-post-interfaces.h"
#include "requests/snapd-post-login.h"
#include "requests/snapd-post-logout.h"
#include "requests/snapd-post-snap.h"
#include "requests/snapd-post-snap-stream.h"
#include "requests/snapd-post-snap-try.h"
#include "requests/snapd-post-snaps.h"
#include "requests/snapd-post-snapctl.h"
#include "requests/snapd-post-themes.h"
#include "requests/snapd-put-snap-conf.h"

/**
 * SECTION:snapd-client
 * @short_description: Client connection to snapd
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdClient is the means of talking to snapd.
 *
 * To communicate with snapd create a client with snapd_client_new() then
 * send requests.
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
 * existence of a define called SNAPD_GLIB_VERSION_&lt;version&gt;, i.e.
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
    GPtrArray *requests;

    /* Whether to send the X-Allow-Interaction request header */
    gboolean allow_interaction;

    /* Data received from snapd */
    GMutex buffer_mutex;
    GByteArray *buffer;

    /* Processed HTTP response */
    guint response_status_code;
    SoupMessageHeaders *response_headers;
    GByteArray *response_body;
    gsize response_body_used;

    /* Maintenance information returned from snapd */
    SnapdMaintenance *maintenance;

    /* Nanoseconds for the since_date_time field, or -1 if not defined */
    int since_date_time_nanoseconds;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

/* snapd API documentation is at https://forum.snapcraft.io/t/snapd-rest-api */

/* Default socket to connect to */
#define SNAPD_SOCKET "/run/snapd.socket"

/* Number of bytes to read at a time */
#define READ_SIZE 1024

/* Number of milliseconds to poll for status in asynchronous operations */
#define ASYNC_POLL_TIME 100

typedef struct
{
    int ref_count;
    SnapdClient *client;
    SnapdRequest *request;
    GSource *read_source;
    GSource *poll_source;
    gulong cancelled_id;
} RequestData;

static RequestData *
request_data_new (SnapdClient *client, SnapdRequest *request)
{
    RequestData *data = g_slice_new0 (RequestData);
    data->ref_count = 1;
    data->client = client;
    data->request = g_object_ref (request);

    return data;
}

static RequestData *
request_data_ref (RequestData *data)
{
    data->ref_count++;
    return data;
}

static void
request_data_unref (RequestData *data)
{
    data->ref_count--;
    if (data->ref_count > 0)
        return;

    if (data->read_source != NULL)
        g_source_destroy (data->read_source);
    g_clear_pointer (&data->read_source, g_source_unref);
    if (data->poll_source != NULL)
        g_source_destroy (data->poll_source);
    g_clear_pointer (&data->poll_source, g_source_unref);
    if (data->cancelled_id != 0)
        g_cancellable_disconnect (_snapd_request_get_cancellable (data->request), data->cancelled_id);
    data->cancelled_id = 0;
    g_clear_object (&data->request);
    g_slice_free (RequestData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RequestData, request_data_unref)

static void send_request (SnapdClient *self, SnapdRequest *request);

static RequestData *
get_request_data (SnapdClient *self, SnapdRequest *request)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    for (guint i = 0; i < priv->requests->len; i++) {
        RequestData *data = g_ptr_array_index (priv->requests, i);
        if (data->request == request)
            return data;
    }

    return NULL;
}

static void
complete_request_unlocked (SnapdClient *self, SnapdRequest *request, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    _snapd_request_return (request, error);

    RequestData *data = get_request_data (self, request);
    g_ptr_array_remove (priv->requests, data);
}

static void
complete_request (SnapdClient *self, SnapdRequest *request, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);
    complete_request_unlocked (self, request, error);
}

static gboolean
async_poll_cb (gpointer data)
{
    RequestData *d = data;

    g_autoptr(SnapdGetChange) change_request = _snapd_request_async_make_get_change_request (SNAPD_REQUEST_ASYNC (d->request));
    send_request (d->client, SNAPD_REQUEST (change_request));

    if (d->poll_source != NULL)
        g_source_destroy (d->poll_source);
    g_clear_pointer (&d->poll_source, g_source_unref);
    return G_SOURCE_REMOVE;
}

static void
schedule_poll (SnapdClient *self, SnapdRequestAsync *request)
{
    RequestData *data = get_request_data (self, SNAPD_REQUEST (request));
    if (data->poll_source != NULL)
        g_source_destroy (data->poll_source);
    g_clear_pointer (&data->poll_source, g_source_unref);
    data->poll_source = g_timeout_source_new (ASYNC_POLL_TIME);
    g_source_set_callback (data->poll_source, async_poll_cb, data, NULL);
    g_source_attach (data->poll_source, _snapd_request_get_context (SNAPD_REQUEST (request)));
}

static void
complete_all_requests (SnapdClient *self, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    /* Disconnect socket - we will reconnect on demand */
    if (priv->snapd_socket != NULL)
        g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);

    /* Cancel synchronous requests (we'll never know the result); reschedule async ones (can reconnect to check result) */
    g_autoptr(GPtrArray) requests_copy = g_ptr_array_new_with_free_func ((GDestroyNotify) request_data_unref);
    for (guint i = 0; i < priv->requests->len; i++)
        g_ptr_array_add (requests_copy, request_data_ref (g_ptr_array_index (priv->requests, i)));
    for (guint i = 0; i < requests_copy->len; i++) {
        RequestData *data = g_ptr_array_index (requests_copy, i);

        if (SNAPD_IS_REQUEST_ASYNC (data->request))
            schedule_poll (self, SNAPD_REQUEST_ASYNC (data->request));
        else
            complete_request_unlocked (self, data->request, error);
    }
}

static void
append_string (GByteArray *array, const gchar *value)
{
    g_byte_array_append (array, (const guint8 *) value, strlen (value));
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
    const char * const * lang_names = g_get_language_names ();
    g_return_val_if_fail (lang_names != NULL, NULL);

    /* Build the array of languages */
    g_autoptr(GPtrArray) langs = g_ptr_array_new_with_free_func (g_free);
    for (guint i = 0; lang_names[i] != NULL; i++) {
        gchar *lang = posix_lang_to_rfc2616 (lang_names[i]);
        if (lang != NULL)
            g_ptr_array_add (langs, lang);
    }

    /* Add quality values */
    int delta;
    if (langs->len < 10)
        delta = 10;
    else if (langs->len < 20)
        delta = 5;
    else
        delta = 1;
    for (guint i = 0; i < langs->len; i++) {
        g_autofree gchar *lang = langs->pdata[i];
        langs->pdata[i] = add_quality_value (lang, 100 - i * delta);
    }

    /* Fallback to "en" if list is empty */
    if (langs->len == 0)
        return g_strdup ("en");

    g_ptr_array_add (langs, NULL);
    return g_strjoinv (", ", (char **)langs->pdata);
}

static SnapdPostChange *
find_post_change_request (SnapdClient *self, const gchar *change_id)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (guint i = 0; i < priv->requests->len; i++) {
        RequestData *data = g_ptr_array_index (priv->requests, i);

        if (SNAPD_IS_POST_CHANGE (data->request) &&
            g_strcmp0 (_snapd_request_async_get_change_id (SNAPD_REQUEST_ASYNC (data->request)), change_id) == 0)
            return SNAPD_POST_CHANGE (data->request);
    }

    return NULL;
}

static void
send_cancel (SnapdClient *self, SnapdRequestAsync *request)
{
    g_autoptr(SnapdPostChange) change_request = find_post_change_request (self, _snapd_request_async_get_change_id (request));
    if (change_request != NULL)
        return;

    change_request = _snapd_request_async_make_post_change_request (request);
    send_request (self, SNAPD_REQUEST (change_request));
}

static SnapdRequestAsync *
find_change_request (SnapdClient *self, const gchar *change_id)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (guint i = 0; i < priv->requests->len; i++) {
        RequestData *data = g_ptr_array_index (priv->requests, i);

        if (SNAPD_IS_REQUEST_ASYNC (data->request) &&
            g_strcmp0 (_snapd_request_async_get_change_id (SNAPD_REQUEST_ASYNC (data->request)), change_id) == 0)
            return SNAPD_REQUEST_ASYNC (data->request);
    }

    return NULL;
}

static SnapdRequest *
get_first_request (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (guint i = 0; i < priv->requests->len; i++) {
        RequestData *data = g_ptr_array_index (priv->requests, i);

        /* Return first non-async request or async request without change id */
        if (SNAPD_IS_REQUEST_ASYNC (data->request)) {
            if (_snapd_request_async_get_change_id (SNAPD_REQUEST_ASYNC (data->request)) == NULL)
                return data->request;
        }
        else
            return data->request;
    }

    return NULL;
}

static gboolean
read_chunk_header (const gchar *body, gsize body_length, gsize *chunk_header_length, gsize *chunk_length)
{
    const gchar *chunk_start = g_strstr_len (body, body_length, "\r\n");
    if (chunk_start == NULL)
        return FALSE;
    *chunk_header_length = chunk_start - body + 2;
    *chunk_length = strtoul (body, NULL, 16);

    return TRUE;
}

static void
complete_change (SnapdClient *self, const gchar *change_id, GError *error)
{
    SnapdRequestAsync *request = find_change_request (self, change_id);
    if (request != NULL)
        complete_request (self, SNAPD_REQUEST (request), error);
}

static void
update_changes (SnapdClient *self, SnapdChange *change, JsonNode *data)
{
    SnapdRequestAsync *request = find_change_request (self, snapd_change_get_id (change));
    if (request == NULL)
        return;

    _snapd_request_async_report_progress (request, self, change);

    /* Complete parent */
    if (snapd_change_get_ready (change)) {
        g_autoptr(GError) error = NULL;
        if (!_snapd_request_async_parse_result (request, data, &error)) {
            complete_request (self, SNAPD_REQUEST (request), error);
            return;
        }

        if (g_cancellable_set_error_if_cancelled (_snapd_request_get_cancellable (SNAPD_REQUEST (request)), &error)) {
            complete_request (self, SNAPD_REQUEST (request), error);
            return;
        }

        if (snapd_change_get_error (change) != NULL) {
            g_set_error_literal (&error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_FAILED,
                                 snapd_change_get_error (change));
            complete_request (self, SNAPD_REQUEST (request), error);
            return;
        }

        complete_request (self, SNAPD_REQUEST (request), NULL);
        return;
    }

    /* Poll for updates */
    schedule_poll (self, request);
}

static void
parse_response (SnapdClient *self, SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    g_clear_object (&priv->maintenance);
    g_autoptr(GError) error = NULL;
    if (!SNAPD_REQUEST_GET_CLASS (request)->parse_response (request, status_code, content_type, body, &priv->maintenance, &error)) {
        if (SNAPD_IS_GET_CHANGE (request)) {
            complete_change (self, _snapd_get_change_get_change_id (SNAPD_GET_CHANGE (request)), error);
            complete_request (self, request, error);
        }
        else if (SNAPD_IS_POST_CHANGE (request)) {
            complete_change (self, _snapd_post_change_get_change_id (SNAPD_POST_CHANGE (request)), error);
            complete_request (self, request, error);
        }
        else
            complete_request (self, request, error);
        return;
    }

    if (SNAPD_IS_GET_CHANGE (request))
        update_changes (self,
                        _snapd_get_change_get_change (SNAPD_GET_CHANGE (request)),
                        _snapd_get_change_get_data (SNAPD_GET_CHANGE (request)));
    else if (SNAPD_IS_POST_CHANGE (request))
        update_changes (self,
                        _snapd_post_change_get_change (SNAPD_POST_CHANGE (request)),
                        _snapd_post_change_get_data (SNAPD_POST_CHANGE (request)));

    if (SNAPD_IS_REQUEST_ASYNC (request)) {
        /* Immediately cancel if requested, otherwise poll for updates */
        if (g_cancellable_is_cancelled (_snapd_request_get_cancellable (request)))
            send_cancel (self, SNAPD_REQUEST_ASYNC (request));
        else
            schedule_poll (self, SNAPD_REQUEST_ASYNC (request));
    }
    else
        complete_request (self, request, NULL);
}

static gboolean
parse_seq (SnapdClient *self, SnapdRequest *request, const char *data, gsize data_length, GError **error)
{
    g_autoptr(JsonParser) parser = json_parser_new ();
    g_autoptr(GError) json_error = NULL;
    if (!json_parser_load_from_data (parser, data, data_length, error)) {
        return FALSE;
    }

    if (SNAPD_REQUEST_GET_CLASS (request)->parse_json_seq == NULL) {
        return TRUE;
    }

    return SNAPD_REQUEST_GET_CLASS (request)->parse_json_seq (request, json_parser_get_root (parser), error);
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->buffer_mutex);

    gsize orig_length = priv->buffer->len;
    g_byte_array_set_size (priv->buffer, orig_length + READ_SIZE);
    g_autoptr(GError) error = NULL;
    gssize n_read = g_socket_receive (socket,
                                      (gchar *) priv->buffer->data + orig_length,
                                      READ_SIZE,
                                      NULL,
                                      &error);
    g_byte_array_set_size (priv->buffer, orig_length + (n_read >= 0 ? n_read : 0));

    if (n_read == 0) {
        g_autoptr(GError) e = g_error_new (SNAPD_ERROR,
                                           SNAPD_ERROR_READ_FAILED,
                                           "snapd connection closed");
        complete_all_requests (self, e);
        return G_SOURCE_REMOVE;
    }

    if (n_read < 0) {
        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
            return G_SOURCE_CONTINUE;

        g_autoptr(GError) e = g_error_new (SNAPD_ERROR,
                                           SNAPD_ERROR_READ_FAILED,
                                           "Failed to read from snapd: %s",
                                           error->message);
        complete_all_requests (self, e);
        return G_SOURCE_REMOVE;
    }

    while (TRUE) {
        /* Process headers */
        if (priv->response_headers == NULL) {
            gchar *body = g_strstr_len ((gchar *) priv->buffer->data, priv->buffer->len, "\r\n\r\n");
            if (body == NULL)
                return G_SOURCE_CONTINUE;
            body += 4;
            gsize header_length = body - (gchar *) priv->buffer->data;

            priv->response_headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_RESPONSE);
            if (!soup_headers_parse_response ((gchar *) priv->buffer->data, header_length, priv->response_headers, NULL, &priv->response_status_code, NULL)) {
                g_autoptr(GError) e = g_error_new (SNAPD_ERROR,
                                                   SNAPD_ERROR_READ_FAILED,
                                                   "Failed to parse headers from snapd");
                complete_all_requests (self, e);
                return G_SOURCE_REMOVE;
            }

            /* Remove headers from buffer */
            g_byte_array_remove_range (priv->buffer, 0, header_length);
        }

        /* Read response body */
        gboolean is_complete = FALSE;
        gsize offset = 0, chunk_header_length, chunk_length, content_length, n;
        g_autoptr(GError) e = NULL;
        switch (soup_message_headers_get_encoding (priv->response_headers)) {
        case SOUP_ENCODING_EOF:
            g_byte_array_append(priv->response_body, priv->buffer->data, priv->buffer->len);
            g_byte_array_set_size(priv->buffer, 0);
            is_complete = g_socket_is_closed (priv->snapd_socket);
            break;

        case SOUP_ENCODING_CHUNKED:
            while (offset < priv->buffer->len && read_chunk_header ((const gchar *) priv->buffer->data + offset, priv->buffer->len - offset, &chunk_header_length, &chunk_length)) {
                gsize chunk_trailer_length = 2;
                gsize chunk_data_offset = offset + chunk_header_length;
                gsize chunk_trailer_offset = chunk_data_offset + chunk_length;
                gsize chunk_end = chunk_trailer_offset + chunk_trailer_length;

                // Haven't yet received all chunk data.
                if (chunk_end > priv->buffer->len) {
                    break;
                }

                const gchar *chunk_trailer = (const gchar *) priv->buffer->data + chunk_trailer_offset;
                if (chunk_trailer[0] != '\r' && chunk_trailer[1] != '\n') {
                    g_warning ("Invalid HTTP chunk");
                    return G_SOURCE_REMOVE;
                }

                g_byte_array_append(priv->response_body, priv->buffer->data + chunk_data_offset, chunk_length);
                offset = chunk_end;

                // Empty chunk is end of data.
                if (chunk_length == 0) {
                    is_complete = TRUE;
                    break;
                }
            }
            g_byte_array_remove_range (priv->buffer, 0, offset);
            break;

        case SOUP_ENCODING_CONTENT_LENGTH:
            content_length = soup_message_headers_get_content_length (priv->response_headers);
            n = content_length - (priv->response_body->len + priv->response_body_used);
            if (n > priv->buffer->len) {
                n = priv->buffer->len;
            }
            g_byte_array_append(priv->response_body, priv->buffer->data, n);
            g_byte_array_remove_range (priv->buffer, 0, n);
            is_complete = (priv->response_body->len + priv->response_body_used) >= content_length;
            break;

        default:
            e = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unable to determine header encoding");
            complete_all_requests (self, e);
            return G_SOURCE_REMOVE;
        }

        /* Match this response to the next uncompleted request */
        SnapdRequest *request = get_first_request (self);
        if (request == NULL) {
            g_warning ("Ignoring unexpected response");
            return G_SOURCE_REMOVE;
        }

        const gchar *content_type = soup_message_headers_get_content_type (priv->response_headers, NULL);
        if (g_strcmp0 (content_type, "application/json-seq") == 0) {
            /* Handle each sequence element */
            while (priv->response_body->len > 0) {
                /* Requests start with a record separator */
                if (priv->response_body->data[0] != 0x1e) {
                    break;
                }
                gsize seq_start = 1, seq_end = 1;
                while (seq_end < priv->response_body->len && priv->response_body->data[seq_end] != 0x1e) {
                    seq_end++;
                }
                gboolean have_end = seq_end < priv->response_body->len && priv->response_body->data[seq_end] == 0x1e;
                if (!have_end && !is_complete) {
                    break;
                }

                g_autoptr(GError) json_error = NULL;
                if (!parse_seq (self, request, (const gchar *) priv->response_body->data + seq_start, seq_end - seq_start, &json_error)) {
                    g_warning ("Ignoring invalid JSON: %s", json_error->message);
                    return G_SOURCE_REMOVE;
                }

                g_byte_array_remove_range (priv->response_body, 0, seq_end);
                priv->response_body_used += seq_end;
            }

            if (is_complete) {
                complete_request (self, request, NULL);
            }
        } else if (is_complete) {
            g_autoptr(GBytes) b = g_bytes_new (priv->response_body->data, priv->response_body->len);
            parse_response (self, request, priv->response_status_code, content_type, b);
        }

        if (is_complete) {
#if SOUP_CHECK_VERSION (2, 99, 2)
            g_clear_pointer (&priv->response_headers, soup_message_headers_unref);
#else
            g_clear_pointer (&priv->response_headers, soup_message_headers_free);
#endif
            g_byte_array_set_size(priv->response_body, 0);
            priv->response_body_used = 0;
       } else {
            return G_SOURCE_CONTINUE;
       }
    }
}

static gboolean
cancel_idle_cb (gpointer user_data)
{
    RequestData *data = user_data;

    g_autoptr(GError) error = NULL;
    g_cancellable_set_error_if_cancelled (_snapd_request_get_cancellable (data->request), &error);
    complete_request (data->client, data->request, error);

    return G_SOURCE_REMOVE;
}

static void
request_cancelled_cb (GCancellable *cancellable, RequestData *data)
{
    /* Asynchronous requests require asking snapd to stop them */
    if (SNAPD_IS_REQUEST_ASYNC (data->request)) {
        SnapdRequestAsync *r = SNAPD_REQUEST_ASYNC (data->request);

        /* Cancel if we have got a response from snapd */
        if (_snapd_request_async_get_change_id (r) != NULL)
            send_cancel (data->client, r);
    }
    else {
        /* Execute in an idle thread so g_cancellable_disconnect doesn't deadlock */
        g_autoptr(GSource) idle_source = g_idle_source_new ();
        g_source_set_callback (idle_source, cancel_idle_cb, request_data_ref (data), (GDestroyNotify) request_data_unref);
        g_source_attach (idle_source, _snapd_request_get_context (data->request));
    }
}

static GSocket *
open_snapd_socket (const gchar *socket_path, GCancellable *cancellable, GError **error)
{
    g_autoptr(GError) error_local = NULL;
    g_autoptr(GSocket) socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                              G_SOCKET_TYPE_STREAM,
                                              G_SOCKET_PROTOCOL_DEFAULT,
                                              &error_local);
    if (socket == NULL) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_CONNECTION_FAILED,
                     "Unable to create snapd socket: %s",
                     error_local->message);
        return NULL;
    }
    g_socket_set_blocking (socket, FALSE);
    g_autoptr(GSocketAddress) address = g_unix_socket_address_new (socket_path);
    if (!g_socket_connect (socket, address, cancellable, &error_local)) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_CONNECTION_FAILED,
                     "Unable to connect snapd socket: %s",
                     error_local->message);
        return NULL;
    }

    return g_steal_pointer (&socket);
}

static GSource *
make_read_source (SnapdClient *self, GMainContext *context)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    g_autoptr(GSource) source = g_socket_create_source (priv->snapd_socket, G_IO_IN, NULL);
    g_source_set_name (source, "snapd-glib-read-source");
    g_source_set_callback (source, (GSourceFunc) read_cb, self, NULL);
    g_source_attach (source, context);

    return g_steal_pointer (&source);
}

static gboolean
write_to_snapd (SnapdClient *self, GByteArray *data, GCancellable *cancellable, GError **error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    guint n_sent = 0;
    while (n_sent < data->len) {
        gssize n_written = g_socket_send (priv->snapd_socket, (const gchar *) (data->data + n_sent), data->len - n_sent, cancellable, error);
        if (n_written < 0)
            return FALSE;

        n_sent += n_written;
    }

    return TRUE;
}

static void
send_request (SnapdClient *self, SnapdRequest *request)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    // This code can be replaced with support in libsoup3 at some point.
    // https://gitlab.gnome.org/GNOME/libsoup/-/issues/75

    _snapd_request_set_source_object (request, G_OBJECT (self));

    g_autoptr(RequestData) data = request_data_new (self, request);
    {
        g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);
        g_ptr_array_add (priv->requests, request_data_ref (data));
    }

    GCancellable *cancellable = _snapd_request_get_cancellable (request);
    if (cancellable != NULL)
        data->cancelled_id = g_cancellable_connect (cancellable, G_CALLBACK (request_cancelled_cb), request_data_new (self, request), (GDestroyNotify) request_data_unref);

    g_autoptr(GBytes) body = NULL;
    SoupMessage *message = _snapd_request_get_message (request, &body);
#if SOUP_CHECK_VERSION (2, 99, 2)
    SoupMessageHeaders *request_headers = soup_message_get_request_headers (message);
#else
    SoupMessageHeaders *request_headers = message->request_headers;
#endif
    soup_message_headers_append (request_headers, "Host", "");
    soup_message_headers_append (request_headers, "Connection", "keep-alive");
    if (priv->user_agent != NULL)
        soup_message_headers_append (request_headers, "User-Agent", priv->user_agent);
    if (priv->allow_interaction)
        soup_message_headers_append (request_headers, "X-Allow-Interaction", "true");

    g_autofree gchar *accept_languages = get_accept_languages ();
    soup_message_headers_append (request_headers, "Accept-Language", accept_languages);

    if (priv->auth_data != NULL) {
        g_autoptr(GString) authorization = g_string_new ("");
        g_string_append_printf (authorization, "Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (priv->auth_data));
        GStrv discharges = snapd_auth_data_get_discharges (priv->auth_data);
        if (discharges != NULL)
            for (gsize i = 0; discharges[i] != NULL; i++)
                g_string_append_printf (authorization, ",discharge=\"%s\"", discharges[i]);
        soup_message_headers_append (request_headers, "Authorization", authorization->str);
    }
    if (body != NULL)
        soup_message_headers_set_content_length (request_headers, g_bytes_get_size (body));

#if SOUP_CHECK_VERSION (2, 99, 2)
    const gchar *method = soup_message_get_method (message);
#else
    const gchar *method = message->method;
#endif
    g_autoptr(GByteArray) request_data = g_byte_array_new ();
    append_string (request_data, method);
    append_string (request_data, " ");
#if SOUP_CHECK_VERSION (2, 99, 2)
    GUri *uri = soup_message_get_uri (message);
    const gchar *uri_path = g_uri_get_path (uri);
    const gchar *uri_query = g_uri_get_query (uri);
#else
    SoupURI *uri = soup_message_get_uri (message);
    const gchar *uri_path = uri->path;
    const gchar *uri_query = uri->query;
#endif
    append_string (request_data, uri_path);
    if (uri_query != NULL) {
        append_string (request_data, "?");
        append_string (request_data, uri_query);
    }
    append_string (request_data, " HTTP/1.1\r\n");
    SoupMessageHeadersIter iter;
    soup_message_headers_iter_init (&iter, request_headers);
    const char *name, *value;
    while (soup_message_headers_iter_next (&iter, &name, &value)) {
        append_string (request_data, name);
        append_string (request_data, ": ");
        append_string (request_data, value);
        append_string (request_data, "\r\n");
    }
    append_string (request_data, "\r\n");

    if (body != NULL)
        g_byte_array_append (request_data, g_bytes_get_data (body, NULL), g_bytes_get_size (body));

    gboolean new_socket = FALSE;
    if (priv->snapd_socket == NULL) {
        g_autoptr(GError) error = NULL;
        priv->snapd_socket = open_snapd_socket (priv->socket_path, cancellable, &error);
        if (priv->snapd_socket == NULL) {
            complete_request (self, request, error);
            return;
        }
        new_socket = TRUE;
    }

    data->read_source = make_read_source (self, _snapd_request_get_context (request));

    /* send HTTP request */
    g_autoptr(GError) error = NULL;
    if (write_to_snapd (self, request_data, cancellable, &error))
        return;

    /* If was re-using closed socket, then reconnect and retry */
    if (!new_socket && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_BROKEN_PIPE)) {
        g_clear_error (&error);
        g_clear_object (&priv->snapd_socket);
        g_source_destroy (data->read_source);
        g_clear_pointer (&data->read_source, g_source_unref);

        priv->snapd_socket = open_snapd_socket (priv->socket_path, cancellable, &error);
        if (priv->snapd_socket == NULL) {
            complete_request (self, request, error);
            return;
        }

        data->read_source = make_read_source (self, _snapd_request_get_context (request));

        if (write_to_snapd (self, request_data, cancellable, &error))
            return;
    }

    g_autoptr(GError) e = g_error_new (SNAPD_ERROR,
                                       SNAPD_ERROR_WRITE_FAILED,
                                       "Failed to write to snapd: %s",
                                       error->message);
    complete_request (self, request, e);
}

typedef struct
{
    SnapdClient *client;
    SnapdLogCallback callback;
    gpointer callback_data;
} FollowLogsData;

static FollowLogsData *
follow_logs_data_new (SnapdClient *client, SnapdLogCallback callback, gpointer callback_data)
{
    FollowLogsData *data = g_slice_new0 (FollowLogsData);
    data->client = client;
    data->callback = callback;
    data->callback_data = callback_data;

    return data;
}

static void
follow_logs_data_free (FollowLogsData *data)
{
    g_slice_free (FollowLogsData, data);
}

static void
log_cb (SnapdGetLogs *request, SnapdLog *log, gpointer user_data)
{
    FollowLogsData *data = user_data;
    data->callback (data->client, log, data->callback_data);
}

/**
 * snapd_client_connect_async:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * This method is no longer required and does nothing, snapd-glib now connects on demand.
 *
 * Since: 1.3
 * Deprecated: 1.24
 */
void
snapd_client_connect_async (SnapdClient *self,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(GTask) task = g_task_new (self, cancellable, callback, user_data);
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
snapd_client_connect_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

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
snapd_client_set_socket_path (SnapdClient *self, const gchar *socket_path)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_free (priv->socket_path);
    if (socket_path != NULL)
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
snapd_client_get_socket_path (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
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
snapd_client_set_user_agent (SnapdClient *self, const gchar *user_agent)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_free (priv->user_agent);
    priv->user_agent = g_strdup (user_agent);
}

/**
 * snapd_client_get_user_agent:
 * @client: a #SnapdClient
 *
 * Get the HTTP user-agent that is sent with each request to snapd.
 *
 * Returns: (allow-none): user agent or %NULL if none set.
 *
 * Since: 1.16
 */
const gchar *
snapd_client_get_user_agent (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
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
snapd_client_set_allow_interaction (SnapdClient *self, gboolean allow_interaction)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    priv->allow_interaction = allow_interaction;
}

/**
 * snapd_client_get_maintenance:
 * @client: a #SnapdClient
 *
 * Get the maintenance information reported by snapd or %NULL if no maintenance is in progress.
 * This information is updated after every request.
 *
 * Returns: (transfer none) (allow-none): a #SnapdMaintenance or %NULL.
 *
 * Since: 1.45
 */
SnapdMaintenance *
snapd_client_get_maintenance (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    return priv->maintenance;
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
snapd_client_get_allow_interaction (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    return priv->allow_interaction;
}

/**
 * snapd_client_login_async:
 * @client: a #SnapdClient.
 * @email: email address to log in with.
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
 * Deprecated: 1.26: Use snapd_client_login2_async()
 */
void
snapd_client_login_async (SnapdClient *self,
                          const gchar *email, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_login2_async (self, email, password, otp, cancellable, callback, user_data);
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
 * Deprecated: 1.26: Use snapd_client_login2_finish()
 */
SnapdAuthData *
snapd_client_login_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_finish (self, result, error);
    if (user_information == NULL)
        return NULL;

    return g_object_ref (snapd_user_information_get_auth_data (user_information));
}

/**
 * snapd_client_login2_async:
 * @client: a #SnapdClient.
 * @email: email address to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get authorization to install/remove snaps.
 * See snapd_client_login2_sync() for more information.
 *
 * Since: 1.26
 */
void
snapd_client_login2_async (SnapdClient *self,
                           const gchar *email, const gchar *password, const gchar *otp,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostLogin) request = _snapd_post_login_new (email, password, otp, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_login2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_login2_async().
 * See snapd_client_login2_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.26
 */
SnapdUserInformation *
snapd_client_login2_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_LOGIN (result), NULL);

    SnapdPostLogin *request = SNAPD_POST_LOGIN (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_post_login_get_user_information (request));
}

/**
 * snapd_client_logout_async:
 * @client: a #SnapdClient.
 * @id: login ID to use.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously log out from the snap store.
 * See snapd_client_logout_sync() for more information.
 *
 * Since: 1.55
 */
void
snapd_client_logout_async (SnapdClient *self,
                           gint64 id,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostLogout) request = _snapd_post_logout_new (id, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_logout_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_logout_async().
 * See snapd_client_logout_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.55
 */
gboolean
snapd_client_logout_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_LOGOUT (result), FALSE);

    SnapdPostLogout *request = SNAPD_POST_LOGOUT (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return FALSE;
    return TRUE;
}

/**
 * snapd_client_set_auth_data:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): a #SnapdAuthData or %NULL.
 *
 * Set the authorization data to use for requests. Authorization data can be
 * obtained by:
 *
 * - Logging into snapd using snapd_client_login_sync()
 *
 * - Using an existing authorization with snapd_auth_data_new().
 *
 * Since: 1.0
 */
void
snapd_client_set_auth_data (SnapdClient *self, SnapdAuthData *auth_data)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_if_fail (SNAPD_IS_CLIENT (self));
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
snapd_client_get_auth_data (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    return priv->auth_data;
}

/**
 * snapd_client_get_notices_async:
 * @client: a #SnapdClient.
 * @since_date_time: send only the notices generated after this moment (NULL for all).
 * @timeout: time, in microseconds, to wait for a new notice (zero to return immediately).
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get notifications that have occurred / are occurring on the snap daemon.
 *
 * The @since_date_time field, being a GDateTime, has a resolution of microseconds, so,
 * if nanosecond resolution is needed, it is mandatory to call
 * #snapd_client_notices_set_after_notice before calling this method.
 *
 * Since: 1.65
 */
void
snapd_client_get_notices_async (SnapdClient *self,
                                GDateTime *since_date_time,
                                GTimeSpan timeout,
                                GCancellable *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    snapd_client_get_notices_with_filters_async (self,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 since_date_time,
                                                 timeout,
                                                 cancellable,
                                                 callback,
                                                 user_data);
}

/**
 * snapd_client_get_notices_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_notices_async().
 *
 * Returns: (transfer container) (element-type SnapdNotice): a #GPtrArray object containing the requested notices, or NULL in case of error.
 *
 * Since: 1.65
 */
GPtrArray *
snapd_client_get_notices_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_NOTICES (result), NULL);

    SnapdGetNotices *request = SNAPD_GET_NOTICES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_notices_get_notices (request));
}

/**
 * snapd_client_get_notices_with_filters_async:
 * @client: a #SnapdClient.
 * @user_id: filter by this user-id (NULL for no filter).
 * @users: filter by this comma-separated list of users (NULL for no filter).
 * @types: filter by this comma-separated list of types (NULL for no filter).
 * @keys: filter by this comma-separated list of keys (NULL for no filter).
 * @since_date_time: send only the notices generated after this moment (NULL for all).
 * @timeout: time, in microseconds, to wait for a new notice (zero to return immediately).
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get notifications that have occurred / are occurring on the snap daemon,
 * allowing to filter the results with several options.
 *
 * The @since_date_time field, being a GDateTime, has a resolution of microseconds, so,
 * if nanosecond resolution is needed, it is mandatory to call
 * #snapd_client_notices_set_after_notice before calling this method.
 *
 * Since: 1.65
 */
void
snapd_client_get_notices_with_filters_async (SnapdClient *self,
                                             gchar *user_id,
                                             gchar *users,
                                             gchar *types,
                                             gchar *keys,
                                             GDateTime *since_date_time,
                                             GTimeSpan timeout,
                                             GCancellable *cancellable,
                                             GAsyncReadyCallback callback,
                                             gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    g_autoptr(SnapdGetNotices) request = _snapd_get_notices_new (user_id,
                                                                 users,
                                                                 types,
                                                                 keys,
                                                                 since_date_time,
                                                                 priv->since_date_time_nanoseconds,
                                                                 timeout,
                                                                 cancellable,
                                                                 callback,
                                                                 user_data);
    // reset the nanoseconds value, to ensure that the wrong value isn't used
    // in subsequent calls.
    priv->since_date_time_nanoseconds = -1;
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_notices_with_filters_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_notices_with_filters_async().
 *
 * Returns: (transfer container) (element-type SnapdNotice): a #GPtrArray object containing the requested notices, or NULL in case of error.
 *
 * Since: 1.65
 */
GPtrArray *
snapd_client_get_notices_with_filters_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_get_notices_finish (self, result, error);
}

/**
 * snapd_client_notices_set_after_notice:
 * @client: a #SnapdClient
 * @notice: the last #SnapdNotice received, to get all the notices after it.
 *
 * Allows to set the "since" parameter with nanosecond accuracy when doing a call
 * to get the notices. This is currently needed because GDateTime has only an
 * accuracy of 1 microsecond, but to receive notice events correctly, without
 * loosing any of them, an accuracy of 1 nanosecond is needed in the value passed
 * on in the @since_date_time parameter.
 *
 * The value is "reset" after any call to snapd_client_get_notices_*(), so it must
 * be set again always before doing any of those calls.
 *
 * Passing NULL will reset the value too, in which case the mili- and micro-seconds defined
 * in the @since_date_time parameter will be used.
 *
 * Since: 1.66
 */
void
snapd_client_notices_set_after_notice (SnapdClient *self, SnapdNotice *notice)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    priv->since_date_time_nanoseconds = (notice == NULL) ? -1 : snapd_notice_get_last_occurred_nanoseconds (notice);
}

/**
 * snapd_client_notices_set_since_nanoseconds:
 * @client: a #SnapdClient
 * @nanoseconds: the nanoseconds value to use to combine with the
 *               #since_date_time property to filter notices after it
 *
 * Allows to set the "since" parameter with nanosecond accuracy when doing a call to get the notices.
 * This is currently needed because GDateTime has only an accuracy of 1 microsecond, but to receive
 * notice events correctly, without loosing any, it is needed 1 nanosecond accuracy in the value
 * passed on in the @since_date_time parameter.
 *
 * The value is "reseted" after any call to snapd_client_get_notices_*(), so it must be set always before
 * doing any of those calls.
 *
 * Passing NULL will reset the value too, in which case the mili- and micro-seconds defined
 * in the @since_date_time parameter will be used.
 *
 * Since: 1.66
 */
void
snapd_client_notices_set_since_nanoseconds (SnapdClient *self, int nanoseconds)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    priv->since_date_time_nanoseconds = nanoseconds;
}

/**
 * snapd_client_get_changes_async:
 * @client: a #SnapdClient.
 * @filter: changes to filter on.
 * @snap_name: (allow-none): name of snap to filter on or %NULL for changes for any snap.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get changes that have occurred / are occurring on the snap daemon.
 * See snapd_client_get_changes_sync() for more information.
 *
 * Since: 1.29
 */
void
snapd_client_get_changes_async (SnapdClient *self,
                                SnapdChangeFilter filter, const gchar *snap_name,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    const gchar *select = NULL;
    switch (filter)
    {
    case SNAPD_CHANGE_FILTER_ALL:
        select = "all";
        break;
    case SNAPD_CHANGE_FILTER_IN_PROGRESS:
        select = "in-progress";
        break;
    case SNAPD_CHANGE_FILTER_READY:
        select = "ready";
        break;
    }

    g_autoptr(SnapdGetChanges) request = _snapd_get_changes_new (select, snap_name, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_changes_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_changes_async().
 * See snapd_client_get_changes_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdChange): an array of #SnapdChange or %NULL on error.
 *
 * Since: 1.29
 */
GPtrArray *
snapd_client_get_changes_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_CHANGES (result), NULL);

    SnapdGetChanges *request = SNAPD_GET_CHANGES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_changes_get_changes (request));
}

/**
 * snapd_client_get_change_async:
 * @client: a #SnapdClient.
 * @id: a change ID to get information on.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on a change.
 * See snapd_client_get_change_sync() for more information.
 *
 * Since: 1.29
 */
void
snapd_client_get_change_async (SnapdClient *self,
                               const gchar *id,
                               GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (id != NULL);

    g_autoptr(SnapdGetChange) request = _snapd_get_change_new (id, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_change_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_change_async().
 * See snapd_client_get_change_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdChange or %NULL on error.
 *
 * Since: 1.29
 */
SnapdChange *
snapd_client_get_change_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_CHANGE (result), NULL);

    SnapdGetChange *request = SNAPD_GET_CHANGE (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    SnapdChange *change = _snapd_get_change_get_change (request);
    return change == NULL ? NULL : g_object_ref (change);
}

/**
 * snapd_client_abort_change_async:
 * @client: a #SnapdClient.
 * @id: a change ID to abort.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously abort a change.
 * See snapd_client_abort_change_sync() for more information.
 *
 * Since: 1.30
 */
void
snapd_client_abort_change_async (SnapdClient *self,
                                 const gchar *id,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (id != NULL);

    g_autoptr(SnapdPostChange) request = _snapd_post_change_new (id, "abort", cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_abort_change_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_abort_change_async().
 * See snapd_client_abort_change_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdChange or %NULL on error.
 *
 * Since: 1.30
 */
SnapdChange *
snapd_client_abort_change_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_CHANGE (result), NULL);

    SnapdPostChange *request = SNAPD_POST_CHANGE (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_post_change_get_change (request));
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
snapd_client_get_system_information_async (SnapdClient *self,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetSystemInfo) request = _snapd_get_system_info_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_get_system_information_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SYSTEM_INFO (result), NULL);

    SnapdGetSystemInfo *request = SNAPD_GET_SYSTEM_INFO (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_system_info_get_system_information (request));
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
 * Deprecated: 1.42: Use snapd_client_get_snap_async()
 */
void
snapd_client_list_one_async (SnapdClient *self,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return snapd_client_get_snap_async (self, name, cancellable, callback, user_data);
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
 * Deprecated: 1.42: Use snapd_client_get_snap_finish()
 */
SnapdSnap *
snapd_client_list_one_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_get_snap_finish (self, result, error);
}

/**
 * snapd_client_get_snap_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information of a single installed snap.
 * See snapd_client_get_snap_sync() for more information.
 *
 * Since: 1.42
 */
void
snapd_client_get_snap_async (SnapdClient *self,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetSnap) request = _snapd_get_snap_new (name, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_snap_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_snap_async().
 * See snapd_client_get_snap_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 *
 * Since: 1.42
 */
SnapdSnap *
snapd_client_get_snap_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SNAP (result), NULL);

    SnapdGetSnap *request = SNAPD_GET_SNAP (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_snap_get_snap (request));
}

/**
 * snapd_client_get_snap_conf_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get configuration from.
 * @keys: (allow-none): keys to returns or %NULL to return all.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get configuration for a snap.
 * See snapd_client_get_snap_conf_sync() for more information.
 *
 * Since: 1.48
 */
void
snapd_client_get_snap_conf_async (SnapdClient *self,
                                  const gchar *name,
                                  GStrv keys,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdGetSnapConf) request = _snapd_get_snap_conf_new (name, keys, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_snap_conf_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_snap_conf_async().
 * See snapd_client_get_snap_conf_sync() for more information.
 *
 * Returns: (transfer container) (element-type utf8 GVariant): a table of configuration values or %NULL on error.
 *
 * Since: 1.48
 */
GHashTable *
snapd_client_get_snap_conf_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SNAP_CONF (result), NULL);

    SnapdGetSnapConf *request = SNAPD_GET_SNAP_CONF (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_hash_table_ref (_snapd_get_snap_conf_get_conf (request));
}

/**
 * snapd_client_set_snap_conf_async:
 * @client: a #SnapdClient.
 * @name: name of snap to set configuration for.
 * @key_values: (element-type utf8 GVariant): Keys to set.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously set configuration for a snap.
 * See snapd_client_set_snap_conf_sync() for more information.
 *
 * Since: 1.48
 */
void
snapd_client_set_snap_conf_async (SnapdClient *self,
                                  const gchar *name,
                                  GHashTable *key_values,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);
    g_return_if_fail (key_values != NULL);

    g_autoptr(SnapdPutSnapConf) request = _snapd_put_snap_conf_new (name, key_values, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_set_snap_conf_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_set_snap_conf_async().
 * See snapd_client_set_snap_conf_sync() for more information.
 *
 * Returns: %TRUE if configuration successfully applied.
 *
 * Since: 1.48
 */
gboolean
snapd_client_set_snap_conf_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_PUT_SNAP_CONF (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_get_apps_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetAppsFlags to control what results are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on installed apps.
 * See snapd_client_get_apps_sync() for more information.
 *
 * Since: 1.25
 * Deprecated: 1.45: Use snapd_client_get_apps2_async()
 */
void
snapd_client_get_apps_async (SnapdClient *self,
                             SnapdGetAppsFlags flags,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return snapd_client_get_apps2_async (self, flags, NULL, cancellable, callback, user_data);
}

/**
 * snapd_client_get_apps_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_apps_async().
 * See snapd_client_get_apps_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdApp): an array of #SnapdApp or %NULL on error.
 *
 * Since: 1.25
 * Deprecated: 1.45: Use snapd_client_get_apps2_finish()
 */
GPtrArray *
snapd_client_get_apps_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_get_apps2_finish (self, result, error);
}

/**
 * snapd_client_get_apps2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetAppsFlags to control what results are returned.
 * @snaps: (allow-none): A list of snap names to return results for. If %NULL or empty then apps for all installed snaps are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on installed apps.
 * See snapd_client_get_apps2_sync() for more information.
 *
 * Since: 1.45
 */
void
snapd_client_get_apps2_async (SnapdClient *self,
                              SnapdGetAppsFlags flags,
                              GStrv snaps,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetApps) request = _snapd_get_apps_new (snaps, cancellable, callback, user_data);
    if ((flags & SNAPD_GET_APPS_FLAGS_SELECT_SERVICES) != 0)
        _snapd_get_apps_set_select (request, "service");
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_apps2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_apps2_async().
 * See snapd_client_get_apps2_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdApp): an array of #SnapdApp or %NULL on error.
 *
 * Since: 1.45
 */
GPtrArray *
snapd_client_get_apps2_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_APPS (result), NULL);

    SnapdGetApps *request = SNAPD_GET_APPS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_apps_get_apps (request));
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
snapd_client_get_icon_async (SnapdClient *self,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetIcon) request = _snapd_get_icon_new (name, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_get_icon_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ICON (result), NULL);

    SnapdGetIcon *request = SNAPD_GET_ICON (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_icon_get_icon (request));
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
 * Deprecated: 1.42: Use snapd_client_get_snaps_async()
 */
void
snapd_client_list_async (SnapdClient *self,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return snapd_client_get_snaps_async (self, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, cancellable, callback, user_data);
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
 * Deprecated: 1.42: Use snapd_client_get_snaps_finish()
 */
GPtrArray *
snapd_client_list_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_get_snaps_finish (self, result, error);
}

/**
 * snapd_client_get_snaps_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetSnapsFlags to control what results are returned.
 * @names: (allow-none): A list of snap names to return results for. If %NULL or empty then all installed snaps are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on installed snaps.
 * See snapd_client_get_snaps_sync() for more information.
 *
 * When settings the @flags variable, only one of SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE and
 * SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED can be set. Setting both results in an error.
 *
 * Since: 1.42
 */
void
snapd_client_get_snaps_async (SnapdClient *self,
                              SnapdGetSnapsFlags flags,
                              GStrv names,
                              GCancellable *cancellable,
                              GAsyncReadyCallback callback,
                              gpointer user_data)
{
    // SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE and SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED
    // are mutually exclusive; only one can be set.
    #define SNAPD_GET_SNAPS_FLAGS_EXCLUSIVE (SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE | SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED)
    g_assert ((flags & SNAPD_GET_SNAPS_FLAGS_EXCLUSIVE) != SNAPD_GET_SNAPS_FLAGS_EXCLUSIVE);
    #undef SNAPD_GET_SNAPS_FLAGS_EXCLUSIVE

    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetSnaps) request = _snapd_get_snaps_new (cancellable, names, callback, user_data);
    if ((flags & SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE) != 0)
        _snapd_get_snaps_set_select (request, "all");
    if ((flags & SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED) != 0)
        _snapd_get_snaps_set_select (request, "refresh-inhibited");
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_snaps_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_snaps_async().
 * See snapd_client_get_snaps_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.42
 */
GPtrArray *
snapd_client_get_snaps_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SNAPS (result), NULL);

    SnapdGetSnaps *request = SNAPD_GET_SNAPS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_snaps_get_snaps (request));
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
snapd_client_get_assertions_async (SnapdClient *self,
                                   const gchar *type,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (type != NULL);

    g_autoptr(SnapdGetAssertions) request = _snapd_get_assertions_new (type, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
GStrv
snapd_client_get_assertions_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ASSERTIONS (result), NULL);

    SnapdGetAssertions *request = SNAPD_GET_ASSERTIONS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_strdupv (_snapd_get_assertions_get_assertions (request));
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
snapd_client_add_assertions_async (SnapdClient *self,
                                   GStrv assertions,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (assertions != NULL);

    g_autoptr(SnapdPostAssertions) request = _snapd_post_assertions_new (assertions, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_add_assertions_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ASSERTIONS (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
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
 * Deprecated: 1.48: Use snapd_client_get_connections2_async()
 */
void
snapd_client_get_interfaces_async (SnapdClient *self,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetInterfacesLegacy) request = _snapd_get_interfaces_legacy_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
 * Deprecated: 1.48: Use snapd_client_get_connections2_finish()
 */
gboolean
snapd_client_get_interfaces_finish (SnapdClient *self, GAsyncResult *result,
                                    GPtrArray **plugs, GPtrArray **slots,
                                    GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_INTERFACES_LEGACY (result), FALSE);

    SnapdGetInterfacesLegacy *request = SNAPD_GET_INTERFACES_LEGACY (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return FALSE;
    if (plugs)
       *plugs = g_ptr_array_ref (_snapd_get_interfaces_legacy_get_plugs (request));
    if (slots)
       *slots = g_ptr_array_ref (_snapd_get_interfaces_legacy_get_slots (request));
    return TRUE;
}

/**
 * snapd_client_get_interfaces2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetInterfacesFlags to control what information is returned about the interfaces.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of interface names or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the installed snap interfaces.
 * See snapd_client_get_interfaces2_sync() for more information.
 *
 * Since: 1.48
 */
void
snapd_client_get_interfaces2_async (SnapdClient *self,
                                    SnapdGetInterfacesFlags flags,
                                    GStrv names,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetInterfaces) request = _snapd_get_interfaces_new (names, cancellable, callback, user_data);
    if ((flags & SNAPD_GET_INTERFACES_FLAGS_INCLUDE_DOCS) != 0)
        _snapd_get_interfaces_set_include_docs (request, TRUE);
    if ((flags & SNAPD_GET_INTERFACES_FLAGS_INCLUDE_PLUGS) != 0)
        _snapd_get_interfaces_set_include_plugs (request, TRUE);
    if ((flags & SNAPD_GET_INTERFACES_FLAGS_INCLUDE_SLOTS) != 0)
        _snapd_get_interfaces_set_include_slots (request, TRUE);
    if ((flags & SNAPD_GET_INTERFACES_FLAGS_ONLY_CONNECTED) != 0)
        _snapd_get_interfaces_set_only_connected (request, TRUE);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_interfaces2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_interfaces2_async().
 * See snapd_client_get_interfaces2_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdInterface): an array of #SnapdInterface or %NULL on error.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_client_get_interfaces2_finish (SnapdClient *self,
                                     GAsyncResult *result,
                                     GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_INTERFACES (result), FALSE);

    SnapdGetInterfaces *request = SNAPD_GET_INTERFACES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_interfaces_get_interfaces (request));
}

/**
 * snapd_client_get_connections_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the installed snap connections.
 * See snapd_client_get_connections_sync() for more information.
 *
 * Since: 1.48
 * Deprecated: 1.49: Use snapd_client_get_connections2_async()
 */
void
snapd_client_get_connections_async (SnapdClient *self,
                                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_get_connections2_async (self, SNAPD_GET_CONNECTIONS_FLAGS_NONE, NULL, NULL, cancellable, callback, user_data);
}

/**
 * snapd_client_get_connections_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @established: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of connections or %NULL.
 * @undesired: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of auto-connected connections that have been manually disconnected or %NULL.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_connections_async().
 * See snapd_client_get_connections_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.48
 * Deprecated: 1.49: Use snapd_client_get_connections2_finish()
 */
gboolean
snapd_client_get_connections_finish (SnapdClient *self, GAsyncResult *result,
                                     GPtrArray **established, GPtrArray **undesired,
                                     GPtrArray **plugs, GPtrArray **slots,
                                     GError **error)
{
    return snapd_client_get_connections2_finish (self, result, established, undesired, plugs, slots, error);
}

/**
 * snapd_client_get_connections2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetConnectionsFlags to control what results are returned.
 * @snap: (allow-none): the name of the snap to get connections for or %NULL for all snaps.
 * @interface: (allow-none): the name of the interface to get connections for or %NULL for all interfaces.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the installed snap connections.
 * See snapd_client_get_connections_sync() for more information.
 *
 * Since: 1.49
 */
void
snapd_client_get_connections2_async (SnapdClient *self,
                                     SnapdGetConnectionsFlags flags, const gchar *snap, const gchar *interface,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    const gchar *select = NULL;
    if ((flags & SNAPD_GET_CONNECTIONS_FLAGS_SELECT_ALL) != 0)
        select = "all";
    g_autoptr(SnapdGetConnections) request = _snapd_get_connections_new (snap, interface, select, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_connections2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @established: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of connections or %NULL.
 * @undesired: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of auto-connected connections that have been manually disconnected or %NULL.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_connections_async().
 * See snapd_client_get_connections_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.49
 */
gboolean
snapd_client_get_connections2_finish (SnapdClient *self, GAsyncResult *result,
                                      GPtrArray **established, GPtrArray **undesired,
                                      GPtrArray **plugs, GPtrArray **slots,
                                      GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_CONNECTIONS (result), FALSE);

    SnapdGetConnections *request = SNAPD_GET_CONNECTIONS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return FALSE;
    if (established)
       *established = g_ptr_array_ref (_snapd_get_connections_get_established (request));
    if (undesired)
       *undesired = g_ptr_array_ref (_snapd_get_connections_get_undesired (request));
    if (plugs)
       *plugs = g_ptr_array_ref (_snapd_get_connections_get_plugs (request));
    if (slots)
       *slots = g_ptr_array_ref (_snapd_get_connections_get_slots (request));
    return TRUE;
}

/**
 * snapd_client_connect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_connect_interface_async (SnapdClient *self,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostInterfaces) request = _snapd_post_interfaces_new ("connect", plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_connect_interface_finish (SnapdClient *self,
                                       GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_INTERFACES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_disconnect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_disconnect_interface_async (SnapdClient *self,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostInterfaces) request = _snapd_post_interfaces_new ("disconnect", plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_disconnect_interface_finish (SnapdClient *self,
                                          GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_INTERFACES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_find_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: (allow-none): query string to send or %NULL to return featured snaps.
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
snapd_client_find_async (SnapdClient *self,
                         SnapdFindFlags flags, const gchar *query,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (query != NULL);
    snapd_client_find_category_async (self, flags, NULL, query, cancellable, callback, user_data);
}

/**
 * snapd_client_find_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
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
snapd_client_find_finish (SnapdClient *self, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    return snapd_client_find_category_finish (self, result, suggested_currency, error);
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
 * Deprecated: 1.64: Use snapd_client_find_category_async()
 */
void
snapd_client_find_section_async (SnapdClient *self,
                                 SnapdFindFlags flags, const gchar *section, const gchar *query,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetFind) request = _snapd_get_find_new (cancellable, callback, user_data);
    if ((flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
        _snapd_get_find_set_name (request, query);
    else if ((flags & SNAPD_FIND_FLAGS_MATCH_COMMON_ID) != 0)
        _snapd_get_find_set_common_id (request, query);
    else
        _snapd_get_find_set_query (request, query);
    if ((flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        _snapd_get_find_set_select (request, "private");
    else if ((flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        _snapd_get_find_set_select (request, "refresh");
    else if ((flags & SNAPD_FIND_FLAGS_SCOPE_WIDE) != 0)
        _snapd_get_find_set_scope (request, "wide");
    _snapd_get_find_set_section (request, section);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_find_section_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.7
 * Deprecated: 1.64: Use snapd_client_find_category_finish()
 */
GPtrArray *
snapd_client_find_section_finish (SnapdClient *self, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_FIND (result), NULL);

    SnapdGetFind *request = SNAPD_GET_FIND (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;

    if (suggested_currency != NULL)
        *suggested_currency = g_strdup (_snapd_get_find_get_suggested_currency (request));
    return g_ptr_array_ref (_snapd_get_find_get_snaps (request));
}

/**
 * snapd_client_find_category_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @category: (allow-none): store category to search in or %NULL to search in all categories.
 * @query: (allow-none): query string to send or %NULL to get all snaps from the given category.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_category_sync() for more information.
 *
 * Since: 1.64
 */
void
snapd_client_find_category_async (SnapdClient *self,
                                 SnapdFindFlags flags, const gchar *category, const gchar *query,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetFind) request = _snapd_get_find_new (cancellable, callback, user_data);
    if ((flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
        _snapd_get_find_set_name (request, query);
    else if ((flags & SNAPD_FIND_FLAGS_MATCH_COMMON_ID) != 0)
        _snapd_get_find_set_common_id (request, query);
    else
        _snapd_get_find_set_query (request, query);
    if ((flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        _snapd_get_find_set_select (request, "private");
    else if ((flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        _snapd_get_find_set_select (request, "refresh");
    else if ((flags & SNAPD_FIND_FLAGS_SCOPE_WIDE) != 0)
        _snapd_get_find_set_scope (request, "wide");
    _snapd_get_find_set_category (request, category);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_find_category_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_find_category_finish (SnapdClient *self, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_FIND (result), NULL);

    SnapdGetFind *request = SNAPD_GET_FIND (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;

    if (suggested_currency != NULL)
        *suggested_currency = g_strdup (_snapd_get_find_get_suggested_currency (request));
    return g_ptr_array_ref (_snapd_get_find_get_snaps (request));
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
snapd_client_find_refreshable_async (SnapdClient *self,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetFind) request = _snapd_get_find_new (cancellable, callback, user_data);
    _snapd_get_find_set_select (request, "refresh");
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_find_refreshable_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_FIND (result), NULL);

    SnapdGetFind *request = SNAPD_GET_FIND (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;

    return g_ptr_array_ref (_snapd_get_find_get_snaps (request));
}

/**
 * snapd_client_install_async:
 * @client: a #SnapdClient.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_install_async (SnapdClient *self,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_install2_async (self, SNAPD_INSTALL_FLAGS_NONE, name, channel, NULL, progress_callback, progress_callback_data, cancellable, callback, user_data);
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
snapd_client_install_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_install2_finish (self, result, error);
}

/**
 * snapd_client_install2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @revision: (allow-none): revision to install or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_install2_async (SnapdClient *self,
                             SnapdInstallFlags flags,
                             const gchar *name, const gchar *channel, const gchar *revision,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "install", progress_callback, progress_callback_data, cancellable, callback, user_data);
    _snapd_post_snap_set_channel (request, channel);
    _snapd_post_snap_set_revision (request, revision);
    if ((flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0)
        _snapd_post_snap_set_classic (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0)
        _snapd_post_snap_set_dangerous (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0)
        _snapd_post_snap_set_devmode (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0)
        _snapd_post_snap_set_jailmode (request, TRUE);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_install2_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

typedef struct
{
    SnapdClient *client;
    SnapdPostSnapStream *request;
    GCancellable *cancellable;
    GInputStream *stream;
} InstallStreamData;

static InstallStreamData *
install_stream_data_new (SnapdClient *client, SnapdPostSnapStream *request, GCancellable *cancellable, GInputStream *stream)
{
    InstallStreamData *data = g_slice_new (InstallStreamData);
    data->client = g_object_ref (client);
    data->request = g_object_ref (request);
    data->cancellable = cancellable != NULL ? g_object_ref (cancellable) : NULL;
    data->stream = g_object_ref (stream);

    return data;
}

static void
install_stream_data_free (InstallStreamData *data)
{
    g_object_unref (data->client);
    g_object_unref (data->request);
    if (data->cancellable != NULL)
        g_object_unref (data->cancellable);
    g_object_unref (data->stream);
    g_slice_free (InstallStreamData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (InstallStreamData, install_stream_data_free)

static void
stream_read_cb (GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(InstallStreamData) data = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(GBytes) read_data = g_input_stream_read_bytes_finish (data->stream, result, &error);
    if (!_snapd_request_propagate_error (SNAPD_REQUEST (data->request), &error))
        return;

    if (g_bytes_get_size (read_data) == 0)
        send_request (data->client, SNAPD_REQUEST (data->request));
    else {
        _snapd_post_snap_stream_append_data (data->request, g_bytes_get_data (read_data, NULL), g_bytes_get_size (read_data));
        InstallStreamData *d = g_steal_pointer (&data);
        g_input_stream_read_bytes_async (d->stream, 65535, G_PRIORITY_DEFAULT, d->cancellable, stream_read_cb, d);
    }
}

/**
 * snapd_client_install_stream_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @stream: a #GInputStream containing the snap file contents to install.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_install_stream_async (SnapdClient *self,
                                   SnapdInstallFlags flags,
                                   GInputStream *stream,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (G_IS_INPUT_STREAM (stream));

    g_autoptr(SnapdPostSnapStream) request = _snapd_post_snap_stream_new (progress_callback, progress_callback_data, cancellable, callback, user_data);
    if ((flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0)
        _snapd_post_snap_stream_set_classic (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0)
        _snapd_post_snap_stream_set_dangerous (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0)
        _snapd_post_snap_stream_set_devmode (request, TRUE);
    if ((flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0)
        _snapd_post_snap_stream_set_jailmode (request, TRUE);
    g_input_stream_read_bytes_async (stream, 65535, G_PRIORITY_DEFAULT, cancellable, stream_read_cb, install_stream_data_new (self, request, cancellable, stream));
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
snapd_client_install_stream_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP_STREAM (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_try_async:
 * @client: a #SnapdClient.
 * @path: path to snap directory to try.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_try_async (SnapdClient *self,
                        const gchar *path,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (path != NULL);

    g_autoptr(SnapdPostSnapTry) request = _snapd_post_snap_try_new (path, progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_try_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP_TRY (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_refresh_async:
 * @client: a #SnapdClient.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_refresh_async (SnapdClient *self,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "refresh", progress_callback, progress_callback_data, cancellable, callback, user_data);
    _snapd_post_snap_set_channel (request, channel);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_refresh_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_refresh_all_async:
 * @client: a #SnapdClient.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_refresh_all_async (SnapdClient *self,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostSnaps) request = _snapd_post_snaps_new ("refresh", progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
GStrv
snapd_client_refresh_all_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAPS (result), FALSE);

    SnapdPostSnaps *request = SNAPD_POST_SNAPS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;

    return g_strdupv (_snapd_post_snaps_get_snap_names (request));
}

/**
 * snapd_client_remove_async:
 * @client: a #SnapdClient.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously uninstall a snap.
 * See snapd_client_remove_sync() for more information.
 *
 * Since: 1.0
 * Deprecated 1.50: Use snapd_client_remove2_async()
 */
void
snapd_client_remove_async (SnapdClient *self,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_remove2_async (self, SNAPD_REMOVE_FLAGS_NONE, name, progress_callback, progress_callback_data, cancellable, callback, user_data);
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
 * Deprecated 1.50: Use snapd_client_remove2_finish()
 */
gboolean
snapd_client_remove_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    return snapd_client_remove2_finish (self, result, error);
}

/**
 * snapd_client_remove2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdRemoveFlags to control remove options.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously uninstall a snap.
 * See snapd_client_remove2_sync() for more information.
 *
 * Since: 1.50
 */
void
snapd_client_remove2_async (SnapdClient *self,
                            SnapdRemoveFlags flags,
                            const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "remove", progress_callback, progress_callback_data, cancellable, callback, user_data);
    if ((flags & SNAPD_REMOVE_FLAGS_PURGE) != 0)
        _snapd_post_snap_set_purge (request, TRUE);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_remove2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_remove2_async().
 * See snapd_client_remove2_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.50
 */
gboolean
snapd_client_remove2_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_enable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_enable_async (SnapdClient *self,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "enable", progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_enable_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_disable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
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
snapd_client_disable_async (SnapdClient *self,
                            const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "disable", progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_disable_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_switch_async:
 * @client: a #SnapdClient.
 * @name: name of snap to switch channel.
 * @channel: channel to track.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously set the tracking channel on an installed snap.
 * See snapd_client_switch_sync() for more information.
 *
 * Since: 1.26
 */
void
snapd_client_switch_async (SnapdClient *self,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostSnap) request = _snapd_post_snap_new (name, "switch", progress_callback, progress_callback_data, cancellable, callback, user_data);
    _snapd_post_snap_set_channel (request, channel);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_switch_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_switch_async().
 * See snapd_client_switch_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.26
 */
gboolean
snapd_client_switch_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
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
snapd_client_check_buy_async (SnapdClient *self,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetBuyReady) request = _snapd_get_buy_ready_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_check_buy_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_BUY_READY (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
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
snapd_client_buy_async (SnapdClient *self,
                        const gchar *id, gdouble amount, const gchar *currency,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (id != NULL);
    g_return_if_fail (currency != NULL);

    g_autoptr(SnapdPostBuy) request = _snapd_post_buy_new (id, amount, currency, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_buy_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_BUY (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
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
snapd_client_create_user_async (SnapdClient *self,
                                const gchar *email, SnapdCreateUserFlags flags,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (email != NULL);

    g_autoptr(SnapdPostCreateUser) request = _snapd_post_create_user_new (email, cancellable, callback, user_data);
    if ((flags & SNAPD_CREATE_USER_FLAGS_SUDO) != 0)
        _snapd_post_create_user_set_sudoer (request, TRUE);
    if ((flags & SNAPD_CREATE_USER_FLAGS_KNOWN) != 0)
        _snapd_post_create_user_set_known (request, TRUE);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_create_user_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_CREATE_USER (result), NULL);

    SnapdPostCreateUser *request = SNAPD_POST_CREATE_USER (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_post_create_user_get_user_information (request));
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
snapd_client_create_users_async (SnapdClient *self,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostCreateUsers) request = _snapd_post_create_users_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_create_users_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_CREATE_USERS (result), NULL);

    SnapdPostCreateUsers *request = SNAPD_POST_CREATE_USERS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_post_create_users_get_users_information (request));
}

/**
 * snapd_client_get_users_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get user accounts that are valid for this device.
 * See snapd_client_get_users_sync() for more information.
 *
 * Since: 1.26
 */
void
snapd_client_get_users_async (SnapdClient *self,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetUsers) request = _snapd_get_users_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_users_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_users_async().
 * See snapd_client_get_users_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.26
 */
GPtrArray *
snapd_client_get_users_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_USERS (result), NULL);

    SnapdGetUsers *request = SNAPD_GET_USERS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_users_get_users_information (request));
}

/**
 * snapd_client_get_sections_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the store sections.
 * See snapd_client_get_sections_sync() for more information.
 *
 * Since: 1.7
 * Deprecated: 1.64: Use snapd_client_get_categories_async()
 */
void
snapd_client_get_sections_async (SnapdClient *self,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetSections) request = _snapd_get_sections_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
 * Deprecated: 1.64: Use snapd_client_get_categories_finish()
 */
GStrv
snapd_client_get_sections_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SECTIONS (result), NULL);

    SnapdGetSections *request = SNAPD_GET_SECTIONS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_strdupv (_snapd_get_sections_get_sections (request));
}

/**
 * snapd_client_get_categories_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the store categories.
 * See snapd_client_get_categories_sync() for more information.
 *
 * Since: 1.64
 */
void
snapd_client_get_categories_async (SnapdClient *self,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetCategories) request = _snapd_get_categories_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_categories_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_categories_async().
 * See snapd_client_get_categories_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdCategoryDetails): an array of #SnapdCategoryDetails or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_get_categories_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_CATEGORIES (result), NULL);

    SnapdGetCategories *request = SNAPD_GET_CATEGORIES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_categories_get_categories (request));
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
snapd_client_get_aliases_async (SnapdClient *self,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetAliases) request = _snapd_get_aliases_new (cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
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
snapd_client_get_aliases_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ALIASES (result), NULL);

    SnapdGetAliases *request = SNAPD_GET_ALIASES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_aliases_get_aliases (request));
}

static void
send_change_aliases_request (SnapdClient *self,
                             const gchar *action,
                             const gchar *snap, const gchar *app, const gchar *alias,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(SnapdPostAliases) request = _snapd_post_aliases_new (action, snap, app, alias, progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_alias_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @app: an app in the snap to make the alias to.
 * @alias: the name of the alias (i.e. the command that will run this app).
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create an alias to an app.
 * See snapd_client_alias_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_alias_async (SnapdClient *self,
                          const gchar *snap, const gchar *app, const gchar *alias,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (app != NULL);
    g_return_if_fail (alias != NULL);
    send_change_aliases_request (self, "alias", snap, app, alias, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_alias_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_alias_async().
 * See snapd_client_alias_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_alias_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_unalias_async:
 * @client: a #SnapdClient.
 * @snap: (allow-none): the name of the snap to modify or %NULL.
 * @alias: (allow-none): the name of the alias to remove or %NULL to remove all aliases for the given snap.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously remove an alias from an app.
 * See snapd_client_unalias_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_unalias_async (SnapdClient *self,
                            const gchar *snap, const gchar *alias,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (alias != NULL);
    send_change_aliases_request (self, "unalias", snap, NULL, alias, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_unalias_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_unalias_async().
 * See snapd_client_unalias_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_unalias_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_prefer_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ???.
 * See snapd_client_prefer_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_prefer_async (SnapdClient *self,
                           const gchar *snap,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (snap != NULL);
    send_change_aliases_request (self, "prefer", snap, NULL, NULL, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_prefer_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_prefer_async().
 * See snapd_client_prefer_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_prefer_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_enable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_enable_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_alias_async()
 */
void
snapd_client_enable_aliases_async (SnapdClient *self,
                                   const gchar *snap, GStrv aliases,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(GTask) task = g_task_new (self, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_enable_aliases_async is deprecated");
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
 * Deprecated: 1.25: Use snapd_client_unalias_finish()
 */
gboolean
snapd_client_enable_aliases_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_disable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_disable_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_unalias_async()
 */
void
snapd_client_disable_aliases_async (SnapdClient *self,
                                    const gchar *snap, GStrv aliases,
                                    SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(GTask) task = g_task_new (self, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_disable_aliases_async is deprecated");
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
 * Deprecated: 1.25: Use snapd_client_unalias_finish()
 */
gboolean
snapd_client_disable_aliases_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_reset_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_reset_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_disable_aliases_async()
 */
void
snapd_client_reset_aliases_async (SnapdClient *self,
                                  const gchar *snap, GStrv aliases,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(GTask) task = g_task_new (self, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_reset_aliases_async is deprecated");
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
 * Deprecated: 1.25: Use snapd_client_disable_aliases_finish()
 */
gboolean
snapd_client_reset_aliases_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, self), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
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
 * Deprecated: 1.59: Use snapd_client_run_snapctl2_async()
 */
void
snapd_client_run_snapctl_async (SnapdClient *self,
                                const gchar *context_id, GStrv args,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return snapd_client_run_snapctl2_async (self, context_id, args, cancellable, callback, user_data);
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
 * Deprecated: 1.59: Use snapd_client_run_snapctl2_finish()
 */
gboolean
snapd_client_run_snapctl_finish (SnapdClient *self, GAsyncResult *result,
                                 gchar **stdout_output, gchar **stderr_output,
                                 GError **error)
{
    g_return_val_if_fail (SNAPD_IS_POST_SNAPCTL (result), FALSE);

    SnapdPostSnapctl *request = SNAPD_POST_SNAPCTL (result);

    /* We need to return an error for unsuccessful commands to match old API */
    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return FALSE;

    return snapd_client_run_snapctl2_finish (self, result, stdout_output, stderr_output, NULL, error);
}

/**
 * snapd_client_run_snapctl2_async:
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
 * Since: 1.59
 */
void
snapd_client_run_snapctl2_async (SnapdClient *self,
                                 const gchar *context_id, GStrv args,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (context_id != NULL);
    g_return_if_fail (args != NULL);

    g_autoptr(SnapdPostSnapctl) request = _snapd_post_snapctl_new (context_id, args, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_run_snapctl2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @stdout_output: (out) (allow-none): the location to write the stdout from the command or %NULL.
 * @stderr_output: (out) (allow-none): the location to write the stderr from the command or %NULL.
 * @exit_code: (out) (allow-none): the location to write the exit code of the command or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_run_snapctl2_async().
 * See snapd_client_run_snapctl2_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.59
 */
gboolean
snapd_client_run_snapctl2_finish (SnapdClient *self, GAsyncResult *result,
                                  gchar **stdout_output, gchar **stderr_output,
                                  int *exit_code,
                                  GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAPCTL (result), FALSE);

    SnapdPostSnapctl *request = SNAPD_POST_SNAPCTL (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error)) {
        if (g_error_matches (*error, SNAPD_ERROR, SNAPD_ERROR_UNSUCCESSFUL)) {
            /* Ignore unsuccessful errors */
            g_clear_error (error);
        } else {
            return FALSE;
        }
    }

    if (stdout_output)
        *stdout_output = g_strdup (_snapd_post_snapctl_get_stdout_output (request));
    if (stderr_output)
        *stderr_output = g_strdup (_snapd_post_snapctl_get_stderr_output (request));
    if (exit_code)
        *exit_code = _snapd_post_snapctl_get_exit_code (request);

    return TRUE;
}

/**
 * snapd_client_download_async:
 * @client: a #SnapdClient.
 * @name: name of snap to download.
 * @channel: (allow-none): channel to download from.
 * @revision: (allow-none): revision to download.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously download a snap.
 * See snapd_client_download_sync() for more information.
 *
 * Since: 1.54
 */
void
snapd_client_download_async (SnapdClient *self,
                             const gchar *name, const gchar *channel, const gchar *revision,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));
    g_return_if_fail (name != NULL);

    g_autoptr(SnapdPostDownload) request = _snapd_post_download_new (name, channel, revision, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_download_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_download_async().
 * See snapd_client_download_sync() for more information.
 *
 * Returns: the snap contents or %NULL on error.
 *
 * Since: 1.54
 */
GBytes *
snapd_client_download_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_DOWNLOAD (result), NULL);

    SnapdPostDownload *request = SNAPD_POST_DOWNLOAD (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;

    return g_bytes_ref (_snapd_post_download_get_data (request));
}

/**
 * snapd_client_check_themes_async:
 * @client: a #SnapdClient.
 * @gtk_theme_names: (allow-none): a list of GTK theme names.
 * @icon_theme_names: (allow-none): a list of icon theme names.
 * @sound_theme_names: (allow-none): a list of sound theme names.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously check for snaps providing the requested desktop themes.
 * See snapd_client_check_themes_sync() for more information.
 *
 * Since: 1.60
 */
void
snapd_client_check_themes_async (SnapdClient *self,
                                 GStrv gtk_theme_names,
                                 GStrv icon_theme_names,
                                 GStrv sound_theme_names,
                                 GCancellable *cancellable,
                                 GAsyncReadyCallback callback,
                                 gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetThemes) request = _snapd_get_themes_new (gtk_theme_names, icon_theme_names, sound_theme_names, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_check_themes_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @gtk_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of GTK themes.
 * @icon_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of icon themes.
 * @sound_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of sound themes.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_check_themes_async().
 * See snapd_client_check_themes_sync() for more information.
 *
 * Returns: %TRUE on success.
 *
 * Since: 1.60
 */
gboolean
snapd_client_check_themes_finish (SnapdClient *self, GAsyncResult *result, GHashTable **gtk_theme_status, GHashTable **icon_theme_status, GHashTable **sound_theme_status, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_THEMES (result), FALSE);

    SnapdGetThemes *request = SNAPD_GET_THEMES (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return FALSE;

    if (gtk_theme_status)
        *gtk_theme_status = g_hash_table_ref (_snapd_get_themes_get_gtk_theme_status (request));
    if (icon_theme_status)
        *icon_theme_status = g_hash_table_ref (_snapd_get_themes_get_icon_theme_status (request));
    if (sound_theme_status)
        *sound_theme_status = g_hash_table_ref (_snapd_get_themes_get_sound_theme_status (request));

    return TRUE;
}

/**
 * snapd_client_install_themes_async:
 * @client: a #SnapdClient.
 * @gtk_theme_names: (allow-none): a list of GTK theme names.
 * @icon_theme_names: (allow-none): a list of icon theme names.
 * @sound_theme_names: (allow-none): a list of sound theme names.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install snaps that provide the requested desktop themes.
 * See snapd_client_install_themes_sync() for more information.
 *
 * Since: 1.60
 */
void
snapd_client_install_themes_async (SnapdClient *self, GStrv gtk_theme_names, GStrv icon_theme_names, GStrv sound_theme_names, SnapdProgressCallback progress_callback, gpointer progress_callback_data, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdPostThemes) request = _snapd_post_themes_new (gtk_theme_names, icon_theme_names, sound_theme_names, progress_callback, progress_callback_data, cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_install_themes_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install_themes_async().
 * See snapd_client_install_themes_sync() for more information.
 *
 * Returns: %TRUE on success.
 *
 * Since: 1.60
 */
gboolean
snapd_client_install_themes_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_THEMES (result), FALSE);

    return _snapd_request_propagate_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_get_logs_async:
 * @client: a #SnapdClient.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of service names or %NULL.
 * @n: the number of logs to return or 0 for default.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get logs for snap services.
 * See snapd_client_get_logs_sync() for more information.
 *
 * Since: 1.64
 */
void
snapd_client_get_logs_async (SnapdClient *self,
                             GStrv names,
                             size_t n,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetLogs) request = _snapd_get_logs_new (names, n,
                                                           FALSE, NULL, NULL, NULL,
                                                           cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_logs_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_logs_async().
 * See snapd_client_get_logs_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdLog): an array of #SnapdLog or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_get_logs_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_LOGS (result), NULL);

    SnapdGetLogs *request = SNAPD_GET_LOGS (result);

    if (!_snapd_request_propagate_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_logs_get_logs (request));
}

/**
 * snapd_client_follow_logs_async:
 * @client: a #SnapdClient.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of service names or %NULL.
 * @log_callback: (scope async): a #SnapdLogCallback to call when a log is received.
 * @log_callback_data: (closure): the data to pass to @log_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Follow logs for snap services. This call will only complete if snapd closes the connection and will
 * stop any other request on this client from being sent.
 *
 * Since: 1.64
 */
void
snapd_client_follow_logs_async (SnapdClient *self, GStrv names,
                                SnapdLogCallback log_callback, gpointer log_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (self));

    g_autoptr(SnapdGetLogs) request = _snapd_get_logs_new (names, 0,
                                                           TRUE, log_cb, follow_logs_data_new (self, log_callback, log_callback_data), (GDestroyNotify) follow_logs_data_free,
                                                           cancellable, callback, user_data);
    send_request (self, SNAPD_REQUEST (request));
}

/**
 * snapd_client_follow_logs_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_follow_logs_async().
 * See snapd_client_follow_logs_sync() for more information.
 *
 * Returns: %TRUE on success.
 *
 * Since: 1.64
 */
gboolean
snapd_client_follow_logs_finish (SnapdClient *self, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_LOGS (result), FALSE);

    SnapdGetLogs *request = SNAPD_GET_LOGS (result);

    return _snapd_request_propagate_error (SNAPD_REQUEST (request), error);
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
    SnapdClient *self = snapd_client_new ();
    SnapdClientPrivate *priv = snapd_client_get_instance_private (SNAPD_CLIENT (self));
    priv->snapd_socket = g_object_ref (socket);
    g_socket_set_blocking (priv->snapd_socket, FALSE);

    return self;
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
    g_clear_pointer (&priv->requests, g_ptr_array_unref);
    if (priv->snapd_socket != NULL)
        g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);
    g_clear_pointer (&priv->buffer, g_byte_array_unref);
#if SOUP_CHECK_VERSION (2, 99, 2)
    g_clear_pointer (&priv->response_headers, soup_message_headers_unref);
#else
    g_clear_pointer (&priv->response_headers, soup_message_headers_free);
#endif
    g_clear_pointer (&priv->response_body, g_byte_array_unref);
    g_clear_object (&priv->maintenance);

    G_OBJECT_CLASS (snapd_client_parent_class)->finalize (object);
}

static void
snapd_client_class_init (SnapdClientClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = snapd_client_finalize;
}

static void
snapd_client_init (SnapdClient *self)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (self);

    priv->socket_path = g_strdup (SNAPD_SOCKET);
    priv->user_agent = g_strdup ("snapd-glib/" VERSION);
    priv->allow_interaction = TRUE;
    priv->requests = g_ptr_array_new_with_free_func ((GDestroyNotify) request_data_unref);
    priv->buffer = g_byte_array_new ();
    priv->response_body = g_byte_array_new ();
    // nanoseconds, by default, is set to -1 to specify that the value
    // is not set, and thus the decimal value from GDateTime should be
    // used when generating the timestamp for the AFTER field in the
    // /v2/notice method.
    priv->since_date_time_nanoseconds = -1;
    g_mutex_init (&priv->requests_mutex);
    g_mutex_init (&priv->buffer_mutex);
}
