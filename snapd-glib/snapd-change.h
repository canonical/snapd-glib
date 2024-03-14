/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_CHANGE_H__
#define __SNAPD_CHANGE_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CHANGE  (snapd_change_get_type ())

G_DECLARE_FINAL_TYPE (SnapdChange, snapd_change, SNAPD, CHANGE, GObject)

const gchar *snapd_change_get_id         (SnapdChange *change);

const gchar *snapd_change_get_kind       (SnapdChange *change);

const gchar *snapd_change_get_summary    (SnapdChange *change);

const gchar *snapd_change_get_status     (SnapdChange *change);

gboolean     snapd_change_get_ready      (SnapdChange *change);

GPtrArray   *snapd_change_get_tasks      (SnapdChange *change);

GDateTime   *snapd_change_get_spawn_time (SnapdChange *change);

GDateTime   *snapd_change_get_ready_time (SnapdChange *change);

const gchar *snapd_change_get_error      (SnapdChange *change);

GHashTable  *snapd_change_get_data       (SnapdChange *self);

G_END_DECLS

#endif /* __SNAPD_CHANGE_H__ */
