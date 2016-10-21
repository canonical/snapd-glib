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

AuthData::AuthData (void *snapd_object, QObject *parent) :
    QObject (parent),
    d_ptr (new AuthDataPrivate (snapd_object))
{
}

AuthData::AuthData (const QString& macaroon, const QStringList& discharges, QObject *parent) :
    QObject (parent),
    d_ptr (new AuthDataPrivate (NULL))
{
    Q_D(AuthData);
    char *strv[discharges.size () + 1];
    int i;
    for (i = 0; i < discharges.size (); i++)
        strv[i] = (char *) discharges.at (i).toStdString ().c_str ();
    strv[i] = NULL;
    d->auth_data = snapd_auth_data_new (macaroon.toStdString ().c_str (), strv);
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
