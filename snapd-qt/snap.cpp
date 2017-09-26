/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/snap.h"

QSnapdSnap::QSnapdSnap (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

int QSnapdSnap::appCount () const
{
    GPtrArray *apps;

    apps = snapd_snap_get_apps (SNAPD_SNAP (wrapped_object));
    return apps != NULL ? apps->len : 0;
}

QSnapdApp *QSnapdSnap::app (int n) const
{
    GPtrArray *apps;

    apps = snapd_snap_get_apps (SNAPD_SNAP (wrapped_object));
    if (apps == NULL || n < 0 || (guint) n >= apps->len)
        return NULL;
    return new QSnapdApp (apps->pdata[n]);
}

QString QSnapdSnap::channel () const
{
    return snapd_snap_get_channel (SNAPD_SNAP (wrapped_object));
}

QSnapdEnums::SnapConfinement QSnapdSnap::confinement () const
{
    switch (snapd_snap_get_confinement (SNAPD_SNAP (wrapped_object)))
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

QString QSnapdSnap::contact () const
{
    return snapd_snap_get_contact (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::description () const
{
    return snapd_snap_get_description (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::developer () const
{
    return snapd_snap_get_developer (SNAPD_SNAP (wrapped_object));
}

bool QSnapdSnap::devmode () const
{
    return snapd_snap_get_devmode (SNAPD_SNAP (wrapped_object));
}

qint64 QSnapdSnap::downloadSize () const
{
    return snapd_snap_get_download_size (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::icon () const
{
    return snapd_snap_get_icon (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::id () const
{
    return snapd_snap_get_id (SNAPD_SNAP (wrapped_object));
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

QDateTime QSnapdSnap::installDate () const
{
    return convertDateTime (snapd_snap_get_install_date (SNAPD_SNAP (wrapped_object)));
}

qint64 QSnapdSnap::installedSize () const
{
    return snapd_snap_get_installed_size (SNAPD_SNAP (wrapped_object));
}

bool QSnapdSnap::jailmode () const
{
    return snapd_snap_get_jailmode (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::license () const
{
    return snapd_snap_get_license (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::name () const
{
    return snapd_snap_get_name (SNAPD_SNAP (wrapped_object));
}

int QSnapdSnap::priceCount () const
{
    GPtrArray *prices;

    prices = snapd_snap_get_prices (SNAPD_SNAP (wrapped_object));
    return prices != NULL ? prices->len : 0;
}

QSnapdPrice *QSnapdSnap::price (int n) const
{
    GPtrArray *prices;

    prices = snapd_snap_get_prices (SNAPD_SNAP (wrapped_object));
    if (prices == NULL || n < 0 || (guint) n >= prices->len)
        return NULL;
    return new QSnapdPrice (prices->pdata[n]);
}

bool QSnapdSnap::isPrivate () const
{
    return snapd_snap_get_private (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::revision () const
{
    return snapd_snap_get_revision (SNAPD_SNAP (wrapped_object));
}

int QSnapdSnap::screenshotCount () const
{
    GPtrArray *screenshots;

    screenshots = snapd_snap_get_screenshots (SNAPD_SNAP (wrapped_object));
    return screenshots != NULL ? screenshots->len : 0;
}

QSnapdScreenshot *QSnapdSnap::screenshot (int n) const
{
    GPtrArray *screenshots;

    screenshots = snapd_snap_get_screenshots (SNAPD_SNAP (wrapped_object));
    if (screenshots == NULL || n < 0 || (guint) n >= screenshots->len)
        return NULL;
    return new QSnapdScreenshot (screenshots->pdata[n]);
}

QSnapdEnums::SnapType QSnapdSnap::snapType () const
{
    switch (snapd_snap_get_snap_type (SNAPD_SNAP (wrapped_object)))
    {
    case SNAPD_SNAP_TYPE_APP:
        return QSnapdEnums::SnapTypeApp;
    case SNAPD_SNAP_TYPE_KERNEL:
        return QSnapdEnums::SnapTypeKernel;
    case SNAPD_SNAP_TYPE_GADGET:
        return QSnapdEnums::SnapTypeGadget;
    case SNAPD_SNAP_TYPE_OS:
        return QSnapdEnums::SnapTypeOperatingSystem;
    case SNAPD_SNAP_TYPE_UNKNOWN:
    default:
        return QSnapdEnums::SnapTypeUnknown;
    }
}

QSnapdEnums::SnapStatus QSnapdSnap::status () const
{
    switch (snapd_snap_get_status (SNAPD_SNAP (wrapped_object)))
    {
    case SNAPD_SNAP_STATUS_AVAILABLE:
        return QSnapdEnums::SnapStatusAvailable;
    case SNAPD_SNAP_STATUS_PRICED:
        return QSnapdEnums::SnapStatusPriced;
    case SNAPD_SNAP_STATUS_INSTALLED:
        return QSnapdEnums::SnapStatusInstalled;
    case SNAPD_SNAP_STATUS_ACTIVE:
        return QSnapdEnums::SnapStatusActive;
    case SNAPD_SNAP_STATUS_UNKNOWN:
    default:
        return QSnapdEnums::SnapStatusUnknown;
    }
}

QString QSnapdSnap::summary () const
{
    return snapd_snap_get_summary (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::title () const
{
    return snapd_snap_get_title (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::trackingChannel () const
{
    return snapd_snap_get_tracking_channel (SNAPD_SNAP (wrapped_object));
}

bool QSnapdSnap::trymode () const
{
    return snapd_snap_get_trymode (SNAPD_SNAP (wrapped_object));
}

QString QSnapdSnap::version () const
{
    return snapd_snap_get_version (SNAPD_SNAP (wrapped_object));
}
