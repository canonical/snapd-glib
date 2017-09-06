/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/channel.h"

QSnapdChannel::QSnapdChannel (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdChannel::epoch () const
{
    return snapd_channel_get_epoch (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::name () const
{
    return snapd_channel_get_name (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::revision () const
{
    return snapd_channel_get_revision (SNAPD_CHANNEL (wrapped_object));
}

qint64 QSnapdChannel::size () const
{
    return snapd_channel_get_size (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::version () const
{
    return snapd_channel_get_version (SNAPD_CHANNEL (wrapped_object));
}
