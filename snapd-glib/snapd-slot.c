/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-slot.h"
#include "snapd-connection.h"
#include "snapd-plug-ref.h"

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
    GPtrArray *legacy_connections;
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
snapd_slot_get_name (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return self->name;
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
snapd_slot_get_snap (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return self->snap;
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
snapd_slot_get_interface (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return self->interface;
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
GStrv
snapd_slot_get_attribute_names (SnapdSlot *self, guint *length)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);

    guint size = g_hash_table_size (self->attributes);
    GStrv names = g_malloc (sizeof (gchar *) * (size + 1));

    GHashTableIter iter;
    g_hash_table_iter_init (&iter, self->attributes);
    guint i;
    gpointer name;
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
snapd_slot_has_attribute (SnapdSlot *self, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), FALSE);
    return g_hash_table_contains (self->attributes, name);
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
snapd_slot_get_attribute (SnapdSlot *self, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return g_hash_table_lookup (self->attributes, name);
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
snapd_slot_get_label (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return self->label;
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
 * Deprecated: 1.48: Use snapd_slot_get_connected_plugs()
 */
GPtrArray *
snapd_slot_get_connections (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);

    if (self->legacy_connections != NULL)
        return self->legacy_connections;

    self->legacy_connections = g_ptr_array_new_with_free_func (g_object_unref);
    for (int i = 0; i < self->connections->len; i++) {
        SnapdPlugRef *plug_ref = g_ptr_array_index (self->connections, i);

        SnapdConnection *connection = g_object_new (SNAPD_TYPE_CONNECTION,
                                                    "name", snapd_plug_ref_get_plug (plug_ref),
                                                    "snap", snapd_plug_ref_get_snap (plug_ref),
                                                    NULL);
        g_ptr_array_add (self->legacy_connections, connection);
    }

    return self->legacy_connections;
}

/**
 * snapd_slot_get_connected_plugs:
 * @slot: a #SnapdSlot.
 *
 * Get the plugs connected to this slot.
 *
 * Returns: (transfer none) (element-type SnapdPlugRef): an array of #SnapdPlugRef.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_slot_get_connected_plugs (SnapdSlot *self)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (self), NULL);
    return self->connections;
}

static void
snapd_slot_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSlot *self = SNAPD_SLOT (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (self->name);
        self->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (self->snap);
        self->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_INTERFACE:
        g_free (self->interface);
        self->interface = g_strdup (g_value_get_string (value));
        break;
    case PROP_LABEL:
        g_free (self->label);
        self->label = g_strdup (g_value_get_string (value));
        break;
    case PROP_CONNECTIONS:
        g_clear_pointer (&self->connections, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->connections = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_ATTRIBUTES:
        g_clear_pointer (&self->attributes, g_hash_table_unref);
        if (g_value_get_boxed (value) != NULL)
            self->attributes = g_hash_table_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_slot_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSlot *self = SNAPD_SLOT (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, self->snap);
        break;
    case PROP_INTERFACE:
        g_value_set_string (value, self->interface);
        break;
    case PROP_LABEL:
        g_value_set_string (value, self->label);
        break;
    case PROP_CONNECTIONS:
        g_value_set_boxed (value, self->connections);
        break;
    case PROP_ATTRIBUTES:
        g_value_set_boxed (value, self->attributes);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_slot_finalize (GObject *object)
{
    SnapdSlot *self = SNAPD_SLOT (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->snap, g_free);
    g_clear_pointer (&self->interface, g_free);
    g_clear_pointer (&self->attributes, g_hash_table_unref);
    g_clear_pointer (&self->label, g_free);
    g_clear_pointer (&self->connections, g_ptr_array_unref);
    g_clear_pointer (&self->legacy_connections, g_ptr_array_unref);

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
snapd_slot_init (SnapdSlot *self)
{
    self->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
}
