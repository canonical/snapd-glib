/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_ICON_H
#define SNAPD_ICON_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdIcon : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString mime_type READ mime_type)
    Q_PROPERTY(QByteArray data READ data)

public:
    explicit QSnapdIcon (void* snapd_object, QObject* parent = 0);

    QString mime_type ();
    QByteArray data ();
};

#endif
