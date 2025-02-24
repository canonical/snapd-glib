/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CHANGE_H
#define SNAPD_CHANGE_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <Snapd/Task>
#include <Snapd/WrappedObject>
#include <Snapd/change-autorefresh-data.h>
#include <Snapd/change-data.h>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdChange : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString id READ id)
  Q_PROPERTY(QString kind READ kind)
  Q_PROPERTY(QString summary READ summary)
  Q_PROPERTY(QString status READ status)
  Q_PROPERTY(bool ready READ ready)
  Q_PROPERTY(int taskCount READ taskCount)
  Q_PROPERTY(QDateTime spawnTime READ spawnTime)
  Q_PROPERTY(QDateTime readyTime READ readyTime)
  Q_PROPERTY(QString error READ error)
  Q_PROPERTY(QSnapdChangeData *data READ data)

public:
  explicit QSnapdChange(void *snapd_object, QObject *parent = 0);

  QString id() const;
  QString kind() const;
  QString summary() const;
  QString status() const;
  bool ready() const;
  int taskCount() const;
  Q_INVOKABLE QSnapdTask *task(int) const;
  QDateTime spawnTime() const;
  QDateTime readyTime() const;
  QString error() const;
  QSnapdChangeData *data() const;
};

#endif
