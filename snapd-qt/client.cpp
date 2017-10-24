/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/client.h"
#include "client-private.h"

class QSnapdClientPrivate
{
public:
    QSnapdClientPrivate ()
    {
        client = snapd_client_new ();
    }

    QSnapdClientPrivate (int fd)
    {
        g_autoptr(GSocket) socket = NULL;
        socket = g_socket_new_from_fd (fd, NULL);
        client = snapd_client_new_from_socket (socket);
    }

    ~QSnapdClientPrivate ()
    {
        g_object_unref (client);
    }

    SnapdClient *client;
};

QSnapdClient::QSnapdClient(QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdClientPrivate()) {}

QSnapdClient::QSnapdClient(int fd, QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdClientPrivate(fd)) {}

QSnapdClient::~QSnapdClient()
{
    delete d_ptr;
}

QSnapdConnectRequest::~QSnapdConnectRequest ()
{
    delete d_ptr;
}

QSnapdConnectRequest *QSnapdClient::connect ()
{
    Q_D(QSnapdClient);
    return new QSnapdConnectRequest (d->client);
}

QSnapdLoginRequest::~QSnapdLoginRequest ()
{
    delete d_ptr;
}

QSnapdLoginRequest *login (const QString& username, const QString& password)
{
    return new QSnapdLoginRequest (NULL, username, password, NULL);
}

QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp)
{
    return new QSnapdLoginRequest (NULL, username, password, otp);
}

QSnapdLoginRequest *QSnapdClient::login (const QString& username, const QString& password)
{
    Q_D(QSnapdClient);
    return new QSnapdLoginRequest (d->client, username, password, NULL);
}

QSnapdLoginRequest *QSnapdClient::login (const QString& username, const QString& password, const QString& otp)
{
    Q_D(QSnapdClient);
    return new QSnapdLoginRequest (d->client, username, password, otp);
}

void QSnapdClient::setSocketPath (const QString &socketPath)
{
    Q_D(QSnapdClient);
    snapd_client_set_socket_path (d->client, socketPath.isNull () ? NULL : socketPath.toStdString ().c_str ());
}

QString QSnapdClient::socketPath () const
{
    Q_D(const QSnapdClient);
    return snapd_client_get_socket_path (d->client);
}

void QSnapdClient::setUserAgent (const QString &userAgent)
{
    Q_D(QSnapdClient);
    snapd_client_set_user_agent (d->client, userAgent.isNull () ? NULL : userAgent.toStdString ().c_str ());
}

QString QSnapdClient::userAgent () const
{
    Q_D(const QSnapdClient);
    return snapd_client_get_user_agent (d->client);
}

void QSnapdClient::setAllowInteraction (bool allowInteraction)
{
    Q_D(QSnapdClient);
    snapd_client_set_allow_interaction (d->client, allowInteraction);
}

bool QSnapdClient::allowInteraction () const
{
    Q_D(const QSnapdClient);
    return snapd_client_get_allow_interaction (d->client);
}

void QSnapdClient::setAuthData (QSnapdAuthData *authData)
{
    Q_D(QSnapdClient);
    snapd_client_set_auth_data (d->client, SNAPD_AUTH_DATA (authData->wrappedObject ()));
}

// NOTE: This should be const but we can't do that without breaking ABI..
QSnapdAuthData *QSnapdClient::authData ()
{
    Q_D(QSnapdClient);
    return new QSnapdAuthData (snapd_client_get_auth_data (d->client));
}

QSnapdGetSystemInformationRequest::~QSnapdGetSystemInformationRequest ()
{
    delete d_ptr;
}

QSnapdGetSystemInformationRequest *QSnapdClient::getSystemInformation ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSystemInformationRequest (d->client);
}

QSnapdListRequest::~QSnapdListRequest ()
{
    delete d_ptr;
}

QSnapdListRequest *QSnapdClient::list ()
{
    Q_D(QSnapdClient);
    return new QSnapdListRequest (d->client);
}

QSnapdListOneRequest::~QSnapdListOneRequest ()
{
    delete d_ptr;
}

QSnapdListOneRequest *QSnapdClient::listOne (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdListOneRequest (name, d->client);
}

QSnapdGetIconRequest::~QSnapdGetIconRequest ()
{
    delete d_ptr;
}

QSnapdGetIconRequest *QSnapdClient::getIcon (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdGetIconRequest (name, d->client);
}

QSnapdGetAssertionsRequest::~QSnapdGetAssertionsRequest ()
{
    delete d_ptr;
}

QSnapdGetAssertionsRequest *QSnapdClient::getAssertions (const QString& type)
{
    Q_D(QSnapdClient);
    return new QSnapdGetAssertionsRequest (type, d->client);
}

QSnapdAddAssertionsRequest::~QSnapdAddAssertionsRequest ()
{
    delete d_ptr;
}

QSnapdAddAssertionsRequest *QSnapdClient::addAssertions (const QStringList& assertions)
{
    Q_D(QSnapdClient);
    return new QSnapdAddAssertionsRequest (assertions, d->client);
}

QSnapdGetInterfacesRequest::~QSnapdGetInterfacesRequest ()
{
    delete d_ptr;
}

QSnapdGetInterfacesRequest *QSnapdClient::getInterfaces ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetInterfacesRequest (d->client);
}

QSnapdConnectInterfaceRequest::~QSnapdConnectInterfaceRequest ()
{
    delete d_ptr;
}

