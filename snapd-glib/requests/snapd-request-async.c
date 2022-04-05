/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-request-async.h"

#include "snapd-client.h"
#include "snapd-json.h"
#include "snapd-task.h"

enum
{
    PROP_PROGRESS_CALLBACK = 1,
    PROP_PROGRESS_CALLBACK_DATA,
    PROP_CHANGE_API_PATH,
    PROP_LAST
};

typedef struct
{
    SnapdProgressCallback progress_callback;
    gpointer progress_callback_data;
    gchar *change_api_path;

    /* Returned change ID for this request */
    gchar *change_id;

    /* Last reported change (so we don't send duplicates) */
    SnapdChange *change;
} SnapdRequestAsyncPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdRequestAsync, snapd_request_async, snapd_request_get_type ())

const gchar *
_snapd_request_async_get_change_id (SnapdRequestAsync *self)
{
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);
    return priv->change_id;
}

gboolean
_snapd_request_async_parse_result (SnapdRequestAsync *self, JsonNode *result, GError **error)
{
    if (SNAPD_REQUEST_ASYNC_GET_CLASS (self)->parse_result == NULL)
        return TRUE;
    return SNAPD_REQUEST_ASYNC_GET_CLASS (self)->parse_result (self, result, error);
}

static gboolean
parse_async_response (SnapdRequest *self, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdRequestAsync *r = SNAPD_REQUEST_ASYNC (self);
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (r);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autofree gchar *change_id = _snapd_json_get_async_result (response, error);
    if (change_id == NULL)
        return FALSE;

    priv->change_id = g_strdup (change_id);

    return TRUE;
}

static gboolean
times_equal (GDateTime *time1, GDateTime *time2)
{
    if (time1 == NULL || time2 == NULL)
        return time1 == time2;
    return g_date_time_equal (time1, time2);
}

static gboolean
tasks_equal (SnapdTask *task1, SnapdTask *task2)
{
    return g_strcmp0 (snapd_task_get_id (task1), snapd_task_get_id (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_kind (task1), snapd_task_get_kind (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_summary (task1), snapd_task_get_summary (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_status (task1), snapd_task_get_status (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_progress_label (task1), snapd_task_get_progress_label (task2)) == 0 &&
           snapd_task_get_progress_done (task1) == snapd_task_get_progress_done (task2) &&
           snapd_task_get_progress_total (task1) == snapd_task_get_progress_total (task2) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2)) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2));
}

static gboolean
changes_equal (SnapdChange *change1, SnapdChange *change2)
{
    if (change1 == NULL || change2 == NULL)
        return change1 == change2;

    GPtrArray *tasks1 = snapd_change_get_tasks (change1);
    GPtrArray *tasks2 = snapd_change_get_tasks (change2);
    if (tasks1 == NULL || tasks2 == NULL) {
        if (tasks1 != tasks2)
            return FALSE;
    }
    else {
        if (tasks1->len != tasks2->len)
            return FALSE;
        for (int i = 0; i < tasks1->len; i++) {
            SnapdTask *t1 = tasks1->pdata[i], *t2 = tasks2->pdata[i];
            if (!tasks_equal (t1, t2))
                return FALSE;
        }
    }

    return g_strcmp0 (snapd_change_get_id (change1), snapd_change_get_id (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_kind (change1), snapd_change_get_kind (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_summary (change1), snapd_change_get_summary (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_status (change1), snapd_change_get_status (change2)) == 0 &&
           !!snapd_change_get_ready (change1) == !!snapd_change_get_ready (change2) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2)) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2));

    return TRUE;
}

void
_snapd_request_async_report_progress (SnapdRequestAsync *self, SnapdClient *client, SnapdChange *change)
{
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);

    if (!changes_equal (priv->change, change)) {
        g_set_object (&priv->change, change);
        if (priv->progress_callback != NULL)
            priv->progress_callback (client,
                                     change,
                                     snapd_change_get_tasks (change), // Passed for ABI compatibility, is deprecated
                                     priv->progress_callback_data);
    }
}

SnapdGetChange *
_snapd_request_async_make_get_change_request (SnapdRequestAsync *self)
{
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);
    SnapdGetChange *request = _snapd_get_change_new (priv->change_id, NULL, NULL, NULL);

    _snapd_get_change_set_api_path (request, priv->change_api_path);
    return request;
}

SnapdPostChange *
_snapd_request_async_make_post_change_request (SnapdRequestAsync *self)
{
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);
    SnapdPostChange *request = _snapd_post_change_new (priv->change_id, "abort", NULL, NULL, NULL);

    _snapd_post_change_set_api_path (request, priv->change_api_path);
    return request;
}

static void
snapd_request_async_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdRequestAsync *self = SNAPD_REQUEST_ASYNC (object);
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);

    switch (prop_id)
    {
    case PROP_PROGRESS_CALLBACK:
        priv->progress_callback = g_value_get_pointer (value);
        break;
    case PROP_PROGRESS_CALLBACK_DATA:
        priv->progress_callback_data = g_value_get_pointer (value);
        break;
    case PROP_CHANGE_API_PATH:
        g_free (priv->change_api_path);
        priv->change_api_path = g_value_dup_string (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_request_async_finalize (GObject *object)
{
    SnapdRequestAsync *self = SNAPD_REQUEST_ASYNC (object);
    SnapdRequestAsyncPrivate *priv = snapd_request_async_get_instance_private (self);

    g_clear_pointer (&priv->change_id, g_free);
    g_clear_object (&priv->change);

    G_OBJECT_CLASS (snapd_request_async_parent_class)->finalize (object);
}

static void
snapd_request_async_class_init (SnapdRequestAsyncClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->parse_response = parse_async_response;
   gobject_class->set_property = snapd_request_async_set_property;
   gobject_class->finalize = snapd_request_async_finalize;

   g_object_class_install_property (gobject_class,
                                    PROP_PROGRESS_CALLBACK,
                                    g_param_spec_pointer ("progress-callback",
                                                          "progress-callback",
                                                          "Progress callback",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_PROGRESS_CALLBACK_DATA,
                                    g_param_spec_pointer ("progress-callback-data",
                                                          "progress-callback-data",
                                                          "Data for progress callback",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_CHANGE_API_PATH,
                                    g_param_spec_string ("change-api-path",
                                                          "change-api-path",
                                                          "change-api-path",
                                                          NULL,
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_request_async_init (SnapdRequestAsync *self)
{
}
