/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-interfaces-legacy.h"

#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-plug.h"
#include "snapd-plug-ref.h"
#include "snapd-slot.h"
#include "snapd-slot-ref.h"

struct _SnapdGetInterfacesLegacy
{
    SnapdRequest parent_instance;
    GPtrArray *plugs;
    GPtrArray *slots;
};

G_DEFINE_TYPE (SnapdGetInterfacesLegacy, snapd_get_interfaces_legacy, snapd_request_get_type ())

SnapdGetInterfacesLegacy *
_snapd_get_interfaces_legacy_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_INTERFACES_LEGACY (g_object_new (snapd_get_interfaces_legacy_get_type (),
                                                      "cancellable", cancellable,
                                                      "ready-callback", callback,
                                                      "ready-callback-data", user_data,
                                                      NULL));
}

GPtrArray *
_snapd_get_interfaces_legacy_get_plugs (SnapdGetInterfacesLegacy *self)
{
    return self->plugs;
}

GPtrArray *
_snapd_get_interfaces_legacy_get_slots (SnapdGetInterfacesLegacy *self)
{
    return self->slots;
}

static SoupMessage *
generate_get_interfaces_legacy_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/interfaces");
}

static gboolean
parse_get_interfaces_legacy_response (SnapdRequest *request, SoupMessage *message, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetInterfacesLegacy *self = SNAPD_GET_INTERFACES_LEGACY (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(JsonArray) plugs = _snapd_json_get_array (result, "plugs");
    g_autoptr(GPtrArray) plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);

        SnapdPlug *plug = _snapd_json_parse_plug (node, error);
        if (plug == NULL)
            return FALSE;

        g_ptr_array_add (plug_array, plug);
    }
    g_autoptr(JsonArray) slots = _snapd_json_get_array (result, "slots");
    g_autoptr(GPtrArray) slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);

        SnapdSlot *slot = _snapd_json_parse_slot (node, error);
        if (slot == NULL)
            return FALSE;

        g_ptr_array_add (slot_array, slot);
    }

    self->plugs = g_steal_pointer (&plug_array);
    self->slots = g_steal_pointer (&slot_array);

    return TRUE;
}

static void
snapd_get_interfaces_legacy_finalize (GObject *object)
{
    SnapdGetInterfacesLegacy *self = SNAPD_GET_INTERFACES_LEGACY (object);

    g_clear_pointer (&self->plugs, g_ptr_array_unref);
    g_clear_pointer (&self->slots, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_interfaces_legacy_parent_class)->finalize (object);
}

static void
snapd_get_interfaces_legacy_class_init (SnapdGetInterfacesLegacyClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_interfaces_legacy_request;
   request_class->parse_response = parse_get_interfaces_legacy_response;
   gobject_class->finalize = snapd_get_interfaces_legacy_finalize;
}

static void
snapd_get_interfaces_legacy_init (SnapdGetInterfacesLegacy *self)
{
}
