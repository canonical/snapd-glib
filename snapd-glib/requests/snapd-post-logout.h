/*
 * Copyright (C) 2020 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostLogout, snapd_post_logout, SNAPD, POST_LOGOUT, SnapdRequest)

SnapdPostLogout *_snapd_post_logout_new                  (gint64               id,
                                                          GCancellable        *cancellable,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);

gboolean         _snapd_post_logout_get_user_information (SnapdPostLogout *request);

G_END_DECLS
