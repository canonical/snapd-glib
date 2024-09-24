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

QSnapdIcon::QSnapdIcon(void *snapd_object, QObject *parent)
    : QSnapdWrappedObject(g_object_ref(snapd_object), g_object_unref, parent) {}

QString QSnapdIcon::mimeType() const {
  return snapd_icon_get_mime_type(SNAPD_ICON(wrapped_object));
}

QByteArray QSnapdIcon::data() const {
  GBytes *data = snapd_icon_get_data(SNAPD_ICON(wrapped_object));
  gsize length;
  gchar *raw_data = (gchar *)g_bytes_get_data(data, &length);
  return QByteArray::fromRawData(raw_data, length);
}
