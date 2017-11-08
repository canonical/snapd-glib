/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-snap.h"

#include "snapd-json.h"

struct _SnapdGetSnap
{
    SnapdRequest parent_instance;
    gchar *name;
    SnapdSnap *snap;
};

G_DEFINE_TYPE (SnapdGetSnap, snapd_get_snap, snapd_request_get_type ())

SnapdGetSnap *
_snapd_get_snap_new (const gchar *name, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSnap *request;

    request = SNAPD_GET_SNAP (g_object_new (snapd_get_snap_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
    request->name = g_strdup (name);

    return request;
}

SnapdSnap *
_snapd_get_snap_get_snap (SnapdGetSnap *request)
{
    return request->snap;
}

static SoupMessage *
generate_get_snap_request (SnapdRequest *request)
{
    SnapdGetSnap *r = SNAPD_GET_SNAP (request);
    g_autofree gchar *escaped = NULL, *path = NULL;

    escaped = soup_uri_encode (r->name, NULL);
    path = g_strdup_printf ("http://snapd/v2/snaps/%s", escaped);

    return soup_message_new ("GET", path);
}

static void
parse_get_snap_response (SnapdRequest *request, SoupMessage *message)
{
    SnapdGetSnap *r = SNAPD_GET_SNAP (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
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

    snap = _snapd_json_parse_snap (result, &error);
    if (snap == NULL) {
        _snapd_request_complete (request, error);
        return;
    }

    r->snap = g_steal_pointer (&snap);
    _snapd_request_complete (request, NULL);
}

static void
snapd_get_snap_finalize (GObject *object)
{
    SnapdGetSnap *request = SNAPD_GET_SNAP (object);

    g_clear_pointer (&request->name, g_free);
    g_clear_object (&request->snap);

    G_OBJECT_CLASS (snapd_get_snap_parent_class)->finalize (object);
}

static void
snapd_get_snap_class_init (SnapdGetSnapClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_snap_request;
   request_class->parse_response = parse_get_snap_response;
   gobject_class->finalize = snapd_get_snap_finalize;
}

static void
snapd_get_snap_init (SnapdGetSnap *request)
{
}
