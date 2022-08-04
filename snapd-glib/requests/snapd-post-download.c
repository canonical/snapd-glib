/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-download.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostDownload
{
    SnapdRequest parent_instance;
    gchar *name;
    gchar *channel;
    gchar *revision;
    GBytes *data;
};

G_DEFINE_TYPE (SnapdPostDownload, snapd_post_download, snapd_request_get_type ())

SnapdPostDownload *
_snapd_post_download_new (const gchar *name, const gchar *channel, const gchar *revision,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostDownload *self = SNAPD_POST_DOWNLOAD (g_object_new (snapd_post_download_get_type (),
                                                                 "cancellable", cancellable,
                                                                 "ready-callback", callback,
                                                                 "ready-callback-data", user_data,
                                                                 NULL));
    self->name = g_strdup (name);
    self->channel = g_strdup (channel);
    self->revision = g_strdup (revision);

    return self;
}

static SoupMessage *
generate_post_download_request (SnapdRequest *request)
{
    SnapdPostDownload *self = SNAPD_POST_DOWNLOAD (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/download");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-name");
    json_builder_add_string_value (builder, self->name);
    if (self->channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, self->channel);
    }
    if (self->revision != NULL) {
        json_builder_set_member_name (builder, "revision");
        json_builder_add_string_value (builder, self->revision);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
parse_post_download_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostDownload *self = SNAPD_POST_DOWNLOAD (request);

    const gchar *content_type = soup_message_headers_get_content_type (message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/octet-stream") != 0) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unknown response");
        return FALSE;
    }

    g_autoptr(SoupBuffer) buffer = soup_message_body_flatten (message->response_body);
    self->data = soup_buffer_get_as_bytes (buffer);

    return TRUE;
}

static void
snapd_post_download_finalize (GObject *object)
{
    SnapdPostDownload *self = SNAPD_POST_DOWNLOAD (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->channel, g_free);
    g_clear_pointer (&self->revision, g_free);
    g_clear_pointer (&self->data, g_bytes_unref);

    G_OBJECT_CLASS (snapd_post_download_parent_class)->finalize (object);
}

static void
snapd_post_download_class_init (SnapdPostDownloadClass *klass)
{
    SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    request_class->generate_request = generate_post_download_request;
    request_class->parse_response = parse_post_download_response;
    gobject_class->finalize = snapd_post_download_finalize;
}

static void
snapd_post_download_init (SnapdPostDownload *self)
{
}

GBytes *
_snapd_post_download_get_data (SnapdPostDownload *self)
{
    g_return_val_if_fail (SNAPD_IS_POST_DOWNLOAD (self), NULL);
    return self->data;
}
