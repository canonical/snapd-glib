/*
 * Copyright (C) 2024 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <snapd-glib/snapd-glib.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdNoticesMonitor, snapd_notices_monitor, SNAPD,
                     NOTICES_MONITOR, GObject)

SnapdNoticesMonitor  *snapd_notices_monitor_new               ();

SnapdNoticesMonitor  *snapd_notices_monitor_new_with_client   (SnapdClient             *client);

gboolean              snapd_notices_monitor_start             (SnapdNoticesMonitor     *monitor,
                                                               GError                 **error);

gboolean              snapd_notices_monitor_stop              (SnapdNoticesMonitor     *monitor,
                                                               GError                 **error);

G_END_DECLS
