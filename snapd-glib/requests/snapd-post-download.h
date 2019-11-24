/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_DOWNLOAD_H__
#define __SNAPD_POST_DOWNLOAD_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostDownload, snapd_post_download, SNAPD, POST_DOWNLOAD, SnapdRequest)

SnapdPostDownload *_snapd_post_download_new      (const gchar         *name,
                                                  const gchar         *channel,
                                                  const gchar         *revision,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data);

GBytes            *_snapd_post_download_get_data (SnapdPostDownload   *request);

G_END_DECLS

#endif /* __SNAPD_POST_DOWNLOAD_H__ */
