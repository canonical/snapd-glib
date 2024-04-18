/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>
#include "Snapd/notice.h"

QSnapdNotice::QSnapdNotice (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdNotice::id () const
{
    return snapd_notice_get_id (SNAPD_NOTICE (wrapped_object));
}

QString QSnapdNotice::userId () const
{
    return snapd_notice_get_user_id (SNAPD_NOTICE (wrapped_object));
}

QSnapdEnums::SnapNoticeType QSnapdNotice::noticeType () const
{
    switch (snapd_notice_get_notice_type (SNAPD_NOTICE (wrapped_object)))
    {
    case SNAPD_NOTICE_TYPE_CHANGE_UPDATE:
        return QSnapdEnums::SnapNoticeTypeChangeUpdate;
    case SNAPD_NOTICE_TYPE_REFRESH_INHIBIT:
        return QSnapdEnums::SnapNoticeTypeRefreshInhibit;
    case SNAPD_NOTICE_TYPE_SNAP_RUN_INHIBIT:
        return QSnapdEnums::SnapNoticeTypeSnapRunInhibit;
    case SNAPD_NOTICE_TYPE_UNKNOWN:
    default:
        return QSnapdEnums::SnapNoticeTypeUnknown;
    }
}

QString QSnapdNotice::key () const
{
    return snapd_notice_get_key (SNAPD_NOTICE (wrapped_object));
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

QDateTime QSnapdNotice::firstOccurred () const
{
    return convertDateTime ((GDateTime*) snapd_notice_get_first_occurred (SNAPD_NOTICE (wrapped_object)));
}

QDateTime QSnapdNotice::lastOccurred () const
{
    return convertDateTime ((GDateTime*) snapd_notice_get_last_occurred (SNAPD_NOTICE (wrapped_object)));
}

QDateTime QSnapdNotice::lastRepeated () const
{
    return convertDateTime ((GDateTime*) snapd_notice_get_last_repeated (SNAPD_NOTICE (wrapped_object)));
}

qint32 QSnapdNotice::occurrences () const
{
    return snapd_notice_get_occurrences (SNAPD_NOTICE (wrapped_object));
}

qint64 QSnapdNotice::repeatAfter () const
{
    return snapd_notice_get_repeat_after (SNAPD_NOTICE (wrapped_object));
}

qint64 QSnapdNotice::expireAfter () const
{
    return snapd_notice_get_expire_after (SNAPD_NOTICE (wrapped_object));
}

qint32 QSnapdNotice::lastOccurredNanoseconds () const
{
    return snapd_notice_get_last_occurred_nanoseconds (SNAPD_NOTICE (wrapped_object));
}

static void
addItemToQHash (gchar *key, gchar *value, QHash<QString, QString> *lastData)
{
    lastData->insert (key, value);
}

QHash<QString, QString> QSnapdNotice::lastData () const
{
    QHash<QString, QString> lastData;
    g_autoptr(GHashTable) last_data = snapd_notice_get_last_data (SNAPD_NOTICE (wrapped_object));
    g_hash_table_foreach (last_data, (GHFunc) addItemToQHash, &lastData);
    return lastData;
}

