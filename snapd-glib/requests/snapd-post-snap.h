/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_SNAP_H__
#define __SNAPD_POST_SNAP_H__

#include "snapd-request-async.h"

#include "snapd-client.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostSnap, snapd_post_snap, SNAPD, POST_SNAP, SnapdRequestAsync)

SnapdPostSnap *_snapd_post_snap_new           (const gchar           *name,
                                               const gchar           *action,
                                               SnapdProgressCallback  progress_callback,
                                               gpointer               progress_callback_data,
                                               GCancellable          *cancellable,
                                               GAsyncReadyCallback    callback,
                                               gpointer               user_data);

void           _snapd_post_snap_set_channel   (SnapdPostSnap         *request,
                                               const gchar           *channel);

void           _snapd_post_snap_set_revision  (SnapdPostSnap         *request,
                                               const gchar           *revision);

void           _snapd_post_snap_set_classic   (SnapdPostSnap         *request,
                                               gboolean               classic);

void           _snapd_post_snap_set_dangerous (SnapdPostSnap         *request,
                                               gboolean               dangerous);

void           _snapd_post_snap_set_devmode   (SnapdPostSnap         *request,
                                              gboolean                devmode);

void           _snapd_post_snap_set_jailmode  (SnapdPostSnap         *request,
                                               gboolean               jailmode);


G_END_DECLS

#endif /* __SNAPD_POST_SNAP_H__ */
