/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CHANNEL_H
#define SNAPD_CHANNEL_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/Enums>

class Q_DECL_EXPORT QSnapdChannel : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString branch READ branch)
    Q_PROPERTY(QSnapdEnums::SnapConfinement confinement READ confinement)
    Q_PROPERTY(QString epoch READ epoch)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString revision READ revision)
    Q_PROPERTY(QString risk READ risk)
    Q_PROPERTY(qint64 size READ size)
    Q_PROPERTY(QString track READ track)
    Q_PROPERTY(QString version READ version)

public:
    explicit QSnapdChannel (void* snapd_object, QObject* parent = 0);

    QString branch () const;
    QSnapdEnums::SnapConfinement confinement () const;
    QString epoch () const;
    QString name () const;
    QString revision () const;
    QString risk () const;
    qint64 size () const;
    QString track () const;
    QString version () const;
};

#endif
