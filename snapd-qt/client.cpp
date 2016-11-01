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

QSnapdClient::QSnapdClient(QObject *parent) :
    QObject (parent),
    d_ptr (new QSnapdClientPrivate())
{
}

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

QSnapdSystemInformationRequest *QSnapdClient::getSystemInformation ()
{
    Q_D(QSnapdClient);
    return new QSnapdSystemInformationRequest (d->client);
}

QSnapdListRequest *QSnapdClient::list ()
{
    Q_D(QSnapdClient);
    return new QSnapdListRequest (d->client);
}

QSnapdListOneRequest *QSnapdClient::listOne (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdListOneRequest (name, d->client);
}

QSnapdIconRequest *QSnapdClient::getIcon (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdIconRequest (name, d->client);
}

QSnapdInterfacesRequest *QSnapdClient::getInterfaces ()
{
    Q_D(QSnapdClient);
    return new QSnapdInterfacesRequest (d->client);
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

QSnapdFindRequest *QSnapdClient::find (FindFlags flags, const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdFindRequest (flags, name, d->client);
}

QSnapdInstallRequest *QSnapdClient::install (const QString &name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdInstallRequest (name, channel, d->client);
}

QSnapdRefreshRequest *QSnapdClient::refresh (const QString &name, const QString& channel)
{
    Q_D(QSnapdClient);
    return new QSnapdRefreshRequest (name, channel, d->client);
}

QSnapdRemoveRequest *QSnapdClient::remove (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdRemoveRequest (name, d->client);
}

QSnapdEnableRequest *QSnapdClient::enable (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdEnableRequest (name, d->client);
}

QSnapdDisableRequest *QSnapdClient::disable (const QString &name)
{
    Q_D(QSnapdClient);
    return new QSnapdDisableRequest (name, d->client);
}

void QSnapdConnectRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_connect_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void QSnapdConnectRequest::runAsync ()
{
    // NOTE: No async method supported
}

void QSnapdLoginRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    if (getClient () != NULL)
        snapd_client_login_sync (SNAPD_CLIENT (getClient ()), username.toStdString ().c_str (), password.toStdString ().c_str (), otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    else
        snapd_login_sync (username.toStdString ().c_str (), password.toStdString ().c_str (), otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);  
    finish (error);
}

static void login_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdLoginRequest *request = (QSnapdLoginRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_get_login_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdLoginRequest::runAsync ()
{
    if (getClient () != NULL)
        snapd_client_login_async (SNAPD_CLIENT (getClient ()), username.toStdString ().c_str (), password.toStdString ().c_str (), otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
    else
        snapd_login_async (username.toStdString ().c_str (), password.toStdString ().c_str (), otp.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), login_ready_cb, (gpointer) this);
}

void QSnapdSystemInformationRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_get_system_information_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void system_information_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdSystemInformationRequest *request = (QSnapdSystemInformationRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdSystemInformationRequest::runAsync ()
{
    snapd_client_get_system_information_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), system_information_ready_cb, (gpointer) this);
}

QSnapdSystemInformation *QSnapdSystemInformationRequest::systemInformation ()
{
    return new QSnapdSystemInformation (result);
}

void QSnapdListRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdListRequest *request = (QSnapdListRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_list_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdListRequest::runAsync ()
{
    snapd_client_list_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), list_ready_cb, (gpointer) this);
}

int QSnapdListRequest::snapCount () const
{
    GPtrArray *array = (GPtrArray *) result;
    return array->len;
}

QSnapdSnap *QSnapdListRequest::snap (int n) const
{
    GPtrArray *array = (GPtrArray *) result;
    if (n < 0 || (guint) n >= array->len)
        return NULL;
    return new QSnapdSnap (array->pdata[n]);
}

void QSnapdListOneRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_one_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_one_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdListOneRequest *request = (QSnapdListOneRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_list_one_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdListOneRequest::runAsync ()
{
    snapd_client_list_one_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, (gpointer) this);
}

QSnapdSnap *QSnapdListOneRequest::snap () const
{
    return new QSnapdSnap (result);
}

void QSnapdIconRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_get_icon_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void icon_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdIconRequest *request = (QSnapdIconRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_get_icon_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdIconRequest::runAsync ()
{
    snapd_client_get_icon_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), icon_ready_cb, (gpointer) this);
}

