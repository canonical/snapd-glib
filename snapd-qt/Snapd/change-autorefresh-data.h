/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_AUTOREFRESH_CHANGE_DATA_H
#define SNAPD_AUTOREFRESH_CHANGE_DATA_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <Snapd/WrappedObject>
#include "change-data.h"

class Q_DECL_EXPORT QSnapdAutorefreshChangeData : public QSnapdChangeData
{
    Q_OBJECT

    Q_PROPERTY(QStringList snapNames READ snapNames)
    Q_PROPERTY(QStringList refreshForced READ refreshForced)

public:
    explicit QSnapdAutorefreshChangeData (void* snapd_object, QObject* parent = 0);

    QStringList snapNames () const;
    QStringList refreshForced () const;
};


#endif
