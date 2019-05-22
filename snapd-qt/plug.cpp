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
#include "variant.h"

QSnapdPlug::QSnapdPlug (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

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

QStringList QSnapdPlug::attributeNames () const
{
    g_auto(GStrv) names = snapd_plug_get_attribute_names (SNAPD_PLUG (wrapped_object), NULL);
    QStringList result;
    for (int i = 0; names[i] != NULL; i++)
        result.append (names[i]);
    return result;
}

bool QSnapdPlug::hasAttribute (const QString &name) const
{
    return snapd_plug_has_attribute (SNAPD_PLUG (wrapped_object), name.toStdString ().c_str ());
}

QVariant QSnapdPlug::attribute (const QString &name) const
{
    GVariant *value = snapd_plug_get_attribute (SNAPD_PLUG (wrapped_object), name.toStdString ().c_str ());
    return gvariant_to_qvariant (value);
}

QString QSnapdPlug::label () const
{
    return snapd_plug_get_label (SNAPD_PLUG (wrapped_object));
}

int QSnapdPlug::connectionCount () const
{
    GPtrArray *connections;

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    connections = snapd_plug_get_connections (SNAPD_PLUG (wrapped_object));
QT_WARNING_POP
    return connections != NULL ? connections->len : 0;
}

QSnapdConnection *QSnapdPlug::connection (int n) const
{
    GPtrArray *connections;

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    connections = snapd_plug_get_connections (SNAPD_PLUG (wrapped_object));
QT_WARNING_POP
    if (connections == NULL || n < 0 || (guint) n >= connections->len)
        return NULL;
    return new QSnapdConnection (connections->pdata[n]);
}

int QSnapdPlug::connectedSlotCount () const
{
    GPtrArray *connections;

    connections = snapd_plug_get_connected_slots (SNAPD_PLUG (wrapped_object));
    return connections != NULL ? connections->len : 0;
}

QSnapdSlotRef *QSnapdPlug::connectedSlot (int n) const
{
    GPtrArray *connections;

    connections = snapd_plug_get_connected_slots (SNAPD_PLUG (wrapped_object));
    if (connections == NULL || n < 0 || (guint) n >= connections->len)
        return NULL;
    return new QSnapdSlotRef (connections->pdata[n]);
}
