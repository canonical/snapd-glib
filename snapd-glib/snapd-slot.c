/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <string.h>

#include "snapd-slot.h"

/**
 * SECTION: snapd-slot
 * @short_description: Snap slots
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdSlot represents a part of a snap that can be connected to by one or
 * more #SnapdPlug from other snaps. Available slots can be queried using
 * snapd_client_get_interfaces_sync(). Plugs can be connected / disconnected
 * using snapd_client_connect_interface_sync() and
 * snapd_client_disconnect_interface_sync().
 */

/**
 * SnapdSlot:
 *
 * #SnapdSlot contains information about a Snap slot.
 *
 * Since: 1.0
 */

struct _SnapdSlot
{
    GObject parent_instance;

    gchar *name;
    gchar *snap;
    gchar *interface;
    GHashTable *attributes;
    gchar *label;
    GPtrArray *connections;
};

enum
{
    PROP_NAME = 1,
    PROP_SNAP,
    PROP_INTERFACE,
    PROP_LABEL,
    PROP_CONNECTIONS,
    PROP_ATTRIBUTES,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdSlot, snapd_slot, G_TYPE_OBJECT)

/**
 * snapd_slot_get_name:
 * @slot: a #SnapdSlot.
 *
 * Get the name of this slot.
 *
 * Returns: a name
 *
 * Since: 1.0
 */
const gchar *
snapd_slot_get_name (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->name;
}

/**
 * snapd_slot_get_snap:
 * @slot: a #SnapdSlot.
 *
 * Get the snap this slot is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.0
 */
const gchar *
snapd_slot_get_snap (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->snap;
}

/**
 * snapd_slot_get_interface:
 * @slot: a #SnapdSlot.
 *
 * Get the name of the interface this slot accepts.
 *
 * Returns: an interface name.
 *
 * Since: 1.0
 */
const gchar *
snapd_slot_get_interface (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->interface;
}

/**
 * snapd_slot_get_attribute_names:
 * @slot: a #SnapdSlot.
 * @length: (out) (allow-none): location to write number of attributes or %NULL if not required.
 *
 * Get the names of the attributes this slot has.
 *
 * Returns: (transfer full) (array zero-terminated=1): a string array of attribute names. Free with g_strfreev().
 *
 * Since: 1.3
 */
gchar **
snapd_slot_get_attribute_names (SnapdSlot *slot, guint *length)
{
    GHashTableIter iter;
    gpointer name;
    gchar **names;
    guint size, i;

    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);

    g_hash_table_iter_init (&iter, slot->attributes);
    size = g_hash_table_size (slot->attributes);
    names = g_malloc (sizeof (gchar *) * (size + 1));
    for (i = 0; g_hash_table_iter_next (&iter, &name, NULL); i++)
        names[i] = g_strdup (name);
    names[i] = NULL;

    if (length != NULL)
        *length = size;
    return names;
}

/**
 * snapd_slot_has_attribute:
 * @slot: a #SnapdSlot.
 * @name: an attribute name.
 *
 * Check if this slot has an attribute.
 *
 * Returns: %TRUE if this attribute exists.
 *
 * Since: 1.3
 */
gboolean
snapd_slot_has_attribute (SnapdSlot *slot, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), FALSE);
    return g_hash_table_contains (slot->attributes, name);
}

/**
 * snapd_slot_get_attribute:
 * @slot: a #SnapdSlot.
 * @name: an attribute name.
 *
 * Get an attribute for this interface.
 *
 * Returns: (transfer none) (allow-none): an attribute value or %NULL if not set.
 *
 * Since: 1.3
 */
GVariant *
snapd_slot_get_attribute (SnapdSlot *slot, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return g_hash_table_lookup (slot->attributes, name);
}

/**
 * snapd_slot_get_label:
 * @slot: a #SnapdSlot.
 *
 * Get a human readable label for this slot.
 *
 * Returns: a label.
 *
 * Since: 1.0
 */
const gchar *
snapd_slot_get_label (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->label;
}

/**
 * snapd_slot_get_connections:
 * @slot: a #SnapdSlot.
 *
 * Get the connections being made with this slot.
 *
 * Returns: (transfer none) (element-type SnapdConnection): an array of #SnapdConnection.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_slot_get_connections (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->connections;
}

static void
snapd_slot_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSlot *slot = SNAPD_SLOT (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (slot->name);
        slot->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (slot->snap);
        slot->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_INTERFACE:
        g_free (slot->interface);
        slot->interface = g_strdup (g_value_get_string (value));
        break;
    case PROP_LABEL:
        g_free (slot->label);
        slot->label = g_strdup (g_value_get_string (value));
        break;
    case PROP_CONNECTIONS:
        g_clear_pointer (&slot->connections, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            slot->connections = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_ATTRIBUTES:
        g_clear_pointer (&slot->attributes, g_hash_table_unref);
        if (g_value_get_boxed (value) != NULL)
            slot->attributes = g_hash_table_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_slot_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSlot *slot = SNAPD_SLOT (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, slot->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, slot->snap);
        break;
    case PROP_INTERFACE:
        g_value_set_string (value, slot->interface);
        break;
    case PROP_LABEL:
        g_value_set_string (value, slot->label);
        break;
    case PROP_CONNECTIONS:
        g_value_set_boxed (value, slot->connections);
        break;
    case PROP_ATTRIBUTES:
        g_value_set_boxed (value, slot->attributes);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_slot_finalize (GObject *object)
{
    SnapdSlot *slot = SNAPD_SLOT (object);

    g_clear_pointer (&slot->name, g_free);
    g_clear_pointer (&slot->snap, g_free);
    g_clear_pointer (&slot->interface, g_free);
    g_clear_pointer (&slot->attributes, g_hash_table_unref);
    g_clear_pointer (&slot->label, g_free);
    g_clear_pointer (&slot->connections, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_slot_parent_class)->finalize (object);
}

static void
snapd_slot_class_init (SnapdSlotClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_slot_set_property;
    gobject_class->get_property = snapd_slot_get_property;
    gobject_class->finalize = snapd_slot_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Slot name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this slot is on",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INTERFACE,
                                     g_param_spec_string ("interface",
                                                          "interface",
                                                          "Interface this slot consumes",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LABEL,
                                     g_param_spec_string ("label",
                                                          "label",
                                                          "Short description of this slot",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CONNECTIONS,
                                     g_param_spec_boxed ("connections",
                                                         "connections",
                                                         "Connections with this slot",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ATTRIBUTES,
                                     g_param_spec_boxed ("attributes",
                                                         "attributes",
                                                         "Attributes for this slot",
                                                         G_TYPE_HASH_TABLE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_slot_init (SnapdSlot *slot)
{
    slot->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
}
