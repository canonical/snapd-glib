/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/auth-data.h"

using namespace Snapd;

struct Snapd::AuthDataPrivate
{
    AuthDataPrivate (void *snapd_object)
    {
        auth_data = SNAPD_AUTH_DATA (g_object_ref (snapd_object));
    }
  
    ~AuthDataPrivate ()
    {
        g_object_unref (auth_data);
    }

    SnapdAuthData *auth_data;
};

AuthData::AuthData (QObject *parent, void *snapd_object) :
    QObject (parent),
    d_ptr (new AuthDataPrivate (snapd_object))
{
}

QString AuthData::macaroon ()
{
    Q_D(AuthData);
    return snapd_auth_data_get_macaroon (d->auth_data);
}

QStringList AuthData::discharges ()
{
    Q_D(AuthData);
    gchar **discharges = snapd_auth_data_get_discharges (d->auth_data);
    QStringList result;
    for (int i = 0; discharges[i] != NULL; i++)
        result.append (discharges[i]);
    return result;
}
