/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_REQUEST_H
#define SNAPD_REQUEST_H

#include <QtCore/QObject>
#include <Snapd/Change>

class QSnapdRequestPrivate;

class Q_DECL_EXPORT QSnapdRequest : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isFinished READ isFinished)
    Q_PROPERTY(QSnapdError error READ error)
    Q_PROPERTY(QString errorString READ errorString)
    Q_PROPERTY(QSnapdChange change READ change)

public:
    enum QSnapdError
    {
        NoError,
        UnknownError,
        ConnectionFailed,
        WriteFailed,
        ReadFailed,
        BadRequest,
        BadResponse,
        AuthDataRequired,
        AuthDataInvalid,
        TwoFactorRequired,
        TwoFactorInvalid,
        PermissionDenied,
        Failed,
        TermsNotAccepted,
        PaymentNotSetup,
        PaymentDeclined,
        AlreadyInstalled,
        NotInstalled,
        NoUpdateAvailable,
        PasswordPolicyError,
        NeedsDevmode,
        NeedsClassic,
        NeedsClassicSystem,
        Cancelled,
        BadQuery,
        NetworkTimeout,
        NotFound,
        NotInStore,
        AuthCancelled,
        NotClassic
    };
    Q_ENUM(QSnapdError)

    explicit QSnapdRequest (void *snapd_client, QObject* parent = 0);
    ~QSnapdRequest ();
    bool isFinished () const;
    QSnapdError error () const;
    QString errorString () const;
    Q_INVOKABLE virtual void runSync () = 0;
    Q_INVOKABLE virtual void runAsync () = 0;
    Q_INVOKABLE void cancel ();
    Q_INVOKABLE QSnapdChange *change () const;
    void handleProgress (void*);

protected:
    void *getClient () const;
    void *getCancellable () const;
    void finish (void *error);

Q_SIGNALS:
    void progress ();
    void complete ();

private:
    QSnapdRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE (QSnapdRequest);
};

#endif
