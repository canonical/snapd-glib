/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/task-data.h"

QSnapdTaskData::QSnapdTaskData (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QStringList QSnapdTaskData::affectedSnaps () const
{
    QStringList retval;
    GPtrArray *data = snapd_task_data_get_affected_snaps (SNAPD_TASK_DATA (wrapped_object));

    if (data != NULL)
        for (gsize i=0; i < data->len; i++)
            retval.append((gchar *) data->pdata[i]);
    return retval;
}
