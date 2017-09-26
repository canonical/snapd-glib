/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/system-information.h"

QSnapdSystemInformation::QSnapdSystemInformation (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdSystemInformation::binariesDirectory () const
{
    return snapd_system_information_get_binaries_directory (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QSnapdEnums::SystemConfinement QSnapdSystemInformation::confinement () const
{
    switch (snapd_system_information_get_confinement (SNAPD_SYSTEM_INFORMATION (wrapped_object)))
    {
    case SNAPD_SYSTEM_CONFINEMENT_STRICT:
        return QSnapdEnums::SystemConfinement::SystemConfinementStrict;
    case SNAPD_SYSTEM_CONFINEMENT_PARTIAL:
        return QSnapdEnums::SystemConfinement::SystemConfinementPartial;
    case SNAPD_SYSTEM_CONFINEMENT_UNKNOWN:
    default:
        return QSnapdEnums::SystemConfinement::SystemConfinementUnknown;
    }
}

QString QSnapdSystemInformation::kernelVersion () const
{
    return snapd_system_information_get_kernel_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::mountDirectory () const
{
    return snapd_system_information_get_mount_directory (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::osId () const
{
    return snapd_system_information_get_os_id (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::osVersion () const
{
    return snapd_system_information_get_os_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::series () const
{
    return snapd_system_information_get_series (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::store () const
{
    return snapd_system_information_get_store (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::version () const
{
    return snapd_system_information_get_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

bool QSnapdSystemInformation::onClassic () const
{
    return snapd_system_information_get_on_classic (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

bool QSnapdSystemInformation::managed () const
{
    return snapd_system_information_get_managed (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}
