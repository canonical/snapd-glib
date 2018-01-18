/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/system-information.h"

QSnapdSystemInformation::QSnapdSystemInformation (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdSystemInformation::binariesDirectory () const
{
    return snapd_system_information_get_binaries_directory (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::buildId () const
{
    return snapd_system_information_get_build_id (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QSnapdEnums::SystemConfinement QSnapdSystemInformation::confinement () const
{
    switch (snapd_system_information_get_confinement (SNAPD_SYSTEM_INFORMATION (wrapped_object)))
    {
    case SNAPD_SYSTEM_CONFINEMENT_STRICT:
        return QSnapdEnums::SystemConfinement::SystemConfinementStrict;
    case SNAPD_SYSTEM_CONFINEMENT_PARTIAL:
        return QSnapdEnums::SystemConfinement::SystemConfinementPartial;
    case SNAPD_SYSTEM_CONFINEMENT_UNKNOWN:
    default:
        return QSnapdEnums::SystemConfinement::SystemConfinementUnknown;
    }
}

QString QSnapdSystemInformation::kernelVersion () const
{
    return snapd_system_information_get_kernel_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::mountDirectory () const
{
    return snapd_system_information_get_mount_directory (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::osId () const
{
    return snapd_system_information_get_os_id (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::osVersion () const
{
    return snapd_system_information_get_os_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::refreshSchedule () const
{
    return snapd_system_information_get_refresh_schedule (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

static QDateTime convertDateTime (GDateTime *datetime)
{
    if (datetime == NULL)
        return QDateTime ();

    QDate date (g_date_time_get_year (datetime),
                g_date_time_get_month (datetime),
                g_date_time_get_day_of_month (datetime));
    QTime time (g_date_time_get_hour (datetime),
                g_date_time_get_minute (datetime),
                g_date_time_get_second (datetime),
                g_date_time_get_microsecond (datetime) / 1000);
    return QDateTime (date, time, Qt::OffsetFromUTC, g_date_time_get_utc_offset (datetime) / 1000000);
}

QDateTime QSnapdSystemInformation::refreshHold () const
{
    return convertDateTime (snapd_system_information_get_refresh_hold (SNAPD_SYSTEM_INFORMATION (wrapped_object)));
}

QDateTime QSnapdSystemInformation::refreshLast () const
{
    return convertDateTime (snapd_system_information_get_refresh_last (SNAPD_SYSTEM_INFORMATION (wrapped_object)));
}

QDateTime QSnapdSystemInformation::refreshNext () const
{
    return convertDateTime (snapd_system_information_get_refresh_next (SNAPD_SYSTEM_INFORMATION (wrapped_object)));
}

QString QSnapdSystemInformation::refreshTimer () const
{
    return snapd_system_information_get_refresh_timer (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QHash<QString, QStringList> QSnapdSystemInformation::sandboxFeatures () const
{
    QHash<QString, QStringList> sandboxFeatures;

    GHashTable *features = snapd_system_information_get_sandbox_features (SNAPD_SYSTEM_INFORMATION (wrapped_object));
    GHashTableIter iter;
    g_hash_table_iter_init (&iter, features);
    gpointer key, value;
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        const gchar *backend = (const gchar *) key;
        GStrv features = (GStrv) value;
        int i;

        for (i = 0; features[i] != NULL; i++)
            sandboxFeatures[backend] << features[i];
    }

    return sandboxFeatures;
}

QString QSnapdSystemInformation::series () const
{
    return snapd_system_information_get_series (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::store () const
{
    return snapd_system_information_get_store (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

QString QSnapdSystemInformation::version () const
{
    return snapd_system_information_get_version (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

bool QSnapdSystemInformation::onClassic () const
{
    return snapd_system_information_get_on_classic (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}

bool QSnapdSystemInformation::managed () const
{
    return snapd_system_information_get_managed (SNAPD_SYSTEM_INFORMATION (wrapped_object));
}
