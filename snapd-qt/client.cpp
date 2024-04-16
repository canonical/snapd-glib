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
#include "variant.h"
#include <QTimeZone>

G_DEFINE_TYPE (CallbackData, callback_data, G_TYPE_OBJECT)

static void
callback_data_init (CallbackData *self) {}

static void
callback_data_class_init (CallbackDataClass *klass) {}

CallbackData *callback_data_new (gpointer request)
{
    CallbackData *data = (CallbackData *) g_object_new (callback_data_get_type (), NULL);
    data->request = request;
    return data;
}

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
    d_ptr (new QSnapdClientPrivate ()) {}

QSnapdClient::QSnapdClient(int fd, QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdClientPrivate (fd)) {}

QSnapdClient::~QSnapdClient()
{}

QSnapdConnectRequest::~QSnapdConnectRequest ()
{}

QSnapdConnectRequest *QSnapdClient::connect ()
{
    Q_D(QSnapdClient);
    return new QSnapdConnectRequest (d->client);
}

QSnapdLoginRequest::~QSnapdLoginRequest ()
{}

QSnapdLoginRequest *login (const QString& email, const QString& password)
{
    return new QSnapdLoginRequest (NULL, email, password, NULL);
}

QSnapdLoginRequest *login (const QString& email, const QString& password, const QString& otp)
{
    return new QSnapdLoginRequest (NULL, email, password, otp);
}

QSnapdLoginRequest *QSnapdClient::login (const QString& email, const QString& password)
{
    Q_D(QSnapdClient);
    return new QSnapdLoginRequest (d->client, email, password, NULL);
}

QSnapdLoginRequest *QSnapdClient::login (const QString& email, const QString& password, const QString& otp)
{
    Q_D(QSnapdClient);
    return new QSnapdLoginRequest (d->client, email, password, otp);
}

QSnapdLogoutRequest::~QSnapdLogoutRequest ()
{}

QSnapdLogoutRequest *QSnapdClient::logout (qint64 id)
{
    Q_D(QSnapdClient);
    return new QSnapdLogoutRequest (d->client, id);
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

QSnapdMaintenance *QSnapdClient::maintenance () const
{
    Q_D(const QSnapdClient);
    SnapdMaintenance *m = snapd_client_get_maintenance (d->client);
    if (m == NULL)
        return NULL;
    else
        return new QSnapdMaintenance (m);
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

QSnapdGetChangesRequest::~QSnapdGetChangesRequest ()
{}

QSnapdGetChangesRequest *QSnapdClient::getChanges ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetChangesRequest (FilterAll, NULL, d->client);
}

QSnapdGetChangesRequest *QSnapdClient::getChanges (ChangeFilter filter)
{
    Q_D(QSnapdClient);
    return new QSnapdGetChangesRequest (filter, NULL, d->client);
}

QSnapdGetChangesRequest *QSnapdClient::getChanges (const QString &snapName)
{
    Q_D(QSnapdClient);
    return new QSnapdGetChangesRequest (FilterAll, snapName, d->client);
}

QSnapdGetChangesRequest *QSnapdClient::getChanges (ChangeFilter filter, const QString &snapName)
{
    Q_D(QSnapdClient);
    return new QSnapdGetChangesRequest (filter, snapName, d->client);
}

QSnapdGetChangeRequest::~QSnapdGetChangeRequest ()
{}

QSnapdGetChangeRequest *QSnapdClient::getChange (const QString& id)
{
    Q_D(QSnapdClient);
    return new QSnapdGetChangeRequest (id, d->client);
}

QSnapdAbortChangeRequest::~QSnapdAbortChangeRequest ()
{}

QSnapdAbortChangeRequest *QSnapdClient::abortChange (const QString& id)
{
    Q_D(QSnapdClient);
    return new QSnapdAbortChangeRequest (id, d->client);
}

QSnapdGetSystemInformationRequest::~QSnapdGetSystemInformationRequest ()
{}

QSnapdGetSystemInformationRequest *QSnapdClient::getSystemInformation ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSystemInformationRequest (d->client);
}

QSnapdListRequest::~QSnapdListRequest ()
{}

QSnapdListRequest *QSnapdClient::list ()
{
    Q_D(QSnapdClient);
    return new QSnapdListRequest (d->client);
}

QSnapdGetSnapsRequest::~QSnapdGetSnapsRequest ()
{}

QSnapdGetSnapsRequest *QSnapdClient::getSnaps (GetSnapsFlags flags, const QStringList &snaps)
{
    Q_D(QSnapdClient);
    return new QSnapdGetSnapsRequest (flags, snaps, d->client);
}

QSnapdGetSnapsRequest *QSnapdClient::getSnaps (GetSnapsFlags flags, const QString &snap)
{
    return getSnaps (flags, QStringList(snap));
}

QSnapdGetSnapsRequest *QSnapdClient::getSnaps (const QStringList &snaps)
{
    return getSnaps (GetSnapsFlags(), snaps);
}

QSnapdGetSnapsRequest *QSnapdClient::getSnaps (const QString &snap)
{
    return getSnaps (GetSnapsFlags(), QStringList(snap));
}

QSnapdGetSnapsRequest *QSnapdClient::getSnaps ()
{
    return getSnaps (GetSnapsFlags(), QStringList ());
}

QSnapdListOneRequest::~QSnapdListOneRequest ()
{}

QSnapdListOneRequest *QSnapdClient::listOne (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdListOneRequest (name, d->client);
}

QSnapdGetSnapRequest::~QSnapdGetSnapRequest ()
{}

QSnapdGetSnapRequest *QSnapdClient::getSnap (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdGetSnapRequest (name, d->client);
}

QSnapdGetSnapConfRequest::~QSnapdGetSnapConfRequest ()
{}

QSnapdGetSnapConfRequest *QSnapdClient::getSnapConf (const QString &name, const QStringList &keys)
{
    Q_D(QSnapdClient);
    return new QSnapdGetSnapConfRequest (name, keys, d->client);
}

QSnapdGetSnapConfRequest *QSnapdClient::getSnapConf (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdGetSnapConfRequest (name, QStringList (), d->client);
}

QSnapdSetSnapConfRequest::~QSnapdSetSnapConfRequest ()
{}

QSnapdSetSnapConfRequest *QSnapdClient::setSnapConf (const QString &name, const QHash<QString, QVariant> &configuration)
{
    Q_D(QSnapdClient);
    return new QSnapdSetSnapConfRequest (name, configuration, d->client);
}

QSnapdGetAppsRequest::~QSnapdGetAppsRequest ()
{}

QSnapdGetAppsRequest *QSnapdClient::getApps (GetAppsFlags flags, const QStringList &snaps)
{
    Q_D(QSnapdClient);
    return new QSnapdGetAppsRequest (flags, snaps, d->client);
}

QSnapdGetAppsRequest *QSnapdClient::getApps (GetAppsFlags flags, const QString &snap)
{
    return getApps (flags, QStringList (snap));
}

QSnapdGetAppsRequest *QSnapdClient::getApps (GetAppsFlags flags)
{
    return getApps (flags, QStringList ());
}

QSnapdGetAppsRequest *QSnapdClient::getApps (const QStringList &snaps)
{
    return getApps (GetAppsFlags(), snaps);
}

QSnapdGetAppsRequest *QSnapdClient::getApps (const QString &snap)
{
    return getApps (GetAppsFlags(), QStringList (snap));
}

QSnapdGetAppsRequest *QSnapdClient::getApps ()
{
    return getApps (GetAppsFlags(), QStringList ());
}

QSnapdGetIconRequest::~QSnapdGetIconRequest ()
{}

QSnapdGetIconRequest *QSnapdClient::getIcon (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdGetIconRequest (name, d->client);
}

QSnapdGetAssertionsRequest::~QSnapdGetAssertionsRequest ()
{}

QSnapdGetAssertionsRequest *QSnapdClient::getAssertions (const QString& type)
{
    Q_D(QSnapdClient);
    return new QSnapdGetAssertionsRequest (type, d->client);
}

QSnapdAddAssertionsRequest::~QSnapdAddAssertionsRequest ()
{}

QSnapdAddAssertionsRequest *QSnapdClient::addAssertions (const QStringList& assertions)
{
    Q_D(QSnapdClient);
    return new QSnapdAddAssertionsRequest (assertions, d->client);
}

QSnapdGetConnectionsRequest::~QSnapdGetConnectionsRequest ()
{}

QSnapdGetConnectionsRequest *QSnapdClient::getConnections ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetConnectionsRequest (0, QString (), QString (), d->client);
}

QSnapdGetConnectionsRequest *QSnapdClient::getConnections (GetConnectionsFlags flags)
{
    Q_D(QSnapdClient);
    return new QSnapdGetConnectionsRequest (flags, QString (), QString (), d->client);
}

QSnapdGetConnectionsRequest *QSnapdClient::getConnections (const QString &snap, const QString &interface)
{
    Q_D(QSnapdClient);
    return new QSnapdGetConnectionsRequest (0, snap, interface, d->client);
}

QSnapdGetConnectionsRequest *QSnapdClient::getConnections (GetConnectionsFlags flags, const QString &snap, const QString &interface)
{
    Q_D(QSnapdClient);
    return new QSnapdGetConnectionsRequest (flags, snap, interface, d->client);
}

QSnapdGetInterfacesRequest::~QSnapdGetInterfacesRequest ()
{}

QSnapdGetInterfacesRequest *QSnapdClient::getInterfaces ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetInterfacesRequest (d->client);
}

QSnapdGetInterfaces2Request::~QSnapdGetInterfaces2Request ()
{}

QSnapdGetInterfaces2Request *QSnapdClient::getInterfaces2 ()
{
    return getInterfaces2 (InterfaceFlags(), QStringList ());
}

QSnapdGetInterfaces2Request *QSnapdClient::getInterfaces2 (InterfaceFlags flags)
{
    return getInterfaces2 (flags, QStringList ());
}

QSnapdGetInterfaces2Request *QSnapdClient::getInterfaces2 (const QStringList &names)
{
    return getInterfaces2 (InterfaceFlags(), names);
}

QSnapdGetInterfaces2Request *QSnapdClient::getInterfaces2 (InterfaceFlags flags, const QStringList &names)
{
    Q_D(QSnapdClient);
    return new QSnapdGetInterfaces2Request (flags, names, d->client);
}

QSnapdConnectInterfaceRequest::~QSnapdConnectInterfaceRequest ()
{}

QSnapdConnectInterfaceRequest *QSnapdClient::connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdConnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdDisconnectInterfaceRequest::~QSnapdDisconnectInterfaceRequest ()
{}

QSnapdDisconnectInterfaceRequest *QSnapdClient::disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisconnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdFindRequest::~QSnapdFindRequest ()
{}

QSnapdFindRequest *QSnapdClient::find (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (QSnapdClient::FindFlag::None, NULL, NULL, name, d->client);
}

QSnapdFindRequest *QSnapdClient::find (FindFlags flags)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, NULL, NULL, NULL, d->client);
}

QSnapdFindRequest *QSnapdClient::find (FindFlags flags, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, NULL, NULL, name, d->client);
}

QSnapdFindRequest *QSnapdClient::findSection (const QString &section, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (QSnapdClient::FindFlag::None, section, NULL, name, d->client);
}

QSnapdFindRequest *QSnapdClient::findSection (FindFlags flags, const QString &section, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, section, NULL, name, d->client);
}

