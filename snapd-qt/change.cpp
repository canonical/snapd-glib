/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/change.h"

QSnapdChange::QSnapdChange(void *snapd_object, QObject *parent)
    : QSnapdWrappedObject(g_object_ref(snapd_object), g_object_unref, parent) {}

QString QSnapdChange::id() const {
  return snapd_change_get_id(SNAPD_CHANGE(wrapped_object));
}

QString QSnapdChange::kind() const {
  return snapd_change_get_kind(SNAPD_CHANGE(wrapped_object));
}

QString QSnapdChange::summary() const {
  return snapd_change_get_summary(SNAPD_CHANGE(wrapped_object));
}

QString QSnapdChange::status() const {
  return snapd_change_get_status(SNAPD_CHANGE(wrapped_object));
}

bool QSnapdChange::ready() const {
  return snapd_change_get_ready(SNAPD_CHANGE(wrapped_object));
}

int QSnapdChange::taskCount() const {
  GPtrArray *tasks;

  tasks = snapd_change_get_tasks(SNAPD_CHANGE(wrapped_object));
  return tasks != NULL ? tasks->len : 0;
}

QSnapdTask *QSnapdChange::task(int n) const {
  GPtrArray *tasks;

  tasks = snapd_change_get_tasks(SNAPD_CHANGE(wrapped_object));
  if (tasks == NULL || n < 0 || (guint)n >= tasks->len)
    return NULL;
  return new QSnapdTask(tasks->pdata[n]);
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

QDateTime QSnapdChange::spawnTime() const {
  return convertDateTime(
      snapd_change_get_spawn_time(SNAPD_CHANGE(wrapped_object)));
}

QDateTime QSnapdChange::readyTime() const {
  return convertDateTime(
      snapd_change_get_ready_time(SNAPD_CHANGE(wrapped_object)));
}

QString QSnapdChange::error() const {
  return snapd_change_get_error(SNAPD_CHANGE(wrapped_object));
}

QSnapdChangeData *QSnapdChange::data() const {
  SnapdAutorefreshChangeData *data = SNAPD_AUTOREFRESH_CHANGE_DATA(
      snapd_change_get_data(SNAPD_CHANGE(wrapped_object)));
  if (data == NULL)
    return NULL;
  return new QSnapdAutorefreshChangeData(SNAPD_CHANGE_DATA(data), NULL);
}