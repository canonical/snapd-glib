/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-interfaces.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetInterfaces
{
    SnapdRequest parent_instance;
    GPtrArray *plugs;
    GPtrArray *slots;
};

G_DEFINE_TYPE (SnapdGetInterfaces, snapd_get_interfaces, snapd_request_get_type ())

SnapdGetInterfaces *
_snapd_get_interfaces_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_INTERFACES (g_object_new (snapd_get_interfaces_get_type (),
                                               "cancellable", cancellable,
                                               "ready-callback", callback,
                                               "ready-callback-data", user_data,
                                               NULL));
}

GPtrArray *
_snapd_get_interfaces_get_plugs (SnapdGetInterfaces *request)
{
    return request->plugs;
}

GPtrArray *
_snapd_get_interfaces_get_slots (SnapdGetInterfaces *request)
{
    return request->slots;
}

static SoupMessage *
generate_get_interfaces_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/interfaces");
}

static gboolean
parse_get_interfaces_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetInterfaces *r = SNAPD_GET_INTERFACES (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    plugs = _snapd_json_get_array (result, "plugs");
    plug_array = _snapd_json_parse_plug_array (plugs, error);
    if (plug_array == NULL)
        return FALSE;
    slots = _snapd_json_get_array (result, "slots");
    slot_array = _snapd_json_parse_slot_array (slots, error);
    if (slot_array == NULL)
        return FALSE;

    r->plugs = g_steal_pointer (&plug_array);
    r->slots = g_steal_pointer (&slot_array);

    return TRUE;
}

static void
snapd_get_interfaces_finalize (GObject *object)
{
    SnapdGetInterfaces *request = SNAPD_GET_INTERFACES (object);

    g_clear_pointer (&request->plugs, g_ptr_array_unref);
    g_clear_pointer (&request->slots, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_interfaces_parent_class)->finalize (object);
}

static void
snapd_get_interfaces_class_init (SnapdGetInterfacesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_interfaces_request;
   request_class->parse_response = parse_get_interfaces_response;
   gobject_class->finalize = snapd_get_interfaces_finalize;
}

static void
snapd_get_interfaces_init (SnapdGetInterfaces *request)
{
}
