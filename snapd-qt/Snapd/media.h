/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_MEDIA_H
#define SNAPD_MEDIA_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdMedia : public QSnapdWrappedObject {
  Q_OBJECT

  Q_PROPERTY(QString type READ type)
  Q_PROPERTY(QString url READ url)
  Q_PROPERTY(quint64 width READ width);
  Q_PROPERTY(quint64 height READ height);

public:
  explicit QSnapdMedia(void *snapd_object, QObject *parent = 0);

  QString type() const;
  QString url() const;
  quint64 width() const;
  quint64 height() const;
};

#endif
