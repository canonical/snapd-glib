/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_INTERFACES_H__
#define __SNAPD_GET_INTERFACES_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetInterfaces, snapd_get_interfaces, SNAPD, GET_INTERFACES, SnapdRequest)

SnapdGetInterfaces *_snapd_get_interfaces_new       (GCancellable        *cancellable,
                                                     GAsyncReadyCallback  callback,
                                                     gpointer             user_data);

GPtrArray          *_snapd_get_interfaces_get_plugs (SnapdGetInterfaces *request);

GPtrArray          *_snapd_get_interfaces_get_slots (SnapdGetInterfaces *request);

G_END_DECLS

#endif /* __SNAPD_GET_INTERFACES_H__ */
