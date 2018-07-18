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
#include "snapd-plug.h"
#include "snapd-slot.h"

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
    guint i;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    plugs = _snapd_json_get_array (result, "plugs");
    plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);
        JsonObject *object;
        g_autoptr(SnapdPlug) plug = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected plug type");
            return FALSE;
        }
        object = json_node_get_object (node);

        plug = _snapd_json_parse_plug (object, error);
        if (plug == NULL)
            return FALSE;
        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = _snapd_json_get_array (result, "slots");
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);
        JsonObject *object;
        g_autoptr(SnapdSlot) slot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected slot type");
            return FALSE;
        }
        object = json_node_get_object (node);

        slot = _snapd_json_parse_slot (object, error);
        if (slot == NULL)
            return FALSE;
        g_ptr_array_add (slot_array, g_steal_pointer (&slot));
    }

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
