/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_REPLY_H
#define SNAPD_REPLY_H

#include <QtCore/QObject>

namespace Snapd
{
enum Error
{
    NoError = -1,
    ConnectionFailed = 0,
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
    PaymentDeclined
};

struct ReplyPrivate;
  
class Q_DECL_EXPORT Reply : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Error error READ error)
    Q_PROPERTY(QString errorString READ errorString)

public:
    explicit Reply (void *snapd_client, QObject* parent = 0);
    bool isFinished ();
    Error error ();
    QString errorString ();
    virtual void runSync () = 0;
    virtual void runAsync () = 0;
    void cancel ();

protected:
    void *getClient ();
    void *getCancellable ();
    void finish (void *error);

signals:
    void complete ();

private:
    ReplyPrivate *d_ptr;
    Q_DECLARE_PRIVATE (Reply);
};

}

#endif
