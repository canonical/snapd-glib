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

#include "snapd-icon.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetIcon, snapd_get_icon, SNAPD, GET_ICON, SnapdRequest)

SnapdGetIcon *_snapd_get_icon_new      (const gchar         *name,
                                        GCancellable        *cancellable,
                                        GAsyncReadyCallback  callback,
                                        gpointer             user_data);

SnapdIcon    *_snapd_get_icon_get_icon (SnapdGetIcon *request);

G_END_DECLS

