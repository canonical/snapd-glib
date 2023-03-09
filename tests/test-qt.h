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

class LogoutHandler: public QObject
{
    Q_OBJECT

public:
    LogoutHandler (GMainLoop *loop, MockSnapd *snapd, qint64 id, QSnapdLogoutRequest *request) : loop (loop), snapd (snapd), id (id), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    qint64 id;
    QSnapdLogoutRequest *request;
    ~LogoutHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetChangesHandler: public QObject
{
    Q_OBJECT

public:
    GetChangesHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdGetChangesRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdGetChangesRequest *request;
    ~GetChangesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetChangeHandler: public QObject
{
    Q_OBJECT

public:
    GetChangeHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdGetChangeRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdGetChangeRequest *request;
    ~GetChangeHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class AbortChangeHandler: public QObject
{
    Q_OBJECT

public:
    AbortChangeHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdAbortChangeRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdAbortChangeRequest *request;
    ~AbortChangeHandler ()
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

class GetSnapsHandler: public QObject
{
    Q_OBJECT

public:
    GetSnapsHandler (GMainLoop *loop, QSnapdGetSnapsRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetSnapsRequest *request;
    ~GetSnapsHandler ()
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

class GetSnapHandler: public QObject
{
    Q_OBJECT

public:
    GetSnapHandler (GMainLoop *loop, QSnapdGetSnapRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetSnapRequest *request;
    ~GetSnapHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetSnapConfHandler: public QObject
{
    Q_OBJECT

public:
    GetSnapConfHandler (GMainLoop *loop, QSnapdGetSnapConfRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetSnapConfRequest *request;
    ~GetSnapConfHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class SetSnapConfHandler: public QObject
{
    Q_OBJECT

public:
    SetSnapConfHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdSetSnapConfRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdSetSnapConfRequest *request;
    ~SetSnapConfHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetAppsHandler: public QObject
{
    Q_OBJECT

public:
    GetAppsHandler (GMainLoop *loop, QSnapdGetAppsRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetAppsRequest *request;
    ~GetAppsHandler ()
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

class GetConnectionsHandler: public QObject
{
    Q_OBJECT

public:
    GetConnectionsHandler (GMainLoop *loop, QSnapdGetConnectionsRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetConnectionsRequest *request;
    ~GetConnectionsHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetInterfacesHandler: public QObject
{
    Q_OBJECT

public:
    GetInterfacesHandler (GMainLoop *loop, QSnapdGetInterfacesRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetInterfacesRequest *request;
    ~GetInterfacesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetInterfaces2Handler: public QObject
{
    Q_OBJECT

public:
    GetInterfaces2Handler (GMainLoop *loop, QSnapdGetInterfaces2Request *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetInterfaces2Request *request;
    ~GetInterfaces2Handler ()
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

class FindRefreshableHandler: public QObject
{
    Q_OBJECT

public:
    FindRefreshableHandler (GMainLoop *loop, QSnapdFindRefreshableRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdFindRefreshableRequest *request;
    ~FindRefreshableHandler ()
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

class InstallStreamHandler: public QObject
{
    Q_OBJECT

public:
    InstallStreamHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdInstallRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdInstallRequest *request;
    ~InstallStreamHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class TryHandler: public QObject
{
    Q_OBJECT

public:
    TryHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdTryRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdTryRequest *request;
    ~TryHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class RefreshHandler: public QObject
{
    Q_OBJECT

public:
    RefreshHandler (GMainLoop *loop, QSnapdRefreshRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdRefreshRequest *request;
    ~RefreshHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class RefreshAllHandler: public QObject
{
    Q_OBJECT

public:
    RefreshAllHandler (GMainLoop *loop, QSnapdRefreshAllRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdRefreshAllRequest *request;
    ~RefreshAllHandler ()
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

class GetUsersHandler: public QObject
{
    Q_OBJECT

public:
    GetUsersHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdGetUsersRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdGetUsersRequest *request;
    ~GetUsersHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetSectionsHandler: public QObject
{
    Q_OBJECT

public:
    GetSectionsHandler (GMainLoop *loop, QSnapdGetSectionsRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetSectionsRequest *request;
    ~GetSectionsHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetCategoriesHandler: public QObject
{
    Q_OBJECT

public:
    GetCategoriesHandler (GMainLoop *loop, QSnapdGetCategoriesRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetCategoriesRequest *request;
    ~GetCategoriesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class GetAliasesHandler: public QObject
{
    Q_OBJECT

public:
    GetAliasesHandler (GMainLoop *loop, QSnapdGetAliasesRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdGetAliasesRequest *request;
    ~GetAliasesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class AliasHandler: public QObject
{
    Q_OBJECT

public:
    AliasHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdAliasRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdAliasRequest *request;
    ~AliasHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class UnaliasHandler: public QObject
{
    Q_OBJECT

public:
    UnaliasHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdUnaliasRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdUnaliasRequest *request;
    ~UnaliasHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class PreferHandler: public QObject
{
    Q_OBJECT

public:
    PreferHandler (GMainLoop *loop, MockSnapd *snapd, QSnapdPreferRequest *request) : loop (loop), snapd (snapd), request (request) {}
    GMainLoop *loop;
    MockSnapd *snapd;
    QSnapdPreferRequest *request;
    ~PreferHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class RunSnapCtlHandler: public QObject
{
    Q_OBJECT

public:
    RunSnapCtlHandler (GMainLoop *loop, QSnapdRunSnapCtlRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdRunSnapCtlRequest *request;
    ~RunSnapCtlHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class DownloadHandler: public QObject
{
    Q_OBJECT

public:
    DownloadHandler (GMainLoop *loop, QSnapdDownloadRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdDownloadRequest *request;
    ~DownloadHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class CheckThemesHandler: public QObject
{
    Q_OBJECT

public:
    CheckThemesHandler (GMainLoop *loop, QSnapdCheckThemesRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdCheckThemesRequest *request;
    ~CheckThemesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};

class InstallThemesHandler: public QObject
{
    Q_OBJECT

public:
    InstallThemesHandler (GMainLoop *loop, QSnapdInstallThemesRequest *request) : loop (loop), request (request) {}
    GMainLoop *loop;
    QSnapdInstallThemesRequest *request;
    ~InstallThemesHandler ()
    {
        delete request;
    }

public slots:
    void onComplete ();
};
