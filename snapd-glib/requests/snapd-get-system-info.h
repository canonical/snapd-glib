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

#include "snapd-system-information.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdGetSystemInfo, snapd_get_system_info, SNAPD,
                     GET_SYSTEM_INFO, SnapdRequest)

SnapdGetSystemInfo *_snapd_get_system_info_new(GCancellable *cancellable,
                                               GAsyncReadyCallback callback,
                                               gpointer user_data);

SnapdSystemInformation *
_snapd_get_system_info_get_system_information(SnapdGetSystemInfo *request);

G_END_DECLS