QSnapdConnectInterfaceRequest *QSnapdClient::connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdConnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdDisconnectInterfaceRequest::~QSnapdDisconnectInterfaceRequest ()
{
    delete d_ptr;
}

QSnapdDisconnectInterfaceRequest *QSnapdClient::disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisconnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdFindRequest::~QSnapdFindRequest ()
{
    delete d_ptr;
}

QSnapdFindRequest *QSnapdClient::find (FindFlags flags)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, NULL, NULL, d->client);
}

QSnapdFindRequest *QSnapdClient::find (FindFlags flags, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, NULL, name, d->client);
}

QSnapdFindRequest *QSnapdClient::findSection (FindFlags flags, const QString &section, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, section, name, d->client);
}

QSnapdFindRefreshableRequest::~QSnapdFindRefreshableRequest ()
{
    delete d_ptr;
}

QSnapdFindRefreshableRequest *QSnapdClient::findRefreshable ()
{
    Q_D(QSnapdClient);
    return new QSnapdFindRefreshableRequest (d->client);
}

QSnapdInstallRequest::~QSnapdInstallRequest ()
{
    delete d_ptr;
}

QSnapdInstallRequest *QSnapdClient::install (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (0, name, NULL, NULL, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (0, name, channel, NULL, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (const QString& name, const QString& channel, const QString &revision)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (0, name, channel, revision, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (InstallFlags flags, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (flags, name, NULL, NULL, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (InstallFlags flags, const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (flags, name, channel, NULL, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (InstallFlags flags, const QString& name, const QString& channel, const QString &revision)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (flags, name, channel, revision, NULL, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (QIODevice *ioDevice)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (0, NULL, NULL, NULL, ioDevice, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (InstallFlags flags, QIODevice *ioDevice)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (flags, NULL, NULL, NULL, ioDevice, d->client);
}

QSnapdTryRequest *QSnapdClient::trySnap (const QString& path)
{
    Q_D(QSnapdClient);
    return new QSnapdTryRequest (path, d->client);
}

QSnapdRefreshRequest::~QSnapdRefreshRequest ()
{
    delete d_ptr;
}

QSnapdRefreshRequest *QSnapdClient::refresh (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshRequest (name, NULL, d->client);
}

QSnapdRefreshRequest *QSnapdClient::refresh (const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshRequest (name, channel, d->client);
}

QSnapdRefreshAllRequest::~QSnapdRefreshAllRequest ()
{
    delete d_ptr;
}

QSnapdRefreshAllRequest *QSnapdClient::refreshAll ()
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshAllRequest (d->client);
}

QSnapdRemoveRequest::~QSnapdRemoveRequest ()
{
    delete d_ptr;
}

QSnapdRemoveRequest *QSnapdClient::remove (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdRemoveRequest (name, d->client);
}

QSnapdEnableRequest::~QSnapdEnableRequest ()
{
    delete d_ptr;
}

QSnapdEnableRequest *QSnapdClient::enable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableRequest (name, d->client);
}

QSnapdDisableRequest::~QSnapdDisableRequest ()
{
    delete d_ptr;
}

QSnapdDisableRequest *QSnapdClient::disable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableRequest (name, d->client);
}

QSnapdCheckBuyRequest::~QSnapdCheckBuyRequest ()
{
    delete d_ptr;
}

QSnapdCheckBuyRequest *QSnapdClient::checkBuy ()
{
    Q_D(QSnapdClient);
    return new QSnapdCheckBuyRequest (d->client);
}

QSnapdBuyRequest::~QSnapdBuyRequest ()
{
    delete d_ptr;
}

QSnapdBuyRequest *QSnapdClient::buy (const QString& id, double amount, const QString& currency)
{
    Q_D(QSnapdClient);
    return new QSnapdBuyRequest (id, amount, currency, d->client);
}

QSnapdGetSectionsRequest::~QSnapdGetSectionsRequest ()
{
    delete d_ptr;
}

QSnapdGetSectionsRequest *QSnapdClient::getSections ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSectionsRequest (d->client);
}

QSnapdGetAliasesRequest::~QSnapdGetAliasesRequest ()
{
    delete d_ptr;
}

QSnapdGetAliasesRequest *QSnapdClient::getAliases ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetAliasesRequest (d->client);
}

QSnapdEnableAliasesRequest::~QSnapdEnableAliasesRequest ()
{
    delete d_ptr;
}

QSnapdEnableAliasesRequest *QSnapdClient::enableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableAliasesRequest (snap, aliases, d->client);
}

QSnapdDisableAliasesRequest::~QSnapdDisableAliasesRequest ()
{
    delete d_ptr;
}

QSnapdDisableAliasesRequest *QSnapdClient::disableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableAliasesRequest (snap, aliases, d->client);
}

QSnapdResetAliasesRequest::~QSnapdResetAliasesRequest ()
{
    delete d_ptr;
}

QSnapdResetAliasesRequest *QSnapdClient::resetAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdResetAliasesRequest (snap, aliases, d->client);
}

QSnapdRunSnapCtlRequest::~QSnapdRunSnapCtlRequest ()
{
    delete d_ptr;
}

QSnapdRunSnapCtlRequest *QSnapdClient::runSnapCtl (const QString contextId, const QStringList &args)
{
    Q_D(QSnapdClient);
    return new QSnapdRunSnapCtlRequest (contextId, args, d->client);
}

QSnapdConnectRequest::QSnapdConnectRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdConnectRequestPrivate()) {}

void QSnapdConnectRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_connect_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdConnectRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_connect_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void connect_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetSystemInformationRequest *request = static_cast<QSnapdGetSystemInformationRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdConnectRequest::runAsync ()
{
    snapd_client_connect_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), connect_ready_cb, (gpointer) this);
}

QSnapdLoginRequest::QSnapdLoginRequest (void *snapd_client, const QString& username, const QString& password, const QString& otp, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdLoginRequestPrivate(username, password, otp))
{
}

void QSnapdLoginRequest::runSync ()
{
    Q_D(QSnapdLoginRequest);
    g_autoptr(GError) error = NULL;
    if (getClient () != NULL)
        d->auth_data = snapd_client_login_sync (SNAPD_CLIENT (getClient ()), d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    else
        d->auth_data = snapd_login_sync (d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdLoginRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    auth_data = snapd_client_login_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdLoginRequest);
    d->auth_data = (SnapdAuthData*) g_steal_pointer (&auth_data);
    finish (error);
}

static void login_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdLoginRequest *request = static_cast<QSnapdLoginRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdLoginRequest::runAsync ()
{
    Q_D(QSnapdLoginRequest);
    if (getClient () != NULL)
        snapd_client_login_async (SNAPD_CLIENT (getClient ()), d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
    else
        snapd_login_async (d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
}

QSnapdAuthData *QSnapdLoginRequest::authData ()
{
    Q_D(QSnapdLoginRequest);
    return new QSnapdAuthData (d->auth_data);
}

QSnapdGetSystemInformationRequest::QSnapdGetSystemInformationRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSystemInformationRequestPrivate ()) {}

void QSnapdGetSystemInformationRequest::runSync ()
{
    Q_D(QSnapdGetSystemInformationRequest);
    g_autoptr(GError) error = NULL;
    d->info = snapd_client_get_system_information_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSystemInformationRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdSystemInformation) info = NULL;
    g_autoptr(GError) error = NULL;

    info = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdGetSystemInformationRequest);
    d->info = (SnapdSystemInformation*) g_steal_pointer (&info);
    finish (error);
}

static void system_information_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetSystemInformationRequest *request = static_cast<QSnapdGetSystemInformationRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetSystemInformationRequest::runAsync ()
{
    snapd_client_get_system_information_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), system_information_ready_cb, (gpointer) this);
}

QSnapdSystemInformation *QSnapdGetSystemInformationRequest::systemInformation ()
{
    Q_D(QSnapdGetSystemInformationRequest);
    return new QSnapdSystemInformation (d->info);
}

QSnapdListRequest::QSnapdListRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdListRequestPrivate ()) {}

void QSnapdListRequest::runSync ()
{
    Q_D(QSnapdListRequest);
    g_autoptr(GError) error = NULL;
    d->snaps = snapd_client_list_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdListRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_list_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdListRequest);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    finish (error);
}

static void list_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdListRequest *request = static_cast<QSnapdListRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdListRequest::runAsync ()
{
    snapd_client_list_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), list_ready_cb, (gpointer) this);
}

int QSnapdListRequest::snapCount () const
{
    Q_D(const QSnapdListRequest);
    return d->snaps != NULL ? d->snaps->len : 0;
}

QSnapdSnap *QSnapdListRequest::snap (int n) const
{
    Q_D(const QSnapdListRequest);
    if (d->snaps == NULL || n < 0 || (guint) n >= d->snaps->len)
        return NULL;
    return new QSnapdSnap (d->snaps->pdata[n]);
}

QSnapdListOneRequest::QSnapdListOneRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdListOneRequestPrivate (name)) {}

void QSnapdListOneRequest::runSync ()
{
    Q_D(QSnapdListOneRequest);
    g_autoptr(GError) error = NULL;
    d->snap = snapd_client_list_one_sync (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdListOneRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snap = snapd_client_list_one_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdListOneRequest);
    d->snap = (SnapdSnap*) g_steal_pointer (&snap);
    finish (error);
}

static void list_one_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdListOneRequest *request = static_cast<QSnapdListOneRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdListOneRequest::runAsync ()
{
    Q_D(QSnapdListOneRequest);
    snapd_client_list_one_async (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, (gpointer) this);
}

QSnapdSnap *QSnapdListOneRequest::snap () const
{
    Q_D(const QSnapdListOneRequest);
    return new QSnapdSnap (d->snap);
}

QSnapdGetIconRequest::QSnapdGetIconRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetIconRequestPrivate (name)) {}

void QSnapdGetIconRequest::runSync ()
{
    Q_D(QSnapdGetIconRequest);
    g_autoptr(GError) error = NULL;
    d->icon = snapd_client_get_icon_sync (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetIconRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdIcon) icon = NULL;
    g_autoptr(GError) error = NULL;

    icon = snapd_client_get_icon_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdGetIconRequest);
    d->icon = (SnapdIcon*) g_steal_pointer (&icon);
    finish (error);
}

static void get_icon_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetIconRequest *request = static_cast<QSnapdGetIconRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetIconRequest::runAsync ()
{
    Q_D(QSnapdGetIconRequest);
    snapd_client_get_icon_async (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_icon_ready_cb, (gpointer) this);
}

QSnapdIcon *QSnapdGetIconRequest::icon () const
{
    Q_D(const QSnapdGetIconRequest);
    return new QSnapdIcon (d->icon);
}

QSnapdGetAssertionsRequest::QSnapdGetAssertionsRequest (const QString& type, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAssertionsRequestPrivate (type)) {}

