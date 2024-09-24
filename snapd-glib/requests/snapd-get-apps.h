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

G_DECLARE_FINAL_TYPE(SnapdGetApps, snapd_get_apps, SNAPD, GET_APPS,
                     SnapdRequest)

SnapdGetApps *_snapd_get_apps_new(GStrv snaps, GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data);

void _snapd_get_apps_set_select(SnapdGetApps *request, const gchar *select);

GPtrArray *_snapd_get_apps_get_apps(SnapdGetApps *request);

G_END_DECLS
