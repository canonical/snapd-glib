/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include <json-glib/json-glib.h>

#include "snapd-request.h"
#include "snapd-change.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostChange, snapd_post_change, SNAPD, POST_CHANGE, SnapdRequest)

SnapdPostChange *_snapd_post_change_new           (const gchar         *change_id,
                                                   const gchar         *action,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);

const gchar     *_snapd_post_change_get_change_id (SnapdPostChange *request);

SnapdChange     *_snapd_post_change_get_change    (SnapdPostChange *request);

JsonNode        *_snapd_post_change_get_data      (SnapdPostChange *request);

void             _snapd_post_change_set_api_path  (SnapdPostChange *request,
                                                   const gchar     *api_path);

G_END_DECLS
