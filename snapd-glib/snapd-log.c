/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-log.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-log
 * @short_description: Snap service log entry
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdLog contains a line from a log for a snap service as returned using
 * snapd_client_get_logs_sync().
 */

/**
 * SnapdLog:
 *
 * #SnapdLog is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.64
 */

struct _SnapdLog {
  GObject parent_instance;

  GDateTime *timestamp;
  gchar *message;
  gchar *sid;
  gchar *pid;
};

enum { PROP_TIMESTAMP = 1, PROP_MESSAGE, PROP_SID, PROP_PID, PROP_LAST };

G_DEFINE_TYPE(SnapdLog, snapd_log, G_TYPE_OBJECT)

/**
 * snapd_log_get_timestamp:
 * @log: a #SnapdLog.
 *
 * Get the time this log was generated.
 *
 * Returns: a timestamp.
 *
 * Since: 1.64
 */
GDateTime *snapd_log_get_timestamp(SnapdLog *self) {
  g_return_val_if_fail(SNAPD_IS_LOG(self), NULL);
  return self->timestamp;
}

/**
 * snapd_log_get_message:
 * @log: a #SnapdLog.
 *
 * Get the message of this log, e.g. "service started"
 *
 * Returns: a log message.
 *
 * Since: 1.64
 */
const gchar *snapd_log_get_message(SnapdLog *self) {
  g_return_val_if_fail(SNAPD_IS_LOG(self), NULL);
  return self->message;
}

/**
 * snapd_log_get_sid:
 * @log: a #SnapdLog.
 *
 * Get the syslog id of this log, e.g. "cups.cups-browsed"
 *
 * Returns: a syslog id.
 *
 * Since: 1.64
 */
const gchar *snapd_log_get_sid(SnapdLog *self) {
  g_return_val_if_fail(SNAPD_IS_LOG(self), NULL);
  return self->sid;
}

/**
 * snapd_log_get_pid:
 * @log: a #SnapdLog.
 *
 * Get the process ID of this log, e.g. "1234"
 *
 * Returns: a process id.
 *
 * Since: 1.64
 */
const gchar *snapd_log_get_pid(SnapdLog *self) {
  g_return_val_if_fail(SNAPD_IS_LOG(self), NULL);
  return self->pid;
}

static void snapd_log_set_property(GObject *object, guint prop_id,
                                   const GValue *value, GParamSpec *pspec) {
  SnapdLog *self = SNAPD_LOG(object);

  switch (prop_id) {
  case PROP_TIMESTAMP:
    g_clear_pointer(&self->timestamp, g_date_time_unref);
    if (g_value_get_boxed(value) != NULL)
      self->timestamp = g_date_time_ref(g_value_get_boxed(value));
    break;
  case PROP_MESSAGE:
    g_free(self->message);
    self->message = g_strdup(g_value_get_string(value));
    break;
  case PROP_SID:
    g_free(self->sid);
    self->sid = g_strdup(g_value_get_string(value));
    break;
  case PROP_PID:
    g_free(self->pid);
    self->pid = g_strdup(g_value_get_string(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_log_get_property(GObject *object, guint prop_id,
                                   GValue *value, GParamSpec *pspec) {
  SnapdLog *self = SNAPD_LOG(object);

  switch (prop_id) {
  case PROP_TIMESTAMP:
    g_value_set_boxed(value, self->timestamp);
    break;
  case PROP_MESSAGE:
    g_value_set_string(value, self->message);
    break;
  case PROP_SID:
    g_value_set_string(value, self->sid);
    break;
  case PROP_PID:
    g_value_set_string(value, self->pid);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_log_finalize(GObject *object) {
  SnapdLog *self = SNAPD_LOG(object);

  g_clear_pointer(&self->timestamp, g_date_time_unref);
  g_clear_pointer(&self->message, g_free);
  g_clear_pointer(&self->sid, g_free);
  g_clear_pointer(&self->pid, g_free);

  G_OBJECT_CLASS(snapd_log_parent_class)->finalize(object);
}

static void snapd_log_class_init(SnapdLogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_log_set_property;
  gobject_class->get_property = snapd_log_get_property;
  gobject_class->finalize = snapd_log_finalize;

  g_object_class_install_property(
      gobject_class, PROP_TIMESTAMP,
      g_param_spec_boxed(
          "timestamp", "timestamp", "Timestamp", G_TYPE_DATE_TIME,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_MESSAGE,
      g_param_spec_string("message", "message", "Message", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SID,
      g_param_spec_string("sid", "sid", "Syslog ID", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_PID,
      g_param_spec_string("pid", "pid", "Process ID", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
}

static void snapd_log_init(SnapdLog *self) {}
