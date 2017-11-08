/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-icon.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetIcon
{
    SnapdRequest parent_instance;
    gchar *name;
    SnapdIcon *icon;
};

G_DEFINE_TYPE (SnapdGetIcon, snapd_get_icon, snapd_request_get_type ())

SnapdGetIcon *
_snapd_get_icon_new (const gchar *name, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetIcon *request;

    request = SNAPD_GET_ICON (g_object_new (snapd_get_icon_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
    request->name = g_strdup (name);

    return request;
}

SnapdIcon *
_snapd_get_icon_get_icon (SnapdGetIcon *request)
{
    return request->icon;
}

static SoupMessage *
generate_get_icon_request (SnapdRequest *request)
{
    SnapdGetIcon *r = SNAPD_GET_ICON (request);
    g_autofree gchar *escaped = NULL, *path = NULL;

    escaped = soup_uri_encode (r->name, NULL);
    path = g_strdup_printf ("http://snapd/v2/icons/%s/icon", escaped);

    return soup_message_new ("GET", path);
}

static void
parse_get_icon_response (SnapdRequest *request, SoupMessage *message)
{
    SnapdGetIcon *r = SNAPD_GET_ICON (request);
    const gchar *content_type;
    g_autoptr(SoupBuffer) buffer = NULL;
    g_autoptr(GBytes) data = NULL;
    g_autoptr(SnapdIcon) icon = NULL;

    content_type = soup_message_headers_get_content_type (message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = NULL;
        g_autoptr(JsonObject) result = NULL;
        GError *error = NULL;

        response = _snapd_json_parse_response (message, &error);
        if (response == NULL) {
            _snapd_request_complete (request, error);
            return;
        }
        result = _snapd_json_get_sync_result_o (response, &error);
        if (result == NULL) {
            _snapd_request_complete (request, error);
            return;
        }

        error = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unknown response");
        _snapd_request_complete (request, error);
        return;
    }

    if (message->status_code != SOUP_STATUS_OK) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_READ_FAILED,
                                     "Got response %u retrieving icon", message->status_code);
        _snapd_request_complete (request, error);
    }

    buffer = soup_message_body_flatten (message->response_body);
    data = soup_buffer_get_as_bytes (buffer);
    icon = g_object_new (SNAPD_TYPE_ICON,
                         "mime-type", content_type,
                         "data", data,
                         NULL);

    r->icon = g_steal_pointer (&icon);
    _snapd_request_complete (request, NULL);
}

static void
snapd_get_icon_finalize (GObject *object)
{
    SnapdGetIcon *request = SNAPD_GET_ICON (object);

    g_clear_pointer (&request->name, g_free);
    g_clear_object (&request->icon);

    G_OBJECT_CLASS (snapd_get_icon_parent_class)->finalize (object);
}

static void
snapd_get_icon_class_init (SnapdGetIconClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_icon_request;
   request_class->parse_response = parse_get_icon_response;
   gobject_class->finalize = snapd_get_icon_finalize;
}

static void
snapd_get_icon_init (SnapdGetIcon *request)
{
}
