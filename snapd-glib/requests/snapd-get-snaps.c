/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-snaps.h"

#include "snapd-json.h"

struct _SnapdGetSnaps
{
    SnapdRequest parent_instance;
    GPtrArray *snaps;
};

G_DEFINE_TYPE (SnapdGetSnaps, snapd_get_snaps, snapd_request_get_type ())

SnapdGetSnaps *
_snapd_get_snaps_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_SNAPS (g_object_new (snapd_get_snaps_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
}

GPtrArray *
_snapd_get_snaps_get_snaps (SnapdGetSnaps *request)
{
    return request->snaps;
}

static SoupMessage *
generate_get_snaps_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/snaps");
}

static gboolean
parse_get_snaps_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetSnaps *r = SNAPD_GET_SNAPS (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    r->snaps = _snapd_json_parse_snap_array (result, error);
    if (r->snaps == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_get_snaps_finalize (GObject *object)
{
    SnapdGetSnaps *request = SNAPD_GET_SNAPS (object);

    g_clear_pointer (&request->snaps, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_snaps_parent_class)->finalize (object);
}

static void
snapd_get_snaps_class_init (SnapdGetSnapsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_snaps_request;
   request_class->parse_response = parse_get_snaps_response;
   gobject_class->finalize = snapd_get_snaps_finalize;
}

static void
snapd_get_snaps_init (SnapdGetSnaps *request)
{
}
