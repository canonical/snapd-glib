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

namespace Snapd
{
struct AuthDataPrivate;

class Q_DECL_EXPORT AuthData : public QObject
{
    Q_OBJECT

public:
    explicit AuthData (QObject* parent, void* snapd_object);
    AuthData (const AuthData&);

    QString macaroon ();
    QStringList discharges ();

private:
    AuthDataPrivate *d_ptr;
    Q_DECLARE_PRIVATE (AuthData);
};

}

#endif
