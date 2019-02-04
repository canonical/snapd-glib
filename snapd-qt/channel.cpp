/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/channel.h"

QSnapdChannel::QSnapdChannel (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdChannel::branch () const
{
    return snapd_channel_get_branch (SNAPD_CHANNEL (wrapped_object));
}

QSnapdEnums::SnapConfinement QSnapdChannel::confinement () const
{
    switch (snapd_channel_get_confinement (SNAPD_CHANNEL (wrapped_object)))
    {
    case SNAPD_CONFINEMENT_STRICT:
        return QSnapdEnums::SnapConfinementStrict;
    case SNAPD_CONFINEMENT_CLASSIC:
        return QSnapdEnums::SnapConfinementClassic;
    case SNAPD_CONFINEMENT_DEVMODE:
        return QSnapdEnums::SnapConfinementDevmode;
    case SNAPD_CONFINEMENT_UNKNOWN:
    default:
        return QSnapdEnums::SnapConfinementUnknown;
    }
}

QString QSnapdChannel::epoch () const
{
    return snapd_channel_get_epoch (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::name () const
{
    return snapd_channel_get_name (SNAPD_CHANNEL (wrapped_object));
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

QDateTime QSnapdChannel::releasedAt () const
{
    return convertDateTime (snapd_channel_get_released_at (SNAPD_CHANNEL (wrapped_object)));
}

QString QSnapdChannel::revision () const
{
    return snapd_channel_get_revision (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::risk () const
{
    return snapd_channel_get_risk (SNAPD_CHANNEL (wrapped_object));
}

qint64 QSnapdChannel::size () const
{
    return snapd_channel_get_size (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::track () const
{
    return snapd_channel_get_track (SNAPD_CHANNEL (wrapped_object));
}

QString QSnapdChannel::version () const
{
    return snapd_channel_get_version (SNAPD_CHANNEL (wrapped_object));
}
