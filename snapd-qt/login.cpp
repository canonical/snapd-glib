/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/login.h"

using namespace Snapd;

AuthData Snapd::loginSync (const QString &username, const QString &password, const QString &otp)
{
    g_autoptr(SnapdAuthData) auth_data = NULL;
    auth_data = snapd_login_sync (username.toLocal8Bit ().data (),
                                  password.toLocal8Bit ().data (),
                                  otp.isEmpty () ? NULL : otp.toLocal8Bit ().data (),
                                  NULL, NULL);

    return AuthData (NULL, auth_data);
}
