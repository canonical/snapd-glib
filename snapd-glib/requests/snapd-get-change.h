/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_CHANGE_H__
#define __SNAPD_GET_CHANGE_H__

#include <json-glib/json-glib.h>

#include "snapd-request.h"
#include "snapd-change.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetChange, snapd_get_change, SNAPD, GET_CHANGE, SnapdRequest)

SnapdGetChange *_snapd_get_change_new           (const gchar         *change_id,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);

const gchar    *_snapd_get_change_get_change_id (SnapdGetChange *request);

SnapdChange    *_snapd_get_change_get_change    (SnapdGetChange *request);

JsonNode       *_snapd_get_change_get_data      (SnapdGetChange *request);

G_END_DECLS

#endif /* __SNAPD_GET_CHANGE_H__ */
