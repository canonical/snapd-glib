/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_AUTH_DATA_H
#define SNAPD_AUTH_DATA_H

#include <QtCore/QObject>
#include <QStringList>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdAuthData : public QSnapdWrappedObject
{
    Q_OBJECT
    Q_PROPERTY(QString macaroon READ macaroon)
    Q_PROPERTY(QStringList discharges READ discharges)

public:
    explicit QSnapdAuthData (void *snapd_object, QObject* parent = 0);
    explicit QSnapdAuthData (const QString& macaroon, const QStringList& discharges, QObject* parent = 0);
    explicit QSnapdAuthData (QObject* parent = 0);

    QString macaroon () const;
    QStringList discharges () const;
};

#endif
