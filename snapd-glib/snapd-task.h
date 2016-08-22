/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_TASK_H__
#define __SNAPD_TASK_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_TASK  (snapd_task_get_type ())

G_DECLARE_FINAL_TYPE (SnapdTask, snapd_task, SNAPD, TASK, GObject)

struct _SnapdTaskClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_task_get_id             (SnapdTask *task);

const gchar *snapd_task_get_kind           (SnapdTask *task);

const gchar *snapd_task_get_summary        (SnapdTask *task);

const gchar *snapd_task_get_status         (SnapdTask *task);

gboolean     snapd_task_get_ready          (SnapdTask *task);

gint64       snapd_task_get_progress_done  (SnapdTask *task);

gint64       snapd_task_get_progress_total (SnapdTask *task);

const gchar *snapd_task_get_spawn_time     (SnapdTask *task);

const gchar *snapd_task_get_ready_time     (SnapdTask *task);

G_END_DECLS

#endif /* __SNAPD_TASK_H__ */
