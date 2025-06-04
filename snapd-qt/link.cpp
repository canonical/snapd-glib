/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/link.h"

QSnapdLink::QSnapdLink(void *snapd_object, QObject *parent)
    : QSnapdWrappedObject(g_object_ref(snapd_object), g_object_unref, parent) {}

QString QSnapdLink::type() const {
  return snapd_link_get_url_type(SNAPD_LINK(wrapped_object));
}

QStringList QSnapdLink::urls() const {
  GStrv array = snapd_link_get_urls(SNAPD_LINK(wrapped_object));
  QStringList urls;
  for (; *array != NULL; array++)
    urls.append(*array);
  return urls;
}