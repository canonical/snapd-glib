/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/screenshot.h"

QSnapdScreenshot::QSnapdScreenshot (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdScreenshot::url () const
{
    return snapd_screenshot_get_url (SNAPD_SCREENSHOT (wrapped_object));  
}

quint64 QSnapdScreenshot::width () const
{
    return snapd_screenshot_get_width (SNAPD_SCREENSHOT (wrapped_object));
}

quint64 QSnapdScreenshot::height () const
{
    return snapd_screenshot_get_height (SNAPD_SCREENSHOT (wrapped_object));
}
