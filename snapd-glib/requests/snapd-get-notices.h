/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

#include "snapd-notice2.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetNotices, snapd_get_notices, SNAPD, GET_NOTICES, SnapdRequest)

SnapdGetNotices *_snapd_get_notices_new      (gchar               *user_id,
                                              gchar               *users,
                                              gchar               *types,
                                              gchar               *keys,
                                              GDateTime           *from_date_time,
                                              int                  from_date_time_nanoseconds,
                                              GTimeSpan            timeout,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);

GPtrArray       *_snapd_get_notices_get_notices (SnapdGetNotices *request);

G_END_DECLS
