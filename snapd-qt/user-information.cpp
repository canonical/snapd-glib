/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/user-information.h"

QSnapdUserInformation::QSnapdUserInformation (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

int QSnapdUserInformation::id () const
{
    return snapd_user_information_get_id (SNAPD_USER_INFORMATION (wrapped_object));
}

QString QSnapdUserInformation::username () const
{
    return snapd_user_information_get_username (SNAPD_USER_INFORMATION (wrapped_object));
}

QString QSnapdUserInformation::email () const
{
    return snapd_user_information_get_email (SNAPD_USER_INFORMATION (wrapped_object));
}

QStringList QSnapdUserInformation::sshKeys () const
{
    gchar **ssh_keys = snapd_user_information_get_ssh_keys (SNAPD_USER_INFORMATION (wrapped_object));
    QStringList result;
    for (int i = 0; ssh_keys[i] != NULL; i++)
        result.append (ssh_keys[i]);
    return result;
}

QSnapdAuthData *QSnapdUserInformation::authData () const
{
    return new QSnapdAuthData (snapd_user_information_get_auth_data (SNAPD_USER_INFORMATION (wrapped_object)));
}
