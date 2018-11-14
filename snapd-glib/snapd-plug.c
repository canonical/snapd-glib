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

#include "snapd-plug.h"

/**
 * SECTION: snapd-plug
 * @short_description: Snap plugs
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPlug represents a part of a snap that can be connected to a
 * #SnapdSlot on another snap. Available plugs can be queried using
 * snapd_client_get_interfaces_sync(). Plugs can be connected / disconnected
 * using snapd_client_connect_interface_sync() and
 * snapd_client_disconnect_interface_sync().
 */

/**
 * SnapdPlug:
 *
 * #SnapdPlug contains information about a Snap plug.
 *
 * Since: 1.0
 */

struct _SnapdPlug
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

G_DEFINE_TYPE (SnapdPlug, snapd_plug, G_TYPE_OBJECT)

/**
 * snapd_plug_get_name:
 * @plug: a #SnapdPlug.
 *
 * Get the name of this plug.
 *
 * Returns: a name.
 *
 * Since: 1.0
 */
const gchar *
snapd_plug_get_name (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->name;
}

/**
 * snapd_plug_get_snap:
 * @plug: a #SnapdPlug.
 *
 * Get the snap this plug is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.0
 */
const gchar *
snapd_plug_get_snap (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->snap;
}

/**
 * snapd_plug_get_interface:
 * @plug: a #SnapdPlug.
 *
 * Get the name of the interface this plug provides.
 *
 * Returns: an interface name.
 *
 * Since: 1.0
 */
const gchar *
snapd_plug_get_interface (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->interface;
}

/**
 * snapd_plug_get_attribute_names:
 * @plug: a #SnapdPlug.
 * @length: (out) (allow-none): location to write number of attributes or %NULL if not required.
 *
 * Get the names of the attributes this plug has.
 *
 * Returns: (transfer full) (array zero-terminated=1): a string array of attribute names. Free with g_strfreev().
 *
 * Since: 1.3
 */
GStrv
snapd_plug_get_attribute_names (SnapdPlug *plug, guint *length)
{
    GHashTableIter iter;
    gpointer name;
    GStrv names;
    guint size, i;

    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);

    g_hash_table_iter_init (&iter, plug->attributes);
    size = g_hash_table_size (plug->attributes);
    names = g_malloc (sizeof (gchar *) * (size + 1));
    for (i = 0; g_hash_table_iter_next (&iter, &name, NULL); i++)
        names[i] = g_strdup (name);
    names[i] = NULL;

    if (length != NULL)
        *length = size;
    return names;
}

/**
 * snapd_plug_has_attribute:
 * @plug: a #SnapdPlug.
 * @name: an attribute name.
 *
 * Check if this plug has an attribute.
 *
 * Returns: %TRUE if this attribute exists.
 *
 * Since: 1.3
 */
gboolean
snapd_plug_has_attribute (SnapdPlug *plug, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), FALSE);
    return g_hash_table_contains (plug->attributes, name);
}

/**
 * snapd_plug_get_attribute:
 * @plug: a #SnapdPlug.
 * @name: an attribute name.
 *
 * Get an attribute for this interface.
 *
 * Returns: (transfer none) (allow-none): an attribute value or %NULL if not set.
 *
 * Since: 1.3
 */
GVariant *
snapd_plug_get_attribute (SnapdPlug *plug, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return g_hash_table_lookup (plug->attributes, name);
}

/**
 * snapd_plug_get_label:
 * @plug: a #SnapdPlug.
 *
 * Get a human readable label for this plug.
 *
 * Returns: a label.
 *
 * Since: 1.0
 */
const gchar *
snapd_plug_get_label (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->label;
}

/**
 * snapd_plug_get_connections:
 * @plug: a #SnapdPlug.
 *
 * Get the connections being made with this plug.
 *
 * Returns: (transfer none) (element-type SnapdConnection): an array of #SnapdConnection.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_plug_get_connections (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->connections;
}

static void
snapd_plug_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (plug->name);
        plug->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (plug->snap);
        plug->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_INTERFACE:
        g_free (plug->interface);
        plug->interface = g_strdup (g_value_get_string (value));
        break;
    case PROP_LABEL:
        g_free (plug->label);
        plug->label = g_strdup (g_value_get_string (value));
        break;
    case PROP_CONNECTIONS:
        g_clear_pointer (&plug->connections, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            plug->connections = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_ATTRIBUTES:
        g_clear_pointer (&plug->attributes, g_hash_table_unref);
        if (g_value_get_boxed (value) != NULL)
            plug->attributes = g_hash_table_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, plug->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, plug->snap);
        break;
    case PROP_INTERFACE:
        g_value_set_string (value, plug->interface);
        break;
    case PROP_LABEL:
        g_value_set_string (value, plug->label);
        break;
    case PROP_CONNECTIONS:
        g_value_set_boxed (value, plug->connections);
        break;
    case PROP_ATTRIBUTES:
        g_value_set_boxed (value, plug->attributes);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_finalize (GObject *object)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    g_clear_pointer (&plug->name, g_free);
    g_clear_pointer (&plug->snap, g_free);
    g_clear_pointer (&plug->interface, g_free);
    g_clear_pointer (&plug->attributes, g_hash_table_unref);
    g_clear_pointer (&plug->label, g_free);
    g_clear_pointer (&plug->connections, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_plug_parent_class)->finalize (object);
}

static void
snapd_plug_class_init (SnapdPlugClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_plug_set_property;
    gobject_class->get_property = snapd_plug_get_property;
    gobject_class->finalize = snapd_plug_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Plug name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this plug is on",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INTERFACE,
                                     g_param_spec_string ("interface",
                                                          "interface",
                                                          "Interface this plug provides",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LABEL,
                                     g_param_spec_string ("label",
                                                          "label",
                                                          "Short description of this plug",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CONNECTIONS,
                                     g_param_spec_boxed ("connections",
                                                         "connections",
                                                         "Connections with this plug",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ATTRIBUTES,
                                     g_param_spec_boxed ("attributes",
                                                         "attributes",
                                                         "Attributes for this plug",
                                                         G_TYPE_HASH_TABLE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_plug_init (SnapdPlug *plug)
{
    plug->attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
}
