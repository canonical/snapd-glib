/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_CHANGES_H__
#define __SNAPD_GET_CHANGES_H__

#include <json-glib/json-glib.h>

#include "snapd-request.h"
#include "snapd-change.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetChanges, snapd_get_changes, SNAPD, GET_CHANGES, SnapdRequest)

typedef void (*SnapdGetChangesChangeCallback) (SnapdGetChanges *request, SnapdChange *change, gpointer user_data);

SnapdGetChanges *_snapd_get_changes_new           (const gchar         *select,
                                                   const gchar         *snap_name,
                                                   gboolean             follow,
                                                   SnapdGetChangesChangeCallback change_callback,
                                                   gpointer             change_callback_data,
                                                   GDestroyNotify       change_callback_destroy_notify,
                                                   GCancellable        *cancellable,
                                                   GAsyncReadyCallback  callback,
                                                   gpointer             user_data);

GPtrArray       *_snapd_get_changes_get_changes   (SnapdGetChanges *request);

G_END_DECLS

#endif /* __SNAPD_GET_CHANGES_H__ */
