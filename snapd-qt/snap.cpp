/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/snap.h"

QSnapdSnap::QSnapdSnap (void *snapd_object, QObject *parent) :
    QSnapdWrappedObject (snapd_object, g_object_unref, parent) {}

int QSnapdSnap::appCount () const
{
    GPtrArray *apps;

    apps = snapd_snap_get_apps (SNAPD_SNAP (wrapped_object));
    return apps != NULL ? apps->len : 0;
}

QSnapdApp *QSnapdSnap::app (int n) const
{
    GPtrArray *apps;

    apps = snapd_snap_get_apps (SNAPD_SNAP (wrapped_object));
    if (apps == NULL || n < 0 || (guint) n >= apps->len)
        return NULL;
    return new QSnapdApp (g_object_ref (apps->pdata[n]));
}

QString QSnapdSnap::channel ()
{
    return snapd_snap_get_channel (SNAPD_SNAP (wrapped_object));
}

/* FIXME
QSnapdSnap::Confinement QSnapdSnap::confinement ()
{
}*/

QString QSnapdSnap::description ()
{
    return snapd_snap_get_description (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::developer ()
{
    return snapd_snap_get_developer (SNAPD_SNAP (wrapped_object));
}

bool QSnapdSnap::devmode ()
{
    return snapd_snap_get_devmode (SNAPD_SNAP (wrapped_object));
}

qint64 QSnapdSnap::downloadSize ()
{
    return snapd_snap_get_download_size (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::icon ()
{
    return snapd_snap_get_icon (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::id ()
{
    return snapd_snap_get_id (SNAPD_SNAP (wrapped_object));
}

/* FIXME
GDateTime QSnapdSnap::installDate ()
{
}*/

qint64 QSnapdSnap::installedSize ()
{
    return snapd_snap_get_installed_size (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::name ()
{
    return snapd_snap_get_name (SNAPD_SNAP (wrapped_object));
}

int QSnapdSnap::priceCount () const
{
    GPtrArray *prices;

    prices = snapd_snap_get_prices (SNAPD_SNAP (wrapped_object));
    return prices != NULL ? prices->len : 0;
}

QSnapdPrice *QSnapdSnap::price (int n) const
{
    GPtrArray *prices;

    prices = snapd_snap_get_prices (SNAPD_SNAP (wrapped_object));
    if (prices == NULL || n < 0 || (guint) n >= prices->len)
        return NULL;
    return new QSnapdPrice (g_object_ref (prices->pdata[n]));
}

bool QSnapdSnap::isPrivate ()
{
    return snapd_snap_get_private (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::revision ()
{
    return snapd_snap_get_revision (SNAPD_SNAP (wrapped_object));
}

/* FIXME
QSnapdSnap::QSnapdSnapType QSnapdSnap::snapType ()
{
}*/

/* FIXME
QSnapdSnap::QSnapdSnapStatus QSnapdSnap::status ()
{
}*/

QString QSnapdSnap::summary ()
{
    return snapd_snap_get_summary (SNAPD_SNAP (wrapped_object));
}

bool QSnapdSnap::trymode ()
{
    return snapd_snap_get_trymode (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::version ()
{
    return snapd_snap_get_version (SNAPD_SNAP (wrapped_object));
}