QSnapdIcon *QSnapdIconRequest::icon () const
{
    return new QSnapdIcon (result);
}

void QSnapdInterfacesRequest::runSync ()
{
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_get_interfaces_sync (SNAPD_CLIENT (getClient ()), &plugs, &slots_, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void interfaces_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdInterfacesRequest *request = (QSnapdInterfacesRequest *) data;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_get_interfaces_finish (SNAPD_CLIENT (object), &plugs, &slots_, result, &error);
    request->finish (error);*/
}

void QSnapdInterfacesRequest::runAsync ()
{
    snapd_client_get_interfaces_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), interfaces_ready_cb, (gpointer) this);
}

int QSnapdInterfacesRequest::plugCount () const
{
    GPtrArray *array = (GPtrArray *) plugs;
    return array->len;
}

QSnapdConnection *QSnapdInterfacesRequest::plug (int n) const
{
    GPtrArray *array = (GPtrArray *) plugs;
    if (n < 0 || (guint) n >= array->len)
        return NULL;
    return new QSnapdConnection (array->pdata[n]);
}

int QSnapdInterfacesRequest::slotCount () const
{
    GPtrArray *array = (GPtrArray *) slots_;
    return array->len;
}

QSnapdConnection *QSnapdInterfacesRequest::slot (int n) const
{
    GPtrArray *array = (GPtrArray *) slots_;
    if (n < 0 || (guint) n >= array->len)
        return NULL;
    return new QSnapdConnection (array->pdata[n]);
}

