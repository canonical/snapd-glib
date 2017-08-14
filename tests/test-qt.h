/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <QtCore/QObject>
#include <Snapd/Client>
#include <glib-object.h>

class ProgressCounter: public QObject
{
    Q_OBJECT

public:
    int progressDone = 0;

public slots:
    void progress ()
    {
        progressDone++;
    }
};

class InstallProgressCounter: public QObject
{
    Q_OBJECT

public:
    InstallProgressCounter (QSnapdInstallRequest *request) : request (request) {}
    QSnapdInstallRequest *request;
    int progressDone = 0;
    QDateTime spawnTime;
    QDateTime readyTime;

public slots:
    void progress ();
};

class GetSystemInformationHandler: public QObject
{
    Q_OBJECT

public:
    GetSystemInformationHandler (GMainLoop *loop, QSnapdGetSystemInformationRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetSystemInformationRequest *request;
    ~GetSystemInformationHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};
