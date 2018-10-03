/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-media.h"

/**
 * SECTION: snapd-media
 * @short_description: Media information
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdMedia represents media (icons, screenshots etc) that are associated with a snap.
 * Snap media can be queried using snapd_snap_get_media().
 */

/**
 * SnapdMedia:
 *
 * #SnapdMedia contains media information.
 *
 * Since: 1.45
 */

struct _SnapdMedia
{
    GObject parent_instance;

    gchar *type;
    gchar *url;
    guint width;
    guint height;
};

enum
{
    PROP_TYPE = 1,
    PROP_URL,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdMedia, snapd_media, G_TYPE_OBJECT)

SnapdMedia *
snapd_media_new (void)
{
    return g_object_new (SNAPD_TYPE_MEDIA, NULL);
}

/**
 * snapd_media_get_media_type:
 * @media: a #SnapdMedia.
 *
 * Get the type for this media, e.g. "icon" or "screenshot".
 *
 * Returns: a type name
 *
 * Since: 1.45
 */
const gchar *
snapd_media_get_media_type (SnapdMedia *media)
{
    g_return_val_if_fail (SNAPD_IS_MEDIA (media), NULL);
    return media->type;
}

/**
 * snapd_media_get_url:
 * @media: a #SnapdMedia.
 *
 * Get the URL for this media, e.g. "http://example.com/media.png"
 *
 * Returns: a URL
 *
 * Since: 1.45
 */
const gchar *
snapd_media_get_url (SnapdMedia *media)
{
    g_return_val_if_fail (SNAPD_IS_MEDIA (media), NULL);
    return media->url;
}

/**
 * snapd_media_get_width:
 * @media: a #SnapdMedia.
 *
 * Get the width of the media in pixels or 0 if unknown.
 *
 * Return: a width
 *
 * Since: 1.45
 */
guint
snapd_media_get_width (SnapdMedia *media)
{
    g_return_val_if_fail (SNAPD_IS_MEDIA (media), 0);
    return media->width;
}

/**
 * snapd_media_get_height:
 * @media: a #SnapdMedia.
 *
 * Get the height of the media in pixels or 0 if unknown.
 *
 * Return: a height
 *
 * Since: 1.45
 */
guint
snapd_media_get_height (SnapdMedia *media)
{
    g_return_val_if_fail (SNAPD_IS_MEDIA (media), 0);
    return media->height;
}

static void
snapd_media_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdMedia *media = SNAPD_MEDIA (object);

    switch (prop_id) {
    case PROP_TYPE:
        g_free (media->type);
        media->type = g_strdup (g_value_get_string (value));
        break;
    case PROP_URL:
        g_free (media->url);
        media->url = g_strdup (g_value_get_string (value));
        break;
    case PROP_WIDTH:
        media->width = g_value_get_uint (value);
        break;
    case PROP_HEIGHT:
        media->height = g_value_get_uint (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_media_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdMedia *media = SNAPD_MEDIA (object);

    switch (prop_id) {
    case PROP_TYPE:
        g_value_set_string (value, media->type);
        break;
    case PROP_URL:
        g_value_set_string (value, media->url);
        break;
    case PROP_WIDTH:
        g_value_set_uint (value, media->width);
        break;
    case PROP_HEIGHT:
        g_value_set_uint (value, media->height);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_media_finalize (GObject *object)
{
    SnapdMedia *media = SNAPD_MEDIA (object);

    g_clear_pointer (&media->type, g_free);
    g_clear_pointer (&media->url, g_free);

    G_OBJECT_CLASS (snapd_media_parent_class)->finalize (object);
}

static void
snapd_media_class_init (SnapdMediaClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_media_set_property;
    gobject_class->get_property = snapd_media_get_property;
    gobject_class->finalize = snapd_media_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_TYPE,
                                     g_param_spec_string ("type",
                                                          "type",
                                                          "Type for this media",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_URL,
                                     g_param_spec_string ("url",
                                                          "url",
                                                          "URL for this media",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_WIDTH,
                                     g_param_spec_uint ("width",
                                                        "width",
                                                        "Width of media in pixels",
                                                        0, G_MAXUINT, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_HEIGHT,
                                     g_param_spec_uint ("height",
                                                        "height",
                                                        "Height of media in pixels",
                                                        0, G_MAXUINT, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_media_init (SnapdMedia *media)
{
}
