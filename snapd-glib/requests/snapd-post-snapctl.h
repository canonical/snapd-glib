/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostSnapctl, snapd_post_snapctl, SNAPD, POST_SNAPCTL, SnapdRequest)

SnapdPostSnapctl *_snapd_post_snapctl_new               (const gchar         *context_id,
                                                         GStrv                args,
                                                         GCancellable        *cancellable,
                                                         GAsyncReadyCallback  callback,
                                                         gpointer             user_data);

const gchar      *_snapd_post_snapctl_get_stdout_output (SnapdPostSnapctl *request);

const gchar      *_snapd_post_snapctl_get_stderr_output (SnapdPostSnapctl *request);

int               _snapd_post_snapctl_get_exit_code     (SnapdPostSnapctl *request);

G_END_DECLS
