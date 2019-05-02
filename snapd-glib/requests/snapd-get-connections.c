/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-connections.h"

#include "snapd-connection.h"
#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-plug.h"
#include "snapd-plug-ref.h"
#include "snapd-slot.h"
#include "snapd-slot-ref.h"

struct _SnapdGetConnections
{
    SnapdRequest parent_instance;
    GPtrArray *established;
    GPtrArray *plugs;
    GPtrArray *slots;
    GPtrArray *undesired;
};

G_DEFINE_TYPE (SnapdGetConnections, snapd_get_connections, snapd_request_get_type ())

SnapdGetConnections *
_snapd_get_connections_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_CONNECTIONS (g_object_new (snapd_get_connections_get_type (),
                                                "cancellable", cancellable,
                                                "ready-callback", callback,
                                                "ready-callback-data", user_data,
                                                NULL));
}

GPtrArray *
_snapd_get_connections_get_established (SnapdGetConnections *request)
{
    return request->established;
}

GPtrArray *
_snapd_get_connections_get_plugs (SnapdGetConnections *request)
{
    return request->plugs;
}

GPtrArray *
_snapd_get_connections_get_slots (SnapdGetConnections *request)
{
    return request->slots;
}

GPtrArray *
_snapd_get_connections_get_undesired (SnapdGetConnections *request)
{
    return request->undesired;
}

static SoupMessage *
generate_get_connections_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/connections");
}

static gboolean
parse_get_connections_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetConnections *r = SNAPD_GET_CONNECTIONS (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(JsonArray) established = NULL;
    g_autoptr(JsonArray) undesired = NULL;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;
    g_autoptr(GPtrArray) established_array = NULL;
    g_autoptr(GPtrArray) undesired_array = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    guint i;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    established = _snapd_json_get_array (result, "established");
    established_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (established); i++) {
        JsonNode *node = json_array_get_element (established, i);
        g_autoptr(SnapdConnection) connection = NULL;

        connection = _snapd_json_parse_connection (node, error);
        if (connection == NULL)
            return FALSE;

        g_ptr_array_add (established_array, g_steal_pointer (&connection));
    }

    undesired = _snapd_json_get_array (result, "undesired");
    undesired_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (undesired); i++) {
        JsonNode *node = json_array_get_element (undesired, i);
        SnapdConnection *connection;

        connection = _snapd_json_parse_connection (node, error);
        if (connection == NULL)
            return FALSE;

        g_ptr_array_add (undesired_array, connection);
    }

    plugs = _snapd_json_get_array (result, "plugs");
    plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);
        SnapdPlug *plug;

        plug = _snapd_json_parse_plug (node, error);
        if (plug == NULL)
            return FALSE;

        g_ptr_array_add (plug_array, plug);
    }

    slots = _snapd_json_get_array (result, "slots");
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);
        SnapdSlot *slot;

        slot = _snapd_json_parse_slot (node, error);
        if (slot == NULL)
            return FALSE;

        g_ptr_array_add (slot_array, slot);
    }

    r->established = g_steal_pointer (&established_array);
    r->undesired = g_steal_pointer (&undesired_array);
    r->plugs = g_steal_pointer (&plug_array);
    r->slots = g_steal_pointer (&slot_array);

    return TRUE;
}

static void
snapd_get_connections_finalize (GObject *object)
{
    SnapdGetConnections *request = SNAPD_GET_CONNECTIONS (object);

    g_clear_pointer (&request->established, g_ptr_array_unref);
    g_clear_pointer (&request->plugs, g_ptr_array_unref);
    g_clear_pointer (&request->slots, g_ptr_array_unref);
    g_clear_pointer (&request->undesired, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_connections_parent_class)->finalize (object);
}

static void
snapd_get_connections_class_init (SnapdGetConnectionsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_connections_request;
   request_class->parse_response = parse_get_connections_response;
   gobject_class->finalize = snapd_get_connections_finalize;
}

static void
snapd_get_connections_init (SnapdGetConnections *request)
{
}
