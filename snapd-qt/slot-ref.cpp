/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/slot-ref.h"

QSnapdSlotRef::QSnapdSlotRef (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdSlotRef::slot () const
{
    return snapd_slot_ref_get_slot (SNAPD_SLOT_REF (wrapped_object));
}

QString QSnapdSlotRef::snap () const
{
    return snapd_slot_ref_get_snap (SNAPD_SLOT_REF (wrapped_object));
}
