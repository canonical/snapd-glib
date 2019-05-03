/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/interface.h"

QSnapdInterface::QSnapdInterface (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdInterface::name () const
{
    return snapd_interface_get_name (SNAPD_INTERFACE (wrapped_object));
}

QString QSnapdInterface::summary () const
{
    return snapd_interface_get_summary (SNAPD_INTERFACE (wrapped_object));
}

QString QSnapdInterface::docUrl () const
{
    return snapd_interface_get_doc_url (SNAPD_INTERFACE (wrapped_object));
}

int QSnapdInterface::slotCount () const
{
    GPtrArray *slots;

    slots = snapd_interface_get_slots (SNAPD_INTERFACE (wrapped_object));
    return slots != NULL ? slots->len : 0;
}

QSnapdSlot *QSnapdInterface::slot (int n) const
{
    GPtrArray *slots;

    slots = snapd_interface_get_slots (SNAPD_INTERFACE (wrapped_object));
    if (slots == NULL || n < 0 || (guint) n >= slots->len)
        return NULL;
    return new QSnapdSlot (slots->pdata[n]);
}

int QSnapdInterface::plugCount () const
{
    GPtrArray *plugs;

    plugs = snapd_interface_get_plugs (SNAPD_INTERFACE (wrapped_object));
    return plugs != NULL ? plugs->len : 0;
}

QSnapdPlug *QSnapdInterface::plug (int n) const
{
    GPtrArray *plugs;

    plugs = snapd_interface_get_plugs (SNAPD_INTERFACE (wrapped_object));
    if (plugs == NULL || n < 0 || (guint) n >= plugs->len)
        return NULL;
    return new QSnapdPlug (plugs->pdata[n]);
}
