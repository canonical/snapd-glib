/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_SYSTEM_INFORMATION_H
#define SNAPD_SYSTEM_INFORMATION_H

#include <QHash>
#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <Snapd/Enums>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdSystemInformation : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString binariesDirectory READ binariesDirectory)
  Q_PROPERTY(QString buildId READ buildId)
  Q_PROPERTY(QSnapdEnums::SystemConfinement confinement READ confinement)
  Q_PROPERTY(QString kernelVersion READ kernelVersion)
  Q_PROPERTY(bool managed READ managed)
  Q_PROPERTY(QString mountDirectory READ mountDirectory)
  Q_PROPERTY(bool onClassic READ onClassic)
  Q_PROPERTY(QString osId READ osId)
  Q_PROPERTY(QString osVersion READ osVersion)
  Q_PROPERTY(QDateTime refreshHold READ refreshHold)
  Q_PROPERTY(QDateTime refreshLast READ refreshLast)
  Q_PROPERTY(QDateTime refreshNext READ refreshNext)
  Q_PROPERTY(QString refreshSchedule READ refreshSchedule)
  Q_PROPERTY(QString refreshTimer READ refreshTimer)
  Q_PROPERTY(QHash<QString, QStringList> sandboxFeatures READ sandboxFeatures)
  Q_PROPERTY(QString series READ series)
  Q_PROPERTY(QString store READ store)
  Q_PROPERTY(QString version READ version)

public:
  explicit QSnapdSystemInformation(void *snapd_object, QObject *parent = 0);

  QString binariesDirectory() const;
  QString buildId() const;
  QSnapdEnums::SystemConfinement confinement() const;
  QString kernelVersion() const;
  bool managed() const;
  QString mountDirectory() const;
  bool onClassic() const;
  QString osId() const;
  QString osVersion() const;
  QDateTime refreshHold() const;
  QDateTime refreshLast() const;
  QDateTime refreshNext() const;
  QString refreshSchedule() const;
  QString refreshTimer() const;
  QHash<QString, QStringList> sandboxFeatures() const;
  QString series() const;
  QString store() const;
  QString version() const;
};

#endif
