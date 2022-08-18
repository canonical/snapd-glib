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
    SnapdGetIcon *self = SNAPD_GET_ICON (g_object_new (snapd_get_icon_get_type (),
                                                       "cancellable", cancellable,
                                                       "ready-callback", callback,
                                                       "ready-callback-data", user_data,
                                                       NULL));
    self->name = g_strdup (name);

    return self;
}

SnapdIcon *
_snapd_get_icon_get_icon (SnapdGetIcon *self)
{
    return self->icon;
}

static SoupMessage *
generate_get_icon_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetIcon *self = SNAPD_GET_ICON (request);

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/icons/");
    g_string_append_uri_escaped (path, self->name, NULL, TRUE);
    g_string_append (path, "/icon");

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_icon_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetIcon *self = SNAPD_GET_ICON (request);

    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
        if (response == NULL)
            return FALSE;
        g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o (response, error);
        if (result == NULL)
            return FALSE;

        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unknown response");
        return FALSE;
    }

    if (status_code != SOUP_STATUS_OK) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Got response %u retrieving icon", status_code);
        return FALSE;
    }

    g_autoptr(SnapdIcon) icon = g_object_new (SNAPD_TYPE_ICON,
                                              "mime-type", content_type,
                                              "data", body,
                                              NULL);

    self->icon = g_steal_pointer (&icon);

    return TRUE;
}

static void
snapd_get_icon_finalize (GObject *object)
{
    SnapdGetIcon *self = SNAPD_GET_ICON (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_object (&self->icon);

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
snapd_get_icon_init (SnapdGetIcon *self)
{
}
