/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_WRAPPED_OBJECT_H
#define SNAPD_WRAPPED_OBJECT_H

#include <QtCore/QObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdWrappedObject : public QObject {
  Q_OBJECT

public:
  explicit QSnapdWrappedObject(void *object, void (*unref_func)(void *),
                               QObject *parent = 0)
      : QObject(parent), wrapped_object(object), unref_func(unref_func) {}
  ~QSnapdWrappedObject() { unref_func(wrapped_object); }

  void *wrappedObject() { return wrapped_object; }

protected:
  void *wrapped_object;

private:
  void (*unref_func)(void *);
};

#endif
