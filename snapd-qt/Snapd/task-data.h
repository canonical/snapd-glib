/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_TASK_DATA_H
#define SNAPD_TASK_DATA_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdTaskData : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QStringList affectedSnaps READ affectedSnaps)

public:
  explicit QSnapdTaskData(void *snapd_object, QObject *parent = 0);

  Q_INVOKABLE QStringList affectedSnaps() const;
};

#endif
