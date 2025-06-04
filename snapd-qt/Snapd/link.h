/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_LINK_H
#define SNAPD_LINK_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdLink : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString type READ type)
  Q_PROPERTY(QStringList urls READ urls)

public:
  explicit QSnapdLink(void *snapd_object, QObject *parent = 0);

  QString type() const;
  QStringList urls() const;
};

#endif