void QSnapdConnectInterfaceRequest::runSync ()
{
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_connect_interface_sync (SNAPD_CLIENT (getClient ()),
                                         plug_snap.toStdString ().c_str (), plug_name.toStdString ().c_str (),
                                         slot_snap.toStdString ().c_str (), slot_name.toStdString ().c_str (),
                                         NULL, NULL, // FIXME: Progress
                                         G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void connect_interface_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdConnectInterfaceRequest *request = (QSnapdConnectInterfaceRequest *) data;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_connect_interface_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdConnectInterfaceRequest::runAsync ()
{
    snapd_client_connect_interface_async (SNAPD_CLIENT (getClient ()),
                                          plug_snap.toStdString ().c_str (), plug_name.toStdString ().c_str (),
                                          slot_snap.toStdString ().c_str (), slot_name.toStdString ().c_str (),
                                          NULL, NULL, // FIXME: Progress
                                          G_CANCELLABLE (getCancellable ()), connect_interface_ready_cb, (gpointer) this);
}

void QSnapdDisconnectInterfaceRequest::runSync ()
{
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_disconnect_interface_sync (SNAPD_CLIENT (getClient ()),
                                            plug_snap.toStdString ().c_str (), plug_name.toStdString ().c_str (),
                                            slot_snap.toStdString ().c_str (), slot_name.toStdString ().c_str (),
                                            NULL, NULL, // FIXME: Progress
                                            G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void disconnect_interface_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdDisconnectInterfaceRequest *request = (QSnapdDisconnectInterfaceRequest *) data;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;
    g_autoptr(GError) error = NULL;
    snapd_client_disconnect_interface_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdDisconnectInterfaceRequest::runAsync ()
{
    snapd_client_disconnect_interface_async (SNAPD_CLIENT (getClient ()),
                                             plug_snap.toStdString ().c_str (), plug_name.toStdString ().c_str (),
                                             slot_snap.toStdString ().c_str (), slot_name.toStdString ().c_str (),
                                             NULL, NULL, // FIXME: Progress
                                             G_CANCELLABLE (getCancellable ()), disconnect_interface_ready_cb, (gpointer) this);
}

static SnapdFindFlags convertFindFlags (int flags)
{
    int result = SNAPD_FIND_FLAGS_NONE;

    if ((flags & QSnapdClient::FindFlag::MatchName) != 0)
        result |= SNAPD_FIND_FLAGS_MATCH_NAME;
    if ((flags & QSnapdClient::FindFlag::SelectPrivate) != 0)
        result |= SNAPD_FIND_FLAGS_SELECT_PRIVATE;
    if ((flags & QSnapdClient::FindFlag::SelectRefresh) != 0)
        result |= SNAPD_FIND_FLAGS_SELECT_REFRESH;

    return (SnapdFindFlags) result;
}

void QSnapdFindRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *suggested_currency = NULL;
    result = snapd_client_find_sync (SNAPD_CLIENT (getClient ()), convertFindFlags (flags), name.toStdString ().c_str (), &suggested_currency, G_CANCELLABLE (getCancellable ()), &error);
    suggestedCurrency_ = suggested_currency;
    finish (error);
}

static void find_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdFindRequest *request = (QSnapdFindRequest *) data;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *suggested_currency = NULL;
    request->snapd_client_find_finish (SNAPD_CLIENT (object), result, &suggested_currency, &error);
    request->finish (error);*/
}

void QSnapdFindRequest::runAsync ()
{
    snapd_client_find_async (SNAPD_CLIENT (getClient ()), convertFindFlags (flags), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), find_ready_cb, (gpointer) this);
}

int QSnapdFindRequest::snapCount () const
{
    GPtrArray *array = (GPtrArray *) result;
    return array->len;
}

QSnapdSnap *QSnapdFindRequest::snap (int n) const
{
    GPtrArray *array = (GPtrArray *) result;
    if (n < 0 || (guint) n >= array->len)
        return NULL;
    return new QSnapdSnap (array->pdata[n]);
}

const QString QSnapdFindRequest::suggestedCurrency () const
{
    return suggestedCurrency_;
}

void QSnapdInstallRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_install_sync (SNAPD_CLIENT (getClient ()),
                               name.toStdString ().c_str (), channel.toStdString ().c_str (),
                               NULL, NULL, // FIXME: Progress
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void install_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdInstallRequest *request = (QSnapdInstallRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_install_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdInstallRequest::runAsync ()
{
    snapd_client_install_async (SNAPD_CLIENT (getClient ()),
                                name.toStdString ().c_str (), channel.toStdString ().c_str (),
                                NULL, NULL, // FIXME: Progress
                                G_CANCELLABLE (getCancellable ()), install_ready_cb, (gpointer) this);
}

void QSnapdRefreshRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_refresh_sync (SNAPD_CLIENT (getClient ()),
                               name.toStdString ().c_str (), channel.toStdString ().c_str (),
                               NULL, NULL, // FIXME: Progress
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void refresh_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdRefreshRequest *request = (QSnapdRefreshRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_refresh_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdRefreshRequest::runAsync ()
{
    snapd_client_refresh_async (SNAPD_CLIENT (getClient ()),
                                name.toStdString ().c_str (), channel.toStdString ().c_str (),
                                NULL, NULL, // FIXME: Progress
                                G_CANCELLABLE (getCancellable ()), refresh_ready_cb, (gpointer) this);
}

void QSnapdRemoveRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_remove_sync (SNAPD_CLIENT (getClient ()),
                              name.toStdString ().c_str (),
                              NULL, NULL, // FIXME: Progress
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void remove_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdRemoveRequest *request = (QSnapdRemoveRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_remove_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdRemoveRequest::runAsync ()
{
    snapd_client_remove_async (SNAPD_CLIENT (getClient ()),
                               name.toStdString ().c_str (),
                               NULL, NULL, // FIXME: Progress
                               G_CANCELLABLE (getCancellable ()), remove_ready_cb, (gpointer) this);
}

void QSnapdEnableRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_enable_sync (SNAPD_CLIENT (getClient ()),
                              name.toStdString ().c_str (),
                              NULL, NULL, // FIXME: Progress
                              G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void enable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdEnableRequest *request = (QSnapdEnableRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_enable_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdEnableRequest::runAsync ()
{
    snapd_client_enable_async (SNAPD_CLIENT (getClient ()),
                               name.toStdString ().c_str (),
                               NULL, NULL, // FIXME: Progress
                               G_CANCELLABLE (getCancellable ()), enable_ready_cb, (gpointer) this);
}

void QSnapdDisableRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_disable_sync (SNAPD_CLIENT (getClient ()),
                               name.toStdString ().c_str (),
                               NULL, NULL, // FIXME: Progress
                               G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void disable_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*QSnapdDisableRequest *request = (QSnapdDisableRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_disable_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void QSnapdDisableRequest::runAsync ()
{
    snapd_client_disable_async (SNAPD_CLIENT (getClient ()),
                                name.toStdString ().c_str (),
                                NULL, NULL, // FIXME: Progress
                                G_CANCELLABLE (getCancellable ()), disable_ready_cb, (gpointer) this);
}
