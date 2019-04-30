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
