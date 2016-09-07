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

public:
    explicit Snap (QObject* parent, void* snapd_object);
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
