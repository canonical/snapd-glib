/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_SCREENSHOT_H__
#define __SNAPD_SCREENSHOT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SCREENSHOT  (snapd_screenshot_get_type ())

G_DECLARE_FINAL_TYPE (SnapdScreenshot, snapd_screenshot, SNAPD, SCREENSHOT, GObject)

struct _SnapdScreenshotClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_screenshot_get_url   (SnapdScreenshot *screenshot);

guint        snapd_screenshot_get_width (SnapdScreenshot *screenshot);

guint        snapd_screenshot_get_height (SnapdScreenshot *screenshot);

G_END_DECLS

#endif /* __SNAPD_SCREENSHOT_H__ */
