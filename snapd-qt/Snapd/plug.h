/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_PLUG_H
#define SNAPD_PLUG_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/Connection>

class Q_DECL_EXPORT QSnapdPlug : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString snap READ snap)
    Q_PROPERTY(QString interface READ interface)
    Q_PROPERTY(QString label READ label)
    Q_PROPERTY(int connectionCount READ connectionCount)

public:
    explicit QSnapdPlug (void* snapd_object, QObject* parent = 0);

    QString name () const;
    QString snap () const;
    QString interface () const;
    QString label () const;
    int connectionCount () const;
    Q_INVOKABLE QSnapdConnection *connection (int) const;
};

#endif
