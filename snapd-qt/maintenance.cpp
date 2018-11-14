/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/maintenance.h"

QSnapdMaintenance::QSnapdMaintenance (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QSnapdEnums::MaintenanceKind QSnapdMaintenance::kind () const
{
    switch (snapd_maintenance_get_kind (SNAPD_MAINTENANCE (wrapped_object)))
    {
    default:
    case SNAPD_MAINTENANCE_KIND_UNKNOWN:
        return QSnapdEnums::MaintenanceKindUnknown;
    case SNAPD_MAINTENANCE_KIND_DAEMON_RESTART:
        return QSnapdEnums::MaintenanceKindDaemonRestart;
    case SNAPD_MAINTENANCE_KIND_SYSTEM_RESTART:
        return QSnapdEnums::MaintenanceKindSystemRestart;
    }
}

QString QSnapdMaintenance::message () const
{
    return snapd_maintenance_get_message (SNAPD_MAINTENANCE (wrapped_object));
}
