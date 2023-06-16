/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_LOGS_H__
#define __SNAPD_GET_LOGS_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetLogs, snapd_get_logs, SNAPD, GET_LOGS, SnapdRequest)

SnapdGetLogs *_snapd_get_logs_new      (gchar              **names,
                                        size_t               n,
                                        gboolean             follow,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data);

GPtrArray    *_snapd_get_logs_get_logs (SnapdGetLogs *request);

G_END_DECLS

#endif /* __SNAPD_GET_LOGS_H__ */
