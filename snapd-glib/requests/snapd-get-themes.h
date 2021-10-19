/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_THEMES_H__
#define __SNAPD_GET_THEMES_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetThemes, snapd_get_themes, SNAPD, GET_THEMES, SnapdRequest)

SnapdGetThemes *_snapd_get_themes_new (GStrv                gtk_theme_names,
                                       GStrv                icon_theme_names,
                                       GStrv                sound_theme_names,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);

GHashTable *_snapd_get_themes_get_gtk_theme_status   (SnapdGetThemes *request);
GHashTable *_snapd_get_themes_get_icon_theme_status  (SnapdGetThemes *request);
GHashTable *_snapd_get_themes_get_sound_theme_status (SnapdGetThemes *request);

G_END_DECLS

#endif /* __SNAPD_GET_THEMES_H__ */
