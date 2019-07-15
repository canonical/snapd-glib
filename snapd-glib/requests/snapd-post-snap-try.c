/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-post-snap-try.h"

#include "snapd-json.h"

struct _SnapdPostSnapTry
{
    SnapdRequestAsync parent_instance;
    gchar *path;
};

G_DEFINE_TYPE (SnapdPostSnapTry, snapd_post_snap_try, snapd_request_async_get_type ())

SnapdPostSnapTry *
_snapd_post_snap_try_new (const gchar *path,
                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapTry *self = SNAPD_POST_SNAP_TRY (g_object_new (snapd_post_snap_try_get_type (),
                                                                "cancellable", cancellable,
                                                                "ready-callback", callback,
                                                                "ready-callback-data", user_data,
                                                                "progress-callback", progress_callback,
                                                                "progress-callback-data", progress_callback_data,
                                                                NULL));
    self->path = g_strdup (path);

    return self;
}

static void
append_multipart_value (SoupMultipart *multipart, const gchar *name, const gchar *value)
{
    g_autoptr(SoupMessageHeaders) headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_MULTIPART);
    g_autoptr(GHashTable) params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup (name));
    soup_message_headers_set_content_disposition (headers, "form-data", params);
    g_autoptr(SoupBuffer) buffer = soup_buffer_new_take ((guchar *) g_strdup (value), strlen (value));
    soup_multipart_append_part (multipart, headers, buffer);
}

static SoupMessage *
generate_post_snap_try_request (SnapdRequest *request)
{
    SnapdPostSnapTry *self = SNAPD_POST_SNAP_TRY (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/snaps");

    g_autoptr(SoupMultipart) multipart = soup_multipart_new ("multipart/form-data");
    append_multipart_value (multipart, "action", "try");
    append_multipart_value (multipart, "snap-path", self->path);
    soup_multipart_to_message (multipart, message->request_headers, message->request_body);
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);

    return message;
}

static void
snapd_post_snap_try_finalize (GObject *object)
{
    SnapdPostSnapTry *self = SNAPD_POST_SNAP_TRY (object);

    g_clear_pointer (&self->path, g_free);

    G_OBJECT_CLASS (snapd_post_snap_try_parent_class)->finalize (object);
}

static void
snapd_post_snap_try_class_init (SnapdPostSnapTryClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_try_request;
   gobject_class->finalize = snapd_post_snap_try_finalize;
}

static void
snapd_post_snap_try_init (SnapdPostSnapTry *self)
{
}
