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

QSnapdAuthData::QSnapdAuthData (void *snapd_object, QObject *parent) :
    QSnapdWrappedObject (snapd_object, g_object_unref, parent) {}

QSnapdAuthData::QSnapdAuthData (const QString& macaroon, const QStringList& discharges, QObject *parent) :
    QSnapdWrappedObject (NULL, g_object_unref, parent)
{
    char *strv[discharges.size () + 1];
    int i;
    for (i = 0; i < discharges.size (); i++)
        strv[i] = (char *) discharges.at (i).toStdString ().c_str ();
    strv[i] = NULL;
    wrapped_object = snapd_auth_data_new (macaroon.toStdString ().c_str (), strv);
}

QString QSnapdAuthData::macaroon ()
{
    return snapd_auth_data_get_macaroon (SNAPD_AUTH_DATA (wrapped_object));
}

QStringList QSnapdAuthData::discharges ()
{
    gchar **discharges = snapd_auth_data_get_discharges (SNAPD_AUTH_DATA (wrapped_object));
    QStringList result;
    for (int i = 0; discharges[i] != NULL; i++)
        result.append (discharges[i]);
    return result;
}