QSnapdFindRequest *QSnapdClient::findCategory (const QString &category, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (QSnapdClient::FindFlag::None, NULL, category, name, d->client);
}

QSnapdFindRequest *QSnapdClient::findCategory (FindFlags flags, const QString &category, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, NULL, category, name, d->client);
}

QSnapdFindRefreshableRequest::~QSnapdFindRefreshableRequest ()
{}

QSnapdFindRefreshableRequest *QSnapdClient::findRefreshable ()
{
    Q_D(QSnapdClient);
    return new QSnapdFindRefreshableRequest (d->client);
}

QSnapdInstallRequest::~QSnapdInstallRequest ()
{}

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

QSnapdTryRequest::~QSnapdTryRequest ()
{}

QSnapdTryRequest *QSnapdClient::trySnap (const QString& path)
{
    Q_D(QSnapdClient);
    return new QSnapdTryRequest (path, d->client);
}

QSnapdRefreshRequest::~QSnapdRefreshRequest ()
{}

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
{}

QSnapdRefreshAllRequest *QSnapdClient::refreshAll ()
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshAllRequest (d->client);
}

QSnapdRemoveRequest::~QSnapdRemoveRequest ()
{}

QSnapdRemoveRequest *QSnapdClient::remove (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdRemoveRequest (0, name, d->client);
}

QSnapdRemoveRequest *QSnapdClient::remove (RemoveFlags flags, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdRemoveRequest (flags, name, d->client);
}

QSnapdEnableRequest::~QSnapdEnableRequest ()
{}

QSnapdEnableRequest *QSnapdClient::enable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableRequest (name, d->client);
}

QSnapdDisableRequest::~QSnapdDisableRequest ()
{}

QSnapdDisableRequest *QSnapdClient::disable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableRequest (name, d->client);
}

QSnapdSwitchChannelRequest::~QSnapdSwitchChannelRequest ()
{}

QSnapdSwitchChannelRequest *QSnapdClient::switchChannel (const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdSwitchChannelRequest (name, channel, d->client);
}

QSnapdCheckBuyRequest::~QSnapdCheckBuyRequest ()
{}

QSnapdCheckBuyRequest *QSnapdClient::checkBuy ()
{
    Q_D(QSnapdClient);
    return new QSnapdCheckBuyRequest (d->client);
}

QSnapdBuyRequest::~QSnapdBuyRequest ()
{}

QSnapdBuyRequest *QSnapdClient::buy (const QString& id, double amount, const QString& currency)
{
    Q_D(QSnapdClient);
    return new QSnapdBuyRequest (id, amount, currency, d->client);
}

QSnapdCreateUserRequest::~QSnapdCreateUserRequest ()
{}

QSnapdCreateUserRequest *QSnapdClient::createUser (const QString& email)
{
    Q_D(QSnapdClient);
    return new QSnapdCreateUserRequest (email, 0, d->client);
}

QSnapdCreateUserRequest *QSnapdClient::createUser (const QString& email, CreateUserFlags flags)
{
    Q_D(QSnapdClient);
    return new QSnapdCreateUserRequest (email, flags, d->client);
}

QSnapdCreateUsersRequest::~QSnapdCreateUsersRequest ()
{}

QSnapdCreateUsersRequest *QSnapdClient::createUsers ()
{
    Q_D(QSnapdClient);
    return new QSnapdCreateUsersRequest (d->client);
}

QSnapdGetUsersRequest::~QSnapdGetUsersRequest ()
{}

QSnapdGetUsersRequest *QSnapdClient::getUsers ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetUsersRequest (d->client);
}

QSnapdGetSectionsRequest::~QSnapdGetSectionsRequest ()
{}

QSnapdGetSectionsRequest *QSnapdClient::getSections ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSectionsRequest (d->client);
}

QSnapdGetCategoriesRequest::~QSnapdGetCategoriesRequest ()
{}

QSnapdGetCategoriesRequest *QSnapdClient::getCategories ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetCategoriesRequest (d->client);
}

QSnapdGetAliasesRequest::~QSnapdGetAliasesRequest ()
{}

QSnapdGetAliasesRequest *QSnapdClient::getAliases ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetAliasesRequest (d->client);
}

QSnapdAliasRequest::~QSnapdAliasRequest ()
{}

QSnapdAliasRequest *QSnapdClient::alias (const QString &snap, const QString &app, const QString &alias)
{
    Q_D(QSnapdClient);
    return new QSnapdAliasRequest (snap, app, alias, d->client);
}

QSnapdUnaliasRequest::~QSnapdUnaliasRequest ()
{}

QSnapdUnaliasRequest *QSnapdClient::unalias (const QString &snap, const QString &alias)
{
    Q_D(QSnapdClient);
    return new QSnapdUnaliasRequest (snap, alias, d->client);
}

QSnapdUnaliasRequest *QSnapdClient::unalias (const QString &alias)
{
    Q_D(QSnapdClient);
    return new QSnapdUnaliasRequest (NULL, alias, d->client);
}

QSnapdPreferRequest::~QSnapdPreferRequest ()
{}

QSnapdPreferRequest *QSnapdClient::prefer (const QString &snap)
{
    Q_D(QSnapdClient);
    return new QSnapdPreferRequest (snap, d->client);
}

QSnapdEnableAliasesRequest::~QSnapdEnableAliasesRequest ()
{}

QSnapdEnableAliasesRequest *QSnapdClient::enableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableAliasesRequest (snap, aliases, d->client);
}

QSnapdDisableAliasesRequest::~QSnapdDisableAliasesRequest ()
{}

QSnapdDisableAliasesRequest *QSnapdClient::disableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableAliasesRequest (snap, aliases, d->client);
}

QSnapdResetAliasesRequest::~QSnapdResetAliasesRequest ()
{}

QSnapdResetAliasesRequest *QSnapdClient::resetAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdResetAliasesRequest (snap, aliases, d->client);
}

QSnapdRunSnapCtlRequest::~QSnapdRunSnapCtlRequest ()
{}

QSnapdRunSnapCtlRequest *QSnapdClient::runSnapCtl (const QString contextId, const QStringList &args)
{
    Q_D(QSnapdClient);
    return new QSnapdRunSnapCtlRequest (contextId, args, d->client);
}

QSnapdDownloadRequest::~QSnapdDownloadRequest ()
{}

QSnapdDownloadRequest *QSnapdClient::download (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdDownloadRequest (name, NULL, NULL, d->client);
}

QSnapdDownloadRequest *QSnapdClient::download (const QString& name, const QString& channel, const QString& revision)
{
    Q_D(QSnapdClient);
    return new QSnapdDownloadRequest (name, channel, revision, d->client);
}

QSnapdCheckThemesRequest::~QSnapdCheckThemesRequest ()
{}

QSnapdCheckThemesRequest *QSnapdClient::checkThemes (const QStringList& gtkThemeNames, const QStringList& iconThemeNames, const QStringList& soundThemeNames)
{
    Q_D(QSnapdClient);
    return new QSnapdCheckThemesRequest (gtkThemeNames, iconThemeNames, soundThemeNames, d->client);
}

QSnapdInstallThemesRequest::~QSnapdInstallThemesRequest ()
{}

QSnapdInstallThemesRequest *QSnapdClient::installThemes (const QStringList& gtkThemeNames, const QStringList& iconThemeNames, const QStringList& soundThemeNames)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallThemesRequest (gtkThemeNames, iconThemeNames, soundThemeNames, d->client);
}

QSnapdConnectRequest::QSnapdConnectRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdConnectRequestPrivate (this)) {}

void QSnapdConnectRequest::runSync ()
{
    finish (NULL);
}

void QSnapdConnectRequest::handleResult (void *object, void *result)
{
    finish (NULL);
}

void QSnapdConnectRequest::runAsync ()
{
    handleResult (NULL, NULL);
}

QSnapdLoginRequest::QSnapdLoginRequest (void *snapd_client, const QString& email, const QString& password, const QString& otp, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdLoginRequestPrivate (this, email, password, otp))
{
}

