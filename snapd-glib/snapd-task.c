/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-change.h"
#include "snapd-task.h"

/**
 * SECTION: snapd-task
 * @short_description: Task progress
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdTask contains information on a task in a #SnapdChange.
 */

/**
 * SnapdTask:
 *
 * #SnapdTask contains progress information for a task in a Snap transaction.
 *
 * Since: 1.0
 */

struct _SnapdTask {
  GObject parent_instance;

  gchar *id;
  gchar *kind;
  gchar *summary;
  gchar *status;
  gchar *progress_label;
  gint64 progress_done;
  gint64 progress_total;
  GDateTime *spawn_time;
  GDateTime *ready_time;
  SnapdTaskData *data;
};

enum {
  PROP_ID = 1,
  PROP_KIND,
  PROP_SUMMARY,
  PROP_STATUS,
  PROP_READY,
  PROP_PROGRESS_DONE,
  PROP_PROGRESS_TOTAL,
  PROP_SPAWN_TIME,
  PROP_READY_TIME,
  PROP_PROGRESS_LABEL,
  PROP_DATA,
  PROP_LAST
};

G_DEFINE_TYPE(SnapdTask, snapd_task, G_TYPE_OBJECT)

/**
 * snapd_task_get_id:
 * @task: a #SnapdTask.
 *
 * Get the unique ID for this task.
 *
 * Returns: an ID.
 *
 * Since: 1.0
 */
