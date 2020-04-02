/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_INTERFACE_H
#define SNAPD_INTERFACE_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/Plug>
#include <Snapd/Slot>

class Q_DECL_EXPORT QSnapdInterface : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString summary READ summary)
    Q_PROPERTY(QString docUrl READ docUrl)
    Q_PROPERTY(int plugCount READ plugCount)
    Q_PROPERTY(int slotCount READ slotCount)

public:
    explicit QSnapdInterface (void* snapd_object, QObject* parent = 0);

    QString name () const;
    QString summary () const;
    QString docUrl () const;
    int plugCount () const;
    Q_INVOKABLE QSnapdPlug *plug (int) const;
    int slotCount () const;
    Q_INVOKABLE QSnapdSlot *slot (int) const;
    Q_INVOKABLE QString makeLabel () const;
};

#endif
