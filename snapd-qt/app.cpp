/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/app.h"

QSnapdApp::QSnapdApp (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdApp::name () const
{
    return snapd_app_get_name (SNAPD_APP (wrapped_object));
}

QStringList QSnapdApp::aliases () const
{
    gchar **aliases = snapd_app_get_aliases (SNAPD_APP (wrapped_object));
    QStringList result;
    for (int i = 0; aliases[i] != NULL; i++)
        result.append (aliases[i]);
    return result;
}

QSnapdEnums::DaemonType QSnapdApp::daemonType () const
{
    switch (snapd_app_get_daemon_type (SNAPD_APP (wrapped_object)))
    {
    case SNAPD_DAEMON_TYPE_NONE:
        return QSnapdEnums::DaemonTypeNone;
    default:
    case SNAPD_DAEMON_TYPE_UNKNOWN:
        return QSnapdEnums::DaemonTypeUnknown;
    case SNAPD_DAEMON_TYPE_SIMPLE:
        return QSnapdEnums::DaemonTypeSimple;
    case SNAPD_DAEMON_TYPE_FORKING:
        return QSnapdEnums::DaemonTypeForking;
    case SNAPD_DAEMON_TYPE_ONESHOT:
        return QSnapdEnums::DaemonTypeOneshot;
    case SNAPD_DAEMON_TYPE_DBUS:
        return QSnapdEnums::DaemonTypeDbus;
    case SNAPD_DAEMON_TYPE_NOTIFY:
        return QSnapdEnums::DaemonTypeNotify;
    }
}

QString QSnapdApp::desktopFile () const
{
    return snapd_app_get_desktop_file (SNAPD_APP (wrapped_object));
}
