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

class Q_DECL_EXPORT QSnapdConnectRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdConnectRequest (void *snapd_client, QObject *parent = 0) : QSnapdRequest (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
};

class Q_DECL_EXPORT QSnapdLoginRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdLoginRequest (void *snapd_client, const QString& username, const QString& password, const QString& otp, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), username (username), password (password), otp (otp) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString username;
    QString password;
    QString otp;
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT QSnapdSystemInformationRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdSystemInformationRequest (void *snapd_client, QObject *parent = 0) : QSnapdRequest (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    QSnapdSystemInformation *systemInformation ();

private:
    // FIXME: Not ABI safe - use private object
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT QSnapdListRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdListRequest (void *snapd_client, QObject *parent = 0) : QSnapdRequest (snapd_client, parent) {}

    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QList<QSnapdSnap*> snaps () const;

private:
    // FIXME: Not ABI safe - use private object
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT QSnapdListOneRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdListOneRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdSnap *snap () const;

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT QSnapdIconRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdIconRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    QSnapdIcon *icon () const;

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    void *result; // FIXME: destroy
};

class Q_DECL_EXPORT QSnapdFindOptions : public QObject
{
    Q_OBJECT
    Q_ENUMS (FindFlags);

public:
    enum FindFlags
    {
        None          = 0,
        MatchName     = 1 << 0,
        SelectPrivate = 1 << 1,
        SelectRefresh = 1 << 2
    };
};

class Q_DECL_EXPORT QSnapdFindRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(QString suggestedCurrency READ suggestedCurrency)

public:      
    explicit QSnapdFindRequest (QSnapdFindOptions::FindFlags flags, const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), flags (flags), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QList<QSnapdSnap*> snaps () const;
    const QString suggestedCurrency () const;

private:
    // FIXME: Not ABI safe - use private object
    QSnapdFindOptions::FindFlags flags;
    QString name;
    void *result; // FIXME: destroy
    QString suggestedCurrency_;
};

class Q_DECL_EXPORT QSnapdInstallRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdInstallRequest (const QString& name, const QString& channel, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name), channel (channel) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    QString channel;
};

class Q_DECL_EXPORT QSnapdRefreshRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name), channel (channel) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
    QString channel;
};

class Q_DECL_EXPORT QSnapdRemoveRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRemoveRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class Q_DECL_EXPORT QSnapdEnableRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdEnableRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class Q_DECL_EXPORT QSnapdDisableRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdDisableRequest (const QString& name, void *snapd_client = 0, QObject *parent = 0) : QSnapdRequest (snapd_client, parent), name (name) {}

    virtual void runSync ();
    virtual void runAsync ();

private:
    // FIXME: Not ABI safe - use private object
    QString name;
};

class QSnapdClientPrivate;
  
Q_INVOKABLE QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp);  

class Q_DECL_EXPORT QSnapdClient : public QObject
{
    Q_OBJECT

public:
    explicit QSnapdClient (QObject* parent=0);
    Q_INVOKABLE QSnapdConnectRequest *connect ();
    Q_INVOKABLE QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp);
    Q_INVOKABLE QSnapdSystemInformationRequest *getSystemInformation ();
    Q_INVOKABLE QSnapdListRequest *list ();
    Q_INVOKABLE QSnapdListOneRequest *listOne (const QString &name);
    Q_INVOKABLE QSnapdIconRequest *getIcon (const QString &name);
    //Q_INVOKABLE QSnapdInterfacesRequest *getInterfaces ();
    //Q_INVOKABLE QSnapdConnectRequest *connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    //Q_INVOKABLE QSnapdDisconnectRequest *disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    Q_INVOKABLE QSnapdFindRequest *find (QSnapdFindOptions::FindFlags flags, const QString &query);
    Q_INVOKABLE QSnapdInstallRequest *install (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdRefreshRequest *refresh (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdRemoveRequest *remove (const QString &name);
    Q_INVOKABLE QSnapdEnableRequest *enable (const QString &name);
    Q_INVOKABLE QSnapdDisableRequest *disable (const QString &name);

private:
    QSnapdClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdClient)
};

#endif
