/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/request.h"

struct QSnapdRequestPrivate
{
    QSnapdRequestPrivate (void *snapd_client)
    {
        client = SNAPD_CLIENT (g_object_ref (snapd_client));
        cancellable = g_cancellable_new ();
    }
  
    ~QSnapdRequestPrivate ()
    {
        g_object_unref (cancellable);      
        g_object_unref (client);
    }

    SnapdClient *client;
    GCancellable *cancellable;
    bool finished;
    QSnapdError error;
    QString errorString;
};

QSnapdRequest::QSnapdRequest (void *snapd_client, QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdRequestPrivate (snapd_client)) {}

void* QSnapdRequest::getClient ()
{
    Q_D(QSnapdRequest);  
    return d->client;
}

void* QSnapdRequest::getCancellable ()
{
    Q_D(QSnapdRequest);
    return d->cancellable;
}

void QSnapdRequest::finish (void *error)
{
    Q_D(QSnapdRequest);

    d->finished = true;
    if (error == NULL) {
        d->error = QSnapdError::NoError;
        d->errorString = "";
    }
    else {
        GError *e = (GError *) error;
        if (e->domain == SNAPD_ERROR)
            d->error = (QSnapdError) e->code;
        else
            d->error = QSnapdError::Failed;
        d->errorString = e->message;
    }
}

bool QSnapdRequest::isFinished ()
{
    Q_D(QSnapdRequest);
    return d->finished;
}

QSnapdError QSnapdRequest::error ()
{
    Q_D(QSnapdRequest);
    return d->error;
}

QString QSnapdRequest::errorString ()
{
    Q_D(QSnapdRequest);
    return d->errorString;
}

void QSnapdRequest::cancel ()
{
    Q_D(QSnapdRequest);
    g_cancellable_cancel (d->cancellable);
}