void QSnapdGetAssertionsRequest::runSync ()
{
    Q_D(QSnapdGetAssertionsRequest);
    g_autoptr(GError) error = NULL;
    d->assertions = snapd_client_get_assertions_sync (SNAPD_CLIENT (getClient ()), d->type.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetAssertionsRequest::handleResult (void *object, void *result)
{
    g_auto(GStrv) assertions = NULL;
    g_autoptr(GError) error = NULL;

    assertions = snapd_client_get_assertions_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdGetAssertionsRequest);
    d->assertions = (gchar **) g_steal_pointer (&assertions);
    finish (error);
}

static void get_assertions_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetAssertionsRequest *request = static_cast<QSnapdGetAssertionsRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetAssertionsRequest::runAsync ()
{
    Q_D(QSnapdGetAssertionsRequest);
    snapd_client_get_assertions_async (SNAPD_CLIENT (getClient ()), d->type.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_assertions_ready_cb, (gpointer) this);
}

QStringList QSnapdGetAssertionsRequest::assertions () const
{
    Q_D(const QSnapdGetAssertionsRequest);
    QStringList result;
    for (int i = 0; d->assertions[i] != NULL; i++)
        result.append (d->assertions[i]);
    return result;
}

QSnapdAddAssertionsRequest::QSnapdAddAssertionsRequest (const QStringList& assertions, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdAddAssertionsRequestPrivate (assertions)) {}

static gchar **
string_list_to_strv (const QStringList& list)
{
    gchar **value;
    int size, i;

    size = list.size ();
    value = (gchar **) malloc (sizeof (gchar *) * (size + 1));
    for (i = 0; i < size; i++)
        value[i] = g_strdup (list[i].toStdString ().c_str ());
    value[size] = NULL;

    return value;
}

void QSnapdAddAssertionsRequest::runSync ()
{
    Q_D(QSnapdAddAssertionsRequest);
    g_auto(GStrv) assertions = NULL;
    g_autoptr(GError) error = NULL;

    assertions = string_list_to_strv (d->assertions);
    snapd_client_add_assertions_sync (SNAPD_CLIENT (getClient ()), assertions, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdAddAssertionsRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_add_assertions_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void add_assertions_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdAddAssertionsRequest *request = static_cast<QSnapdAddAssertionsRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdAddAssertionsRequest::runAsync ()
{
    Q_D(QSnapdAddAssertionsRequest);
    g_auto(GStrv) assertions = NULL;

    assertions = string_list_to_strv (d->assertions);
    snapd_client_add_assertions_async (SNAPD_CLIENT (getClient ()), assertions, G_CANCELLABLE (getCancellable ()), add_assertions_ready_cb, (gpointer) this);
}

QSnapdGetInterfacesRequest::QSnapdGetInterfacesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetInterfacesRequestPrivate ()) {}

void QSnapdGetInterfacesRequest::runSync ()
{
    Q_D(QSnapdGetInterfacesRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_get_interfaces_sync (SNAPD_CLIENT (getClient ()), &d->plugs, &d->slots_, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetInterfacesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;

    snapd_client_get_interfaces_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &plugs, &slots_, &error);

    Q_D(QSnapdGetInterfacesRequest);
    d->plugs = (GPtrArray*) g_steal_pointer (&plugs);
    d->slots_ = (GPtrArray*) g_steal_pointer (&slots_);
    finish (error);
}

static void get_interfaces_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetInterfacesRequest *request = static_cast<QSnapdGetInterfacesRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetInterfacesRequest::runAsync ()
{
    snapd_client_get_interfaces_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), get_interfaces_ready_cb, (gpointer) this);
}

int QSnapdGetInterfacesRequest::plugCount () const
{
    Q_D(const QSnapdGetInterfacesRequest);
    return d->plugs != NULL ? d->plugs->len : 0;
}

QSnapdPlug *QSnapdGetInterfacesRequest::plug (int n) const
{
    Q_D(const QSnapdGetInterfacesRequest);
    if (d->plugs == NULL || n < 0 || (guint) n >= d->plugs->len)
        return NULL;
    return new QSnapdPlug (d->plugs->pdata[n]);
}

int QSnapdGetInterfacesRequest::slotCount () const
{
    Q_D(const QSnapdGetInterfacesRequest);
    return d->slots_ != NULL ? d->slots_->len : 0;
}

QSnapdSlot *QSnapdGetInterfacesRequest::slot (int n) const
{
    Q_D(const QSnapdGetInterfacesRequest);
    if (d->slots_ == NULL || n < 0 || (guint) n >= d->slots_->len)
        return NULL;
    return new QSnapdSlot (d->slots_->pdata[n]);
}

QSnapdConnectInterfaceRequest::QSnapdConnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdConnectInterfaceRequestPrivate (plug_snap, plug_name, slot_snap, slot_name)) {}

static void progress_cb (SnapdClient *client, SnapdChange *change, gpointer, gpointer data)
{
    QSnapdRequest *request = static_cast<QSnapdRequest*>(data);
    request->handleProgress (change);
}

