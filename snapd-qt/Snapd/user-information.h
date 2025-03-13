/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_USER_INFORMATION_H
#define SNAPD_USER_INFORMATION_H

#include <QtCore/QObject>
#include <Snapd/AuthData>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdUserInformation : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(int id READ id)
  Q_PROPERTY(QString username READ username)
  Q_PROPERTY(QString email READ email)
  Q_PROPERTY(QStringList sshKeys READ sshKeys)
  Q_PROPERTY(QSnapdAuthData *authData READ authData)

public:
  explicit QSnapdUserInformation(void *snapd_object, QObject *parent = 0);

  int id() const;
  QString username() const;
  QString email() const;
  QStringList sshKeys() const;
  QSnapdAuthData *authData() const;
};

#endif
