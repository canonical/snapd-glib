/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <string.h>

#include "snapd-change.h"

/**
 * SECTION: snapd-change
 * @short_description: Change progress
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdChange contains information on how a request is progressing. Progress
 * information is returned in a #SnapdProgressCallback.
 */

/**
 * SnapdChange:
 *
 * #SnapdChange contains information on a current Snap transaction.
 *
 * Since: 1.5
 */

struct _SnapdChange
{
    GObject parent_instance;

    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *status;
    GPtrArray *tasks;
    gboolean ready;
    GDateTime *spawn_time;
    GDateTime *ready_time;
};

enum
{
    PROP_ID = 1,
    PROP_KIND,
    PROP_SUMMARY,
    PROP_STATUS,
    PROP_TASKS,
    PROP_READY,
    PROP_SPAWN_TIME,
    PROP_READY_TIME,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdChange, snapd_change, G_TYPE_OBJECT)

/**
 * snapd_change_get_id:
 * @change: a #SnapdChange.
 *
 * Get the unique ID for this change.
 *
 * Returns: an ID.
 *
 * Since: 1.5
 */
const gchar *
snapd_change_get_id (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->id;
}

/**
 * snapd_change_get_kind:
 * @change: a #SnapdChange.
 *
 * Gets the kind of change this is.
 *
 * Returns: the kind of change.
 *
 * Since: 1.5
 */
const gchar *
snapd_change_get_kind (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->kind;
}

/**
 * snapd_change_get_summary:
 * @change: a #SnapdChange.
 *
 * Get a human readable description of the change.
 *
 * Returns: a string describing the change.
 *
 * Since: 1.5
 */
const gchar *
snapd_change_get_summary (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->summary;
}

/**
 * snapd_change_get_status:
 * @change: a #SnapdChange.
 *
 * Get the status of the change.
 *
 * Returns: a status string.
 *
 * Since: 1.5
 */
const gchar *
snapd_change_get_status (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->status;
}

/**
 * snapd_change_get_tasks:
 * @change: a #SnapdChange.
 *
 * Get the tasks that are in this change.
 *
 * Returns: (transfer none) (element-type SnapdTask): an array of #SnapdTask.
 *
 * Since: 1.5
 */
GPtrArray *
snapd_change_get_tasks (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->tasks;
}

/**
 * snapd_change_get_ready:
 * @change: a #SnapdChange.
 *
 * Get if this change is completed.
 *
 * Returns: %TRUE if this change is complete.
 *
 * Since: 1.5
 */
gboolean
snapd_change_get_ready (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), FALSE);
    return change->ready;
}

/**
 * snapd_change_get_spawn_time:
 * @change: a #SnapdChange.
 *
 * Get the time this change started.
 *
 * Returns: (transfer none): a #GDateTime.
 *
 * Since: 1.5
 */
GDateTime *
snapd_change_get_spawn_time (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->spawn_time;
}

/**
 * snapd_change_get_ready_time:
 * @change: a #SnapdChange.
 *
 * Get the time this taksk completed or %NULL if not yet completed.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime or %NULL.
 *
 * Since: 1.5
 */
GDateTime *
snapd_change_get_ready_time (SnapdChange *change)
{
    g_return_val_if_fail (SNAPD_IS_CHANGE (change), NULL);
    return change->ready_time;
}

static void
snapd_change_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdChange *change = SNAPD_CHANGE (object);

    switch (prop_id) {
    case PROP_ID:
        g_free (change->id);
        change->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_KIND:
        g_free (change->kind);
        change->kind = g_strdup (g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        g_free (change->summary);
        change->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_STATUS:
        g_free (change->status);
        change->status = g_strdup (g_value_get_string (value));
        break;
    case PROP_TASKS:
        g_clear_pointer (&change->tasks, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            change->tasks = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_READY:
        change->ready = g_value_get_boolean (value);
        break;
    case PROP_SPAWN_TIME:
        g_clear_pointer (&change->spawn_time, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            change->spawn_time = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_READY_TIME:
        g_clear_pointer (&change->ready_time, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            change->ready_time = g_date_time_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_change_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdChange *change = SNAPD_CHANGE (object);

    switch (prop_id) {
    case PROP_ID:
        g_value_set_string (value, change->id);
        break;
    case PROP_KIND:
        g_value_set_string (value, change->kind);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, change->summary);
        break;
    case PROP_STATUS:
        g_value_set_string (value, change->status);
        break;
    case PROP_TASKS:
        g_value_set_boxed (value, change->tasks);
        break;
    case PROP_READY:
        g_value_set_boolean (value, change->ready);
        break;
    case PROP_SPAWN_TIME:
        g_value_set_boxed (value, change->spawn_time);
        break;
    case PROP_READY_TIME:
        g_value_set_boxed (value, change->ready_time);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_change_finalize (GObject *object)
{
    SnapdChange *change = SNAPD_CHANGE (object);

    g_clear_pointer (&change->id, g_free);
    g_clear_pointer (&change->kind, g_free);
    g_clear_pointer (&change->summary, g_free);
    g_clear_pointer (&change->status, g_free);
    g_clear_pointer (&change->tasks, g_ptr_array_unref);
    g_clear_pointer (&change->spawn_time, g_date_time_unref);
    g_clear_pointer (&change->ready_time, g_date_time_unref);

    G_OBJECT_CLASS (snapd_change_parent_class)->finalize (object);
}

static void
snapd_change_class_init (SnapdChangeClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_change_set_property;
    gobject_class->get_property = snapd_change_get_property;
    gobject_class->finalize = snapd_change_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          "id",
                                                          "ID of change",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_KIND,
                                     g_param_spec_string ("kind",
                                                          "kind",
                                                          "Kind of change",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary",
                                                          "summary",
                                                          "Summary of change",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STATUS,
                                     g_param_spec_string ("status",
                                                          "status",
                                                          "Status of change",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TASKS,
                                     g_param_spec_boxed ("tasks",
                                                         "tasks",
                                                         "Tasks in this change",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_READY,
                                     g_param_spec_boolean ("ready",
                                                           "ready",
                                                           "TRUE when change complete",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SPAWN_TIME,
                                     g_param_spec_boxed ("spawn-time",
                                                         "spawn-time",
                                                         "Time this change started",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_READY_TIME,
                                     g_param_spec_boxed ("ready-time",
                                                         "ready-time",
                                                         "Time this change completed",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_change_init (SnapdChange *change)
{
}
