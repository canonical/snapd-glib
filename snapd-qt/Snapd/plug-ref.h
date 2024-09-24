/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_PLUG_REF_H
#define SNAPD_PLUG_REF_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdPlugRef : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString plug READ plug)
  Q_PROPERTY(QString snap READ snap)

public:
  explicit QSnapdPlugRef(void *snapd_object, QObject *parent = 0);

  QString plug() const;
  QString snap() const;
};

#endif
