/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_SNAP_STREAM_H__
#define __SNAPD_POST_SNAP_STREAM_H__

#include "snapd-request-async.h"

#include "snapd-client.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostSnapStream, snapd_post_snap_stream, SNAPD, POST_SNAP_STREAM, SnapdRequestAsync)

SnapdPostSnapStream *_snapd_post_snap_stream_new           (SnapdProgressCallback  progress_callback,
                                                            gpointer               progress_callback_data,
                                                            GCancellable          *cancellable,
                                                            GAsyncReadyCallback    callback,
                                                            gpointer               user_data);

void                 _snapd_post_snap_stream_set_classic   (SnapdPostSnapStream   *request,
                                                            gboolean               classic);

void                 _snapd_post_snap_stream_set_dangerous (SnapdPostSnapStream   *request,
                                                            gboolean               dangerous);

void                 _snapd_post_snap_stream_set_devmode   (SnapdPostSnapStream   *request,
                                                            gboolean                devmode);

void                 _snapd_post_snap_stream_set_jailmode  (SnapdPostSnapStream   *request,
                                                            gboolean               jailmode);

void                 _snapd_post_snap_stream_append_data   (SnapdPostSnapStream   *request,
                                                            const guint8          *data,
                                                            guint                  len);

G_END_DECLS

#endif /* __SNAPD_POST_SNAP_STREAM_H__ */
