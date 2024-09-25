/*
 * Copyright (C) 2024 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "snapd-notices-monitor.h"

/**
 * SECTION: snapd-notices-monitor
 * @short_description: Allows to receive events from snapd.
 * @include: snapd-glib/snapd-glib.h
 *
 * #SnapdNoticesMonitor allows to receive in real time events from
 * snapd, like status changes in an ongoing refresh, inhibited refreshes
 * due to the snap being active, or inhibited launches due to an ongoing
 * refresh.
 */

/**
 * SnapdNoticesMonitor:
 *
 * #SnapdNoticesMonitor allows to receive in real time events from
 * snapd, like status changes in an ongoing refresh, inhibited refreshes
 * due to the snap being active, or inhibited launches due to an ongoing
 * refresh.
 *
 * Since: 1.66
 */

struct _SnapdNoticesMonitor {
  GObject parent_instance;

  SnapdClient *client;
  GCancellable *cancellable;
  SnapdNotice *last_notice;
  gboolean running;
};

enum { PROP_CLIENT = 1, PROP_LAST };

G_DEFINE_TYPE(SnapdNoticesMonitor, snapd_notices_monitor, G_TYPE_OBJECT)

static void begin_monitor(SnapdNoticesMonitor *self);

static void monitor_cb(SnapdClient *source, GAsyncResult *res,
                       SnapdNoticesMonitor *self) {
  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) notices =
      snapd_client_get_notices_finish(source, res, &error);

  if (error != NULL) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      g_signal_emit_by_name(self, "error-event", error);
    self->running = FALSE;
    g_clear_object(&self->cancellable);
    self->cancellable = g_cancellable_new();
    g_object_unref(self);
    return;
  }

  if (notices != NULL) {
    gboolean first_run = self->last_notice == NULL;
    for (int i = 0; i < notices->len; i++) {
      g_autoptr(SnapdNotice) notice = g_object_ref(notices->pdata[i]);

      if (self->last_notice == NULL ||
          snapd_notice_compare_last_occurred(self->last_notice, notice) <= 0) {
        g_clear_object(&self->last_notice);
        self->last_notice = g_object_ref(notice);
      }
      g_signal_emit_by_name(self, "notice-event", notice, first_run);
    }
  }
  begin_monitor(self); // continue waiting for new notifications
}

static void begin_monitor(SnapdNoticesMonitor *self) {
  snapd_client_notices_set_after_notice(self->client, self->last_notice);
  GDateTime *last_date_time =
      self->last_notice == NULL
          ? NULL
          : (GDateTime *)snapd_notice_get_last_occurred2(self->last_notice);
  snapd_client_get_notices_async(
      self->client, last_date_time,
      2000000000000000, // "infinity" (there is a limit in snapd, around 9
                        // billion seconds)
      self->cancellable, (GAsyncReadyCallback)monitor_cb, self);
}

/**
 * snapd_notices_monitor_start:
 * @monitor: a #SnapdNoticesMonitor
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Starts the asynchronous listening proccess, that will wait for new
 * notices and emit a "notice-event" signal with the new notice as
 * parameter.
 *
 * Returns: FALSE if there was an error, TRUE if everything worked fine
 * and the object is listening for events.
 *
 * Since: 1.66
 */
gboolean snapd_notices_monitor_start(SnapdNoticesMonitor *self,
                                     GError **error) {
  g_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);
  g_return_val_if_fail(SNAPD_IS_NOTICES_MONITOR(self), FALSE);
  if (self->running) {
    *error = g_error_new(SNAPD_ERROR, SNAPD_ERROR_ALREADY_RUNNING,
                         "The notices monitor is already running.");
    return FALSE;
  }
  self->running = TRUE;
  begin_monitor(g_object_ref(self));
  return TRUE;
}

/**
 * snapd_notices_monitor_stop:
 * @monitor: a #SnapdNoticesMonitor
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Stops the asynchronous listening proccess started with
 * #snapd_notices_monitor_start.
 *
 * Returns: FALSE if there was an error, TRUE if everything worked fine.
 *
 * Since: 1.66
 */
gboolean snapd_notices_monitor_stop(SnapdNoticesMonitor *self, GError **error) {
  g_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);
  g_return_val_if_fail(SNAPD_IS_NOTICES_MONITOR(self), FALSE);
  if (!self->running) {
    *error = g_error_new(SNAPD_ERROR, SNAPD_ERROR_NOT_RUNNING,
                         "The notices monitor isn't running.");
    return FALSE;
  }
  g_cancellable_cancel(self->cancellable);
  return TRUE;
}

static void snapd_notices_monitor_dispose(GObject *object) {
  SnapdNoticesMonitor *self = SNAPD_NOTICES_MONITOR(object);

  if (self->cancellable != NULL)
    g_cancellable_cancel(self->cancellable);
  g_clear_object(&self->client);
  g_clear_object(&self->cancellable);
  g_clear_object(&self->last_notice);

  G_OBJECT_CLASS(snapd_notices_monitor_parent_class)->dispose(object);
}

static void snapd_notices_monitor_set_property(GObject *object, guint prop_id,
                                               const GValue *value,
                                               GParamSpec *pspec) {
  SnapdNoticesMonitor *self = SNAPD_NOTICES_MONITOR(object);

  switch (prop_id) {
  case PROP_CLIENT:
    g_clear_object(&self->client);
    if (g_value_get_object(value) != NULL)
      self->client = g_object_ref(g_value_get_object(value));
    break;
  }
}

static void snapd_notices_monitor_init(SnapdNoticesMonitor *self) {
  self->cancellable = g_cancellable_new();
}

static void snapd_notices_monitor_class_init(SnapdNoticesMonitorClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_notices_monitor_set_property;
  gobject_class->dispose = snapd_notices_monitor_dispose;

  g_object_class_install_property(
      gobject_class, PROP_CLIENT,
      g_param_spec_object(
          "client", "client", "SnapdClient to use to communicate",
          SNAPD_TYPE_CLIENT, G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_signal_new("notice-event", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0,
               NULL, NULL, NULL, G_TYPE_NONE, 2, SNAPD_TYPE_NOTICE,
               G_TYPE_BOOLEAN);
  g_signal_new("error-event", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0,
               NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_ERROR);
}

/**
 * snapd_notices_monitor_new:
 *
 * Creates a new #SnapdNoticesMonitor to receive events.
 *
 * Returns: (transfer full): a new #SnapdNoticesMonitor
 *
 * Since: 1.66
 */

SnapdNoticesMonitor *snapd_notices_monitor_new(void) {
  g_autoptr(SnapdClient) client = snapd_client_new();
  SnapdNoticesMonitor *self =
      g_object_new(snapd_notices_monitor_get_type(), "client", client, NULL);
  return self;
}

/**
 * snapd_notices_monitor_new_with_client:
 * @client: a #SnapdClient object
 *
 * Creates a new #SnapdNoticesMonitor to receive events, using the
 * specified #SnapdClient object to ask info about each task.
 *
 * Returns: (transfer full): a new #SnapdNoticesMonitor
 *
 * Since: 1.66
 */

SnapdNoticesMonitor *
snapd_notices_monitor_new_with_client(SnapdClient *client) {
  SnapdNoticesMonitor *self =
      g_object_new(snapd_notices_monitor_get_type(), "client", client, NULL);
  return self;
}
