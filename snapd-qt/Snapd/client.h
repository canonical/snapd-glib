/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CLIENT_H
#define SNAPD_CLIENT_H

#include <QtCore/QObject>
#include <Snapd/AuthData>
#include <Snapd/Icon>
#include <Snapd/Snap>
#include <Snapd/SystemInformation>

namespace Snapd
{
class Q_DECL_EXPORT ConnectReply : public Reply
{
    Q_OBJECT

public:
    explicit ConnectReply (void *snapd_client, QObject *parent = 0) : Reply (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
};

class Q_DECL_EXPORT SystemInformationReply : public Reply
{
    Q_OBJECT

public:
    explicit SystemInformationReply (void *snapd_client, QObject *parent = 0) : Reply (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    SystemInformation *systemInformation ();

private:
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT ListReply : public Reply
{
    Q_OBJECT

public:
    explicit ListReply (void *snapd_client, QObject *parent = 0) : Reply (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    QList<Snap*> snaps ();

private:
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT ListOneReply : public Reply
{
    Q_OBJECT

public:
    explicit ListOneReply (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Reply (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    Snap *snap ();

private:
    void *result; // FIXME: destroy
    QString name;
};

class ClientPrivate;

class Q_DECL_EXPORT Client : public QObject
{
    Q_OBJECT

public:
    explicit Client (QObject* parent=0);
    ConnectReply *connect ();
    AuthData login (const QString &username, const QString &password, const QString &otp = "");
    void setAuthData (const AuthData& auth_data);
    AuthData authData ();
    SystemInformationReply *getSystemInformation ();
    ListReply *list ();
    ListOneReply *listOne (const QString &name);
    Icon getIcon (const QString &name);
    //FIXMEvoid getInterfacesSync (GPtrArray **plugs, GPtrArray **slots);
    /*void connectInterfaceSync (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void disconnectInterfaceSync (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    QList<Snap> findSync (SnapdFindFlags flags, const QString &query, gchar **suggested_currency);
    void installSync (const QString &name, const QString &channel, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void refreshSync (const QString &name, const QString &channel, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void removeSync (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void enableSync (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void disableSync (const QString &name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);*/

private:
    ClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Client)
};
}

#endif
