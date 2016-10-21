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

Snap::Snap (void *snapd_object, QObject *parent) :
    WrappedObject (snapd_object, g_object_unref, parent) {}

/* FIXME
QList<Snapd::App> Snap::apps ()
{
}*/

QString Snap::channel ()
{
    return snapd_snap_get_channel (SNAPD_SNAP (wrapped_object));
}

/* FIXME
Snapd::Confinement Snap::confinement ()
{
}*/

QString Snap::description ()
{
    return snapd_snap_get_description (SNAPD_SNAP (wrapped_object));
}

QString Snap::developer ()
{
    return snapd_snap_get_developer (SNAPD_SNAP (wrapped_object));
}

bool Snap::devmode ()
{
    return snapd_snap_get_devmode (SNAPD_SNAP (wrapped_object));
}

qint64 Snap::downloadSize ()
{
    return snapd_snap_get_download_size (SNAPD_SNAP (wrapped_object));
}

QString Snap::icon ()
{
    return snapd_snap_get_icon (SNAPD_SNAP (wrapped_object));
}

QString Snap::id ()
{
    return snapd_snap_get_id (SNAPD_SNAP (wrapped_object));
}

/* FIXME
GDateTime Snap::installDate ()
{
}*/

qint64 Snap::installedSize ()
{
    return snapd_snap_get_installed_size (SNAPD_SNAP (wrapped_object));
}

QString Snap::name ()
{
    return snapd_snap_get_name (SNAPD_SNAP (wrapped_object));
}

bool Snap::isPrivate ()
{
    return snapd_snap_get_private (SNAPD_SNAP (wrapped_object));
}

QString Snap::revision ()
{
    return snapd_snap_get_revision (SNAPD_SNAP (wrapped_object));
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
    return snapd_snap_get_summary (SNAPD_SNAP (wrapped_object));
}

bool Snap::trymode ()
{
    return snapd_snap_get_trymode (SNAPD_SNAP (wrapped_object));
}

QString Snap::version ()
{
    return snapd_snap_get_version (SNAPD_SNAP (wrapped_object));
}
