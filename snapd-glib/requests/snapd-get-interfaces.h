/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_INTERFACE_H__
#define __SNAPD_GET_INTERFACE_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetInterfaces, snapd_get_interfaces, SNAPD, GET_INTERFACES, SnapdRequest)

SnapdGetInterfaces *_snapd_get_interfaces_new                (gchar              **names,
                                                              GCancellable        *cancellable,
                                                              GAsyncReadyCallback  callback,
                                                              gpointer             user_data);

void                _snapd_get_interfaces_set_include_docs   (SnapdGetInterfaces  *request,
                                                              gboolean             include_docs);

void                _snapd_get_interfaces_set_include_plugs  (SnapdGetInterfaces  *request,
                                                              gboolean             include_plugs);

void                _snapd_get_interfaces_set_include_slots  (SnapdGetInterfaces  *request,
                                                              gboolean             include_slots);

void                _snapd_get_interfaces_set_only_connected (SnapdGetInterfaces  *request,
                                                              gboolean             only_connected);

GPtrArray          *_snapd_get_interfaces_get_interfaces     (SnapdGetInterfaces  *request);

G_END_DECLS

#endif /* __SNAPD_GET_INTERFACE_H__ */
