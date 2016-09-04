/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <string.h>

#include "snapd-task.h"

/**
 * SECTION: snapd-task
 * @short_description: Task progress
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdTask contains information on how a request is progressing. Progress
 * information is returned in a #SnapdProgressCallback.
 */

/**
 * SnapdTask:
 *
 * #SnapdTask is an opaque data structure and can only be accessed
 * using the provided functions.
 */

struct _SnapdTask
{
    GObject parent_instance;

    gchar *id;
    gchar *kind;  
    gchar *summary;
    gchar *status;  
    gboolean ready;
    gint64 progress_done;
    gint64 progress_total;
    GDateTime *spawn_time;
    GDateTime *ready_time;
};

enum 
{
    PROP_ID = 1,
    PROP_KIND,  
    PROP_SUMMARY,
    PROP_STATUS,  
    PROP_READY,
    PROP_PROGRESS_DONE,
    PROP_PROGRESS_TOTAL,
    PROP_SPAWN_TIME,
    PROP_READY_TIME,  
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdTask, snapd_task, G_TYPE_OBJECT)

/**
 * snapd_task_get_id:
 * @task: a #SnapdTask.
 *
 * Get the unique ID for this task.
 *
 * Returns: an ID.
 */
const gchar *
snapd_task_get_id (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->id;
}

/**
 * snapd_task_get_kind:
 * @task: a #SnapdTask.
 *
 * Gets the kind of task this is.
 *
 * Returns: the kind of task.
 */
const gchar *
snapd_task_get_kind (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->kind;
}

/**
 * snapd_task_get_summary:
 * @task: a #SnapdTask.
 * 
 * Get a human readable description of the task.
 *
 * Returns: a string describing the task.
 */
const gchar *
snapd_task_get_summary (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->summary;
}

/**
 * snapd_task_get_status:
 * @task: a #SnapdTask.
 *
 * Get the status of the task.
 *
 * Returns: a status string.
 */
const gchar *
snapd_task_get_status (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->status;
}

/**
 * snapd_task_get_ready:
 * @task: a #SnapdTask.
 *
 * Get if this task is completed.
 *
 * Returns: %TRUE if this task is complete.
 */
gboolean
snapd_task_get_ready (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), FALSE);
    return task->ready;
}

/**
 * snapd_task_get_progress_done:
 * @task: a #SnapdTask.
 *
 * Get the the number of items completed in this task.
 *
 * Returns: a count.
 */
gint64
snapd_task_get_progress_done (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), 0);
    return task->progress_done;
}

/**
 * snapd_task_get_progress_total:
 * @task: a #SnapdTask.
 *
 * Get the the total number of items to be completed in this task.
 *
 * Returns: a count.
 */
gint64
snapd_task_get_progress_total (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), 0);
    return task->progress_total;
}

/**
 * snapd_task_get_spawn_time:
 * @task: a #SnapdTask.
 *
 * Get the time this task started.
 *
 * Returns: a #GDateTime.
 */
GDateTime *
snapd_task_get_spawn_time (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->spawn_time;
}

/**
 * snapd_task_get_ready_time:
 * @task: a #SnapdTask.
 *
 * Get the time this taksk completed or %NULL if not yet completed.
 *
 * Returns: (allow-none): a #GDateTime or %NULL.
 */
GDateTime *
snapd_task_get_ready_time (SnapdTask *task)
{
    g_return_val_if_fail (SNAPD_IS_TASK (task), NULL);
    return task->ready_time;
}

static void
snapd_task_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdTask *task = SNAPD_TASK (object);

    switch (prop_id) {
    case PROP_ID:
        g_free (task->id);
        task->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_KIND:
        g_free (task->kind);
        task->kind = g_strdup (g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        g_free (task->summary);
        task->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_STATUS:
        g_free (task->status);
        task->status = g_strdup (g_value_get_string (value));
        break;
    case PROP_READY:
        task->ready = g_value_get_boolean (value);
        break;
    case PROP_PROGRESS_DONE:
        task->progress_done = g_value_get_int64 (value);
        break;
    case PROP_PROGRESS_TOTAL:
        task->progress_total = g_value_get_int64 (value);
        break;
    case PROP_SPAWN_TIME:
        g_clear_pointer (&task->spawn_time, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            task->spawn_time = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_READY_TIME:
        g_clear_pointer (&task->ready_time, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            task->ready_time = g_date_time_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_task_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdTask *task = SNAPD_TASK (object);

    switch (prop_id) {
    case PROP_ID:
        g_value_set_string (value, task->id);
        break;
    case PROP_KIND:
        g_value_set_string (value, task->kind);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, task->summary);
        break;
    case PROP_STATUS:
        g_value_set_string (value, task->status);
        break;
    case PROP_PROGRESS_DONE:
        g_value_set_int64 (value, task->progress_done);
        break;
    case PROP_PROGRESS_TOTAL:
        g_value_set_int64 (value, task->progress_total);
        break;
    case PROP_SPAWN_TIME:
        g_value_set_boxed (value, task->spawn_time);
        break;
    case PROP_READY_TIME:
        g_value_set_boxed (value, task->ready_time);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_task_finalize (GObject *object)
{
    SnapdTask *task = SNAPD_TASK (object);

    g_clear_pointer (&task->id, g_free);
    g_clear_pointer (&task->kind, g_free);
    g_clear_pointer (&task->summary, g_free);
    g_clear_pointer (&task->status, g_free);  
    g_clear_pointer (&task->spawn_time, g_date_time_unref);
    g_clear_pointer (&task->ready_time, g_date_time_unref);  
}

static void
snapd_task_class_init (SnapdTaskClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_task_set_property;
    gobject_class->get_property = snapd_task_get_property; 
    gobject_class->finalize = snapd_task_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          "id",
                                                          "ID of task",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_KIND,
                                     g_param_spec_string ("kind",
                                                          "kind",
                                                          "Kind of task",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary",
                                                          "summary",
                                                          "Summary of task",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STATUS,
                                     g_param_spec_string ("status",
                                                          "status",
                                                          "Status of task",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PROGRESS_DONE,
                                     g_param_spec_int64 ("progress-done",
                                                         "progress-done",
                                                         "Number of items done in this task",
                                                         0, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PROGRESS_TOTAL,
                                     g_param_spec_int64 ("progress-total",
                                                         "progress-total",
                                                         "Total number of items to be done in this task",
                                                         0, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_READY,
                                     g_param_spec_boolean ("ready",
                                                           "ready",
                                                           "TRUE when task complete",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SPAWN_TIME,
                                     g_param_spec_boxed ("spawn-time",
                                                         "spawn-time",
                                                         "Time this task started",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_READY_TIME,
                                     g_param_spec_boxed ("ready-time",
                                                         "ready-time",
                                                         "Time this task completed",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_task_init (SnapdTask *task)
{
}