void QSnapdConnectInterfaceRequest::runSync ()
{
    Q_D(QSnapdConnectInterfaceRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_connect_interface_sync (SNAPD_CLIENT (getClient ()),
                                         d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                         d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                         progress_cb, this,
                                         G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdConnectInterfaceRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_connect_interface_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void connect_interface_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdConnectInterfaceRequest *request = static_cast<QSnapdConnectInterfaceRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdConnectInterfaceRequest::runAsync ()
{
    Q_D(QSnapdConnectInterfaceRequest);
    snapd_client_connect_interface_async (SNAPD_CLIENT (getClient ()),
                                          d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                          d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                          progress_cb, this,
                                          G_CANCELLABLE (getCancellable ()), connect_interface_ready_cb, (gpointer) this);
}

QSnapdDisconnectInterfaceRequest::QSnapdDisconnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisconnectInterfaceRequestPrivate (plug_snap, plug_name, slot_snap, slot_name)) {}

void QSnapdDisconnectInterfaceRequest::runSync ()
{
    Q_D(QSnapdDisconnectInterfaceRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_disconnect_interface_sync (SNAPD_CLIENT (getClient ()),
                                            d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                            d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                            progress_cb, this,
                                            G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdDisconnectInterfaceRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_disconnect_interface_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void disconnect_interface_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdDisconnectInterfaceRequest *request = static_cast<QSnapdDisconnectInterfaceRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdDisconnectInterfaceRequest::runAsync ()
{
    Q_D(QSnapdDisconnectInterfaceRequest);
    snapd_client_disconnect_interface_async (SNAPD_CLIENT (getClient ()),
                                             d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                             d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                             progress_cb, this,
                                             G_CANCELLABLE (getCancellable ()), disconnect_interface_ready_cb, (gpointer) this);
}

static SnapdFindFlags convertFindFlags (int flags)
{
    int result = SNAPD_FIND_FLAGS_NONE;

    if ((flags & QSnapdClient::FindFlag::MatchName) != 0)
        result |= SNAPD_FIND_FLAGS_MATCH_NAME;
    if ((flags & QSnapdClient::FindFlag::SelectPrivate) != 0)
        result |= SNAPD_FIND_FLAGS_SELECT_PRIVATE;

    return (SnapdFindFlags) result;
}

QSnapdFindRequest::QSnapdFindRequest (int flags, const QString& section, const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdFindRequestPrivate (flags, section, name)) {}

void QSnapdFindRequest::runSync ()
{
    Q_D(QSnapdFindRequest);
    g_autoptr(GError) error = NULL;
    g_autofree gchar *suggested_currency = NULL;
    d->snaps = snapd_client_find_section_sync (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.isNull () ? NULL : d->section.toStdString().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), &suggested_currency, G_CANCELLABLE (getCancellable ()), &error);
    d->suggestedCurrency = suggested_currency;
    finish (error);
}

void QSnapdFindRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_find_section_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &suggested_currency, &error);

    Q_D(QSnapdFindRequest);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    d->suggestedCurrency = suggested_currency;
    finish (error);
}

static void find_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdFindRequest *request = static_cast<QSnapdFindRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdFindRequest::runAsync ()
{
    Q_D(QSnapdFindRequest);
    snapd_client_find_section_async (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.isNull () ? NULL : d->section.toStdString ().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), find_ready_cb, (gpointer) this);
}

int QSnapdFindRequest::snapCount () const
{
    Q_D(const QSnapdFindRequest);
    return d->snaps != NULL ? d->snaps->len : 0;
}

QSnapdSnap *QSnapdFindRequest::snap (int n) const
{
    Q_D(const QSnapdFindRequest);
    if (d->snaps == NULL || n < 0 || (guint) n >= d->snaps->len)
        return NULL;
    return new QSnapdSnap (d->snaps->pdata[n]);
}

const QString QSnapdFindRequest::suggestedCurrency () const
{
    Q_D(const QSnapdFindRequest);
    return d->suggestedCurrency;
}

QSnapdFindRefreshableRequest::QSnapdFindRefreshableRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdFindRefreshableRequestPrivate ()) {}

void QSnapdFindRefreshableRequest::runSync ()
{
    Q_D(QSnapdFindRefreshableRequest);
    g_autoptr(GError) error = NULL;
    d->snaps = snapd_client_find_refreshable_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdFindRefreshableRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_find_refreshable_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdFindRefreshableRequest);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    finish (error);
}

static void find_refreshable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdFindRefreshableRequest *request = static_cast<QSnapdFindRefreshableRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdFindRefreshableRequest::runAsync ()
{
    snapd_client_find_refreshable_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), find_refreshable_ready_cb, (gpointer) this);
}

int QSnapdFindRefreshableRequest::snapCount () const
{
    Q_D(const QSnapdFindRefreshableRequest);
    return d->snaps != NULL ? d->snaps->len : 0;
}

QSnapdSnap *QSnapdFindRefreshableRequest::snap (int n) const
{
    Q_D(const QSnapdFindRefreshableRequest);
    if (d->snaps == NULL || n < 0 || (guint) n >= d->snaps->len)
        return NULL;
    return new QSnapdSnap (d->snaps->pdata[n]);
}

static SnapdInstallFlags convertInstallFlags (int flags)
{
    int result = SNAPD_INSTALL_FLAGS_NONE;

    if ((flags & QSnapdClient::InstallFlag::Classic) != 0)
        result |= SNAPD_INSTALL_FLAGS_CLASSIC;
    if ((flags & QSnapdClient::InstallFlag::Dangerous) != 0)
        result |= SNAPD_INSTALL_FLAGS_DANGEROUS;
    if ((flags & QSnapdClient::InstallFlag::Devmode) != 0)
        result |= SNAPD_INSTALL_FLAGS_DEVMODE;
    if ((flags & QSnapdClient::InstallFlag::Jailmode) != 0)
        result |= SNAPD_INSTALL_FLAGS_JAILMODE;

    return (SnapdInstallFlags) result;
}

