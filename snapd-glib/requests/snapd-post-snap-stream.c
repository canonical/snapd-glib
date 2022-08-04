/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-post-snap-stream.h"

#include "snapd-json.h"

struct _SnapdPostSnapStream
{
    SnapdRequestAsync parent_instance;
    gboolean classic;
    gboolean dangerous;
    gboolean devmode;
    gboolean jailmode;
    GByteArray *snap_contents;
};

G_DEFINE_TYPE (SnapdPostSnapStream, snapd_post_snap_stream, snapd_request_async_get_type ())

SnapdPostSnapStream *
_snapd_post_snap_stream_new (SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapStream *self = SNAPD_POST_SNAP_STREAM (g_object_new (snapd_post_snap_stream_get_type (),
                                                                      "cancellable", cancellable,
                                                                      "ready-callback", callback,
                                                                      "ready-callback-data", user_data,
                                                                      "progress-callback", progress_callback,
                                                                      "progress-callback-data", progress_callback_data,
                                                                      NULL));

    return self;
}

void
_snapd_post_snap_stream_set_classic (SnapdPostSnapStream *self, gboolean classic)
{
    self->classic = classic;
}

void
_snapd_post_snap_stream_set_dangerous (SnapdPostSnapStream *self, gboolean dangerous)
{
    self->dangerous = dangerous;
}

void
_snapd_post_snap_stream_set_devmode (SnapdPostSnapStream *self, gboolean devmode)
{
    self->devmode = devmode;
}

void
_snapd_post_snap_stream_set_jailmode (SnapdPostSnapStream *self, gboolean jailmode)
{
    self->jailmode = jailmode;
}

void
_snapd_post_snap_stream_append_data (SnapdPostSnapStream *self, const guint8 *data, guint len)
{
    g_byte_array_append (self->snap_contents, data, len);
}

static void
append_multipart_value (SoupMultipart *multipart, const gchar *name, const gchar *value)
{
    g_autoptr(SoupMessageHeaders) headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_MULTIPART);
    g_autoptr(GHashTable) params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup (name));
    soup_message_headers_set_content_disposition (headers, "form-data", params);
#if SOUP_CHECK_VERSION (2, 99, 2)
    g_autoptr(GBytes) buffer = g_bytes_new (value, strlen (value));
#else
    g_autoptr(SoupBuffer) buffer = soup_buffer_new_take ((guchar *) g_strdup (value), strlen (value));
#endif
    soup_multipart_append_part (multipart, headers, buffer);
}

static SoupMessage *
generate_post_snap_stream_request (SnapdRequest *request, GBytes **body)
{
    SnapdPostSnapStream *self = SNAPD_POST_SNAP_STREAM (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/snaps");

    g_autoptr(SoupMultipart) multipart = soup_multipart_new ("multipart/form-data");
    if (self->classic)
        append_multipart_value (multipart, "classic", "true");
    if (self->dangerous)
        append_multipart_value (multipart, "dangerous", "true");
    if (self->devmode)
        append_multipart_value (multipart, "devmode", "true");
    if (self->jailmode)
        append_multipart_value (multipart, "jailmode", "true");

    g_autoptr(GHashTable) params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup ("snap"));
    g_hash_table_insert (params, g_strdup ("filename"), g_strdup ("x"));
#if SOUP_CHECK_VERSION (2, 99, 2)
    SoupMessageHeaders *request_headers = soup_message_get_request_headers (message);
#else
    SoupMessageHeaders *request_headers = message->request_headers;
#endif
    soup_message_headers_set_content_disposition (request_headers, "form-data", params);
    soup_message_headers_set_content_type (request_headers, "application/vnd.snap", NULL);
#if SOUP_CHECK_VERSION (2, 99, 2)
    g_autoptr(GBytes) part_buffer = g_bytes_new_static (self->snap_contents->data, self->snap_contents->len);
#else
    g_autoptr(SoupBuffer) part_buffer = soup_buffer_new (SOUP_MEMORY_STATIC, self->snap_contents->data, self->snap_contents->len);
#endif
    soup_multipart_append_part (multipart, request_headers, part_buffer);
#if SOUP_CHECK_VERSION (2, 99, 2)
    soup_multipart_to_message (multipart, request_headers, body);
#else
    g_autoptr(SoupMessageBody) b = soup_message_body_new ();
    soup_multipart_to_message (multipart, request_headers, b);
    g_autoptr(SoupBuffer) buffer = soup_message_body_flatten (b);
    *body = g_bytes_new (buffer->data, buffer->length);
#endif

    return message;
}

static void
snapd_post_snap_stream_finalize (GObject *object)
{
    SnapdPostSnapStream *self = SNAPD_POST_SNAP_STREAM (object);

    g_clear_pointer (&self->snap_contents, g_byte_array_unref);

    G_OBJECT_CLASS (snapd_post_snap_stream_parent_class)->finalize (object);
}

static void
snapd_post_snap_stream_class_init (SnapdPostSnapStreamClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_stream_request;
   gobject_class->finalize = snapd_post_snap_stream_finalize;
}

static void
snapd_post_snap_stream_init (SnapdPostSnapStream *self)
{
    self->snap_contents = g_byte_array_new ();
}
