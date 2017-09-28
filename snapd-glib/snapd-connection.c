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

#include "snapd-connection.h"

/**
 * SECTION: snapd-connection
 * @short_description: Plug to slot connections
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdConnection contains information about how a #SnapdPlug is connected
 * to a #SnapdSlot. Connections are queried using snapd_plug_get_connections()
 * and snapd_slot_get_connections().
 */

/**
 * SnapdConnection:
 *
 * #SnapdConnection contains the state of Snap a interface connection.
 *
 * Since: 1.0
 */

struct _SnapdConnection
{
    GObject parent_instance;

    gchar *name;
    gchar *snap;
};

enum
{
    PROP_NAME = 1,
    PROP_SNAP,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdConnection, snapd_connection, G_TYPE_OBJECT)

/**
 * snapd_connection_get_name:
 * @connection: a #SnapdConnection.
 *
 * Get the name of this connection (i.e. a slot or plug name).
 *
 * Returns: a name.
 *
 * Since: 1.0
 */
const gchar *
snapd_connection_get_name (SnapdConnection *connection)
{
    g_return_val_if_fail (SNAPD_IS_CONNECTION (connection), NULL);
    return connection->name;
}

/**
 * snapd_connection_get_snap:
 * @connection: a #SnapdConnection.
 *
 * Get the snap this connection is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.0
 */
const gchar *
snapd_connection_get_snap (SnapdConnection *connection)
{
    g_return_val_if_fail (SNAPD_IS_CONNECTION (connection), NULL);
    return connection->snap;
}

static void
snapd_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdConnection *connection = SNAPD_CONNECTION (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (connection->name);
        connection->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (connection->snap);
        connection->snap = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_connection_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdConnection *connection = SNAPD_CONNECTION (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, connection->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, connection->snap);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_connection_finalize (GObject *object)
{
    SnapdConnection *connection = SNAPD_CONNECTION (object);

    g_clear_pointer (&connection->name, g_free);
    g_clear_pointer (&connection->snap, g_free);
}

static void
snapd_connection_class_init (SnapdConnectionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_connection_set_property;
    gobject_class->get_property = snapd_connection_get_property;
    gobject_class->finalize = snapd_connection_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Name of connection/plug on snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this connection is made to",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_connection_init (SnapdConnection *connection)
{
}
