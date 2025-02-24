/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_PRICE_H
#define SNAPD_PRICE_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdPrice : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(double amount READ amount)
  Q_PROPERTY(QString currency READ currency)

public:
  explicit QSnapdPrice(void *snapd_object, QObject *parent = 0);

  double amount() const;
  QString currency() const;
};

#endif
