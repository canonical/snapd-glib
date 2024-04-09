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

G_DECLARE_FINAL_TYPE (SnapdGetInterfacesLegacy, snapd_get_interfaces_legacy, SNAPD, GET_INTERFACES_LEGACY, SnapdRequest)

SnapdGetInterfacesLegacy *_snapd_get_interfaces_legacy_new       (GCancellable             *cancellable,
                                                                  GAsyncReadyCallback       callback,
                                                                  gpointer                  user_data);

GPtrArray                *_snapd_get_interfaces_legacy_get_plugs (SnapdGetInterfacesLegacy *request);

GPtrArray                *_snapd_get_interfaces_legacy_get_slots (SnapdGetInterfacesLegacy *request);

G_END_DECLS
