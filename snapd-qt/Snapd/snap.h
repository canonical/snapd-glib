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

namespace Snapd
{
struct SnapPrivate;

class Q_DECL_EXPORT Snap : public QObject
{
    Q_OBJECT

    // FIXME Q_PROPERTY(QList<Snapd::App> apps READ apps)
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
    Q_PROPERTY(bool isPrivate READ isPrivate)
    Q_PROPERTY(QString revision READ revision)
    // FIXME Q_PROPERTY(Snapd::SnapType snapType READ snapType)
    // FIXME Q_PROPERTY(Snapd::SnapStatus status READ status)
    Q_PROPERTY(QString summary READ summary)
    Q_PROPERTY(bool trymode READ trymode)
    Q_PROPERTY(QString version READ version) 

public:
    explicit Snap (void* snapd_object, QObject* parent = 0);
    Snap (const Snap&);

    // FIXME QList<Snapd::App> apps ();
    QString channel ();
    // FIXME Snapd::Confinement confinement ();
    QString description ();
    QString developer ();
    bool devmode ();
    qint64 downloadSize ();
    QString icon ();
    QString id ();
    // FIXME GDateTime installDate ();
    qint64 installedSize ();
    QString name ();
    bool isPrivate ();
    QString revision ();
    // FIXME Snapd::SnapType snapType ();
    // FIXME Snapd::SnapStatus status ();
    QString summary ();
    bool trymode ();
    QString version (); 

private:
    SnapPrivate *d_ptr;
    Q_DECLARE_PRIVATE (Snap);
};

}

#endif
