/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/icon.h"

using namespace Snapd;

struct Snapd::IconPrivate
{
    IconPrivate (void *snapd_object)
    {
        icon = SNAPD_ICON (g_object_ref (snapd_object));
    }
  
    ~IconPrivate ()
    {
        g_object_unref (icon);
    }

    SnapdIcon *icon;
};

Icon::Icon (void *snapd_object, QObject *parent) :
    QObject (parent),
    d_ptr (new IconPrivate (snapd_object))
{
}

QString Icon::mime_type ()
{
    Q_D(Icon);
    return snapd_icon_get_mime_type (d->icon);
}

QByteArray Icon::data ()
{
    Q_D(Icon);
    GBytes *data = snapd_icon_get_data (d->icon);
    gsize length;
    gchar *raw_data = (gchar *) g_bytes_get_data (data, &length);
    return QByteArray::fromRawData (raw_data, length);
}
