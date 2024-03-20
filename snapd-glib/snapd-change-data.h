/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CHANGE_DATA  (snapd_change_data_get_type ())
G_DECLARE_FINAL_TYPE (SnapdChangeData, snapd_change_data, SNAPD, CHANGE_DATA, GObject)

GStrv     snapd_change_data_get_snap_names         (SnapdChangeData *change_data);

GStrv     snapd_change_data_get_refresh_forced     (SnapdChangeData *change_data);

G_END_DECLS