/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_THEMES_H__
#define __SNAPD_POST_THEMES_H__

#include "snapd-request-async.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostThemes, snapd_post_themes, SNAPD, POST_THEMES, SnapdRequestAsync)

SnapdPostThemes *_snapd_post_themes_new (GStrv                 gtk_theme_names,
                                         GStrv                 icon_theme_names,
                                         GStrv                 sound_theme_names,
                                         SnapdProgressCallback progress_callback,
                                         gpointer              progress_callback_data,
                                         GCancellable         *cancellable,
                                         GAsyncReadyCallback   callback,
                                         gpointer              user_data);

G_END_DECLS

#endif /* __SNAPD_POST_THEMES_H__ */
