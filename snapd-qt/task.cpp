/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/task.h"

QSnapdTask::QSnapdTask(void *snapd_object, QObject *parent)
    : QSnapdWrappedObject(g_object_ref(snapd_object), g_object_unref, parent) {}

QString QSnapdTask::id() const {
  return snapd_task_get_id(SNAPD_TASK(wrapped_object));
}

QString QSnapdTask::kind() const {
  return snapd_task_get_kind(SNAPD_TASK(wrapped_object));
}

QString QSnapdTask::summary() const {
  return snapd_task_get_summary(SNAPD_TASK(wrapped_object));
}

QString QSnapdTask::status() const {
  return snapd_task_get_status(SNAPD_TASK(wrapped_object));
}

QString QSnapdTask::progressLabel() const {
  return snapd_task_get_progress_label(SNAPD_TASK(wrapped_object));
}

qint64 QSnapdTask::progressDone() const {
  return snapd_task_get_progress_done(SNAPD_TASK(wrapped_object));
}

qint64 QSnapdTask::progressTotal() const {
  return snapd_task_get_progress_total(SNAPD_TASK(wrapped_object));
}

static QDateTime convertDateTime(GDateTime *datetime) {
  if (datetime == NULL)
    return QDateTime();

  QDate date(g_date_time_get_year(datetime), g_date_time_get_month(datetime),
             g_date_time_get_day_of_month(datetime));
  QTime time(g_date_time_get_hour(datetime), g_date_time_get_minute(datetime),
             g_date_time_get_second(datetime),
             g_date_time_get_microsecond(datetime) / 1000);
  return QDateTime(date, time, Qt::OffsetFromUTC,
                   g_date_time_get_utc_offset(datetime) / 1000000);
}

QDateTime QSnapdTask::spawnTime() const {
  return convertDateTime(snapd_task_get_spawn_time(SNAPD_TASK(wrapped_object)));
}

QDateTime QSnapdTask::readyTime() const {
  return convertDateTime(snapd_task_get_ready_time(SNAPD_TASK(wrapped_object)));
}

QSnapdTaskData *QSnapdTask::taskData() const {
  SnapdTaskData *data =
      SNAPD_TASK_DATA(snapd_task_get_data(SNAPD_TASK(wrapped_object)));
  if (data == NULL)
    return NULL;
  return new QSnapdTaskData(SNAPD_TASK_DATA(data), NULL);
}
