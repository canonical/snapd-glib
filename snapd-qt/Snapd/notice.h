/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_NOTICE_H
#define SNAPD_NOTICE_H

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <Snapd/Enums>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdNotice : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString id READ id CONSTANT)
  Q_PROPERTY(QString userId READ userId CONSTANT)
  Q_PROPERTY(QSnapdEnums::SnapNoticeType noticeType READ noticeType CONSTANT)
  Q_PROPERTY(QString key READ key CONSTANT)
  Q_PROPERTY(QDateTime firstOccurred READ firstOccurred CONSTANT)
  Q_PROPERTY(QDateTime lastOccurred READ lastOccurred CONSTANT)
  Q_PROPERTY(QDateTime lastRepeated READ lastRepeated CONSTANT)
  Q_PROPERTY(qint32 occurrences READ occurrences CONSTANT)
  Q_PROPERTY(qint64 repeatAfter READ repeatAfter CONSTANT)
  Q_PROPERTY(qint64 expireAfter READ expireAfter CONSTANT)
  Q_PROPERTY(QHash<QString, QString> lastData READ lastData CONSTANT)
  Q_PROPERTY(
      qint32 lastOccurredNanoseconds READ lastOccurredNanoseconds CONSTANT)

public:
  explicit QSnapdNotice(void *snapd_object, QObject *parent = 0);

  QString id() const;
  QString userId() const;
  QSnapdEnums::SnapNoticeType noticeType() const;
  QString key() const;
  QDateTime firstOccurred() const;
  QDateTime lastOccurred() const;
  QDateTime lastRepeated() const;
  qint32 occurrences() const;
  qint64 repeatAfter() const;
  qint64 expireAfter() const;
  QHash<QString, QString> lastData() const;
  qint32 lastOccurredNanoseconds() const;
};

#endif
