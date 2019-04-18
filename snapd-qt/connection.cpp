/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/connection.h"

QSnapdConnection::QSnapdConnection (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QSnapdSlotRef *QSnapdConnection::slot () const
{
    SnapdSlotRef *slot_ref = snapd_connection_get_slot (SNAPD_CONNECTION (wrapped_object));
    if (slot_ref == NULL)
        return NULL;
    return new QSnapdSlotRef (slot_ref);
}

QSnapdPlugRef *QSnapdConnection::plug () const
{
    SnapdPlugRef *plug_ref = snapd_connection_get_plug (SNAPD_CONNECTION (wrapped_object));
    if (plug_ref == NULL)
        return NULL;
    return new QSnapdPlugRef (plug_ref);
}

QString QSnapdConnection::interface () const
{
    return snapd_connection_get_interface (SNAPD_CONNECTION (wrapped_object));
}

bool QSnapdConnection::manual () const
{
    return snapd_connection_get_manual (SNAPD_CONNECTION (wrapped_object));
}

bool QSnapdConnection::gadget () const
{
    return snapd_connection_get_gadget (SNAPD_CONNECTION (wrapped_object));
}

QString QSnapdConnection::name () const
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    return snapd_connection_get_name (SNAPD_CONNECTION (wrapped_object));
QT_WARNING_POP
}

QString QSnapdConnection::snap () const
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    return snapd_connection_get_snap (SNAPD_CONNECTION (wrapped_object));
QT_WARNING_POP
}
