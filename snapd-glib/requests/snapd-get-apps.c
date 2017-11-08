/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-apps.h"

#include "snapd-json.h"

struct _SnapdGetApps
{
    SnapdRequest parent_instance;
    SnapdGetAppsFlags flags;
    GPtrArray *apps;
};

G_DEFINE_TYPE (SnapdGetApps, snapd_get_apps, snapd_request_get_type ())

SnapdGetApps *
_snapd_get_apps_new (SnapdGetAppsFlags flags, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetApps *request;

    request = SNAPD_GET_APPS (g_object_new (snapd_get_apps_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
    request->flags = flags;

    return request;
}

GPtrArray *
_snapd_get_apps_get_apps (SnapdGetApps *request)
{
    return request->apps;
}

static SoupMessage *
generate_get_apps_request (SnapdRequest *request)
{
    SnapdGetApps *r = SNAPD_GET_APPS (request);
    if ((r->flags & SNAPD_GET_APPS_FLAGS_SELECT_SERVICES) != 0)
        return soup_message_new ("GET", "http://snapd/v2/apps?select=service");
    else
        return soup_message_new ("GET", "http://snapd/v2/apps");
}

static gboolean
parse_get_apps_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetApps *r = SNAPD_GET_APPS (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    GPtrArray *apps;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    apps = _snapd_json_parse_app_array (result, error);
    if (apps == NULL)
        return FALSE;

    r->apps = g_steal_pointer (&apps);

    return TRUE;
}

static void
snapd_get_apps_finalize (GObject *object)
{
    SnapdGetApps *request = SNAPD_GET_APPS (object);

    g_clear_pointer (&request->apps, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_apps_parent_class)->finalize (object);
}

static void
snapd_get_apps_class_init (SnapdGetAppsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_apps_request;
   request_class->parse_response = parse_get_apps_response;
   gobject_class->finalize = snapd_get_apps_finalize;
}

static void
snapd_get_apps_init (SnapdGetApps *request)
{
}
