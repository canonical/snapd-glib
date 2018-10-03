/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/media.h"

QSnapdMedia::QSnapdMedia (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdMedia::type () const
{
    return snapd_media_get_media_type (SNAPD_MEDIA (wrapped_object));
}

QString QSnapdMedia::url () const
{
    return snapd_media_get_url (SNAPD_MEDIA (wrapped_object));
}

quint64 QSnapdMedia::width () const
{
    return snapd_media_get_width (SNAPD_MEDIA (wrapped_object));
}

quint64 QSnapdMedia::height () const
{
    return snapd_media_get_height (SNAPD_MEDIA (wrapped_object));
}
