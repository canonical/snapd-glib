/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_ASSERTION_H
#define SNAPD_ASSERTION_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdAssertion : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString authorityId READ authorityId)
    Q_PROPERTY(QString revision READ revision)
    Q_PROPERTY(QString signKeySHA3_384 READ signKeySHA3_384)
    Q_PROPERTY(QString body READ body)
    Q_PROPERTY(QString signature READ signature)

public:
    explicit QSnapdAssertion (void* snapd_object, QObject* parent = 0);

    QString type () const;
    QString authorityId () const;
    QString revision () const;
    QString signKeySHA3_384 () const;
    QString header (const QString& name) const;
    QString body () const;
    QString signature () const;
};

#endif
