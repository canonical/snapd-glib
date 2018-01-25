/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "mock-snapd.h"

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

class LoginHandler: public QObject
{
    Q_OBJECT

public:
    LoginHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdLoginRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdLoginRequest *request;
    ~LoginHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class ListHandler: public QObject
{
    Q_OBJECT

public:
    ListHandler (GMainLoop *loop, QSnapdListRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdListRequest *request;
    ~ListHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class ListOneHandler: public QObject
{
    Q_OBJECT

public:
    ListOneHandler (GMainLoop *loop, QSnapdListOneRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdListOneRequest *request;
    ~ListOneHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetIconHandler: public QObject
{
    Q_OBJECT

public:
    GetIconHandler (GMainLoop *loop, QSnapdGetIconRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetIconRequest *request;
    ~GetIconHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class ConnectInterfaceHandler: public QObject
{
    Q_OBJECT

public:
    ConnectInterfaceHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdConnectInterfaceRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdConnectInterfaceRequest *request;
    ~ConnectInterfaceHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class DisconnectInterfaceHandler: public QObject
{
    Q_OBJECT

public:
    DisconnectInterfaceHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdDisconnectInterfaceRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdDisconnectInterfaceRequest *request;
    ~DisconnectInterfaceHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class FindHandler: public QObject
{
    Q_OBJECT

public:
    FindHandler (GMainLoop *loop, QSnapdFindRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdFindRequest *request;
    ~FindHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class InstallHandler: public QObject
{
    Q_OBJECT

public:
    InstallHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdInstallRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdInstallRequest *request;
    ~InstallHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class InstallErrorHandler: public QObject
{
    Q_OBJECT

public:
    InstallErrorHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdInstallRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdInstallRequest *request;
    ~InstallErrorHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class InstallCancelHandler: public QObject
{
    Q_OBJECT

public:
    InstallCancelHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdInstallRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdInstallRequest *request;
    ~InstallCancelHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class InstallMultipleHandler: public QObject
{
    Q_OBJECT

public:
    InstallMultipleHandler (GMainLoop *loop, MockSnapd *snapd, QList<QSnapdInstallRequest*> requests) : loop (loop), snapd (snapd), requests (requests) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    int counter = 0;
    QList<QSnapdInstallRequest*> requests;
    ~InstallMultipleHandler ()
    {
        QSnapdInstallRequest *request;
        foreach (request, requests)
            delete request;
    }

public slots:
    void onComplete ();
};

class InstallMultipleCancelFirstHandler: public QObject
{
    Q_OBJECT

public:
    InstallMultipleCancelFirstHandler (GMainLoop *loop, MockSnapd *snapd, QList<QSnapdInstallRequest*> requests) : loop (loop), snapd (snapd), requests (requests) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    int counter = 0;
    QList<QSnapdInstallRequest*> requests;
    ~InstallMultipleCancelFirstHandler ()
    {
        QSnapdInstallRequest *request;
        foreach (request, requests)
            delete request;
    }

private:
    void checkComplete ();

public slots:
    void onCompleteSnap1 ();
    void onCompleteSnap2 ();
    void onCompleteSnap3 ();
};

class InstallMultipleCancelLastHandler: public QObject
{
    Q_OBJECT

public:
    InstallMultipleCancelLastHandler (GMainLoop *loop, MockSnapd *snapd, QList<QSnapdInstallRequest*> requests) : loop (loop), snapd (snapd), requests (requests) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    int counter = 0;
    QList<QSnapdInstallRequest*> requests;
    ~InstallMultipleCancelLastHandler ()
    {
        QSnapdInstallRequest *request;
        foreach (request, requests)
            delete request;
    }

private:
    void checkComplete ();

public slots:
    void onCompleteSnap1 ();
    void onCompleteSnap2 ();
    void onCompleteSnap3 ();
};

class RemoveHandler: public QObject
{
    Q_OBJECT

public:
    RemoveHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdRemoveRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdRemoveRequest *request;
    ~RemoveHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class RemoveErrorHandler: public QObject
{
    Q_OBJECT

public:
    RemoveErrorHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdRemoveRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdRemoveRequest *request;
    ~RemoveErrorHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class RemoveCancelHandler: public QObject
{
    Q_OBJECT

public:
    RemoveCancelHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdRemoveRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdRemoveRequest *request;
    ~RemoveCancelHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class EnableHandler: public QObject
{
    Q_OBJECT

public:
    EnableHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdEnableRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdEnableRequest *request;
    ~EnableHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class DisableHandler: public QObject
{
    Q_OBJECT

public:
    DisableHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdDisableRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdDisableRequest *request;
    ~DisableHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class SwitchChannelHandler: public QObject
{
    Q_OBJECT

public:
    SwitchChannelHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdSwitchChannelRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdSwitchChannelRequest *request;
    ~SwitchChannelHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class CheckBuyHandler: public QObject
{
    Q_OBJECT

public:
    CheckBuyHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdCheckBuyRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdCheckBuyRequest *request;
    ~CheckBuyHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class BuyHandler: public QObject
{
    Q_OBJECT

public:
    BuyHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdBuyRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdBuyRequest *request;
    ~BuyHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};
