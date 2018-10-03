/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_MEDIA_H__
#define __SNAPD_MEDIA_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_MEDIA  (snapd_media_get_type ())

G_DECLARE_FINAL_TYPE (SnapdMedia, snapd_media, SNAPD, MEDIA, GObject)

const gchar *snapd_media_get_media_type (SnapdMedia *media);

const gchar *snapd_media_get_url        (SnapdMedia *media);

guint        snapd_media_get_width      (SnapdMedia *media);

guint        snapd_media_get_height     (SnapdMedia *media);

G_END_DECLS

#endif /* __SNAPD_MEDIA_H__ */