void QSnapdLoginRequest::runSync ()
{
    Q_D(QSnapdLoginRequest);

    g_autoptr(GError) error = NULL;
    if (getClient () != NULL)
        d->user_information = snapd_client_login2_sync (SNAPD_CLIENT (getClient ()), d->email.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    else
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        d->auth_data = snapd_login_sync (d->email.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdLoginRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdLoginRequest);

    g_autoptr(GError) error = NULL;
    if (getClient () != NULL)
        d->user_information = snapd_client_login2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    else
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        d->auth_data = snapd_login_finish (G_ASYNC_RESULT (result), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

static void login_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdLoginRequest *request = static_cast<QSnapdLoginRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdLoginRequest::runAsync ()
{
    Q_D(QSnapdLoginRequest);

    if (getClient () != NULL)
        snapd_client_login2_async (SNAPD_CLIENT (getClient ()), d->email.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, g_object_ref (d->callback_data));
    else
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        snapd_login_async (d->email.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.isNull () ? NULL : d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
}

QSnapdUserInformation *QSnapdLoginRequest::userInformation ()
{
    Q_D(QSnapdLoginRequest);
    return new QSnapdUserInformation (d->user_information);
}

QSnapdAuthData *QSnapdLoginRequest::authData ()
{
    Q_D(QSnapdLoginRequest);

    if (d->auth_data != NULL)
        return new QSnapdAuthData (d->auth_data);
    else
        return new QSnapdAuthData (snapd_user_information_get_auth_data (d->user_information));
}

QSnapdLogoutRequest::QSnapdLogoutRequest (void *snapd_client, qint64 id, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdLogoutRequestPrivate (this, id))
{
}

void QSnapdLogoutRequest::runSync ()
{
    Q_D(QSnapdLogoutRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_logout_sync (SNAPD_CLIENT (getClient ()), d->id, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdLogoutRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_logout_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void logout_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdLogoutRequest *request = static_cast<QSnapdLogoutRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdLogoutRequest::runAsync ()
{
    Q_D(QSnapdLogoutRequest);
    snapd_client_logout_async (SNAPD_CLIENT (getClient ()), d->id, G_CANCELLABLE (getCancellable ()), logout_ready_cb, g_object_ref (d->callback_data));
}

static SnapdChangeFilter convertChangeFilter (int filter)
{
    switch (filter)
    {
    default:
    case QSnapdClient::ChangeFilter::FilterAll:
        return SNAPD_CHANGE_FILTER_ALL;
    case QSnapdClient::ChangeFilter::FilterInProgress:
        return SNAPD_CHANGE_FILTER_IN_PROGRESS;
    case QSnapdClient::ChangeFilter::FilterReady:
        return SNAPD_CHANGE_FILTER_READY;
    }
}

QSnapdGetChangesRequest::QSnapdGetChangesRequest (int filter, const QString& snapName, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetChangesRequestPrivate (this, filter, snapName)) {}

void QSnapdGetChangesRequest::runSync ()
{
    Q_D(QSnapdGetChangesRequest);

    g_autoptr(GError) error = NULL;
    d->changes = snapd_client_get_changes_sync (SNAPD_CLIENT (getClient ()), convertChangeFilter (d->filter), d->snapName.isNull () ? NULL : d->snapName.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetChangesRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetChangesRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) changes = snapd_client_get_changes_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->changes = (GPtrArray*) g_steal_pointer (&changes);
    finish (error);
}

static void changes_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetChangesRequest *request = static_cast<QSnapdGetChangesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetChangesRequest::runAsync ()
{
    Q_D(QSnapdGetChangesRequest);
    snapd_client_get_changes_async (SNAPD_CLIENT (getClient ()), convertChangeFilter (d->filter), d->snapName.isNull () ? NULL : d->snapName.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), changes_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetChangesRequest::changeCount () const
{
    Q_D(const QSnapdGetChangesRequest);
    return d->changes != NULL ? d->changes->len : 0;
}

QSnapdChange *QSnapdGetChangesRequest::change (int n) const
{
    Q_D(const QSnapdGetChangesRequest);
    if (d->changes == NULL || n < 0 || (guint) n >= d->changes->len)
        return NULL;
    return new QSnapdChange (d->changes->pdata[n]);
}

QSnapdGetChangeRequest::QSnapdGetChangeRequest (const QString& id, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetChangeRequestPrivate (this, id)) {}

void QSnapdGetChangeRequest::runSync ()
{
    Q_D(QSnapdGetChangeRequest);

    g_autoptr(GError) error = NULL;
    d->change = snapd_client_get_change_sync (SNAPD_CLIENT (getClient ()), d->id.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetChangeRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetChangeRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdChange) change = snapd_client_get_change_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->change = (SnapdChange*) g_steal_pointer (&change);
    finish (error);
}

static void change_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetChangeRequest *request = static_cast<QSnapdGetChangeRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetChangeRequest::runAsync ()
{
    Q_D(QSnapdGetChangeRequest);
    snapd_client_get_change_async (SNAPD_CLIENT (getClient ()), d->id.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), change_ready_cb, g_object_ref (d->callback_data));
}

QSnapdChange *QSnapdGetChangeRequest::change () const
{
    Q_D(const QSnapdGetChangeRequest);
    return new QSnapdChange (d->change);
}

QSnapdAbortChangeRequest::QSnapdAbortChangeRequest (const QString& id, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdAbortChangeRequestPrivate (this, id)) {}

void QSnapdAbortChangeRequest::runSync ()
{
    Q_D(QSnapdAbortChangeRequest);

    g_autoptr(GError) error = NULL;
    d->change = snapd_client_abort_change_sync (SNAPD_CLIENT (getClient ()), d->id.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdAbortChangeRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdAbortChangeRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdChange) change = snapd_client_abort_change_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->change = (SnapdChange*) g_steal_pointer (&change);
    finish (error);
}

static void abort_change_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdAbortChangeRequest *request = static_cast<QSnapdAbortChangeRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdAbortChangeRequest::runAsync ()
{
    Q_D(QSnapdAbortChangeRequest);
    snapd_client_abort_change_async (SNAPD_CLIENT (getClient ()), d->id.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), abort_change_ready_cb, g_object_ref (d->callback_data));
}

QSnapdChange *QSnapdAbortChangeRequest::change () const
{
    Q_D(const QSnapdAbortChangeRequest);
    return new QSnapdChange (d->change);
}

QSnapdGetSystemInformationRequest::QSnapdGetSystemInformationRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSystemInformationRequestPrivate (this)) {}

void QSnapdGetSystemInformationRequest::runSync ()
{
    Q_D(QSnapdGetSystemInformationRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_get_system_information_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSystemInformationRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetSystemInformationRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->info = (SnapdSystemInformation*) g_steal_pointer (&info);
    finish (error);
}

static void system_information_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetSystemInformationRequest *request = static_cast<QSnapdGetSystemInformationRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetSystemInformationRequest::runAsync ()
{
    Q_D(QSnapdGetSystemInformationRequest);
    snapd_client_get_system_information_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), system_information_ready_cb, g_object_ref (d->callback_data));
}

QSnapdSystemInformation *QSnapdGetSystemInformationRequest::systemInformation ()
{
    Q_D(QSnapdGetSystemInformationRequest);
    return new QSnapdSystemInformation (d->info);
}

QSnapdListRequest::QSnapdListRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdListRequestPrivate (this)) {}

void QSnapdListRequest::runSync ()
{
    Q_D(QSnapdListRequest);

    g_autoptr(GError) error = NULL;
    d->snaps = snapd_client_get_snaps_sync (SNAPD_CLIENT (getClient ()), SNAPD_GET_SNAPS_FLAGS_NONE, NULL, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdListRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdListRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    finish (error);
}

static void list_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdListRequest *request = static_cast<QSnapdListRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdListRequest::runAsync ()
{
    Q_D(QSnapdListRequest);
    snapd_client_get_snaps_async (SNAPD_CLIENT (getClient ()), SNAPD_GET_SNAPS_FLAGS_NONE, NULL, G_CANCELLABLE (getCancellable ()), list_ready_cb, g_object_ref (d->callback_data));
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

static GStrv
string_list_to_strv (const QStringList& list)
{
    GStrv value;
    int size, i;

    size = list.size ();
    value = (GStrv) malloc (sizeof (gchar *) * (size + 1));
    for (i = 0; i < size; i++)
        value[i] = g_strdup (list[i].toStdString ().c_str ());
    value[size] = NULL;

    return value;
}

static SnapdGetSnapsFlags convertGetSnapsFlags (int flags)
{
    int result = SNAPD_GET_SNAPS_FLAGS_NONE;

    if ((flags & QSnapdClient::GetSnapsFlag::IncludeInactive) != 0)
        result |= SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE;

    if ((flags & QSnapdClient::GetSnapsFlag::RefreshInhibited) != 0)
        result |= SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED;

    return (SnapdGetSnapsFlags) result;
}

QSnapdGetSnapsRequest::QSnapdGetSnapsRequest (int flags, const QStringList& snaps, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSnapsRequestPrivate (this, flags, snaps)) {}

void QSnapdGetSnapsRequest::runSync ()
{
    Q_D(QSnapdGetSnapsRequest);

    g_auto(GStrv) snaps = string_list_to_strv (d->filter_snaps);
    g_autoptr(GError) error = NULL;
    d->snaps = snapd_client_get_snaps_sync (SNAPD_CLIENT (getClient ()), convertGetSnapsFlags (d->flags), snaps, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSnapsRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetSnapsRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    finish (error);
}

static void get_snaps_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetSnapsRequest *request = static_cast<QSnapdGetSnapsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetSnapsRequest::runAsync ()
{
    Q_D(QSnapdGetSnapsRequest);

    g_auto(GStrv) snaps = string_list_to_strv (d->filter_snaps);
    snapd_client_get_snaps_async (SNAPD_CLIENT (getClient ()), convertGetSnapsFlags (d->flags), snaps, G_CANCELLABLE (getCancellable ()), get_snaps_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetSnapsRequest::snapCount () const
{
    Q_D(const QSnapdGetSnapsRequest);
    return d->snaps != NULL ? d->snaps->len : 0;
}

QSnapdSnap *QSnapdGetSnapsRequest::snap (int n) const
{
    Q_D(const QSnapdGetSnapsRequest);

    if (d->snaps == NULL || n < 0 || (guint) n >= d->snaps->len)
        return NULL;
    return new QSnapdSnap (d->snaps->pdata[n]);
}

QSnapdListOneRequest::QSnapdListOneRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdListOneRequestPrivate (this, name)) {}

void QSnapdListOneRequest::runSync ()
{
    Q_D(QSnapdListOneRequest);

    g_autoptr(GError) error = NULL;
    d->snap = snapd_client_get_snap_sync (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdListOneRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdListOneRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSnap) snap = snapd_client_get_snap_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snap = (SnapdSnap*) g_steal_pointer (&snap);
    finish (error);
}

static void list_one_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdListOneRequest *request = static_cast<QSnapdListOneRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdListOneRequest::runAsync ()
{
    Q_D(QSnapdListOneRequest);
    snapd_client_get_snap_async (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, g_object_ref (d->callback_data));
}

QSnapdSnap *QSnapdListOneRequest::snap () const
{
    Q_D(const QSnapdListOneRequest);
    return new QSnapdSnap (d->snap);
}

QSnapdGetSnapRequest::QSnapdGetSnapRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSnapRequestPrivate (this, name)) {}

void QSnapdGetSnapRequest::runSync ()
{
    Q_D(QSnapdGetSnapRequest);

    g_autoptr(GError) error = NULL;
    d->snap = snapd_client_get_snap_sync (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSnapRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetSnapRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSnap) snap = snapd_client_get_snap_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snap = (SnapdSnap*) g_steal_pointer (&snap);
    finish (error);
}

static void get_snap_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetSnapRequest *request = static_cast<QSnapdGetSnapRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetSnapRequest::runAsync ()
{
    Q_D(QSnapdGetSnapRequest);
    snapd_client_get_snap_async (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_snap_ready_cb, g_object_ref (d->callback_data));
}

QSnapdSnap *QSnapdGetSnapRequest::snap () const
{
    Q_D(const QSnapdGetSnapRequest);
    return new QSnapdSnap (d->snap);
}

QSnapdGetSnapConfRequest::QSnapdGetSnapConfRequest (const QString& name, const QStringList& keys, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSnapConfRequestPrivate (this, name, keys)) {}

void QSnapdGetSnapConfRequest::runSync ()
{
    Q_D(QSnapdGetSnapConfRequest);

    g_auto(GStrv) keys = string_list_to_strv (d->keys);
    g_autoptr(GError) error = NULL;
    d->configuration = snapd_client_get_snap_conf_sync (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), keys, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetSnapConfRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetSnapConfRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GHashTable) configuration = snapd_client_get_snap_conf_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->configuration = (GHashTable*) g_steal_pointer (&configuration);
    finish (error);
}

static void get_snap_conf_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetSnapConfRequest *request = static_cast<QSnapdGetSnapConfRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetSnapConfRequest::runAsync ()
{
    Q_D(QSnapdGetSnapConfRequest);

    g_auto(GStrv) keys = string_list_to_strv (d->keys);
    snapd_client_get_snap_conf_async (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), keys, G_CANCELLABLE (getCancellable ()), get_snap_conf_ready_cb, g_object_ref (d->callback_data));
}

QHash<QString, QVariant> *QSnapdGetSnapConfRequest::configuration () const
{
    Q_D(const QSnapdGetSnapConfRequest);

    QHash<QString, QVariant> *conf = new QHash<QString, QVariant> ();
    GHashTableIter iter;
    g_hash_table_iter_init (&iter, d->configuration);
    gpointer key, value;
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        const gchar *conf_key = (const gchar *) key;
        GVariant *conf_value = (GVariant *) value;
        conf->insert (conf_key, gvariant_to_qvariant (conf_value));
    }

    return conf;
}

QSnapdSetSnapConfRequest::QSnapdSetSnapConfRequest (const QString& name, const QHash<QString, QVariant>& configuration, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdSetSnapConfRequestPrivate (this, name, configuration)) {}

static GVariant *
qvariant_to_gvariant (const QVariant& variant)
{
    if (variant.isNull ())
        return g_variant_new ("mv", NULL);

    switch (variant.type ()) {
    case QMetaType::Bool:
        return g_variant_new_boolean (variant.value<bool>());
    case QMetaType::Int:
        return g_variant_new_int64 (variant.value<int>());
    case QMetaType::LongLong:
        return g_variant_new_int64 (variant.value<qlonglong>());
    case QMetaType::QString:
        return g_variant_new_string (variant.value<QString>().toStdString ().c_str ());
    case QMetaType::Double:
        return g_variant_new_double (variant.value<double>());
    case QMetaType::QVariantList: {
        g_autoptr(GVariantBuilder) builder = g_variant_builder_new (G_VARIANT_TYPE ("av"));
        QList<QVariant> list = variant.toList ();
        for (int i = 0; i < list.length (); i++) {
            GVariant *v = qvariant_to_gvariant (list[i]);
            g_variant_builder_add (builder, "v", v);
        }
        return g_variant_ref_sink (g_variant_builder_end (builder));
    }
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash: {
        g_autoptr(GVariantBuilder) builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
        QMap<QString, QVariant> map = variant.toMap ();
        QMapIterator<QString, QVariant> i (map);
        while (i.hasNext ()) {
            i.next ();
            GVariant *v = qvariant_to_gvariant (i.value ());
            g_variant_builder_add (builder, "{sv}", i.key ().toStdString ().c_str (), v);
        }
        return g_variant_ref_sink (g_variant_builder_end (builder));
    }
    default:
        return g_variant_new ("mv", NULL);
    }
}

static GHashTable *
configuration_to_key_values (const QHash<QString, QVariant>& configuration)
{
    g_autoptr(GHashTable) conf = NULL;

    conf = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);
    QHashIterator<QString, QVariant> i (configuration);
    while (i.hasNext ()) {
        i.next ();
        g_hash_table_insert (conf, g_strdup (i.key ().toStdString ().c_str ()), qvariant_to_gvariant (i.value ()));
    }

    return (GHashTable *) g_steal_pointer (&conf);
}

void QSnapdSetSnapConfRequest::runSync ()
{
    Q_D(QSnapdSetSnapConfRequest);

    g_autoptr(GHashTable) key_values = configuration_to_key_values (d->configuration);
    g_autoptr(GError) error = NULL;
    snapd_client_set_snap_conf_sync (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), key_values, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdSetSnapConfRequest::handleResult (void *object, void *result)
{
    g_autoptr(GHashTable) configuration = NULL;
    g_autoptr(GError) error = NULL;

    snapd_client_set_snap_conf_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void set_snap_conf_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdSetSnapConfRequest *request = static_cast<QSnapdSetSnapConfRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdSetSnapConfRequest::runAsync ()
{
    Q_D(QSnapdSetSnapConfRequest);

    g_autoptr(GHashTable) key_values = configuration_to_key_values (d->configuration);
    snapd_client_set_snap_conf_async (SNAPD_CLIENT (getClient ()), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), key_values, G_CANCELLABLE (getCancellable ()), set_snap_conf_ready_cb, g_object_ref (d->callback_data));
}

static SnapdGetAppsFlags convertGetAppsFlags (int flags)
{
    int result = SNAPD_GET_APPS_FLAGS_NONE;

    if ((flags & QSnapdClient::GetAppsFlag::SelectServices) != 0)
        result |= SNAPD_GET_APPS_FLAGS_SELECT_SERVICES;

    return (SnapdGetAppsFlags) result;
}

QSnapdGetAppsRequest::QSnapdGetAppsRequest (int flags, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAppsRequestPrivate (this, flags, QStringList ())) {}

QSnapdGetAppsRequest::QSnapdGetAppsRequest (int flags, const QStringList& snaps, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAppsRequestPrivate (this, flags, snaps)) {}

void QSnapdGetAppsRequest::runSync ()
{
    Q_D(QSnapdGetAppsRequest);

    g_auto(GStrv) snaps = string_list_to_strv (d->filter_snaps);
    g_autoptr(GError) error = NULL;
    d->apps = snapd_client_get_apps2_sync (SNAPD_CLIENT (getClient ()), convertGetAppsFlags (d->flags), snaps, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetAppsRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetAppsRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) apps = snapd_client_get_apps2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->apps = (GPtrArray*) g_steal_pointer (&apps);
    finish (error);
}

static void get_apps_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetAppsRequest *request = static_cast<QSnapdGetAppsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetAppsRequest::runAsync ()
{
    Q_D(QSnapdGetAppsRequest);

    g_auto(GStrv) snaps = string_list_to_strv (d->filter_snaps);
    snapd_client_get_apps2_async (SNAPD_CLIENT (getClient ()), convertGetAppsFlags (d->flags), snaps, G_CANCELLABLE (getCancellable ()), get_apps_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetAppsRequest::appCount () const
{
    Q_D(const QSnapdGetAppsRequest);
    return d->apps != NULL ? d->apps->len : 0;
}

QSnapdApp *QSnapdGetAppsRequest::app (int n) const
{
    Q_D(const QSnapdGetAppsRequest);

    if (d->apps == NULL || n < 0 || (guint) n >= d->apps->len)
        return NULL;
    return new QSnapdApp (d->apps->pdata[n]);
}

QSnapdGetIconRequest::QSnapdGetIconRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetIconRequestPrivate (this, name)) {}

void QSnapdGetIconRequest::runSync ()
{
    Q_D(QSnapdGetIconRequest);

    g_autoptr(GError) error = NULL;
    d->icon = snapd_client_get_icon_sync (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetIconRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetIconRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdIcon) icon = snapd_client_get_icon_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    d->icon = (SnapdIcon*) g_steal_pointer (&icon);
    finish (error);
}

static void get_icon_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetIconRequest *request = static_cast<QSnapdGetIconRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetIconRequest::runAsync ()
{
    Q_D(QSnapdGetIconRequest);
    snapd_client_get_icon_async (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_icon_ready_cb, g_object_ref (d->callback_data));
}

QSnapdIcon *QSnapdGetIconRequest::icon () const
{
    Q_D(const QSnapdGetIconRequest);
    return new QSnapdIcon (d->icon);
}

QSnapdGetAssertionsRequest::QSnapdGetAssertionsRequest (const QString& type, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAssertionsRequestPrivate (this, type)) {}

void QSnapdGetAssertionsRequest::runSync ()
{
    Q_D(QSnapdGetAssertionsRequest);

    g_autoptr(GError) error = NULL;
    d->assertions = snapd_client_get_assertions_sync (SNAPD_CLIENT (getClient ()), d->type.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetAssertionsRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetAssertionsRequest);

    g_autoptr(GError) error = NULL;
    g_auto(GStrv) assertions = snapd_client_get_assertions_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);

    d->assertions = (GStrv) g_steal_pointer (&assertions);
    finish (error);
}

static void get_assertions_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetAssertionsRequest *request = static_cast<QSnapdGetAssertionsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetAssertionsRequest::runAsync ()
{
    Q_D(QSnapdGetAssertionsRequest);
    snapd_client_get_assertions_async (SNAPD_CLIENT (getClient ()), d->type.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_assertions_ready_cb, g_object_ref (d->callback_data));
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
    d_ptr (new QSnapdAddAssertionsRequestPrivate (this, assertions)) {}

void QSnapdAddAssertionsRequest::runSync ()
{
    Q_D(QSnapdAddAssertionsRequest);

    g_auto(GStrv) assertions = string_list_to_strv (d->assertions);
    g_autoptr(GError) error = NULL;
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdAddAssertionsRequest *request = static_cast<QSnapdAddAssertionsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdAddAssertionsRequest::runAsync ()
{
    Q_D(QSnapdAddAssertionsRequest);

    g_auto(GStrv) assertions = string_list_to_strv (d->assertions);
    snapd_client_add_assertions_async (SNAPD_CLIENT (getClient ()), assertions, G_CANCELLABLE (getCancellable ()), add_assertions_ready_cb, g_object_ref (d->callback_data));
}

static SnapdGetConnectionsFlags convertGetConnectionsFlags (int flags)
{
    int result = SNAPD_GET_CONNECTIONS_FLAGS_NONE;

    if ((flags & QSnapdClient::GetConnectionsFlag::SelectAll) != 0)
        result |= SNAPD_GET_CONNECTIONS_FLAGS_SELECT_ALL;

    return (SnapdGetConnectionsFlags) result;
}

QSnapdGetConnectionsRequest::QSnapdGetConnectionsRequest (int flags, const QString &snap, const QString &interface, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetConnectionsRequestPrivate (this, flags, snap, interface)) {}

void QSnapdGetConnectionsRequest::runSync ()
{
    Q_D(QSnapdGetConnectionsRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_get_connections2_sync (SNAPD_CLIENT (getClient ()), convertGetConnectionsFlags (d->flags), d->snap.isNull () ? NULL : d->snap.toStdString ().c_str (), d->interface.isNull () ? NULL : d->interface.toStdString ().c_str (), &d->established, &d->undesired, &d->plugs, &d->slots_, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetConnectionsRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetConnectionsRequest);

    g_autoptr(GPtrArray) established = NULL;
    g_autoptr(GPtrArray) undesired = NULL;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_get_connections2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &established, &undesired, &plugs, &slots_, &error);
    d->established = (GPtrArray*) g_steal_pointer (&established);
    d->undesired = (GPtrArray*) g_steal_pointer (&undesired);
    d->plugs = (GPtrArray*) g_steal_pointer (&plugs);
    d->slots_ = (GPtrArray*) g_steal_pointer (&slots_);
    finish (error);
}

static void get_connections_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetConnectionsRequest *request = static_cast<QSnapdGetConnectionsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetConnectionsRequest::runAsync ()
{
    Q_D(QSnapdGetConnectionsRequest);
    snapd_client_get_connections2_async (SNAPD_CLIENT (getClient ()), convertGetConnectionsFlags (d->flags), d->snap.isNull () ? NULL : d->snap.toStdString ().c_str (), d->interface.isNull () ? NULL : d->interface.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), get_connections_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetConnectionsRequest::establishedCount () const
{
    Q_D(const QSnapdGetConnectionsRequest);
    return d->established != NULL ? d->established->len : 0;
}

QSnapdConnection *QSnapdGetConnectionsRequest::established (int n) const
{
    Q_D(const QSnapdGetConnectionsRequest);

    if (d->established == NULL || n < 0 || (guint) n >= d->established->len)
        return NULL;
    return new QSnapdConnection (d->established->pdata[n]);
}

int QSnapdGetConnectionsRequest::undesiredCount () const
{
    Q_D(const QSnapdGetConnectionsRequest);
    return d->undesired != NULL ? d->undesired->len : 0;
}

QSnapdConnection *QSnapdGetConnectionsRequest::undesired (int n) const
{
    Q_D(const QSnapdGetConnectionsRequest);

    if (d->undesired == NULL || n < 0 || (guint) n >= d->undesired->len)
        return NULL;
    return new QSnapdConnection (d->undesired->pdata[n]);
}

int QSnapdGetConnectionsRequest::plugCount () const
{
    Q_D(const QSnapdGetConnectionsRequest);
    return d->plugs != NULL ? d->plugs->len : 0;
}

QSnapdPlug *QSnapdGetConnectionsRequest::plug (int n) const
{
    Q_D(const QSnapdGetConnectionsRequest);

    if (d->plugs == NULL || n < 0 || (guint) n >= d->plugs->len)
        return NULL;
    return new QSnapdPlug (d->plugs->pdata[n]);
}

int QSnapdGetConnectionsRequest::slotCount () const
{
    Q_D(const QSnapdGetConnectionsRequest);
    return d->slots_ != NULL ? d->slots_->len : 0;
}

QSnapdSlot *QSnapdGetConnectionsRequest::slot (int n) const
{
    Q_D(const QSnapdGetConnectionsRequest);

    if (d->slots_ == NULL || n < 0 || (guint) n >= d->slots_->len)
        return NULL;
    return new QSnapdSlot (d->slots_->pdata[n]);
}

QSnapdGetInterfacesRequest::QSnapdGetInterfacesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetInterfacesRequestPrivate (this)) {}

void QSnapdGetInterfacesRequest::runSync ()
{
    Q_D(QSnapdGetInterfacesRequest);

    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_interfaces_sync (SNAPD_CLIENT (getClient ()), &d->plugs, &d->slots_, G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdGetInterfacesRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetInterfacesRequest);

    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_interfaces_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &plugs, &slots_, &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    d->plugs = (GPtrArray*) g_steal_pointer (&plugs);
    d->slots_ = (GPtrArray*) g_steal_pointer (&slots_);
    finish (error);
}

static void get_interfaces_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetInterfacesRequest *request = static_cast<QSnapdGetInterfacesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetInterfacesRequest::runAsync ()
{
    Q_D(QSnapdGetInterfacesRequest);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_interfaces_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), get_interfaces_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
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

QSnapdGetInterfaces2Request::QSnapdGetInterfaces2Request (int flags, const QStringList &names, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetInterfaces2RequestPrivate (this, flags, names)) {}

static SnapdGetInterfacesFlags convertInterfaceFlags (int flags)
{
    int result = SNAPD_GET_INTERFACES_FLAGS_NONE;

    if ((flags & QSnapdClient::InterfaceFlag::IncludeDocs) != 0)
        result |= SNAPD_GET_INTERFACES_FLAGS_INCLUDE_DOCS;
    if ((flags & QSnapdClient::InterfaceFlag::IncludePlugs) != 0)
        result |= SNAPD_GET_INTERFACES_FLAGS_INCLUDE_PLUGS;
    if ((flags & QSnapdClient::InterfaceFlag::IncludeSlots) != 0)
        result |= SNAPD_GET_INTERFACES_FLAGS_INCLUDE_SLOTS;
    if ((flags & QSnapdClient::InterfaceFlag::OnlyConnected) != 0)
        result |= SNAPD_GET_INTERFACES_FLAGS_ONLY_CONNECTED;

    return (SnapdGetInterfacesFlags) result;
}

void QSnapdGetInterfaces2Request::runSync ()
{
    Q_D(QSnapdGetInterfaces2Request);

    g_auto(GStrv) names = string_list_to_strv (d->names);
    g_autoptr(GError) error = NULL;
    d->interfaces = snapd_client_get_interfaces2_sync (SNAPD_CLIENT (getClient ()), convertInterfaceFlags (d->flags), names, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetInterfaces2Request::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetInterfaces2Request);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) interfaces = snapd_client_get_interfaces2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->interfaces = (GPtrArray*) g_steal_pointer (&interfaces);
    finish (error);
}

static void get_interfaces2_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetInterfaces2Request *request = static_cast<QSnapdGetInterfaces2Request*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetInterfaces2Request::runAsync ()
{
    Q_D(QSnapdGetInterfaces2Request);

    g_auto(GStrv) names = string_list_to_strv (d->names);
    snapd_client_get_interfaces2_async (SNAPD_CLIENT (getClient ()), convertInterfaceFlags (d->flags), names, G_CANCELLABLE (getCancellable ()), get_interfaces2_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetInterfaces2Request::interfaceCount () const
{
    Q_D(const QSnapdGetInterfaces2Request);
    return d->interfaces != NULL ? d->interfaces->len : 0;
}

QSnapdInterface *QSnapdGetInterfaces2Request::interface (int n) const
{
    Q_D(const QSnapdGetInterfaces2Request);

    if (d->interfaces == NULL || n < 0 || (guint) n >= d->interfaces->len)
        return NULL;
    return new QSnapdInterface (d->interfaces->pdata[n]);
}

QSnapdConnectInterfaceRequest::QSnapdConnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdConnectInterfaceRequestPrivate (this, plug_snap, plug_name, slot_snap, slot_name)) {}

static void progress_cb (SnapdClient *client, SnapdChange *change, gpointer, gpointer data)
{
    CallbackData *callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdRequest *request = static_cast<QSnapdRequest*>(callback_data->request);
        request->handleProgress (change);
    }
}

void QSnapdConnectInterfaceRequest::runSync ()
{
    Q_D(QSnapdConnectInterfaceRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_connect_interface_sync (SNAPD_CLIENT (getClient ()),
                                         d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                         d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                         progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdConnectInterfaceRequest *request = static_cast<QSnapdConnectInterfaceRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdConnectInterfaceRequest::runAsync ()
{
    Q_D(QSnapdConnectInterfaceRequest);
    snapd_client_connect_interface_async (SNAPD_CLIENT (getClient ()),
                                          d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                          d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                          progress_cb, d->callback_data,
                                          G_CANCELLABLE (getCancellable ()), connect_interface_ready_cb, g_object_ref (d->callback_data));
}

QSnapdDisconnectInterfaceRequest::QSnapdDisconnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisconnectInterfaceRequestPrivate (this, plug_snap, plug_name, slot_snap, slot_name)) {}

void QSnapdDisconnectInterfaceRequest::runSync ()
{
    Q_D(QSnapdDisconnectInterfaceRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_disconnect_interface_sync (SNAPD_CLIENT (getClient ()),
                                            d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                            d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                            progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdDisconnectInterfaceRequest *request = static_cast<QSnapdDisconnectInterfaceRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdDisconnectInterfaceRequest::runAsync ()
{
    Q_D(QSnapdDisconnectInterfaceRequest);
    snapd_client_disconnect_interface_async (SNAPD_CLIENT (getClient ()),
                                             d->plug_snap.toStdString ().c_str (), d->plug_name.toStdString ().c_str (),
                                             d->slot_snap.toStdString ().c_str (), d->slot_name.toStdString ().c_str (),
                                             progress_cb, d->callback_data,
                                             G_CANCELLABLE (getCancellable ()), disconnect_interface_ready_cb, g_object_ref (d->callback_data));
}

static SnapdFindFlags convertFindFlags (int flags)
{
    int result = SNAPD_FIND_FLAGS_NONE;

    if ((flags & QSnapdClient::FindFlag::MatchName) != 0)
        result |= SNAPD_FIND_FLAGS_MATCH_NAME;
    if ((flags & QSnapdClient::FindFlag::MatchCommonId) != 0)
        result |= SNAPD_FIND_FLAGS_MATCH_COMMON_ID;
    if ((flags & QSnapdClient::FindFlag::SelectPrivate) != 0)
        result |= SNAPD_FIND_FLAGS_SELECT_PRIVATE;
    if ((flags & QSnapdClient::FindFlag::ScopeWide) != 0)
        result |= SNAPD_FIND_FLAGS_SCOPE_WIDE;

    return (SnapdFindFlags) result;
}

QSnapdFindRequest::QSnapdFindRequest (int flags, const QString& section, const QString& category, const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdFindRequestPrivate (this, flags, section, category, name)) {}

void QSnapdFindRequest::runSync ()
{
    Q_D(QSnapdFindRequest);

    g_autoptr(GError) error = NULL;
    g_autofree gchar *suggested_currency = NULL;
    if (d->section.isNull ()) {
        d->snaps = snapd_client_find_category_sync (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->category.isNull () ? NULL : d->category.toStdString().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), &suggested_currency, G_CANCELLABLE (getCancellable ()), &error);
    }
    else {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        d->snaps = snapd_client_find_section_sync (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.isNull () ? NULL : d->section.toStdString().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), &suggested_currency, G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
    d->suggestedCurrency = suggested_currency;
    finish (error);
}

void QSnapdFindRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdFindRequest);

    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    if (d->section.isNull ()) {
        snaps = snapd_client_find_category_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &suggested_currency, &error);
    }
    else {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        snaps = snapd_client_find_section_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &suggested_currency, &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    d->suggestedCurrency = suggested_currency;
    finish (error);
}

static void find_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdFindRequest *request = static_cast<QSnapdFindRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdFindRequest::runAsync ()
{
    Q_D(QSnapdFindRequest);
    if (d->section.isNull ()) {
        snapd_client_find_category_async (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->category.isNull () ? NULL : d->category.toStdString ().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), find_ready_cb, g_object_ref (d->callback_data));
    }
    else {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        snapd_client_find_section_async (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.isNull () ? NULL : d->section.toStdString ().c_str (), d->name.isNull () ? NULL : d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), find_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
    }
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
    d_ptr (new QSnapdFindRefreshableRequestPrivate (this)) {}

void QSnapdFindRefreshableRequest::runSync ()
{
    Q_D(QSnapdFindRefreshableRequest);

    g_autoptr(GError) error = NULL;
    d->snaps = snapd_client_find_refreshable_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdFindRefreshableRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdFindRefreshableRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_refreshable_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snaps = (GPtrArray*) g_steal_pointer (&snaps);
    finish (error);
}

static void find_refreshable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdFindRefreshableRequest *request = static_cast<QSnapdFindRefreshableRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdFindRefreshableRequest::runAsync ()
{
    Q_D(QSnapdFindRefreshableRequest);
    snapd_client_find_refreshable_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), find_refreshable_ready_cb, g_object_ref (d->callback_data));
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
    d_ptr (new QSnapdInstallRequestPrivate (this, flags, name, channel, revision, ioDevice)) {}

void QSnapdInstallRequest::runSync ()
{
    Q_D(QSnapdInstallRequest);

    g_autoptr(GError) error = NULL;
    if (d->wrapper != NULL) {
        snapd_client_install_stream_sync (SNAPD_CLIENT (getClient ()),
                                          convertInstallFlags (d->flags),
                                          G_INPUT_STREAM (d->wrapper),
                                          progress_cb, d->callback_data,
                                          G_CANCELLABLE (getCancellable ()), &error);
    }
    else {
        snapd_client_install2_sync (SNAPD_CLIENT (getClient ()),
                                    convertInstallFlags (d->flags),
                                    d->name.toStdString ().c_str (),
                                    d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                    d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                    progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdInstallRequest *request = static_cast<QSnapdInstallRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdInstallRequest::runAsync ()
{
    Q_D(QSnapdInstallRequest);

    if (d->wrapper != NULL)
        snapd_client_install_stream_async (SNAPD_CLIENT (getClient ()),
                                           convertInstallFlags (d->flags),
                                           G_INPUT_STREAM (d->wrapper),
                                           progress_cb, d->callback_data,
                                           G_CANCELLABLE (getCancellable ()), install_ready_cb, g_object_ref (d->callback_data));
    else
        snapd_client_install2_async (SNAPD_CLIENT (getClient ()),
                                     convertInstallFlags (d->flags),
                                     d->name.toStdString ().c_str (),
                                     d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                     d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                     progress_cb, d->callback_data,
                                     G_CANCELLABLE (getCancellable ()), install_ready_cb, g_object_ref (d->callback_data));
}

QSnapdTryRequest::QSnapdTryRequest (const QString& path, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdTryRequestPrivate (this, path)) {}

void QSnapdTryRequest::runSync ()
{
    Q_D(QSnapdTryRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_try_sync (SNAPD_CLIENT (getClient ()),
                           d->path.toStdString ().c_str (),
                           progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdTryRequest *request = static_cast<QSnapdTryRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdTryRequest::runAsync ()
{
    Q_D(QSnapdTryRequest);
    snapd_client_try_async (SNAPD_CLIENT (getClient ()),
                            d->path.toStdString ().c_str (),
                            progress_cb, d->callback_data,
                            G_CANCELLABLE (getCancellable ()), try_ready_cb, g_object_ref (d->callback_data));
}

QSnapdRefreshRequest::QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshRequestPrivate (this, name, channel)) {}

void QSnapdRefreshRequest::runSync ()
{
    Q_D(QSnapdRefreshRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_refresh_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (), d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                               progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdRefreshRequest *request = static_cast<QSnapdRefreshRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdRefreshRequest::runAsync ()
{
    Q_D(QSnapdRefreshRequest);
    snapd_client_refresh_async (SNAPD_CLIENT (getClient ()),
                                d->name.toStdString ().c_str (), d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                progress_cb, d->callback_data,
                                G_CANCELLABLE (getCancellable ()), refresh_ready_cb, g_object_ref (d->callback_data));
}

QSnapdRefreshAllRequest::QSnapdRefreshAllRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshAllRequestPrivate (this)) {}

void QSnapdRefreshAllRequest::runSync ()
{
    Q_D(QSnapdRefreshAllRequest);

    g_autoptr(GError) error = NULL;
    d->snap_names = snapd_client_refresh_all_sync (SNAPD_CLIENT (getClient ()),
                                                   progress_cb, d->callback_data,
                                                   G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRefreshAllRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdRefreshAllRequest);

    g_autoptr(GError) error = NULL;
    g_auto(GStrv) snap_names = snapd_client_refresh_all_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->snap_names = (GStrv) g_steal_pointer (&snap_names);
    finish (error);
}

static void refresh_all_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdRefreshAllRequest *request = static_cast<QSnapdRefreshAllRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdRefreshAllRequest::runAsync ()
{
    Q_D(QSnapdRefreshAllRequest);
    snapd_client_refresh_all_async (SNAPD_CLIENT (getClient ()),
                                    progress_cb, d->callback_data,
                                    G_CANCELLABLE (getCancellable ()), refresh_all_ready_cb, g_object_ref (d->callback_data));
}

QStringList QSnapdRefreshAllRequest::snapNames () const
{
    Q_D(const QSnapdRefreshAllRequest);

    QStringList result;
    for (int i = 0; d->snap_names[i] != NULL; i++)
        result.append (d->snap_names[i]);
    return result;
}

static SnapdRemoveFlags convertRemoveFlags (int flags)
{
    int result = SNAPD_REMOVE_FLAGS_NONE;

    if ((flags & QSnapdClient::RemoveFlag::Purge) != 0)
        result |= SNAPD_REMOVE_FLAGS_PURGE;

    return (SnapdRemoveFlags) result;
}

QSnapdRemoveRequest::QSnapdRemoveRequest (int flags, const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRemoveRequestPrivate (this, flags, name)) {}

void QSnapdRemoveRequest::runSync ()
{
    Q_D(QSnapdRemoveRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_remove2_sync (SNAPD_CLIENT (getClient ()),
                               convertRemoveFlags (d->flags),
                               d->name.toStdString ().c_str (),
                               progress_cb, d->callback_data,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRemoveRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_remove2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void remove_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdRemoveRequest *request = static_cast<QSnapdRemoveRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdRemoveRequest::runAsync ()
{
    Q_D(QSnapdRemoveRequest);
    snapd_client_remove2_async (SNAPD_CLIENT (getClient ()),
                                convertRemoveFlags (d->flags),
                                d->name.toStdString ().c_str (),
                                progress_cb, d->callback_data,
                                G_CANCELLABLE (getCancellable ()), remove_ready_cb, g_object_ref (d->callback_data));
}

QSnapdEnableRequest::QSnapdEnableRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdEnableRequestPrivate (this, name)) {}

void QSnapdEnableRequest::runSync ()
{
    Q_D(QSnapdEnableRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_enable_sync (SNAPD_CLIENT (getClient ()),
                              d->name.toStdString ().c_str (),
                              progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdEnableRequest *request = static_cast<QSnapdEnableRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdEnableRequest::runAsync ()
{
    Q_D(QSnapdEnableRequest);
    snapd_client_enable_async (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               progress_cb, d->callback_data,
                               G_CANCELLABLE (getCancellable ()), enable_ready_cb, g_object_ref (d->callback_data));
}

QSnapdDisableRequest::QSnapdDisableRequest (const QString& name, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisableRequestPrivate (this, name)) {}

void QSnapdDisableRequest::runSync ()
{
    Q_D(QSnapdDisableRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_disable_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               progress_cb, d->callback_data,
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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdDisableRequest *request = static_cast<QSnapdDisableRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdDisableRequest::runAsync ()
{
    Q_D(QSnapdDisableRequest);

    snapd_client_disable_async (SNAPD_CLIENT (getClient ()),
                                d->name.toStdString ().c_str (),
                                progress_cb, d->callback_data,
                                G_CANCELLABLE (getCancellable ()), disable_ready_cb, g_object_ref (d->callback_data));
}

QSnapdSwitchChannelRequest::QSnapdSwitchChannelRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdSwitchChannelRequestPrivate (this, name, channel)) {}

void QSnapdSwitchChannelRequest::runSync ()
{
    Q_D(QSnapdSwitchChannelRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_switch_sync (SNAPD_CLIENT (getClient ()),
                              d->name.toStdString ().c_str (),
                              d->channel.toStdString ().c_str (),
                              progress_cb, d->callback_data,
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdSwitchChannelRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_switch_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void switch_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdSwitchChannelRequest *request = static_cast<QSnapdSwitchChannelRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdSwitchChannelRequest::runAsync ()
{
    Q_D(QSnapdSwitchChannelRequest);

    snapd_client_switch_async (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (),
                               d->channel.toStdString ().c_str (),
                               progress_cb, d->callback_data,
                               G_CANCELLABLE (getCancellable ()), switch_ready_cb, g_object_ref (d->callback_data));
}

QSnapdCheckBuyRequest::QSnapdCheckBuyRequest (void *snapd_client, QObject *parent):
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdCheckBuyRequestPrivate (this)) {}

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
    Q_D(QSnapdCheckBuyRequest);

    g_autoptr(GError) error = NULL;
    gboolean can_buy = snapd_client_check_buy_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->canBuy = can_buy;
    finish (error);
}

static void check_buy_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdCheckBuyRequest *request = static_cast<QSnapdCheckBuyRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdCheckBuyRequest::runAsync ()
{
    Q_D(QSnapdCheckBuyRequest);
    snapd_client_check_buy_async (SNAPD_CLIENT (getClient ()),
                                  G_CANCELLABLE (getCancellable ()), check_buy_ready_cb, g_object_ref (d->callback_data));
}

bool QSnapdCheckBuyRequest::canBuy () const
{
    Q_D(const QSnapdCheckBuyRequest);
    return d->canBuy;
}

QSnapdBuyRequest::QSnapdBuyRequest (const QString& id, double amount, const QString& currency, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdBuyRequestPrivate (this, id, amount, currency)) {}

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
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdBuyRequest *request = static_cast<QSnapdBuyRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdBuyRequest::runAsync ()
{
    Q_D(QSnapdBuyRequest);
    snapd_client_buy_async (SNAPD_CLIENT (getClient ()),
                            d->id.toStdString ().c_str (), d->amount, d->currency.toStdString ().c_str (),
                            G_CANCELLABLE (getCancellable ()), buy_ready_cb, g_object_ref (d->callback_data));
}

static SnapdCreateUserFlags convertCreateUserFlags (int flags)
{
    int result = SNAPD_CREATE_USER_FLAGS_NONE;

    if ((flags & QSnapdClient::CreateUserFlag::Sudo) != 0)
        result |= SNAPD_CREATE_USER_FLAGS_SUDO;
    if ((flags & QSnapdClient::CreateUserFlag::Known) != 0)
        result |= SNAPD_CREATE_USER_FLAGS_KNOWN;

    return (SnapdCreateUserFlags) result;
}

QSnapdCreateUserRequest::QSnapdCreateUserRequest (const QString& email, int flags, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdCreateUserRequestPrivate (this, email, flags)) {}

void QSnapdCreateUserRequest::runSync ()
{
    Q_D(QSnapdCreateUserRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_create_user_sync (SNAPD_CLIENT (getClient ()),
                                             d->email.toStdString ().c_str (), convertCreateUserFlags (d->flags),
                                             G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdCreateUserRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdCreateUserRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_create_user_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void create_user_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdCreateUserRequest *request = static_cast<QSnapdCreateUserRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdCreateUserRequest::runAsync ()
{
    Q_D(QSnapdCreateUserRequest);
    snapd_client_create_user_async (SNAPD_CLIENT (getClient ()),
                                    d->email.toStdString ().c_str (), convertCreateUserFlags (d->flags),
                                    G_CANCELLABLE (getCancellable ()), create_user_ready_cb, g_object_ref (d->callback_data));
}

QSnapdUserInformation *QSnapdCreateUserRequest::userInformation () const
{
    Q_D(const QSnapdCreateUserRequest);
    return new QSnapdUserInformation (d->info);
}

QSnapdCreateUsersRequest::QSnapdCreateUsersRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdCreateUsersRequestPrivate (this)) {}

void QSnapdCreateUsersRequest::runSync ()
{
    Q_D(QSnapdCreateUsersRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_create_users_sync (SNAPD_CLIENT (getClient ()),
                                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdCreateUsersRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdCreateUsersRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_create_users_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void create_users_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdCreateUsersRequest *request = static_cast<QSnapdCreateUsersRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdCreateUsersRequest::runAsync ()
{
    Q_D(QSnapdCreateUsersRequest);
    snapd_client_create_users_async (SNAPD_CLIENT (getClient ()),
                                     G_CANCELLABLE (getCancellable ()), create_users_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdCreateUsersRequest::userInformationCount () const
{
    Q_D(const QSnapdCreateUsersRequest);
    return d->info != NULL ? d->info->len : 0;
}

QSnapdUserInformation *QSnapdCreateUsersRequest::userInformation (int n) const
{
    Q_D(const QSnapdCreateUsersRequest);

    if (d->info == NULL || n < 0 || (guint) n >= d->info->len)
        return NULL;
    return new QSnapdUserInformation (d->info->pdata[n]);
}

QSnapdGetUsersRequest::QSnapdGetUsersRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetUsersRequestPrivate (this)) {}

void QSnapdGetUsersRequest::runSync ()
{
    Q_D(QSnapdGetUsersRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_get_users_sync (SNAPD_CLIENT (getClient ()),
                                           G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetUsersRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetUsersRequest);

    g_autoptr(GError) error = NULL;
    d->info = snapd_client_get_users_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void get_users_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetUsersRequest *request = static_cast<QSnapdGetUsersRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetUsersRequest::runAsync ()
{
    Q_D(QSnapdGetUsersRequest);
    snapd_client_get_users_async (SNAPD_CLIENT (getClient ()),
                                  G_CANCELLABLE (getCancellable ()), get_users_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetUsersRequest::userInformationCount () const
{
    Q_D(const QSnapdGetUsersRequest);
    return d->info != NULL ? d->info->len : 0;
}

QSnapdUserInformation *QSnapdGetUsersRequest::userInformation (int n) const
{
    Q_D(const QSnapdGetUsersRequest);

    if (d->info == NULL || n < 0 || (guint) n >= d->info->len)
        return NULL;
    return new QSnapdUserInformation (d->info->pdata[n]);
}

QSnapdGetSectionsRequest::QSnapdGetSectionsRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetSectionsRequestPrivate (this)) {}

void QSnapdGetSectionsRequest::runSync ()
{
    Q_D(QSnapdGetSectionsRequest);

    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    d->sections = snapd_client_get_sections_sync (SNAPD_CLIENT (getClient ()),
                                                  G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdGetSectionsRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetSectionsRequest);

    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    g_auto(GStrv) sections = snapd_client_get_sections_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
G_GNUC_END_IGNORE_DEPRECATIONS

    d->sections = (GStrv) g_steal_pointer (&sections);
    finish (error);
}

static void get_sections_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetSectionsRequest *request = static_cast<QSnapdGetSectionsRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetSectionsRequest::runAsync ()
{
    Q_D(QSnapdGetSectionsRequest);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_sections_async (SNAPD_CLIENT (getClient ()),
                                     G_CANCELLABLE (getCancellable ()), get_sections_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
}

QStringList QSnapdGetSectionsRequest::sections () const
{
    Q_D(const QSnapdGetSectionsRequest);

    QStringList result;
    for (int i = 0; d->sections[i] != NULL; i++)
        result.append (d->sections[i]);
    return result;
}

QSnapdGetCategoriesRequest::QSnapdGetCategoriesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetCategoriesRequestPrivate (this)) {}

void QSnapdGetCategoriesRequest::runSync ()
{
    Q_D(QSnapdGetCategoriesRequest);

    g_autoptr(GError) error = NULL;
    d->categories = snapd_client_get_categories_sync (SNAPD_CLIENT (getClient ()),
                                                      G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdGetCategoriesRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdGetCategoriesRequest);

    g_autoptr(GError) error = NULL;
    d->categories = snapd_client_get_categories_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void get_categories_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetCategoriesRequest *request = static_cast<QSnapdGetCategoriesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetCategoriesRequest::runAsync ()
{
    Q_D(QSnapdGetCategoriesRequest);
    snapd_client_get_categories_async (SNAPD_CLIENT (getClient ()),
                                       G_CANCELLABLE (getCancellable ()), get_categories_ready_cb, g_object_ref (d->callback_data));
}

int QSnapdGetCategoriesRequest::categoryCount () const
{
    Q_D(const QSnapdGetCategoriesRequest);
    return d->categories != NULL ? d->categories->len : 0;
}

QSnapdCategoryDetails *QSnapdGetCategoriesRequest::category (int n) const
{
    Q_D(const QSnapdGetCategoriesRequest);

    if (d->categories == NULL || n < 0 || (guint) n >= d->categories->len)
        return NULL;
    return new QSnapdCategoryDetails (d->categories->pdata[n]);
}

QSnapdGetAliasesRequest::QSnapdGetAliasesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdGetAliasesRequestPrivate (this)) {}

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
    Q_D(QSnapdGetAliasesRequest);

    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) aliases = snapd_client_get_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    d->aliases = (GPtrArray*) g_steal_pointer (&aliases);
    finish (error);
}

static void get_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdGetAliasesRequest *request = static_cast<QSnapdGetAliasesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdGetAliasesRequest::runAsync ()
{
    Q_D(QSnapdGetAliasesRequest);
    snapd_client_get_aliases_async (SNAPD_CLIENT (getClient ()),
                                    G_CANCELLABLE (getCancellable ()), get_aliases_ready_cb, g_object_ref (d->callback_data));
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

QSnapdAliasRequest::QSnapdAliasRequest (const QString& snap, const QString& app, const QString& alias, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdAliasRequestPrivate (this, snap, app, alias)) {}

void QSnapdAliasRequest::runSync ()
{
    Q_D(QSnapdAliasRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_alias_sync (SNAPD_CLIENT (getClient ()),
                             d->snap.toStdString ().c_str (),
                             d->app.toStdString ().c_str (),
                             d->alias.toStdString ().c_str (),
                             progress_cb, d->callback_data,
                             G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdAliasRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_alias_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void alias_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdAliasRequest *request = static_cast<QSnapdAliasRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdAliasRequest::runAsync ()
{
    Q_D(QSnapdAliasRequest);

    snapd_client_alias_async (SNAPD_CLIENT (getClient ()),
                              d->snap.toStdString ().c_str (),
                              d->app.toStdString ().c_str (),
                              d->alias.toStdString ().c_str (),
                              progress_cb, d->callback_data,
                              G_CANCELLABLE (getCancellable ()), alias_ready_cb, g_object_ref (d->callback_data));
}

QSnapdUnaliasRequest::QSnapdUnaliasRequest (const QString& snap, const QString& alias, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdUnaliasRequestPrivate (this, snap, alias)) {}

void QSnapdUnaliasRequest::runSync ()
{
    Q_D(QSnapdUnaliasRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_unalias_sync (SNAPD_CLIENT (getClient ()),
                               d->snap.isNull () ? NULL : d->snap.toStdString ().c_str (),
                               d->alias.isNull () ? NULL : d->alias.toStdString ().c_str (),
                               progress_cb, d->callback_data,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdUnaliasRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_unalias_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void unalias_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdUnaliasRequest *request = static_cast<QSnapdUnaliasRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdUnaliasRequest::runAsync ()
{
    Q_D(QSnapdUnaliasRequest);

    snapd_client_unalias_async (SNAPD_CLIENT (getClient ()),
                                d->snap.isNull () ? NULL : d->snap.toStdString ().c_str (),
                                d->alias.isNull () ? NULL : d->alias.toStdString ().c_str (),
                                progress_cb, d->callback_data,
                                G_CANCELLABLE (getCancellable ()), unalias_ready_cb, g_object_ref (d->callback_data));
}

QSnapdPreferRequest::QSnapdPreferRequest (const QString& snap, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdPreferRequestPrivate (this, snap)) {}

void QSnapdPreferRequest::runSync ()
{
    Q_D(QSnapdPreferRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_prefer_sync (SNAPD_CLIENT (getClient ()),
                              d->snap.toStdString ().c_str (),
                              progress_cb, d->callback_data,
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdPreferRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_prefer_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void prefer_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdPreferRequest *request = static_cast<QSnapdPreferRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdPreferRequest::runAsync ()
{
    Q_D(QSnapdPreferRequest);

    snapd_client_prefer_async (SNAPD_CLIENT (getClient ()),
                               d->snap.toStdString ().c_str (),
                               progress_cb, d->callback_data,
                               G_CANCELLABLE (getCancellable ()), prefer_ready_cb, g_object_ref (d->callback_data));
}

QSnapdEnableAliasesRequest::QSnapdEnableAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdEnableAliasesRequestPrivate (this, name, aliases)) {}

void QSnapdEnableAliasesRequest::runSync ()
{
    Q_D(QSnapdEnableAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_enable_aliases_sync (SNAPD_CLIENT (getClient ()),
                                      d->snap.toStdString ().c_str (), aliases,
                                      progress_cb, d->callback_data,
                                      G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdEnableAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_enable_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

static void enable_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdEnableAliasesRequest *request = static_cast<QSnapdEnableAliasesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdEnableAliasesRequest::runAsync ()
{
    Q_D(QSnapdEnableAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_disable_aliases_async (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, d->callback_data,
                                       G_CANCELLABLE (getCancellable ()), enable_aliases_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
}

QSnapdDisableAliasesRequest::QSnapdDisableAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDisableAliasesRequestPrivate (this, name, aliases)) {}

void QSnapdDisableAliasesRequest::runSync ()
{
    Q_D(QSnapdDisableAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_disable_aliases_sync (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, d->callback_data,
                                       G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdDisableAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_disable_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

static void disable_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdDisableAliasesRequest *request = static_cast<QSnapdDisableAliasesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdDisableAliasesRequest::runAsync ()
{
    Q_D(QSnapdDisableAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_disable_aliases_async (SNAPD_CLIENT (getClient ()),
                                       d->snap.toStdString ().c_str (), aliases,
                                       progress_cb, d->callback_data,
                                       G_CANCELLABLE (getCancellable ()), disable_aliases_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
}

QSnapdResetAliasesRequest::QSnapdResetAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdResetAliasesRequestPrivate (this, name, aliases)) {}

void QSnapdResetAliasesRequest::runSync ()
{
    Q_D(QSnapdResetAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_reset_aliases_sync (SNAPD_CLIENT (getClient ()),
                                     d->snap.toStdString ().c_str (), aliases,
                                     progress_cb, d->callback_data,
                                     G_CANCELLABLE (getCancellable ()), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

void QSnapdResetAliasesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_reset_aliases_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    finish (error);
}

static void reset_aliases_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdResetAliasesRequest *request = static_cast<QSnapdResetAliasesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdResetAliasesRequest::runAsync ()
{
    Q_D(QSnapdResetAliasesRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->aliases);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_reset_aliases_async (SNAPD_CLIENT (getClient ()),
                                      d->snap.toStdString ().c_str (), aliases,
                                      progress_cb, d->callback_data,
                                      G_CANCELLABLE (getCancellable ()), reset_aliases_ready_cb, g_object_ref (d->callback_data));
G_GNUC_END_IGNORE_DEPRECATIONS
}

QSnapdRunSnapCtlRequest::QSnapdRunSnapCtlRequest (const QString& contextId, const QStringList& args, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRunSnapCtlRequestPrivate (this, contextId, args)) {}

void QSnapdRunSnapCtlRequest::runSync ()
{
    Q_D(QSnapdRunSnapCtlRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->args);
    g_autoptr(GError) error = NULL;
    snapd_client_run_snapctl2_sync (SNAPD_CLIENT (getClient ()),
                                    d->contextId.toStdString ().c_str (), aliases,
                                    &d->stdout_output, &d->stderr_output,
                                    &d->exit_code,
                                    G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRunSnapCtlRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdRunSnapCtlRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_run_snapctl2_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &d->stdout_output, &d->stderr_output, &d->exit_code, &error);
    finish (error);
}

static void run_snapctl_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdRunSnapCtlRequest *request = static_cast<QSnapdRunSnapCtlRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdRunSnapCtlRequest::runAsync ()
{
    Q_D(QSnapdRunSnapCtlRequest);

    g_auto(GStrv) aliases = string_list_to_strv (d->args);
    snapd_client_run_snapctl2_async (SNAPD_CLIENT (getClient ()),
                                     d->contextId.toStdString ().c_str (), aliases,
                                     G_CANCELLABLE (getCancellable ()), run_snapctl_ready_cb, g_object_ref (d->callback_data));
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

int QSnapdRunSnapCtlRequest::exitCode () const
{
    Q_D(const QSnapdRunSnapCtlRequest);
    return d->exit_code;
}

QSnapdDownloadRequest::QSnapdDownloadRequest (const QString& name, const QString &channel, const QString &revision, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdDownloadRequestPrivate (this, name, channel, revision)) {}

void QSnapdDownloadRequest::runSync ()
{
    Q_D(QSnapdDownloadRequest);

    g_autoptr(GError) error = NULL;
    d->data = snapd_client_download_sync (SNAPD_CLIENT (getClient ()),
                                          d->name.toStdString ().c_str (),
                                          d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                          d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                          G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdDownloadRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdDownloadRequest);

    g_autoptr(GError) error = NULL;
    d->data = snapd_client_download_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void download_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
      if (callback_data->request != NULL) {
        QSnapdDownloadRequest *request = static_cast<QSnapdDownloadRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdDownloadRequest::runAsync ()
{
    Q_D(QSnapdDownloadRequest);

    snapd_client_download_async (SNAPD_CLIENT (getClient ()),
                                 d->name.toStdString ().c_str (),
                                 d->channel.isNull () ? NULL : d->channel.toStdString ().c_str (),
                                 d->revision.isNull () ? NULL : d->revision.toStdString ().c_str (),
                                 G_CANCELLABLE (getCancellable ()), download_ready_cb, g_object_ref (d->callback_data));
}

QByteArray QSnapdDownloadRequest::data () const
{
    Q_D(const QSnapdDownloadRequest);

    gsize length;
    gchar *raw_data = (gchar *) g_bytes_get_data (d->data, &length);
    return QByteArray::fromRawData (raw_data, length);
}

QSnapdCheckThemesRequest::QSnapdCheckThemesRequest (const QStringList& gtkThemeNames, const QStringList& iconThemeNames, const QStringList& soundThemeNames, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdCheckThemesRequestPrivate (this, gtkThemeNames, iconThemeNames, soundThemeNames)) {}

void QSnapdCheckThemesRequest::runSync ()
{
    Q_D(QSnapdCheckThemesRequest);

    g_auto(GStrv) gtk_theme_names = string_list_to_strv (d->gtkThemeNames);
    g_auto(GStrv) icon_theme_names = string_list_to_strv (d->iconThemeNames);
    g_auto(GStrv) sound_theme_names = string_list_to_strv (d->soundThemeNames);
    g_autoptr(GError) error = NULL;
    snapd_client_check_themes_sync (SNAPD_CLIENT (getClient ()),
                                    gtk_theme_names,
                                    icon_theme_names,
                                    sound_theme_names,
                                    &d->gtk_theme_status,
                                    &d->icon_theme_status,
                                    &d->sound_theme_status,
                                    G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdCheckThemesRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdCheckThemesRequest);

    g_autoptr(GError) error = NULL;
    snapd_client_check_themes_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &d->gtk_theme_status, &d->icon_theme_status, &d->sound_theme_status, &error);
    finish (error);
}

static void check_themes_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
      if (callback_data->request != NULL) {
        QSnapdCheckThemesRequest *request = static_cast<QSnapdCheckThemesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdCheckThemesRequest::runAsync ()
{
    Q_D(QSnapdCheckThemesRequest);

    g_auto(GStrv) gtk_theme_names = string_list_to_strv (d->gtkThemeNames);
    g_auto(GStrv) icon_theme_names = string_list_to_strv (d->iconThemeNames);
    g_auto(GStrv) sound_theme_names = string_list_to_strv (d->soundThemeNames);
    snapd_client_check_themes_async (SNAPD_CLIENT (getClient ()),
                                     gtk_theme_names,
                                     icon_theme_names,
                                     sound_theme_names,
                                     G_CANCELLABLE (getCancellable ()), check_themes_ready_cb, g_object_ref (d->callback_data));
}

static QSnapdCheckThemesRequest::ThemeStatus convertThemeStatus (int status)
{
    switch (status)
    {
    case SNAPD_THEME_STATUS_INSTALLED:
        return QSnapdCheckThemesRequest::ThemeInstalled;
    case SNAPD_THEME_STATUS_AVAILABLE:
        return QSnapdCheckThemesRequest::ThemeAvailable;
    default:
    case SNAPD_THEME_STATUS_UNAVAILABLE:
        return QSnapdCheckThemesRequest::ThemeUnavailable;
    }
}

QSnapdCheckThemesRequest::ThemeStatus QSnapdCheckThemesRequest::gtkThemeStatus (const QString& name) const
{
    Q_D(const QSnapdCheckThemesRequest);

    return convertThemeStatus (GPOINTER_TO_INT (g_hash_table_lookup (d->gtk_theme_status, name.toStdString ().c_str ())));
}

QSnapdCheckThemesRequest::ThemeStatus QSnapdCheckThemesRequest::iconThemeStatus (const QString& name) const
{
    Q_D(const QSnapdCheckThemesRequest);

    return convertThemeStatus (GPOINTER_TO_INT (g_hash_table_lookup (d->icon_theme_status, name.toStdString ().c_str ())));
}

QSnapdCheckThemesRequest::ThemeStatus QSnapdCheckThemesRequest::soundThemeStatus (const QString& name) const
{
    Q_D(const QSnapdCheckThemesRequest);

    return convertThemeStatus (GPOINTER_TO_INT (g_hash_table_lookup (d->sound_theme_status, name.toStdString ().c_str ())));
}

QSnapdInstallThemesRequest::QSnapdInstallThemesRequest (const QStringList& gtkThemeNames, const QStringList& iconThemeNames, const QStringList& soundThemeNames, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdInstallThemesRequestPrivate (this, gtkThemeNames, iconThemeNames, soundThemeNames)) {}

void QSnapdInstallThemesRequest::runSync ()
{
    Q_D(QSnapdInstallThemesRequest);

    g_auto(GStrv) gtk_theme_names = string_list_to_strv (d->gtkThemeNames);
    g_auto(GStrv) icon_theme_names = string_list_to_strv (d->iconThemeNames);
    g_auto(GStrv) sound_theme_names = string_list_to_strv (d->soundThemeNames);
    g_autoptr(GError) error = NULL;
    snapd_client_install_themes_sync (SNAPD_CLIENT (getClient ()),
                                      gtk_theme_names,
                                      icon_theme_names,
                                      sound_theme_names,
                                      progress_cb, d->callback_data,
                                      G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdInstallThemesRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_install_themes_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    finish (error);
}

static void install_themes_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
      if (callback_data->request != NULL) {
        QSnapdInstallThemesRequest *request = static_cast<QSnapdInstallThemesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdInstallThemesRequest::runAsync ()
{
    Q_D(QSnapdInstallThemesRequest);

    g_auto(GStrv) gtk_theme_names = string_list_to_strv (d->gtkThemeNames);
    g_auto(GStrv) icon_theme_names = string_list_to_strv (d->iconThemeNames);
    g_auto(GStrv) sound_theme_names = string_list_to_strv (d->soundThemeNames);
    snapd_client_install_themes_async (SNAPD_CLIENT (getClient ()),
                                       gtk_theme_names,
                                       icon_theme_names,
                                       sound_theme_names,
                                       progress_cb, d->callback_data,
                                       G_CANCELLABLE (getCancellable ()), install_themes_ready_cb, g_object_ref (d->callback_data));
}

QSnapdNoticesRequest *QSnapdClient::getNotices ()
{
    Q_D(QSnapdClient);
    return new QSnapdNoticesRequest (d->client);
}

QSnapdNoticesRequest::QSnapdNoticesRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdNoticesRequestPrivate (this)) {
        this->timeout = 0;
        this->resetFilters();
    }

QSnapdNoticesRequest::~QSnapdNoticesRequest ()
{}

void QSnapdNoticesRequest::resetFilters ()
{
    this->sinceFilterSet = false;
    this->userIdFilter = "";
    this->usersFilter = "";
    this->typesFilter = "";
    this->keysFilter = "";
    this->sinceDateFilter = "";
}

static GDateTime *
getSinceDateTime (QDateTime &dateTime)
{
#if GLIB_CHECK_VERSION(2, 58, 0)
    g_autoptr (GTimeZone) timeZone = g_time_zone_new_offset (dateTime.timeZone().offsetFromUtc(dateTime));
#else
    int timeOffset = dateTime.timeZone().offsetFromUtc(dateTime) / 60;
    g_autofree gchar *timeOffsetStr = g_strdup_printf("%02d:%02d", timeOffset / 60, timeOffset % 60);
    g_autoptr (GTimeZone) timeZone = g_time_zone_new (timeOffsetStr);
#endif
    GDateTime *gDateTime = g_date_time_new (timeZone,
                                            dateTime.date().year(),
                                            dateTime.date().month(),
                                            dateTime.date().day(),
                                            dateTime.time().hour(),
                                            dateTime.time().minute(),
                                            dateTime.time().second());
    return gDateTime;
}

void QSnapdNoticesRequest::runSync ()
{
    Q_D(QSnapdNoticesRequest);

    if (this->sinceDateFilter != "")
        snapd_client_notices_set_since_date (SNAPD_CLIENT (getClient ()), this->sinceDateFilter.toStdString().c_str());
    this->sinceDateFilter = "";
    g_autoptr(GError) error = NULL;
    g_autoptr (GDateTime) dateTime = this->sinceFilterSet ? getSinceDateTime (this->sinceFilter) : NULL;
    d->updateNoticesData (snapd_client_get_notices_with_filters_sync (SNAPD_CLIENT (getClient ()),
                                                                      (gchar *) this->userIdFilter.toStdString ().c_str (),
                                                                      (gchar *) this->usersFilter.toStdString ().c_str (),
                                                                      (gchar *) this->typesFilter.toStdString ().c_str (),
                                                                      (gchar *) this->keysFilter.toStdString ().c_str (),
                                                                      dateTime,
                                                                      this->timeout,
                                                                      G_CANCELLABLE (getCancellable ()),
                                                                      &error));
    finish (error);
}

void QSnapdNoticesRequest::handleResult (void *object, void *result)
{
    Q_D(QSnapdNoticesRequest);

    g_autoptr(GError) error = NULL;
    d->updateNoticesData (snapd_client_get_notices_with_filters_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error));
    finish (error);
}

static void notices_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    g_autoptr(CallbackData) callback_data = (CallbackData *) data;
    if (callback_data->request != NULL) {
        QSnapdNoticesRequest *request = static_cast<QSnapdNoticesRequest*>(callback_data->request);
        request->handleResult (object, result);
    }
}

void QSnapdNoticesRequest::runAsync ()
{
    Q_D(QSnapdNoticesRequest);

    if (this->sinceDateFilter != "")
        snapd_client_notices_set_since_date (SNAPD_CLIENT (getClient ()), this->sinceDateFilter.toStdString().c_str());
    this->sinceDateFilter = "";
    g_autoptr (GDateTime) dateTime = this->sinceFilterSet ? getSinceDateTime (this->sinceFilter) : NULL;
    snapd_client_get_notices_with_filters_async (SNAPD_CLIENT (getClient ()),
                                                 (gchar *) this->userIdFilter.toStdString ().c_str (),
                                                 (gchar *) this->usersFilter.toStdString ().c_str (),
                                                 (gchar *) this->typesFilter.toStdString ().c_str (),
                                                 (gchar *) this->keysFilter.toStdString ().c_str (),
                                                 dateTime,
                                                 this->timeout,
                                                 G_CANCELLABLE (getCancellable ()),
                                                 notices_ready_cb,
                                                 g_object_ref (d->callback_data));
}

uint QSnapdNoticesRequest::noticesCount () const
{
    Q_D(const QSnapdNoticesRequest);
    return d->notices->len;
}

QSnapdNotice *QSnapdNoticesRequest::getNotice (quint64 n) const
{
    Q_D(const QSnapdNoticesRequest);

    if (d->notices == NULL || n >= d->notices->len)
        return NULL;
    return new QSnapdNotice (SNAPD_NOTICE(d->notices->pdata[n]));
}
