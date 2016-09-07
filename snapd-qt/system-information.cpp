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

using namespace Snapd;

struct Snapd::SystemInformationPrivate
{
    SystemInformationPrivate (void *snapd_object)
    {
        system_information = SNAPD_SYSTEM_INFORMATION (g_object_ref (snapd_object));
    }
  
    ~SystemInformationPrivate ()
    {
        g_object_unref (system_information);
    }

    SnapdSystemInformation *system_information;
};

SystemInformation::SystemInformation (QObject *parent, void *snapd_object) :
    QObject (parent),
    d_ptr (new SystemInformationPrivate (snapd_object)) {}

QString SystemInformation::osId ()
{
    Q_D(SystemInformation);
    return snapd_system_information_get_os_id (d->system_information);
}

QString SystemInformation::osVersion ()
{
    Q_D(SystemInformation);
    return snapd_system_information_get_os_version (d->system_information);
}

QString SystemInformation::series ()
{
    Q_D(SystemInformation);
    return snapd_system_information_get_series (d->system_information);
}

QString SystemInformation::version ()
{
    Q_D(SystemInformation);
    return snapd_system_information_get_version (d->system_information);
}
