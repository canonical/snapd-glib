/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_MAINTENANCE_H__
#define __SNAPD_MAINTENANCE_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_MAINTENANCE (snapd_maintenance_get_type())

G_DECLARE_FINAL_TYPE(SnapdMaintenance, snapd_maintenance, SNAPD, MAINTENANCE,
                     GObject)

/**
 * SnapdMaintenanceKind:
 * @SNAPD_MAINTENANCE_KIND_UNKNOWN: an unknown maintenance kind is occurring.
 * @SNAPD_MAINTENANCE_KIND_DAEMON_RESTART: the daemon is restarting.
 * @SNAPD_MAINTENANCE_KIND_SYSTEM_RESTART: the system is restarting.
 *
 * Type of snap.
 *
 * Since: 1.45
 */
typedef enum {
  SNAPD_MAINTENANCE_KIND_UNKNOWN,
  SNAPD_MAINTENANCE_KIND_DAEMON_RESTART,
  SNAPD_MAINTENANCE_KIND_SYSTEM_RESTART
} SnapdMaintenanceKind;

SnapdMaintenanceKind snapd_maintenance_get_kind(SnapdMaintenance *maintenance);

const gchar *snapd_maintenance_get_message(SnapdMaintenance *maintenance);

G_END_DECLS

#endif /* __SNAPD_MAINTENANCE_H__ */