QSnapdInstallRequest::QSnapdInstallRequest (int flags, const QString& name, const QString& channel, const QString& revision, QIODevice *ioDevice, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdInstallRequestPrivate (flags, name, channel, revision, ioDevice)) {}

void QSnapdInstallRequest::runSync ()
{
    Q_D(QSnapdInstallRequest);
    g_autoptr(GError) error = NULL;
    if (d->wrapper != NULL) {
        snapd_client_install_stream_sync (SNAPD_CLIENT (getClient ()),
                                          convertInstallFlags (d->flags),
                                          G_INPUT_STREAM (d->wrapper),
                                          progress_cb, this,
                                          G_CANCELLABLE (getCancellable ()), &error);
    }
    else {
        snapd_client_install2_sync (SNAPD_CLIENT (getClient ()),
                                    convertInstallFlags (d->flags),
                                    d->name.toStdString ().c_str (),
                                    d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                    d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                    progress_cb, this,
                                    G_CANCELLABLE (getCancellable ()), &error);
    }
    finish (error);
}

void QSnapdInstallRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdInstallRequest);
    g_autoptr(GError) error = NULL;

    if (d->wrapper != NULL)
        snapd_client_install_stream_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    else
        snapd_client_install2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void install_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdInstallRequest *request = static_cast<QSnapdInstallRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdInstallRequest::runAsync ()
{
    Q_D(QSnapdInstallRequest);
    if (d->wrapper != NULL)
        snapd_client_install_stream_async (SNAPD_CLIENT (getClient ()),
                                           convertInstallFlags (d->flags),
                                           G_INPUT_STREAM (d->wrapper),
                                           progress_cb, this,
                                           G_CANCELLABLE (getCancellable ()), install_ready_cb, (gpointer) this);
    else
        snapd_client_install2_async (SNAPD_CLIENT (getClient ()),
                                     convertInstallFlags (d->flags),
                                     d->name.toStdString ().c_str (),
                                     d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                     d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                     progress_cb, this,
                                     G_CANCELLABLE (getCancellable ()), install_ready_cb, (gpointer) this);
}

QSnapdTryRequest::QSnapdTryRequest (const QString& path, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdTryRequestPrivate (path)) {}

