/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CATEGORY_DETAILS_H
#define SNAPD_CATEGORY_DETAILS_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdCategoryDetails : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString name READ name)

public:
  explicit QSnapdCategoryDetails(void *snapd_object, QObject *parent = 0);

  QString name() const;
};

#endif
