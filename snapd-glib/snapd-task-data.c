/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-task-data.h"

/**
 * SECTION: snapd-task-data
 * @short_description: Task extra data
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdTaskData contains extra information on a task in a #SnapdChange.
 */

/**
 * SnapdTaskData:
 *
 * #SnapdTaskData contains extra information for a task in a Snap transaction.
 *
 * Since: 1.66
 */

struct _SnapdTaskData {
  GObject parent_instance;
  GStrv affected_snaps;
};

enum { PROP_ID = 1, PROP_AFFECTED_SNAPS, PROP_LAST };

G_DEFINE_TYPE(SnapdTaskData, snapd_task_data, G_TYPE_OBJECT)

/**
 * snapd_task_data_get_affected_snaps:
 * @task_data: a #SnapdTaskData.
 *
 * Get the list of snaps that are affected by this task, or %NULL if snapd
 * doesn't send this data.
 *
 * Returns: (transfer none) (allow-none): a #GStrv or %NULL.
 *
 * Since: 1.66
 */
GStrv snapd_task_data_get_affected_snaps(SnapdTaskData *self) {
  g_return_val_if_fail(SNAPD_IS_TASK_DATA(self), NULL);
  return self->affected_snaps;
}

static void snapd_task_data_set_property(GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec) {
  SnapdTaskData *self = SNAPD_TASK_DATA(object);

  switch (prop_id) {
  case PROP_AFFECTED_SNAPS:
    g_clear_pointer(&self->affected_snaps, g_strfreev);
    self->affected_snaps = g_strdupv(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_task_data_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec) {
  SnapdTaskData *self = SNAPD_TASK_DATA(object);

  switch (prop_id) {
  case PROP_AFFECTED_SNAPS:
    g_value_set_boxed(value, self->affected_snaps);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_task_data_finalize(GObject *object) {
  SnapdTaskData *self = SNAPD_TASK_DATA(object);

  g_clear_pointer(&self->affected_snaps, g_strfreev);
  G_OBJECT_CLASS(snapd_task_data_parent_class)->finalize(object);
}

static void snapd_task_data_class_init(SnapdTaskDataClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_task_data_set_property;
  gobject_class->get_property = snapd_task_data_get_property;
  gobject_class->finalize = snapd_task_data_finalize;

  g_object_class_install_property(
      gobject_class, PROP_AFFECTED_SNAPS,
      g_param_spec_boxed("affected-snaps", "affected-snaps",
                         "Snaps affected by this task", G_TYPE_STRV,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void snapd_task_data_init(SnapdTaskData *self) {}
