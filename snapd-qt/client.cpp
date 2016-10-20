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

using namespace Snapd;

struct Snapd::ClientPrivate
{
    ClientPrivate ()
    {
        client = snapd_client_new ();
    }
  
    ~ClientPrivate ()
    {
        g_object_unref (client);
    }

    SnapdClient *client;
};

Client::Client(QObject *parent) :
    QObject (parent),
    d_ptr (new ClientPrivate())
{
}

ConnectRequest *Client::connect ()
{
    Q_D(Client);
    return new ConnectRequest (d->client, this);
}

AuthData Client::login (const QString &username, const QString &password, const QString &otp)
{
    Q_D(Client);

    g_autoptr(SnapdAuthData) auth_data = NULL;
    auth_data = snapd_client_login_sync (d->client,
                                         username.toLocal8Bit ().data (),
                                         password.toLocal8Bit ().data (),
                                         otp.isEmpty () ? NULL : otp.toLocal8Bit ().data (),
                                         NULL, NULL);

    return AuthData (this, auth_data);
}

void Client::setAuthData (const AuthData& auth_data)
{
    Q_D(Client);

    // FIXME: populate data fields
    g_autoptr(SnapdAuthData) value = snapd_auth_data_new ("", NULL);
    snapd_client_set_auth_data (d->client, value);
}

AuthData Client::authData ()
{
    Q_D(Client);
    return AuthData (this, snapd_client_get_auth_data (d->client));
}

SystemInformationRequest *Client::getSystemInformation ()
{
    Q_D(Client);
    return new SystemInformationRequest (d->client, this);
}

ListRequest *Client::list ()
{
    Q_D(Client);
    return new ListRequest (d->client, this);
}

ListOneRequest *Client::listOne (const QString &name)
{
    Q_D(Client);
    return new ListOneRequest (name, d->client, this);
}

Icon Client::getIcon (const QString &name)
{
    Q_D(Client);

    g_autoptr(SnapdIcon) icon = NULL;
    icon = snapd_client_get_icon_sync (d->client, name.toLocal8Bit ().data (), NULL, NULL);
    if (icon == NULL) {
        // FIXME: Throw exception
    }

    return Icon (this, icon);
}

InstallRequest *Client::install (const QString &name, const QString& channel)
{
    Q_D(Client);
    return new InstallRequest (name, channel, d->client, this);
}

RefreshRequest *Client::refresh (const QString &name, const QString& channel)
{
    Q_D(Client);
    return new RefreshRequest (name, channel, d->client, this);
}

RemoveRequest *Client::remove (const QString &name)
{
    Q_D(Client);
    return new RemoveRequest (name, d->client, this);
}

EnableRequest *Client::enable (const QString &name)
{
    Q_D(Client);
    return new EnableRequest (name, d->client, this);
}

DisableRequest *Client::disable (const QString &name)
{
    Q_D(Client);
    return new DisableRequest (name, d->client, this);
}

/*FIXME
void Client::getInterfaces (GPtrArray **plugs, GPtrArray **slots)
{
    Q_D(Client);

    g_autoptr(GPtrArray) plugs_ = NULL;
    g_autoptr(GPtrArray) slots_ = NULL;  
    if (!snapd_client_get_interfaces_sync (d->client, &plugs_, &slots_, NULL, NULL)) {
        // FIXME: Thow exception
    }

    // FIXME: Convert to QList<Snapd::Plug> and QList<Snapd::Slot> and return
}*/

/*FIXME
void Client::connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_connect_interface_sync (d->client,
                                              plug_snap.toLocal8Bit ().data (),
                                              plug_name.toLocal8Bit ().data (),
                                              slot_snap.toLocal8Bit ().data (),
                                              slot_name.toLocal8Bit ().data (),
                                              progress_callback, progress_callback_data,
                                              NULL, NULL)) {
        // FIXME: Throw exception
    }
}

void Client::disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_disconnect_interface_sync (d->client,
                                                 plug_snap.toLocal8Bit ().data (),
                                                 plug_name.toLocal8Bit ().data (),
                                                 slot_snap.toLocal8Bit ().data (),
                                                 slot_name.toLocal8Bit ().data (),
                                                 progress_callback, progress_callback_data,
                                                 NULL, NULL)) {
        // FIXME: Throw exception
    }
}

QList<Snap> Client::find (SnapdFindFlags flags, const QString &query, gchar **suggested_currency)
{
    Q_D(Client);

    g_autoptr(GPtrArray) snaps = NULL;
    g_autofree gchar *suggested_currency_;
    snaps = snapd_client_find_sync (d->client, flags, query.toLocal8Bit ().data (), &suggested_currency_, NULL, NULL);
    if (snaps == NULL) {
        // FIXME: Throw exception
    }

    QList<Snap> result;
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = SNAPD_SNAP (snaps->pdata[i]);
        result.append (Snap (this, snap));
    }

    return result;
}*/

void ConnectRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_connect_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void ConnectRequest::runAsync ()
{
    // NOTE: No async method supported
}

void SystemInformationRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_get_system_information_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void system_information_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::SystemInformationRequest *request = (Snapd::SystemInformationRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void SystemInformationRequest::runAsync ()
{
    snapd_client_get_system_information_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), system_information_ready_cb, (gpointer) this);
}

SystemInformation *SystemInformationRequest::systemInformation ()
{
    return new SystemInformation (parent (), result);
}

void ListRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::ListRequest *request = (Snapd::ListRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_list_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void ListRequest::runAsync ()
{
    snapd_client_list_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), list_ready_cb, (gpointer) this);
}

QList<Snap*> ListRequest::snaps ()
{
    QList<Snap*> snaps;
    GPtrArray *array = (GPtrArray *) result;
    guint i;

    for (i = 0; i < array->len; i++)
        snaps.append (new Snap (array->pdata[i], parent ()));

    return snaps;
}

void ListOneRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_one_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_one_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::ListOneRequest *request = (Snapd::ListOneRequest *) data;
    g_autoptr(GError) error = NULL;
    request->result = snapd_client_list_one_finish (SNAPD_CLIENT (object), result, &error);
    request->finish (error);*/
}

void ListOneRequest::runAsync ()
{
    snapd_client_list_one_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, (gpointer) this);
}

Snap *ListOneRequest::snap ()
{
    return new Snap (result, parent ());
}

void InstallRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_install_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), channel.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void install_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::InstallRequest *request = (Snapd::InstallRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_install_finish (SNAPD_CLIENT (object), result, NULL, NULL, &error);
    request->finish (error);*/
}

void InstallRequest::runAsync ()
{
    snapd_client_install_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), channel.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), install_ready_cb, (gpointer) this);
}

void RefreshRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_refresh_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), channel.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void refresh_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::RefreshRequest *request = (Snapd::RefreshRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_refresh_finish (SNAPD_CLIENT (object), result, NULL, NULL, &error);
    request->finish (error);*/
}

void RefreshRequest::runAsync ()
{
    snapd_client_refresh_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), channel.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), refresh_ready_cb, (gpointer) this);
}

void RemoveRequest::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_remove_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void remove_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::RemoveRequest *request = (Snapd::RemoveRequest *) data;
    g_autoptr(GError) error = NULL;
    request->snapd_client_remove_finish (SNAPD_CLIENT (object), result, NULL, NULL, &error);
    request->finish (error);*/
}

void RemoveRequest::runAsync ()
{
    snapd_client_remove_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), NULL, NULL, G_CANCELLABLE (getCancellable ()), remove_ready_cb, (gpointer) this);
}
