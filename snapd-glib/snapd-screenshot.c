/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-screenshot.h"

/**
 * SECTION: snapd-screenshot
 * @short_description: Screenshot information
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdScreenshot represents a screenshot that shows a snap.
 * Screenshots can be queried using snapd_snap_get_screenshots().
 */

/**
 * SnapdScreenshot:
 *
 * #SnapdScreenshot contains screenshot information.
 *
 * Since: 1.0
 */

struct _SnapdScreenshot
{
    GObject parent_instance;

    gchar *url;
    guint width;
    guint height;
};

enum
{
    PROP_URL = 1,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdScreenshot, snapd_screenshot, G_TYPE_OBJECT)

SnapdScreenshot *
snapd_screenshot_new (void)
{
    return g_object_new (SNAPD_TYPE_SCREENSHOT, NULL);
}

/**
 * snapd_screenshot_get_url:
 * @screenshot: a #SnapdScreenshot.
 *
 * Get the URL for this screenshot, e.g. "http://example.com/screenshot.png"
 *
 * Returns: a URL
 *
 * Since: 1.0
 */
const gchar *
snapd_screenshot_get_url (SnapdScreenshot *screenshot)
{
    g_return_val_if_fail (SNAPD_IS_SCREENSHOT (screenshot), NULL);
    return screenshot->url;
}

/**
 * snapd_screenshot_get_width:
 * @screenshot: a #SnapdScreenshot.
 *
 * Get the width of the screenshot in pixels or 0 if unknown.
 *
 * Return: a width
 *
 * Since: 1.0
 */
guint
snapd_screenshot_get_width (SnapdScreenshot *screenshot)
{
    g_return_val_if_fail (SNAPD_IS_SCREENSHOT (screenshot), 0);
    return screenshot->width;
}

/**
 * snapd_screenshot_get_height:
 * @screenshot: a #SnapdScreenshot.
 *
 * Get the height of the screenshot in pixels or 0 if unknown.
 *
 * Return: a height
 *
 * Since: 1.0
 */
guint
snapd_screenshot_get_height (SnapdScreenshot *screenshot)
{
    g_return_val_if_fail (SNAPD_IS_SCREENSHOT (screenshot), 0);
    return screenshot->height;
}

static void
snapd_screenshot_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdScreenshot *screenshot = SNAPD_SCREENSHOT (object);

    switch (prop_id) {
    case PROP_URL:
        g_free (screenshot->url);
        screenshot->url = g_strdup (g_value_get_string (value));
        break;
    case PROP_WIDTH:
        screenshot->width = g_value_get_uint (value);
        break;
    case PROP_HEIGHT:
        screenshot->height = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_screenshot_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdScreenshot *screenshot = SNAPD_SCREENSHOT (object);

    switch (prop_id) {
    case PROP_URL:
        g_value_set_string (value, screenshot->url);
        break;
    case PROP_WIDTH:
        g_value_set_uint (value, screenshot->width);
        break;
    case PROP_HEIGHT:
        g_value_set_uint (value, screenshot->height);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_screenshot_finalize (GObject *object)
{
    SnapdScreenshot *screenshot = SNAPD_SCREENSHOT (object);

    g_clear_pointer (&screenshot->url, g_free);

    G_OBJECT_CLASS (snapd_screenshot_parent_class)->finalize (object);
}

static void
snapd_screenshot_class_init (SnapdScreenshotClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_screenshot_set_property;
    gobject_class->get_property = snapd_screenshot_get_property;
    gobject_class->finalize = snapd_screenshot_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_URL,
                                     g_param_spec_string ("url",
                                                          "url",
                                                          "URL for this screenshot",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_WIDTH,
                                     g_param_spec_uint ("width",
                                                        "width",
                                                        "Width of screenshot in pixels",
                                                        0, G_MAXUINT, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_HEIGHT,
                                     g_param_spec_uint ("height",
                                                        "height",
                                                        "Height of screenshot in pixels",
                                                        0, G_MAXUINT, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_screenshot_init (SnapdScreenshot *screenshot)
{
}
