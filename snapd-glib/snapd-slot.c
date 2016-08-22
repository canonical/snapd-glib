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

struct _SnapdSlot
{
    GObject parent_instance;

    gchar *name;
    gchar *snap;
    gchar *interface;
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
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdSlot, snapd_slot, G_TYPE_OBJECT)

/**
 * snapd_slot_get_name:
 * @slot: a #SnapdSlot.
 *
 * Returns: the name of this slot.
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
 * Returns: the snap this slot is on.
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
 * Returns: the name of the interface this slot accepts.
 */
const gchar *
snapd_slot_get_interface (SnapdSlot *slot)
{
    g_return_val_if_fail (SNAPD_IS_SLOT (slot), NULL);
    return slot->interface;
}

/**
 * snapd_slot_get_label:
 * @slot: a #SnapdSlot.
 *
 * Returns: a human readable label for this slot.
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
 * Returns: (transfer none) (element-type SnapdConnection): connections being made with this slot.
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
        if (slot->connections)
            g_ptr_array_unref (slot->connections);
        slot->connections = g_ptr_array_ref (g_value_get_boxed (value));
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
    g_clear_pointer (&slot->label, g_free);
    if (slot->connections != NULL)
        g_clear_pointer (&slot->connections, g_ptr_array_unref);
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
}

static void
snapd_slot_init (SnapdSlot *slot)
{
}
