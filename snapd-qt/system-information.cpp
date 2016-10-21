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

SystemInformation::SystemInformation (void *snapd_object, QObject *parent) : WrappedObject (snapd_object, g_object_unref, parent) {}

QString SystemInformation::osId ()
{
    return snapd_system_information_get_os_id (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString SystemInformation::osVersion ()
{
    return snapd_system_information_get_os_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString SystemInformation::series ()
{
    return snapd_system_information_get_series (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString SystemInformation::version ()
{
    return snapd_system_information_get_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}