const gchar *snapd_task_get_id(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_id(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->id;
}

/**
 * snapd_task_get_kind:
 * @task: a #SnapdTask.
 *
 * Gets the kind of task this is.
 *
 * Returns: the kind of task.
 *
 * Since: 1.0
 */
const gchar *snapd_task_get_kind(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_kind(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->kind;
}

/**
 * snapd_task_get_summary:
 * @task: a #SnapdTask.
 *
 * Get a human readable description of the task.
 *
 * Returns: a string describing the task.
 *
 * Since: 1.0
 */
const gchar *snapd_task_get_summary(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_summary(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->summary;
}

/**
 * snapd_task_get_status:
 * @task: a #SnapdTask.
 *
 * Get the status of the task.
 *
 * Returns: a status string.
 *
 * Since: 1.0
 */
const gchar *snapd_task_get_status(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_status(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->status;
}

/**
 * snapd_task_get_ready:
 * @task: a #SnapdTask.
 *
 * Get if this task is completed.
 *
 * Deprecated: 1.5: Use snapd_change_get_ready() instead.
 *
 * Returns: %TRUE if this task is complete.
 *
 * Since: 1.0
 */
gboolean snapd_task_get_ready(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_ready(SNAPD_CHANGE(self));

  return FALSE;
}

/**
 * snapd_task_get_progress_label:
 * @task: a #SnapdTask.
 *
 * Get the the label associated with the progress.
 *
 * Returns: a label string.
 *
 * Since: 1.5
 */
const gchar *snapd_task_get_progress_label(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return NULL;

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->progress_label;
}

/**
 * snapd_task_get_progress_done:
 * @task: a #SnapdTask.
 *
 * Get the the number of items completed in this task.
 *
 * Returns: a count.
 *
 * Since: 1.0
 */
gint64 snapd_task_get_progress_done(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return 0;

  g_return_val_if_fail(SNAPD_IS_TASK(self), 0);
  return self->progress_done;
}

/**
 * snapd_task_get_progress_total:
 * @task: a #SnapdTask.
 *
 * Get the the total number of items to be completed in this task.
 *
 * Returns: a count.
 *
 * Since: 1.0
 */
gint64 snapd_task_get_progress_total(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return 0;

  g_return_val_if_fail(SNAPD_IS_TASK(self), 0);
  return self->progress_total;
}

/**
 * snapd_task_get_spawn_time:
 * @task: a #SnapdTask.
 *
 * Get the time this task started.
 *
 * Returns: (transfer none): a #GDateTime.
 *
 * Since: 1.0
 */
GDateTime *snapd_task_get_spawn_time(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_spawn_time(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->spawn_time;
}

/**
 * snapd_task_get_ready_time:
 * @task: a #SnapdTask.
 *
 * Get the time this task completed or %NULL if not yet completed.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime or %NULL.
 *
 * Since: 1.0
 */
GDateTime *snapd_task_get_ready_time(SnapdTask *self) {
  /* Workaround to handle API change in SnapdProgressCallback */
  if (SNAPD_IS_CHANGE(self))
    return snapd_change_get_ready_time(SNAPD_CHANGE(self));

  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->ready_time;
}

/**
 * snapd_task_get_data:
 * @task: a #SnapdTask.
 *
 * Get the extra data associated with the progress.
 *
 * Returns: (transfer none) (allow-none): a #SnapdTaskData or NULL.
 *
 * Since: 1.66
 */
SnapdTaskData *snapd_task_get_data(SnapdTask *self) {
  g_return_val_if_fail(SNAPD_IS_TASK(self), NULL);
  return self->data;
}

static void snapd_task_set_property(GObject *object, guint prop_id,
                                    const GValue *value, GParamSpec *pspec) {
  SnapdTask *self = SNAPD_TASK(object);

  switch (prop_id) {
  case PROP_ID:
    g_free(self->id);
    self->id = g_strdup(g_value_get_string(value));
    break;
  case PROP_KIND:
    g_free(self->kind);
    self->kind = g_strdup(g_value_get_string(value));
    break;
  case PROP_SUMMARY:
    g_free(self->summary);
    self->summary = g_strdup(g_value_get_string(value));
    break;
  case PROP_STATUS:
    g_free(self->status);
    self->status = g_strdup(g_value_get_string(value));
    break;
  case PROP_READY:
    // Deprecated
    break;
  case PROP_PROGRESS_LABEL:
    g_free(self->progress_label);
    self->progress_label = g_strdup(g_value_get_string(value));
    break;
  case PROP_PROGRESS_DONE:
    self->progress_done = g_value_get_int64(value);
    break;
  case PROP_PROGRESS_TOTAL:
    self->progress_total = g_value_get_int64(value);
    break;
  case PROP_SPAWN_TIME:
    g_clear_pointer(&self->spawn_time, g_date_time_unref);
    if (g_value_get_boxed(value) != NULL)
      self->spawn_time = g_date_time_ref(g_value_get_boxed(value));
    break;
  case PROP_READY_TIME:
    g_clear_pointer(&self->ready_time, g_date_time_unref);
    if (g_value_get_boxed(value) != NULL)
      self->ready_time = g_date_time_ref(g_value_get_boxed(value));
    break;
  case PROP_DATA:
    g_clear_object(&self->data);
    if (g_value_get_object(value) != NULL)
      self->data = g_object_ref(g_value_get_object(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_task_get_property(GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec) {
  SnapdTask *self = SNAPD_TASK(object);

  switch (prop_id) {
  case PROP_ID:
    g_value_set_string(value, self->id);
    break;
  case PROP_KIND:
    g_value_set_string(value, self->kind);
    break;
  case PROP_SUMMARY:
    g_value_set_string(value, self->summary);
    break;
  case PROP_STATUS:
    g_value_set_string(value, self->status);
    break;
  case PROP_PROGRESS_LABEL:
    g_value_set_string(value, self->progress_label);
    break;
  case PROP_PROGRESS_DONE:
    g_value_set_int64(value, self->progress_done);
    break;
  case PROP_PROGRESS_TOTAL:
    g_value_set_int64(value, self->progress_total);
    break;
  case PROP_READY:
    g_value_set_boolean(value, FALSE);
    break;
  case PROP_SPAWN_TIME:
    g_value_set_boxed(value, self->spawn_time);
    break;
  case PROP_READY_TIME:
    g_value_set_boxed(value, self->ready_time);
    break;
  case PROP_DATA:
    g_value_set_object(value, self->data);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_task_finalize(GObject *object) {
  SnapdTask *self = SNAPD_TASK(object);

  g_clear_pointer(&self->id, g_free);
  g_clear_pointer(&self->kind, g_free);
  g_clear_pointer(&self->summary, g_free);
  g_clear_pointer(&self->status, g_free);
  g_clear_pointer(&self->progress_label, g_free);
  g_clear_pointer(&self->spawn_time, g_date_time_unref);
  g_clear_pointer(&self->ready_time, g_date_time_unref);
  g_clear_object(&self->data);

  G_OBJECT_CLASS(snapd_task_parent_class)->finalize(object);
}

static void snapd_task_class_init(SnapdTaskClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_task_set_property;
  gobject_class->get_property = snapd_task_get_property;
  gobject_class->finalize = snapd_task_finalize;

  g_object_class_install_property(
      gobject_class, PROP_ID,
      g_param_spec_string("id", "id", "ID of task", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_KIND,
      g_param_spec_string("kind", "kind", "Kind of task", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SUMMARY,
      g_param_spec_string("summary", "summary", "Summary of task", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_STATUS,
      g_param_spec_string("status", "status", "Status of task", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_PROGRESS_LABEL,
      g_param_spec_string(
          "progress-label", "progress-label", "Label for progress", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_PROGRESS_DONE,
      g_param_spec_int64("progress-done", "progress-done",
                         "Number of items done in this task", 0, G_MAXINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                             G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_PROGRESS_TOTAL,
      g_param_spec_int64(
          "progress-total", "progress-total",
          "Total number of items to be done in this task", 0, G_MAXINT64, 0,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_READY,
      g_param_spec_boolean("ready", "ready", "TRUE when task complete", FALSE,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                               G_PARAM_DEPRECATED | G_PARAM_STATIC_NAME |
                               G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SPAWN_TIME,
      g_param_spec_boxed("spawn-time", "spawn-time", "Time this task started",
                         G_TYPE_DATE_TIME,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                             G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_READY_TIME,
      g_param_spec_boxed("ready-time", "ready-time", "Time this task completed",
                         G_TYPE_DATE_TIME,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                             G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_DATA,
      g_param_spec_object(
          "data", "data", "Extra data of task", SNAPD_TYPE_TASK_DATA,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void snapd_task_init(SnapdTask *self) {}
