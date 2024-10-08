/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-logs.h"

#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-log.h"

struct _SnapdGetLogs {
  SnapdRequest parent_instance;
  gchar **names;
  size_t n;
  gboolean follow;
  SnapdGetLogsLogCallback log_callback;
  gpointer log_callback_data;
  GDestroyNotify log_callback_destroy_notify;
  GPtrArray *logs;
};

G_DEFINE_TYPE(SnapdGetLogs, snapd_get_logs, snapd_request_get_type())

SnapdGetLogs *_snapd_get_logs_new(gchar **names, size_t n, gboolean follow,
                                  SnapdGetLogsLogCallback log_callback,
                                  gpointer log_callback_data,
                                  GDestroyNotify log_callback_destroy_notify,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data) {
  SnapdGetLogs *self = SNAPD_GET_LOGS(g_object_new(
      snapd_get_logs_get_type(), "cancellable", cancellable, "ready-callback",
      callback, "ready-callback-data", user_data, NULL));

  if (names != NULL && names[0] != NULL)
    self->names = g_strdupv(names);
  self->n = n;
  self->follow = follow;
  self->log_callback = log_callback;
  self->log_callback_data = log_callback_data;
  self->log_callback_destroy_notify = log_callback_destroy_notify;

  return self;
}

GPtrArray *_snapd_get_logs_get_logs(SnapdGetLogs *self) { return self->logs; }

static SoupMessage *generate_get_logs_request(SnapdRequest *request,
                                              GBytes **body) {
  SnapdGetLogs *self = SNAPD_GET_LOGS(request);

  g_autoptr(GPtrArray) query_attributes =
      g_ptr_array_new_with_free_func(g_free);
  if (self->names != NULL) {
    g_autofree gchar *names_list = g_strjoinv(",", self->names);
    g_ptr_array_add(query_attributes, g_strdup_printf("names=%s", names_list));
  }
  if (self->n != 0) {
    g_ptr_array_add(query_attributes, g_strdup_printf("n=%zi", self->n));
  }
  if (self->follow) {
    g_ptr_array_add(query_attributes, g_strdup("follow=true"));
  }

  g_autoptr(GString) path = g_string_new("http://snapd/v2/logs");
  if (query_attributes->len > 0) {
    g_string_append_c(path, '?');
    for (guint i = 0; i < query_attributes->len; i++) {
      if (i != 0)
        g_string_append_c(path, '&');
      g_string_append(path, (gchar *)query_attributes->pdata[i]);
    }
  }

  return soup_message_new("GET", path->str);
}

static gboolean parse_get_logs_response(SnapdRequest *request,
                                        guint status_code,
                                        const gchar *content_type, GBytes *body,
                                        SnapdMaintenance **maintenance,
                                        GError **error) {
  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, NULL, NULL, error);
  if (response == NULL)
    return FALSE;
  g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a(response, error);
  if (result != NULL) {
    g_set_error(error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED,
                "Unexpected snap log response");
  }

  return FALSE;
}

static gboolean parse_get_logs_json_seq(SnapdRequest *request, JsonNode *seq,
                                        GError **error) {
  g_autoptr(GDateTime) timestamp = NULL;
  const gchar *message, *sid, *pid;
  g_autoptr(SnapdLog) log = NULL;

  SnapdGetLogs *self = SNAPD_GET_LOGS(request);

  if (!JSON_NODE_HOLDS_OBJECT(seq)) {
    g_set_error(error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED,
                "Unexpected snap log type");
    return FALSE;
  }
  JsonObject *object = json_node_get_object(seq);
  timestamp = _snapd_json_get_date_time(object, "timestamp", NULL);
  message = _snapd_json_get_string(object, "message", NULL);
  sid = _snapd_json_get_string(object, "sid", NULL);
  pid = _snapd_json_get_string(object, "pid", NULL);
  log = g_object_new(snapd_log_get_type(), "timestamp", timestamp, "message",
                     message, "sid", sid, "pid", pid, NULL);

  if (self->log_callback != NULL) {
    self->log_callback(self, log, self->log_callback_data);
  } else {
    g_ptr_array_add(self->logs, g_steal_pointer(&log));
  }

  return TRUE;
}

static void snapd_get_logs_finalize(GObject *object) {
  SnapdGetLogs *self = SNAPD_GET_LOGS(object);

  g_clear_pointer(&self->names, g_strfreev);
  g_clear_pointer(&self->logs, g_ptr_array_unref);
  if (self->log_callback_destroy_notify) {
    self->log_callback_destroy_notify(self->log_callback_data);
  }

  G_OBJECT_CLASS(snapd_get_logs_parent_class)->finalize(object);
}

static void snapd_get_logs_class_init(SnapdGetLogsClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_get_logs_request;
  request_class->parse_response = parse_get_logs_response;
  request_class->parse_json_seq = parse_get_logs_json_seq;
  gobject_class->finalize = snapd_get_logs_finalize;
}

static void snapd_get_logs_init(SnapdGetLogs *self) {
  self->logs = g_ptr_array_new_with_free_func(g_object_unref);
}
