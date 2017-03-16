/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/plug.h"

QSnapdPlug::QSnapdPlug (void *snapd_object, QObject *parent) : QSnapdWrappedObject (snapd_object, g_object_unref, parent) {}

QString QSnapdPlug::name () const
{
    return snapd_plug_get_name (SNAPD_PLUG (wrapped_object));  
}

QString QSnapdPlug::snap () const
{
    return snapd_plug_get_snap (SNAPD_PLUG (wrapped_object));  
}

QString QSnapdPlug::interface () const
{
    return snapd_plug_get_interface (SNAPD_PLUG (wrapped_object));  
}

QString QSnapdPlug::label () const
{
    return snapd_plug_get_label (SNAPD_PLUG (wrapped_object));  
}

int QSnapdPlug::connectionCount () const
{
    GPtrArray *connections;

    connections = snapd_plug_get_connections (SNAPD_PLUG (wrapped_object));
    return connections != NULL ? connections->len : 0;
}

QSnapdConnection *QSnapdPlug::connection (int n) const
{
    GPtrArray *connections;

    connections = snapd_plug_get_connections (SNAPD_PLUG (wrapped_object));
    if (connections == NULL || n < 0 || (guint) n >= connections->len)
        return NULL;
    return new QSnapdConnection (g_object_ref (connections->pdata[n]));
}
