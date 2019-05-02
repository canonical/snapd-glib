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
#include "snapd-plug-ref.h"
#include "snapd-slot.h"
#include "snapd-slot-ref.h"

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

static GVariant *node_to_variant (JsonNode *node);

static GVariant *
object_to_variant (JsonObject *object)
{
    JsonObjectIter iter;
    GType container_type = G_TYPE_INVALID;
    const gchar *name;
    JsonNode *node;
    GVariantBuilder builder;

    /* If has a consistent type, make an array of that type */
    json_object_iter_init (&iter, object);
    while (json_object_iter_next (&iter, &name, &node)) {
        GType type;
        type = json_node_get_value_type (node);
        if (container_type == G_TYPE_INVALID || type == container_type)
            container_type = type;
        else {
            container_type = G_TYPE_INVALID;
            break;
        }
    }

    switch (container_type)
    {
    case G_TYPE_BOOLEAN:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sb}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sb}", name, json_node_get_boolean (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_INT64:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sx}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sx}", name, json_node_get_int (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_DOUBLE:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sd}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sd}", name, json_node_get_double (node));
        return g_variant_builder_end (&builder);
    case G_TYPE_STRING:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{ss}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{ss}", name, json_node_get_string (node));
        return g_variant_builder_end (&builder);
    default:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
        json_object_iter_init (&iter, object);
        while (json_object_iter_next (&iter, &name, &node))
            g_variant_builder_add (&builder, "{sv}", name, node_to_variant (node));
        return g_variant_builder_end (&builder);
    }
}

static GVariant *
array_to_variant (JsonArray *array)
{
    guint i, length;
    GType container_type = G_TYPE_INVALID;
    GVariantBuilder builder;

    /* If has a consistent type, make an array of that type */
    length = json_array_get_length (array);
    for (i = 0; i < length; i++) {
        GType type;
        type = json_node_get_value_type (json_array_get_element (array, i));
        if (container_type == G_TYPE_INVALID || type == container_type)
            container_type = type;
        else {
            container_type = G_TYPE_INVALID;
            break;
        }
    }

    switch (container_type)
    {
    case G_TYPE_BOOLEAN:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ab"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "b", json_array_get_boolean_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_INT64:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ax"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "x", json_array_get_int_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_DOUBLE:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("ad"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "d", json_array_get_double_element (array, i));
        return g_variant_builder_end (&builder);
    case G_TYPE_STRING:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "s", json_array_get_string_element (array, i));
        return g_variant_builder_end (&builder);
    default:
        g_variant_builder_init (&builder, G_VARIANT_TYPE ("av"));
        for (i = 0; i < length; i++)
            g_variant_builder_add (&builder, "v", node_to_variant (json_array_get_element (array, i)));
        return g_variant_builder_end (&builder);
    }
}

static GVariant *
node_to_variant (JsonNode *node)
{
    switch (json_node_get_node_type (node))
    {
    case JSON_NODE_OBJECT:
        return object_to_variant (json_node_get_object (node));
    case JSON_NODE_ARRAY:
        return array_to_variant (json_node_get_array (node));
    case JSON_NODE_VALUE:
        switch (json_node_get_value_type (node))
        {
        case G_TYPE_BOOLEAN:
            return g_variant_new_boolean (json_node_get_boolean (node));
        case G_TYPE_INT64:
            return g_variant_new_int64 (json_node_get_int (node));
        case G_TYPE_DOUBLE:
            return g_variant_new_double (json_node_get_double (node));
        case G_TYPE_STRING:
            return g_variant_new_string (json_node_get_string (node));
        default:
            /* Should never occur - as the above are all the valid types */
            return g_variant_new ("mv", NULL);
        }
    default:
        return g_variant_new ("mv", NULL);
    }
}

static GHashTable *
get_attributes (JsonObject *object, const gchar *name, GError **error)
{
    JsonObject *attrs;
    JsonObjectIter iter;
    GHashTable *attributes;
    const gchar *attribute_name;
    JsonNode *node;

    attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    attrs = _snapd_json_get_object (object, "attrs");
    if (attrs == NULL)
        return attributes;

    json_object_iter_init (&iter, attrs);
    while (json_object_iter_next (&iter, &attribute_name, &node))
        g_hash_table_insert (attributes, g_strdup (attribute_name), node_to_variant (node));

    return attributes;
}

static gboolean
parse_get_interfaces_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetInterfaces *r = SNAPD_GET_INTERFACES (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(GPtrArray) plug_array = NULL;
    g_autoptr(GPtrArray) slot_array = NULL;
    g_autoptr(JsonArray) plugs = NULL;
    g_autoptr(JsonArray) slots = NULL;
    guint i;

    response = _snapd_json_parse_response (message, maintenance, error);
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
        g_autoptr(GPtrArray) slot_refs = NULL;
        g_autoptr(GHashTable) attributes = NULL;
        g_autoptr(SnapdPlug) plug = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected plug type");
            return FALSE;
        }
        object = json_node_get_object (node);

        if (json_object_has_member (object, "connections")) {
            slot_refs = _snapd_json_parse_slot_ref_array (json_object_get_member (object, "connections"), error);
            if (slot_refs == NULL)
                return FALSE;
        }
        else
            slot_refs = g_ptr_array_new_with_free_func (g_object_unref);

        attributes = get_attributes (object, "slot", error);
        if (attributes == NULL)
            return FALSE;

        plug = g_object_new (SNAPD_TYPE_PLUG,
                             "name", _snapd_json_get_string (object, "plug", NULL),
                             "snap", _snapd_json_get_string (object, "snap", NULL),
                             "interface", _snapd_json_get_string (object, "interface", NULL),
                             "label", _snapd_json_get_string (object, "label", NULL),
                             "connections", slot_refs,
                             "attributes", attributes,
                             // FIXME: apps
                             NULL);
        g_ptr_array_add (plug_array, g_steal_pointer (&plug));
    }
    slots = _snapd_json_get_array (result, "slots");
    slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);
        JsonObject *object;
        g_autoptr(GPtrArray) plug_refs = NULL;
        g_autoptr(GHashTable) attributes = NULL;
        g_autoptr(SnapdSlot) slot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected slot type");
            return FALSE;
        }
        object = json_node_get_object (node);

        if (json_object_has_member (object, "connections")) {
            plug_refs = _snapd_json_parse_plug_ref_array (json_object_get_member (object, "connections"), error);
            if (plug_refs == NULL)
                return FALSE;
        }
        else
            plug_refs = g_ptr_array_new_with_free_func (g_object_unref);

        attributes = get_attributes (object, "plug", error);
        if (attributes == NULL)
            return FALSE;

        slot = g_object_new (SNAPD_TYPE_SLOT,
                             "name", _snapd_json_get_string (object, "slot", NULL),
                             "snap", _snapd_json_get_string (object, "snap", NULL),
                             "interface", _snapd_json_get_string (object, "interface", NULL),
                             "label", _snapd_json_get_string (object, "label", NULL),
                             "connections", plug_refs,
                             "attributes", attributes,
                             // FIXME: apps
                             NULL);
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
