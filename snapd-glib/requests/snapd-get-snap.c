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
    SnapdGetSnap *self;

    self = SNAPD_GET_SNAP (g_object_new (snapd_get_snap_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
    self->name = g_strdup (name);

    return self;
}

SnapdSnap *
_snapd_get_snap_get_snap (SnapdGetSnap *self)
{
    return self->snap;
}

static SoupMessage *
generate_get_snap_request (SnapdRequest *self)
{
    SnapdGetSnap *r = SNAPD_GET_SNAP (self);
    g_autofree gchar *escaped = NULL, *path = NULL;

    escaped = soup_uri_encode (r->name, NULL);
    path = g_strdup_printf ("http://snapd/v2/snaps/%s", escaped);

    return soup_message_new ("GET", path);
}

static gboolean
parse_get_snap_response (SnapdRequest *self, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetSnap *r = SNAPD_GET_SNAP (self);
    g_autoptr(JsonObject) response = NULL;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result;
    g_autoptr(SnapdSnap) snap = NULL;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    snap = _snapd_json_parse_snap (result, error);
    json_node_unref (result);
    if (snap == NULL)
        return FALSE;

    r->snap = g_steal_pointer (&snap);

    return TRUE;
}

static void
snapd_get_snap_finalize (GObject *object)
{
    SnapdGetSnap *self = SNAPD_GET_SNAP (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_object (&self->snap);

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
snapd_get_snap_init (SnapdGetSnap *self)
{
}
