/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_INTERFACE_INFO_H__
#define __SNAPD_GET_INTERFACE_INFO_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetInterfaceInfo, snapd_get_interface_info, SNAPD, GET_INTERFACE_INFO, SnapdRequest)

SnapdGetInterfaceInfo *_snapd_get_interface_info_new           (gchar              **names,
                                                                gboolean             include_docs,
                                                                gboolean             include_plugs,
                                                                gboolean             include_slots,
                                                                gboolean             only_connected,
                                                                GCancellable        *cancellable,
                                                                GAsyncReadyCallback  callback,
                                                                gpointer             user_data);

GPtrArray            *_snapd_get_interface_info_get_interfaces (SnapdGetInterfaceInfo *request);

G_END_DECLS

#endif /* __SNAPD_GET_INTERFACE_INFO_H__ */
