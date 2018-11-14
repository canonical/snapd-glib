/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_SNAPS_H__
#define __SNAPD_POST_SNAPS_H__

#include "snapd-request-async.h"

#include "snapd-client.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostSnaps, snapd_post_snaps, SNAPD, POST_SNAPS, SnapdRequestAsync)

SnapdPostSnaps *_snapd_post_snaps_new            (const gchar           *action,
                                                  SnapdProgressCallback  progress_callback,
                                                  gpointer               progress_callback_data,
                                                  GCancellable          *cancellable,
                                                  GAsyncReadyCallback    callback,
                                                  gpointer               user_data);

GStrv           _snapd_post_snaps_get_snap_names (SnapdPostSnaps        *request);

G_END_DECLS

#endif /* __SNAPD_POST_SNAPS_H__ */
