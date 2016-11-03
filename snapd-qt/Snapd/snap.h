/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_SNAP_H
#define SNAPD_SNAP_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>
#include <Snapd/App>
#include <Snapd/Price>

class Q_DECL_EXPORT QSnapdSnap : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(int appCount READ appCount)
    Q_PROPERTY(QString channel READ channel)
    // FIXME Q_PROPERTY(Snapd::Confinement confinement READ confinement)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QString developer READ developer)
    Q_PROPERTY(bool devmode READ devmode)
    Q_PROPERTY(qint64 downaloadSize READ downloadSize)
    Q_PROPERTY(QString icon READ icon)
    Q_PROPERTY(QString id READ id)
    // FIXME Q_PROPERTY(GDateTime installDate READ installDate)
    Q_PROPERTY(qint64 installedSize READ installedSize)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(int priceCount READ priceCount)
    Q_PROPERTY(bool isPrivate READ isPrivate)
    Q_PROPERTY(QString revision READ revision)
    // FIXME Q_PROPERTY(Snapd::SnapType snapType READ snapType)
    // FIXME Q_PROPERTY(Snapd::SnapStatus status READ status)
    Q_PROPERTY(QString summary READ summary)
    Q_PROPERTY(bool trymode READ trymode)
    Q_PROPERTY(QString version READ version) 

public:
    explicit QSnapdSnap (void* snapd_object, QObject* parent = 0);

    int appCount () const;
    Q_INVOKABLE QSnapdApp *app (int) const;
    QString channel () const;
    // FIXME Snapd::Confinement confinement () const;
    QString description () const;
    QString developer () const;
    bool devmode () const;
    qint64 downloadSize () const;
    QString icon () const;
    QString id () const;
    // FIXME GDateTime installDate () const;
    qint64 installedSize () const;
    QString name () const;
    int priceCount () const;
    Q_INVOKABLE QSnapdPrice *price (int) const;
    bool isPrivate () const;
    QString revision () const;
    // FIXME Snapd::SnapType snapType () const;
    // FIXME Snapd::SnapStatus status () const;
    QString summary () const;
    bool trymode () const;
    QString version () const; 
};

#endif
