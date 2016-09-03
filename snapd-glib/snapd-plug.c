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
 * SnapdPlug:
 *
 * #SnapdPlug is an opaque data structure and can only be accessed
 * using the provided functions.
 */

struct _SnapdPlug
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
 
G_DEFINE_TYPE (SnapdPlug, snapd_plug, G_TYPE_OBJECT)

/**
 * snapd_plug_get_name:
 * @plug: a #SnapdPlug.
 *
 * Get the name of this plug.
 *
 * Returns: a name.
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
 */
const gchar *
snapd_plug_get_interface (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->interface;
}

/**
 * snapd_plug_get_label:
 * @plug: a #SnapdPlug.
 *
 * Get a human readable label for this plug.
 *
 * Returns: a label.
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
    g_clear_pointer (&plug->label, g_free);
    if (plug->connections != NULL)
        g_clear_pointer (&plug->connections, g_ptr_array_unref);
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
}

static void
snapd_plug_init (SnapdPlug *plug)
{
}
