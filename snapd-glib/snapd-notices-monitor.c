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

struct _SnapdNoticesMonitor {
    GObject parent_instance;

    SnapdClient *client;
    GCancellable *cancellable;
    GDateTime *last_date_time;
    SnapdNotice *last_notice;
};

G_DEFINE_TYPE(SnapdNoticesMonitor, snapd_notices_monitor, G_TYPE_OBJECT)

static void begin_monitor(SnapdNoticesMonitor *self);

static void monitor_cb(SnapdClient* source,
                       GAsyncResult* res,
                       SnapdNoticesMonitor *self) {

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) notices = snapd_client_get_notices_finish(source, res, &error);

    if (g_cancellable_is_cancelled(self->cancellable)) {
        g_clear_object(&self->cancellable);
        g_clear_pointer(&self->last_date_time, g_date_time_unref);
        g_clear_object(&self->last_notice);
        return;
    }

    if (error != NULL) {
        g_clear_object(&self->cancellable);
        g_clear_pointer(&self->last_date_time, g_date_time_unref);
        g_clear_object(&self->last_notice);
        g_signal_emit_by_name(self, "error-event", error);
        return;
    }

    if ((error == NULL) && (notices != NULL)) {
        for (int i = 0; i < notices->len; i++) {
            g_autoptr(SnapdNotice) notice = g_object_ref(notices->pdata[i]);
            g_autoptr(GDateTime) last_occurred = (GDateTime *)snapd_notice_get_last_occurred(notice);

            if (self->last_date_time == NULL || g_date_time_compare(self->last_date_time, last_occurred) <= 0) {
                g_clear_pointer (&self->last_date_time, g_date_time_unref);
                self->last_date_time = g_date_time_ref(last_occurred);
                g_clear_object(&self->last_notice);
                self->last_notice = g_object_ref(notice);
            }
            g_signal_emit_by_name(self, "notice-event", notice);
        }
    }
    begin_monitor(self); // continue waiting for new notifications
}

static void begin_monitor(SnapdNoticesMonitor *self) {
    snapd_client_notices_set_after_notice (self->client, self->last_notice);
    snapd_client_get_notices_async(self->client,
                                   self->last_date_time,
                                   2000000000000000, // "infinity" (there is a limit in snapd, around 9 billion seconds)
                                   self->cancellable,
                                   (GAsyncReadyCallback)monitor_cb,
                                   self);
}

/**
 * snapd_notices_monitor_start:
 * @monitor: a #SnapdNoticesMonitor
 *
 * Starts the asynchronous listening proccess, that will wait for new
 * notices and emit a "notice-event" signal with the new notice as
 * parameter.
 *
 * @return: FALSE if there was an error, TRUE if everything worked fine
 * and the object is listening for events.
 */
gboolean snapd_notices_monitor_start(SnapdNoticesMonitor *self, GError **error) {

    g_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);
    g_return_val_if_fail(SNAPD_IS_NOTICES_MONITOR(self), FALSE);
    if (self->cancellable != NULL) {
        *error = g_error_new(SNAPD_ERROR, SNAPD_ERROR_ALREADY_RUNNING, "The notices monitor is already running.");
        return FALSE;
    }
    self->cancellable = g_cancellable_new();
    begin_monitor(self);
    return TRUE;
}

/**
 * snapd_notices_monitor_stop:
 * @monitor: a #SnapdNoticesMonitor
 *
 * Stops the asynchronous listening proccess started with #snapd_notices_monitor_start.
 *
 * @return: FALSE if there was an error, TRUE if everything worked fine.
 */
gboolean snapd_notices_monitor_stop(SnapdNoticesMonitor *self, GError **error) {

    g_return_val_if_fail((error == NULL) || (*error == NULL), FALSE);
    g_return_val_if_fail(SNAPD_IS_NOTICES_MONITOR(self), FALSE);
    if (self->cancellable == NULL) {
        *error = g_error_new(SNAPD_ERROR, SNAPD_ERROR_NOT_RUNNING, "The notices monitor isn't running.");
        return FALSE;
    }
    g_cancellable_cancel(self->cancellable);
    g_clear_object(&self->cancellable);
    return TRUE;
}

/**
 * snapd_notices_monitor_get_client:
 * @monitor: a #SnapdNoticesMonitor
 *
 * Returns the internal #SnapdClient used by this object. Useful if some
 * kind of extra initialization must be done before begin to read notices.
 * This client must NOT be used for anything else, because the get_notices
 * call will block the connection until a response is generated. This client
 * MUST be used only by this object.
 *
 * @returns: (transfer none): a #SnapdClient
 */
SnapdClient *snapd_notices_monitor_get_client(SnapdNoticesMonitor *self) {
    g_return_val_if_fail(SNAPD_IS_NOTICES_MONITOR(self), NULL);
    return self->client;
}

static void snapd_notices_monitor_dispose(GObject *object) {
    SnapdNoticesMonitor *self = SNAPD_NOTICES_MONITOR(object);

    g_clear_object(&self->client);
    g_clear_object(&self->cancellable);
    g_clear_pointer(&self->last_date_time, g_date_time_unref);
    g_clear_object(&self->last_notice);

    G_OBJECT_CLASS(snapd_notices_monitor_parent_class)->dispose(object);
}

void snapd_notices_monitor_init(SnapdNoticesMonitor *self) {
    self->client = snapd_client_new();
}

void snapd_notices_monitor_class_init(SnapdNoticesMonitorClass *klass) {
    G_OBJECT_CLASS(klass)->dispose = snapd_notices_monitor_dispose;
    g_signal_new ("notice-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  SNAPD_TYPE_NOTICE);
    g_signal_new ("error-event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_ERROR);
}

SnapdNoticesMonitor *snapd_notices_monitor_new(void) {
    SnapdNoticesMonitor *self = g_object_new(snapd_notices_monitor_get_type(), NULL);
    return self;
}
