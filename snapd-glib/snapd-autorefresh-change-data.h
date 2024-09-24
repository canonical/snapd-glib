/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_AUTOREFRESH_CHANGE_DATA_H__
#define __SNAPD_AUTOREFRESH_CHANGE_DATA_H__

#include "snapd-change-data.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_AUTOREFRESH_CHANGE_DATA                                     \
  (snapd_autorefresh_change_data_get_type())
G_DECLARE_FINAL_TYPE(SnapdAutorefreshChangeData, snapd_autorefresh_change_data,
                     SNAPD, AUTOREFRESH_CHANGE_DATA, SnapdChangeData)

GStrv snapd_autorefresh_change_data_get_snap_names(
    SnapdAutorefreshChangeData *change_data);

GStrv snapd_autorefresh_change_data_get_refresh_forced(
    SnapdAutorefreshChangeData *change_data);

G_END_DECLS

#endif /* __SNAPD_AUTOREFRESH_CHANGE_DATA_H__ */