void QSnapdTryRequest::runSync ()
{
    Q_D(QSnapdTryRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_try_sync (SNAPD_CLIENT (getClient ()),
                           d->path.toStdString ().c_str (),
                           progress_cb, this,
                           G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdTryRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_try_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void try_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdTryRequest *request = static_cast<QSnapdTryRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdTryRequest::runAsync ()
{
    Q_D(QSnapdTryRequest);
    snapd_client_try_async (SNAPD_CLIENT (getClient ()),
                            d->path.toStdString ().c_str (),
                            progress_cb, this,
                            G_CANCELLABLE (getCancellable ()), try_ready_cb, (gpointer) this);
}

QSnapdRefreshRequest::QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshRequestPrivate (name, channel)) {}

void QSnapdRefreshRequest::runSync ()
{
    Q_D(QSnapdRefreshRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_refresh_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (), d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRefreshRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_refresh_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void refresh_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdRefreshRequest *request = static_cast<QSnapdRefreshRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdRefreshRequest::runAsync ()
{
    Q_D(QSnapdRefreshRequest);
    snapd_client_refresh_async (SNAPD_CLIENT (getClient ()),
                                d->name.toStdString ().c_str (), d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                progress_cb, this,
                                G_CANCELLABLE (getCancellable ()), refresh_ready_cb, (gpointer) this);
}

QSnapdRefreshAllRequest::QSnapdRefreshAllRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshAllRequestPrivate ()) {}

void QSnapdRefreshAllRequest::runSync ()
{
    Q_D(QSnapdRefreshAllRequest);
    g_autoptr(GError) error = NULL;
    d->snap_names = snapd_client_refresh_all_sync (SNAPD_CLIENT (getClient ()),
                                                   progress_cb, this,
                                                   G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRefreshAllRequest::handleResult (void *object, void *result)
{
    g_auto(GStrv) snap_names = NULL;
    g_autoptr(GError) error = NULL;

    snap_names = snapd_client_refresh_all_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdRefreshAllRequest);
    d->snap_names = (gchar **) g_steal_pointer (&snap_names);

    finish (error);
}

static void refresh_all_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdRefreshAllRequest *request = static_cast<QSnapdRefreshAllRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdRefreshAllRequest::runAsync ()
{
    snapd_client_refresh_all_async (SNAPD_CLIENT (getClient ()),
                                    progress_cb, this,
                                    G_CANCELLABLE (getCancellable ()), refresh_all_ready_cb, (gpointer) this);
}

QStringList QSnapdRefreshAllRequest::snapNames () const
{
    Q_D(const QSnapdRefreshAllRequest);
    QStringList result;
    for (int i = 0; d->snap_names[i] != NULL; i++)
        result.append (d->snap_names[i]);
    return result;
}

QSnapdRemoveRequest::QSnapdRemoveRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRemoveRequestPrivate (name)) {}

void QSnapdRemoveRequest::runSync ()
{
    Q_D(QSnapdRemoveRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_remove_sync (SNAPD_CLIENT (getClient ()),
                              d->name.toStdString ().c_str (),
                              progress_cb, this,
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRemoveRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_remove_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void remove_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdRemoveRequest *request = static_cast<QSnapdRemoveRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdRemoveRequest::runAsync ()
{
    Q_D(QSnapdRemoveRequest);
    snapd_client_remove_async (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), remove_ready_cb, (gpointer) this);
}

QSnapdEnableRequest::QSnapdEnableRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdEnableRequestPrivate (name)) {}

void QSnapdEnableRequest::runSync ()
{
    Q_D(QSnapdEnableRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_enable_sync (SNAPD_CLIENT (getClient ()),
                              d->name.toStdString ().c_str (),
                              progress_cb, this,
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdEnableRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_enable_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void enable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdEnableRequest *request = static_cast<QSnapdEnableRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdEnableRequest::runAsync ()
{
    Q_D(QSnapdEnableRequest);
    snapd_client_enable_async (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), enable_ready_cb, (gpointer) this);
}

QSnapdDisableRequest::QSnapdDisableRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisableRequestPrivate (name)) {}

void QSnapdDisableRequest::runSync ()
{
    Q_D(QSnapdDisableRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_disable_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdDisableRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_disable_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void disable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdDisableRequest *request = static_cast<QSnapdDisableRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdDisableRequest::runAsync ()
{
    Q_D(QSnapdDisableRequest);
    snapd_client_disable_async (SNAPD_CLIENT (getClient ()),
                                d->name.toStdString ().c_str (),
                                progress_cb, this,
                                G_CANCELLABLE (getCancellable ()), disable_ready_cb, (gpointer) this);
}

QSnapdCheckBuyRequest::QSnapdCheckBuyRequest (void *snapd_client, QObject *parent):
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdCheckBuyRequestPrivate ()) {}

void QSnapdCheckBuyRequest::runSync ()
{
    Q_D(QSnapdCheckBuyRequest);
    g_autoptr(GError) error = NULL;
    d->canBuy = snapd_client_check_buy_sync (SNAPD_CLIENT (getClient ()),
                                             G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdCheckBuyRequest::handleResult (void *object, void *result)
{
    gboolean can_buy;
    g_autoptr(GError) error = NULL;

    can_buy = snapd_client_check_buy_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdCheckBuyRequest);
    d->canBuy = can_buy;
    finish (error);
}

static void check_buy_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdCheckBuyRequest *request = static_cast<QSnapdCheckBuyRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdCheckBuyRequest::runAsync ()
{
    snapd_client_check_buy_async (SNAPD_CLIENT (getClient ()),
                                  G_CANCELLABLE (getCancellable ()), check_buy_ready_cb, (gpointer) this);
}

bool QSnapdCheckBuyRequest::canBuy () const
{
    Q_D(const QSnapdCheckBuyRequest);
    return d->canBuy;
}

QSnapdBuyRequest::QSnapdBuyRequest (const QString& id, double amount, const QString& currency, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdBuyRequestPrivate (id, amount, currency)) {}

void QSnapdBuyRequest::runSync ()
{
    Q_D(QSnapdBuyRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_buy_sync (SNAPD_CLIENT (getClient ()),
                           d->id.toStdString ().c_str (), d->amount, d->currency.toStdString ().c_str (),
                           G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdBuyRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_buy_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void buy_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdBuyRequest *request = static_cast<QSnapdBuyRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdBuyRequest::runAsync ()
{
    Q_D(QSnapdBuyRequest);
    snapd_client_buy_async (SNAPD_CLIENT (getClient ()),
                            d->id.toStdString ().c_str (), d->amount, d->currency.toStdString ().c_str (),
                            G_CANCELLABLE (getCancellable ()), buy_ready_cb, (gpointer) this);
}

QSnapdGetSectionsRequest::QSnapdGetSectionsRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSectionsRequestPrivate ()) {}

void QSnapdGetSectionsRequest::runSync ()
{
    Q_D(QSnapdGetSectionsRequest);
    g_autoptr(GError) error = NULL;
    d->sections = snapd_client_get_sections_sync (SNAPD_CLIENT (getClient ()),
                                                  G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSectionsRequest::handleResult (void *object, void *result)
{
    g_auto(GStrv) sections = NULL;
    g_autoptr(GError) error = NULL;

    sections = snapd_client_get_sections_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdGetSectionsRequest);
    d->sections = (gchar**) g_steal_pointer (&sections);
    finish (error);
}

static void get_sections_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetSectionsRequest *request = static_cast<QSnapdGetSectionsRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetSectionsRequest::runAsync ()
{
    snapd_client_get_sections_async (SNAPD_CLIENT (getClient ()),
                                     G_CANCELLABLE (getCancellable ()), get_sections_ready_cb, (gpointer) this);
}

QStringList QSnapdGetSectionsRequest::sections () const
{
    Q_D(const QSnapdGetSectionsRequest);
    QStringList result;
    for (int i = 0; d->sections[i] != NULL; i++)
        result.append (d->sections[i]);
    return result;
}

QSnapdGetAliasesRequest::QSnapdGetAliasesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAliasesRequestPrivate ()) {}

void QSnapdGetAliasesRequest::runSync ()
{
    Q_D(QSnapdGetAliasesRequest);
    g_autoptr(GError) error = NULL;
    d->aliases = snapd_client_get_aliases_sync (SNAPD_CLIENT (getClient ()),
                                                G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = snapd_client_get_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    Q_D(QSnapdGetAliasesRequest);
    d->aliases = (GPtrArray*) g_steal_pointer (&aliases);
    finish (error);
}

static void get_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdGetAliasesRequest *request = static_cast<QSnapdGetAliasesRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdGetAliasesRequest::runAsync ()
{
    snapd_client_get_aliases_async (SNAPD_CLIENT (getClient ()),
                                    G_CANCELLABLE (getCancellable ()), get_aliases_ready_cb, (gpointer) this);
}

int QSnapdGetAliasesRequest::aliasCount () const
{
    Q_D(const QSnapdGetAliasesRequest);
    return d->aliases != NULL ? d->aliases->len : 0;
}

QSnapdAlias *QSnapdGetAliasesRequest::alias (int n) const
{
    Q_D(const QSnapdGetAliasesRequest);
    if (d->aliases == NULL || n < 0 || (guint) n >= d->aliases->len)
        return NULL;
    return new QSnapdAlias (d->aliases->pdata[n]);
}

QSnapdEnableAliasesRequest::QSnapdEnableAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdEnableAliasesRequestPrivate (name, aliases)) {}

void QSnapdEnableAliasesRequest::runSync ()
{
    Q_D(QSnapdEnableAliasesRequest);
    g_auto(GStrv) aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_enable_aliases_sync (SNAPD_CLIENT (getClient ()),
                                      d->snap.toStdString ().c_str (), aliases,
                                      progress_cb, this,
                                      G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdEnableAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_enable_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void enable_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdEnableAliasesRequest *request = static_cast<QSnapdEnableAliasesRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdEnableAliasesRequest::runAsync ()
{
    Q_D(QSnapdEnableAliasesRequest);
    g_auto(GStrv) aliases = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_disable_aliases_async (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, this,
                                       G_CANCELLABLE (getCancellable ()), enable_aliases_ready_cb, (gpointer) this);
}

QSnapdDisableAliasesRequest::QSnapdDisableAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisableAliasesRequestPrivate (name, aliases)) {}

void QSnapdDisableAliasesRequest::runSync ()
{
    Q_D(QSnapdDisableAliasesRequest);
    g_auto(GStrv) aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_disable_aliases_sync (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, this,
                                       G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdDisableAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_disable_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void disable_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdDisableAliasesRequest *request = static_cast<QSnapdDisableAliasesRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdDisableAliasesRequest::runAsync ()
{
    Q_D(QSnapdDisableAliasesRequest);
    g_auto(GStrv) aliases = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_disable_aliases_async (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, this,
                                       G_CANCELLABLE (getCancellable ()), disable_aliases_ready_cb, (gpointer) this);
}

QSnapdResetAliasesRequest::QSnapdResetAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdResetAliasesRequestPrivate (name, aliases)) {}

void QSnapdResetAliasesRequest::runSync ()
{
    Q_D(QSnapdResetAliasesRequest);
    g_auto(GStrv) aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_reset_aliases_sync (SNAPD_CLIENT (getClient ()),
                                     d->snap.toStdString ().c_str (), aliases,
                                     progress_cb, this,
                                     G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdResetAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_reset_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    finish (error);
}

static void reset_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdResetAliasesRequest *request = static_cast<QSnapdResetAliasesRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdResetAliasesRequest::runAsync ()
{
    Q_D(QSnapdResetAliasesRequest);
    g_auto(GStrv) aliases = NULL;

    aliases = string_list_to_strv (d->aliases);
    snapd_client_reset_aliases_async (SNAPD_CLIENT (getClient ()),
                                      d->snap.toStdString ().c_str (), aliases,
                                      progress_cb, this,
                                      G_CANCELLABLE (getCancellable ()), reset_aliases_ready_cb, (gpointer) this);
}

QSnapdRunSnapCtlRequest::QSnapdRunSnapCtlRequest (const QString& contextId, const QStringList& args, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRunSnapCtlRequestPrivate (contextId, args)) {}

void QSnapdRunSnapCtlRequest::runSync ()
{
    Q_D(QSnapdRunSnapCtlRequest);
    g_auto(GStrv) aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = string_list_to_strv (d->args);
    snapd_client_run_snapctl_sync (SNAPD_CLIENT (getClient ()),
                                   d->contextId.toStdString ().c_str (), aliases,
                                   &d->stdout_output, &d->stderr_output,
                                   G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRunSnapCtlRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    Q_D(QSnapdRunSnapCtlRequest);

    snapd_client_run_snapctl_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &d->stdout_output, &d->stderr_output, &error);

    finish (error);
}

static void run_snapctl_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    QSnapdRunSnapCtlRequest *request = static_cast<QSnapdRunSnapCtlRequest*>(data);
    request->handleResult (object, result);
}

void QSnapdRunSnapCtlRequest::runAsync ()
{
    Q_D(QSnapdRunSnapCtlRequest);
    g_auto(GStrv) aliases = NULL;

    aliases = string_list_to_strv (d->args);
    snapd_client_run_snapctl_async (SNAPD_CLIENT (getClient ()),
                                    d->contextId.toStdString ().c_str (), aliases,
                                    G_CANCELLABLE (getCancellable ()), run_snapctl_ready_cb, (gpointer) this);
}

QString QSnapdRunSnapCtlRequest::stdout () const
{
    Q_D(const QSnapdRunSnapCtlRequest);
    return d->stdout_output;
}

QString QSnapdRunSnapCtlRequest::stderr () const
{
    Q_D(const QSnapdRunSnapCtlRequest);
    return d->stderr_output;
}
