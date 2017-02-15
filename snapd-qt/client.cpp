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

struct QSnapdClientPrivate
{
    QSnapdClientPrivate ()
    {
        client = snapd_client_new ();
    }

    ~QSnapdClientPrivate ()
    {
        g_object_unref (client);
    }

    SnapdClient *client;
};

struct QSnapdConnectRequestPrivate
{
};

struct QSnapdLoginRequestPrivate
{
    QSnapdLoginRequestPrivate (const QString& username, const QString& password, const QString& otp) :
        username(username), password(password), otp(otp) {}
    ~QSnapdLoginRequestPrivate ()
    {
        g_object_unref (auth_data);
    }
    QString username;
    QString password;
    QString otp;
    SnapdAuthData *auth_data;
};

struct QSnapdGetSystemInformationRequestPrivate
{
    ~QSnapdGetSystemInformationRequestPrivate ()
    {
        g_object_unref (info);
    }
    SnapdSystemInformation *info;
};

struct QSnapdListRequestPrivate
{
    ~QSnapdListRequestPrivate ()
    {
        g_ptr_array_unref (snaps);
    }
    GPtrArray *snaps;
};

struct QSnapdListOneRequestPrivate
{
    QSnapdListOneRequestPrivate (const QString& name) :
        name(name) {}
    ~QSnapdListOneRequestPrivate ()
    {
        g_object_unref (snap);
    }
    QString name;
    SnapdSnap *snap;
};

struct QSnapdGetIconRequestPrivate
{
    QSnapdGetIconRequestPrivate (const QString& name) :
        name(name) {}
    ~QSnapdGetIconRequestPrivate ()
    {
        g_object_unref (icon);
    }
    QString name;
    SnapdIcon *icon;
};

struct QSnapdGetInterfacesRequestPrivate
{
    ~QSnapdGetInterfacesRequestPrivate ()
    {
        g_ptr_array_unref (plugs);
        g_ptr_array_unref (slots_);
    }
    GPtrArray *plugs;
    GPtrArray *slots_;
};

struct QSnapdConnectInterfaceRequestPrivate
{
    QSnapdConnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

struct QSnapdDisconnectInterfaceRequestPrivate
{
    QSnapdDisconnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

struct QSnapdFindRequestPrivate
{
    QSnapdFindRequestPrivate (int flags, const QString& section, const QString& name) :
        flags (flags), section (section), name (name) {}
    ~QSnapdFindRequestPrivate ()
    {
        g_ptr_array_unref (snaps);
    }
    int flags;
    QString section;  
    QString name;
    GPtrArray *snaps;
    QString suggestedCurrency;
};

struct QSnapdFindRefreshableRequestPrivate
{
    QSnapdFindRefreshableRequestPrivate () {}
    ~QSnapdFindRefreshableRequestPrivate ()
    {
        g_ptr_array_unref (snaps);
    }
    GPtrArray *snaps;
};

struct QSnapdInstallRequestPrivate
{
    QSnapdInstallRequestPrivate (const QString& name, const QString& channel) :
        name(name), channel(channel) {}
    QString name;
    QString channel;
};

struct QSnapdRefreshRequestPrivate
{
    QSnapdRefreshRequestPrivate (const QString& name, const QString& channel) :
        name(name), channel(channel) {}
    QString name;
    QString channel;
};

struct QSnapdRefreshAllRequestPrivate
{
    QSnapdRefreshAllRequestPrivate () {}
};

struct QSnapdRemoveRequestPrivate
{
    QSnapdRemoveRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdEnableRequestPrivate
{
    QSnapdEnableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdDisableRequestPrivate
{
    QSnapdDisableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdCheckBuyRequestPrivate
{
    bool canBuy;
};

struct QSnapdBuyRequestPrivate
{
    QSnapdBuyRequestPrivate (const QString& id, double amount, const QString& currency) :
      id(id), amount(amount), currency(currency) {}
    QString id;
    double amount;
    QString currency;
};

struct QSnapdGetSectionsRequestPrivate
{
    ~QSnapdGetSectionsRequestPrivate ()
    {
        g_strfreev (sections);
    }
    gchar **sections;
};

struct QSnapdGetAliasesRequestPrivate
{
    ~QSnapdGetAliasesRequestPrivate ()
    {
        g_ptr_array_unref (aliases);
    }
    GPtrArray *aliases;
};

struct QSnapdEnableAliasesRequestPrivate
{
    QSnapdEnableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdDisableAliasesRequestPrivate
{
    QSnapdDisableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdResetAliasesRequestPrivate
{
    QSnapdResetAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdRunSnapCtlRequestPrivate
{
    QSnapdRunSnapCtlRequestPrivate (const QString &contextId, const QStringList& args) :
        contextId (contextId), args (args) {}
    ~QSnapdRunSnapCtlRequestPrivate ()
    {
        g_object_unref (output);
    }
    QString contextId;
    QStringList args;
    SnapdSnapCtlOutput *output;
};

QSnapdClient::QSnapdClient(QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdClientPrivate()) {}

QSnapdConnectRequest *QSnapdClient::connect ()
{
    Q_D(QSnapdClient);
    return new QSnapdConnectRequest (d->client);
}

QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp)
{
    return new QSnapdLoginRequest (NULL, username, password, otp);
}

QSnapdLoginRequest *QSnapdClient::login (const QString& username, const QString& password, const QString& otp)
{
    Q_D(QSnapdClient);
    return new QSnapdLoginRequest (d->client, username, password, otp);
}

QSnapdGetSystemInformationRequest *QSnapdClient::getSystemInformation ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSystemInformationRequest (d->client);
}

QSnapdListRequest *QSnapdClient::list ()
{
    Q_D(QSnapdClient);
    return new QSnapdListRequest (d->client);
}

QSnapdListOneRequest *QSnapdClient::listOne (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdListOneRequest (name, d->client);
}

QSnapdGetIconRequest *QSnapdClient::getIcon (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdGetIconRequest (name, d->client);
}

QSnapdGetInterfacesRequest *QSnapdClient::getInterfaces ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetInterfacesRequest (d->client);
}

QSnapdConnectInterfaceRequest *QSnapdClient::connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdConnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdDisconnectInterfaceRequest *QSnapdClient::disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisconnectInterfaceRequest (plug_snap, plug_name, slot_snap, slot_name, d->client);
}

QSnapdFindRequest *QSnapdClient::find (FindFlags flags, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, QString (), name, d->client);
}

QSnapdFindRequest *QSnapdClient::findSection (FindFlags flags, const QString &section, const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, section, name, d->client);
}

QSnapdFindRefreshableRequest *QSnapdClient::findRefreshable ()
{
    Q_D(QSnapdClient);
    return new QSnapdFindRefreshableRequest (d->client);
}

QSnapdInstallRequest *QSnapdClient::install (const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (name, channel, d->client);
}

QSnapdRefreshRequest *QSnapdClient::refresh (const QString& name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshRequest (name, channel, d->client);
}

QSnapdRefreshAllRequest *QSnapdClient::refreshAll ()
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshAllRequest (d->client);
}

QSnapdRemoveRequest *QSnapdClient::remove (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdRemoveRequest (name, d->client);
}

QSnapdEnableRequest *QSnapdClient::enable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableRequest (name, d->client);
}

