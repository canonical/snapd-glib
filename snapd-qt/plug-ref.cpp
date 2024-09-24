/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/plug-ref.h"

QSnapdPlugRef::QSnapdPlugRef(void *snapd_object, QObject *parent)
    : QSnapdWrappedObject(g_object_ref(snapd_object), g_object_unref, parent) {}

QString QSnapdPlugRef::plug() const {
  return snapd_plug_ref_get_plug(SNAPD_PLUG_REF(wrapped_object));
}

QString QSnapdPlugRef::snap() const {
  return snapd_plug_ref_get_snap(SNAPD_PLUG_REF(wrapped_object));
}
