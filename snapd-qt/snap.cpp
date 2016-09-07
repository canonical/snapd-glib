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

using namespace Snapd;

struct Snapd::SnapPrivate
{
    SnapPrivate (void *snapd_object)
    {
        snap = SNAPD_SNAP (g_object_ref (snapd_object));
    }

    ~SnapPrivate ()
    {
        g_object_unref (snap);
    }

    SnapdSnap *snap;
};

Snap::Snap (QObject *parent, void *snapd_object) :
    QObject (parent),
    d_ptr (new SnapPrivate (snapd_object)) {}

Snap::Snap (const Snap& copy) :
    QObject (copy.parent ()),
    d_ptr (new SnapPrivate (copy.d_ptr->snap)) {}

/* FIXME
QList<Snapd::App> Snap::apps ()
{
}*/

QString Snap::channel ()
{
    Q_D(Snap);
    return snapd_snap_get_channel (d->snap);
}

/* FIXME
Snapd::Confinement Snap::confinement ()
{
}*/

QString Snap::description ()
{
    Q_D(Snap);
    return snapd_snap_get_description (d->snap);
}

QString Snap::developer ()
{
    Q_D(Snap);
    return snapd_snap_get_developer (d->snap);
}

bool Snap::devmode ()
{
    Q_D(Snap);
    return snapd_snap_get_devmode (d->snap);
}

qint64 Snap::downloadSize ()
{
    Q_D(Snap);
    return snapd_snap_get_download_size (d->snap);
}

QString Snap::icon ()
{
    Q_D(Snap);
    return snapd_snap_get_icon (d->snap);
}

QString Snap::id ()
{
    Q_D(Snap);
    return snapd_snap_get_id (d->snap);
}

/* FIXME
GDateTime Snap::installDate ()
{
}*/

qint64 Snap::installedSize ()
{
    Q_D(Snap);
    return snapd_snap_get_installed_size (d->snap);
}

QString Snap::name ()
{
    Q_D(Snap);
    return snapd_snap_get_name (d->snap);
}

bool Snap::isPrivate ()
{
    Q_D(Snap);
    return snapd_snap_get_private (d->snap);
}

QString Snap::revision ()
{
    Q_D(Snap);
    return snapd_snap_get_revision (d->snap);
}

/* FIXME
Snapd::SnapType Snap::snapType ()
{
}*/

/* FIXME
Snapd::SnapStatus Snap::status ()
{
}*/

QString Snap::summary ()
{
    Q_D(Snap);
    return snapd_snap_get_summary (d->snap);
}

bool Snap::trymode ()
{
    Q_D(Snap);
    return snapd_snap_get_trymode (d->snap);
}

QString Snap::version ()
{
    Q_D(Snap);
    return snapd_snap_get_version (d->snap);
}