QSnapdDisableRequest *QSnapdClient::disable (const QString& name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableRequest (name, d->client);
}

QSnapdCheckBuyRequest *QSnapdClient::checkBuy ()
{
    Q_D(QSnapdClient);
    return new QSnapdCheckBuyRequest (d->client);
}

QSnapdBuyRequest *QSnapdClient::buy (const QString& id, double amount, const QString& currency)
{
    Q_D(QSnapdClient);
    return new QSnapdBuyRequest (id, amount, currency, d->client);
}

QSnapdGetSectionsRequest *QSnapdClient::getSections ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetSectionsRequest (d->client);
}

QSnapdGetAliasesRequest *QSnapdClient::getAliases ()
{
    Q_D(QSnapdClient);
    return new QSnapdGetAliasesRequest (d->client);
}

QSnapdEnableAliasesRequest *QSnapdClient::enableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableAliasesRequest (snap, aliases, d->client);
}

QSnapdDisableAliasesRequest *QSnapdClient::disableAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableAliasesRequest (snap, aliases, d->client);
}

QSnapdResetAliasesRequest *QSnapdClient::resetAliases (const QString snap, const QStringList &aliases)
{
    Q_D(QSnapdClient);
    return new QSnapdResetAliasesRequest (snap, aliases, d->client);
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
        snapd_client_login_sync (SNAPD_CLIENT (getClient ()), d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    else
        snapd_login_sync (d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdLoginRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    auth_data = snapd_client_login_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
        snapd_client_login_async (SNAPD_CLIENT (getClient ()), d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
    else
        snapd_login_async (d->username.toStdString ().c_str (), d->password.toStdString ().c_str (), d->otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    d->snap = snapd_client_list_one_sync (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdListOneRequest::handleResult (void *object, void *result)
{
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snap = snapd_client_list_one_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    snapd_client_list_one_async (SNAPD_CLIENT (getClient ()), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, (gpointer) this);
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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

QSnapdConnection *QSnapdGetInterfacesRequest::plug (int n) const
{
    Q_D(const QSnapdGetInterfacesRequest);
    if (d->plugs == NULL || n < 0 || (guint) n >= d->plugs->len)
        return NULL;
    return new QSnapdConnection (d->plugs->pdata[n]);
}

int QSnapdGetInterfacesRequest::slotCount () const
{
    Q_D(const QSnapdGetInterfacesRequest);
    return d->slots_ != NULL ? d->slots_->len : 0;
}

QSnapdConnection *QSnapdGetInterfacesRequest::slot (int n) const
{
    Q_D(const QSnapdGetInterfacesRequest);
    if (d->slots_ == NULL || n < 0 || (guint) n >= d->slots_->len)
        return NULL;
    return new QSnapdConnection (d->slots_->pdata[n]);
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
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    d->snaps = snapd_client_find_section_sync (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.toStdString().c_str (), d->name.toStdString ().c_str (), &suggested_currency, G_CANCELLABLE (getCancellable ()), &error);
    d->suggestedCurrency = suggested_currency;
    finish (error);
}

void QSnapdFindRequest::handleResult (void *object, void *result)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_find_section_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &suggested_currency, &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    snapd_client_find_section_async (SNAPD_CLIENT (getClient ()), convertFindFlags (d->flags), d->section.toStdString ().c_str (), d->name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), find_ready_cb, (gpointer) this);
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

QSnapdInstallRequest::QSnapdInstallRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdInstallRequestPrivate (name, channel)) {}

void QSnapdInstallRequest::runSync ()
{
    Q_D(QSnapdInstallRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_install_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (), d->channel.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdInstallRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_install_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    snapd_client_install_async (SNAPD_CLIENT (getClient ()),
                                d->name.toStdString ().c_str (), d->channel.toStdString ().c_str (),
                                progress_cb, this,
                                G_CANCELLABLE (getCancellable ()), install_ready_cb, (gpointer) this);
}

QSnapdRefreshRequest::QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshRequestPrivate (name, channel)) {}

void QSnapdRefreshRequest::runSync ()
{
    Q_D(QSnapdRefreshRequest);
    g_autoptr(GError) error = NULL;
    snapd_client_refresh_sync (SNAPD_CLIENT (getClient ()),
                               d->name.toStdString ().c_str (), d->channel.toStdString ().c_str (),
                               progress_cb, this,
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRefreshRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_refresh_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
                                d->name.toStdString ().c_str (), d->channel.toStdString ().c_str (),
                                progress_cb, this,
                                G_CANCELLABLE (getCancellable ()), refresh_ready_cb, (gpointer) this);
}

QSnapdRefreshAllRequest::QSnapdRefreshAllRequest (void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdRefreshAllRequestPrivate ()) {}

void QSnapdRefreshAllRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_refresh_all_sync (SNAPD_CLIENT (getClient ()),
                                   progress_cb, this,
                                   G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRefreshAllRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;

    snapd_client_refresh_all_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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

static gchar **
string_list_to_strv (const QStringList& list)
{
    gchar **value;
    int size, i;

    size = list.size ();
    value = (gchar **) malloc (sizeof (gchar *) * (size + 1));
    for (i = 0; i < size; i++)
        value[i] = (gchar *) list[i].toStdString ().c_str ();
    value[size] = NULL;

    return value;
}

QSnapdEnableAliasesRequest::QSnapdEnableAliasesRequest (const QString& name, const QStringList& aliases, void *snapd_client, QObject *parent) :
    QSnapdRequest (snapd_client, parent),
    d_ptr (new QSnapdEnableAliasesRequestPrivate (name, aliases)) {}

void QSnapdEnableAliasesRequest::runSync ()
{
    Q_D(QSnapdEnableAliasesRequest);
    g_autofree gchar **aliases = NULL;
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    g_autofree gchar **aliases = NULL;

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
    g_autofree gchar **aliases = NULL;
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    g_autofree gchar **aliases = NULL;

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
    g_autofree gchar **aliases = NULL;
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    g_autofree gchar **aliases = NULL;

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
    g_autofree gchar **aliases = NULL;
    g_autoptr(GError) error = NULL;

    aliases = string_list_to_strv (d->args);
    snapd_client_run_snapctl_sync (SNAPD_CLIENT (getClient ()),
                                   d->contextId.toStdString ().c_str (), aliases,
                                   G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdRunSnapCtlRequest::handleResult (void *object, void *result)
{
    g_autoptr(GError) error = NULL;
    snapd_client_run_snapctl_finish (SNAPD_CLIENT (object), G_ASYNC_RESULT (result), &error);
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        return;

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
    g_autofree gchar **aliases = NULL;

    aliases = string_list_to_strv (d->args);
    snapd_client_run_snapctl_async (SNAPD_CLIENT (getClient ()),
                                    d->contextId.toStdString ().c_str (), aliases,
                                    G_CANCELLABLE (getCancellable ()), run_snapctl_ready_cb, (gpointer) this);
}

QString QSnapdRunSnapCtlRequest::stdout () const
{
    Q_D(const QSnapdRunSnapCtlRequest);
    return QString (snapd_snapctl_output_get_stdout (d->output));
}

QString QSnapdRunSnapCtlRequest::stderr () const
{
    Q_D(const QSnapdRunSnapCtlRequest);
    return QString (snapd_snapctl_output_get_stderr (d->output));
}
