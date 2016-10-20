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
#include <Snapd/Request>
#include <Snapd/Snap>
#include <Snapd/SystemInformation>

namespace Snapd
{
class Q_DECL_EXPORT ConnectRequest : public Request
{
    Q_OBJECT

public:
    explicit ConnectRequest (void *snapd_client, QObject *parent = 0) : Request (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
};

class Q_DECL_EXPORT SystemInformationRequest : public Request
{
    Q_OBJECT

public:
    explicit SystemInformationRequest (void *snapd_client, QObject *parent = 0) : Request (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    SystemInformation *systemInformation ();

private:
    // FIXME: Not ABI safe - use private object
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT ListRequest : public Request
{
    Q_OBJECT

public:
    explicit ListRequest (void *snapd_client, QObject *parent = 0) : Request (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    QList<Snap*> snaps () const;

private:
    // FIXME: Not ABI safe - use private object
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT ListOneRequest : public Request
{
    Q_OBJECT

public:
    explicit ListOneRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    Snap *snap () const;

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT IconRequest : public Request
{
    Q_OBJECT

public:
    explicit IconRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    Icon *icon () const;

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    void *result; // FIXME: destroy
};
  
enum FindFlags 
{
    None          = 0,
    MatchName     = 1 << 0,
    SelectPrivate = 1 << 1,
    SelectRefresh = 1 << 2     
};

class Q_DECL_EXPORT FindRequest : public Request
{
    Q_OBJECT

public:
    explicit FindRequest (FindFlags flags, const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), flags (flags), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    QList<Snap*> snaps () const;
    const QString suggestedCurrency () const;

private:
    // FIXME: Not ABI safe - use private object
    FindFlags flags;
    QString name;
    void *result; // FIXME: destroy
    QString suggestedCurrency_;
};

class Q_DECL_EXPORT InstallRequest : public Request
{
    Q_OBJECT

public:
    explicit InstallRequest (const QString& name, const QString& channel, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name), channel (channel) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    QString channel;
};

class Q_DECL_EXPORT RefreshRequest : public Request
{
    Q_OBJECT

public:
    explicit RefreshRequest (const QString& name, const QString& channel, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name), channel (channel) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    QString channel;
};

class Q_DECL_EXPORT RemoveRequest : public Request
{
    Q_OBJECT

public:
    explicit RemoveRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class Q_DECL_EXPORT EnableRequest : public Request
{
    Q_OBJECT

public:
    explicit EnableRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class Q_DECL_EXPORT DisableRequest : public Request
{
    Q_OBJECT

public:
    explicit DisableRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : Request (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class ClientPrivate;

class Q_DECL_EXPORT Client : public QObject
{
    Q_OBJECT

public:
    explicit Client (QObject* parent=0);
    ConnectRequest *connect ();
    AuthData login (const QString &username, const QString &password, const QString &otp = "");
    void setAuthData (const AuthData& auth_data);
    AuthData authData ();
    SystemInformationRequest *getSystemInformation ();
    ListRequest *list ();
    ListOneRequest *listOne (const QString &name);
    IconRequest *getIcon (const QString &name);
    //FIXMEvoid getInterfaces (GPtrArray **plugs, GPtrArray **slots);
    /*void connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);*/
    FindRequest *find (FindFlags flags, const QString &query);
    InstallRequest *install (const QString &name, const QString &channel);
    RefreshRequest *refresh (const QString &name, const QString &channel);
    RemoveRequest *remove (const QString &name);
    EnableRequest *enable (const QString &name);
    DisableRequest *disable (const QString &name);

private:
    ClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Client)
};
}

#endif
