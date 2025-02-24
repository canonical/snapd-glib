/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_MAINTENANCE_H
#define SNAPD_MAINTENANCE_H

#include <QtCore/QObject>
#include <Snapd/Enums>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdMaintenance : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QSnapdEnums::MaintenanceKind kind READ kind)
  Q_PROPERTY(QString message READ message)

public:
  explicit QSnapdMaintenance(void *snapd_object, QObject *parent = 0);

  QSnapdEnums::MaintenanceKind kind() const;
  QString message() const;
};

#endif
