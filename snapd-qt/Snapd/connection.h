/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CONNECTION_H
#define SNAPD_CONNECTION_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/PlugRef>
#include <Snapd/SlotRef>

class Q_DECL_EXPORT QSnapdConnection : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QSnapdSlotRef slot READ slot)
    Q_PROPERTY(QSnapdPlugRef plug READ plug)
    Q_PROPERTY(QString interface READ interface)
    Q_PROPERTY(bool manual READ manual)
    Q_PROPERTY(bool gadget READ gadget)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString snap READ snap)

public:
    explicit QSnapdConnection (void* snapd_object, QObject* parent = 0);

    QSnapdSlotRef *slot () const;
    QSnapdPlugRef *plug () const;
    QString interface () const;
    bool manual () const;
    bool gadget () const;
    QString name () const;
    QString snap () const;
};

#endif
