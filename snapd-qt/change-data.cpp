/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/change-data.h"

QSnapdChangeData::QSnapdChangeData (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

static QStringList
gstrv_to_qstringlist (GStrv data)
{
    QStringList retval;

    if (data != NULL)
        for (gchar **element = data; *element != NULL; element++)
            retval << *element;
    return retval;
}

QStringList QSnapdChangeData::snap_names () const
{
    QStringList list;

    GStrv data = snapd_change_data_get_snap_names (SNAPD_CHANGE_DATA (wrapped_object));
    return gstrv_to_qstringlist (data);
}

QStringList QSnapdChangeData::refresh_forced () const
{
    QStringList list;

    GStrv data = snapd_change_data_get_refresh_forced (SNAPD_CHANGE_DATA (wrapped_object));
    return gstrv_to_qstringlist (data);
}
