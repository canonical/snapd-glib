/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_ALIAS_H
#define SNAPD_ALIAS_H

#include <QtCore/QObject>
#include <Snapd/Enums>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdAlias : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString app READ app)
  Q_PROPERTY(QString appAuto READ appAuto)
  Q_PROPERTY(QString command READ command)
  Q_PROPERTY(QString appManual READ appManual)
  Q_PROPERTY(QString name READ name)
  Q_PROPERTY(QString snap READ snap)
  Q_PROPERTY(QSnapdEnums::AliasStatus status READ status)

public:
  explicit QSnapdAlias(void *snapd_object, QObject *parent = 0);

  Q_DECL_DEPRECATED QString app() const;
  QString appAuto() const;
  QString command() const;
  QString appManual() const;
  QString name() const;
  QString snap() const;
  QSnapdEnums::AliasStatus status() const;
};

#endif
