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

using namespace Snapd;

struct Snapd::RequestPrivate
{
    RequestPrivate (void *snapd_client)
    {
        client = SNAPD_CLIENT (g_object_ref (snapd_client));
        cancellable = g_cancellable_new ();
    }
  
    ~RequestPrivate ()
    {
        g_object_unref (cancellable);      
        g_object_unref (client);
    }

    SnapdClient *client;
    GCancellable *cancellable;
    bool finished;
    Error error;
    QString errorString;
};

Request::Request (void *snapd_client, QObject *parent) :
    QObject (parent),
    d_ptr (new RequestPrivate (snapd_client)) {}

void* Request::getClient ()
{
    Q_D(Request);  
    return d->client;
}

void* Request::getCancellable ()
{
    Q_D(Request);
    return d->cancellable;
}

void Request::finish (void *error)
{
    Q_D(Request);

    d->finished = true;
    if (error == NULL) {
        d->error = Error::NoError;
        d->errorString = "";
    }
    else {
        GError *e = (GError *) error;
        if (e->domain == SNAPD_ERROR)
            d->error = (Error) e->code;
        else
            d->error = Error::Failed;
        d->errorString = e->message;
    }
}

bool Request::isFinished ()
{
    Q_D(Request);
    return d->finished;
}

Error Request::error ()
{
    Q_D(Request);
    return d->error;
}

QString Request::errorString ()
{
    Q_D(Request);
    return d->errorString;
}

void Request::cancel ()
{
    Q_D(Request);
    g_cancellable_cancel (d->cancellable);
}
