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

ConnectReply *Client::connect ()
{
    Q_D(Client);
    return new ConnectReply (d->client, this);
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

SystemInformationReply *Client::getSystemInformation ()
{
    Q_D(Client);
    return new SystemInformationReply (d->client, this);
}

ListReply *Client::list ()
{
    Q_D(Client);
    return new ListReply (d->client, this);
}

ListOneReply *Client::listOne (const QString &name)
{
    Q_D(Client);
    return new ListOneReply (name, d->client, this);
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
}

void Client::install (const QString &name, const QString &channel, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_install_sync (d->client,
                                    name.toLocal8Bit ().data (),
                                    channel.isEmpty () ? NULL : channel.toLocal8Bit ().data (),                               
                                    progress_callback, progress_callback_data,
                                    NULL, NULL)) {
        // FIXME: Throw exception
    }
}

void Client::refresh (const QString &name, const QString &channel, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_refresh_sync (d->client,
                                    name.toLocal8Bit ().data (),
                                    channel.isEmpty () ? NULL : channel.toLocal8Bit ().data (),                               
                                    progress_callback, progress_callback_data,
                                    NULL, NULL)) {
        // FIXME: Throw exception
    }
}

void Client::remove (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_remove_sync (d->client,
                                   name.toLocal8Bit ().data (),
                                   progress_callback, progress_callback_data,
                                   NULL, NULL)) {
        // FIXME: Throw exception
    }
}

void Client::enable (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_enable_sync (d->client,
                                   name.toLocal8Bit ().data (),
                                   progress_callback, progress_callback_data,
                                   NULL, NULL)) {
        // FIXME: Throw exception
    }
}

void Client::disable (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data)
{
    Q_D(Client);
    if (!snapd_client_disable_sync (d->client,
                                    name.toLocal8Bit ().data (),
                                    progress_callback, progress_callback_data,
                                    NULL, NULL)) {
        // FIXME: Throw exception
    }
}*/

void ConnectReply::runSync ()
{
    g_autoptr(GError) error = NULL;
    snapd_client_connect_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

void ConnectReply::runAsync ()
{
    // NOTE: No async method supported
}

void SystemInformationReply::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_get_system_information_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void system_information_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::SystemInformationReply *reply = (Snapd::SystemInformationReply *) data;
    g_autoptr(GError) error = NULL;
    reply->result = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), result, &error);
    reply->finish (error);*/
}

void SystemInformationReply::runAsync ()
{
    snapd_client_get_system_information_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), system_information_ready_cb, (gpointer) this);
}

SystemInformation *SystemInformationReply::systemInformation ()
{
    return new SystemInformation (parent (), result);
}

void ListReply::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_sync (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::ListReply *reply = (Snapd::ListReply *) data;
    g_autoptr(GError) error = NULL;
    reply->result = snapd_client_list_finish (SNAPD_CLIENT (object), result, &error);
    reply->finish (error);*/
}

void ListReply::runAsync ()
{
    snapd_client_list_async (SNAPD_CLIENT (getClient ()), G_CANCELLABLE (getCancellable ()), list_ready_cb, (gpointer) this);
}

QList<Snap*> ListReply::snaps ()
{
    QList<Snap*> snaps;
    GPtrArray *array = (GPtrArray *) result;
    guint i;

    for (i = 0; i < array->len; i++)
        snaps.append (new Snap (array->pdata[i], parent ()));

    return snaps;
}

void ListOneReply::runSync ()
{
    g_autoptr(GError) error = NULL;
    result = snapd_client_list_one_sync (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), &error);
    finish (error);
}

static void list_one_ready_cb (GObject *object, GAsyncResult *result, gpointer data)
{
    /*Snapd::ListOneReply *reply = (Snapd::ListOneReply *) data;
    g_autoptr(GError) error = NULL;
    reply->result = snapd_client_list_one_finish (SNAPD_CLIENT (object), result, &error);
    reply->finish (error);*/
}

void ListOneReply::runAsync ()
{
    snapd_client_list_one_async (SNAPD_CLIENT (getClient ()), name.toStdString ().c_str (), G_CANCELLABLE (getCancellable ()), list_one_ready_cb, (gpointer) this);
}

Snap *ListOneReply::snap ()
{
    return new Snap (result, parent ());
}
