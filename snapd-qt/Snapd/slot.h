/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_SLOT_H
#define SNAPD_SLOT_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/Connection>
#include <Snapd/PlugRef>

class Q_DECL_EXPORT QSnapdSlot : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString snap READ snap)
    Q_PROPERTY(QString interface READ interface)
    Q_PROPERTY(QString label READ label)
    Q_PROPERTY(int connectionCount READ connectionCount)
    Q_PROPERTY(int connectedPlugCount READ connectedPlugCount)

public:
    explicit QSnapdSlot (void* snapd_object, QObject* parent = 0);

    QString name () const;
    QString snap () const;
    QString interface () const;
    Q_INVOKABLE QStringList attributeNames () const;
    Q_INVOKABLE bool hasAttribute (const QString &name) const;
    Q_INVOKABLE QVariant attribute (const QString &name) const;
    QString label () const;
    Q_DECL_DEPRECATED_X("Use connectedPlugCount()") int connectionCount () const;
    Q_INVOKABLE Q_DECL_DEPRECATED_X("Use connectedPlug()") QSnapdConnection *connection (int) const;
    int connectedPlugCount () const;
    Q_INVOKABLE QSnapdPlugRef *connectedPlug (int) const;
};

#endif
