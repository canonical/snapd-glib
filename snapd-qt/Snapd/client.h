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
#include <QLocalSocket>
#include <Snapd/Alias>
#include <Snapd/AuthData>
#include <Snapd/Connection>
#include <Snapd/Icon>
#include <Snapd/Request>
#include <Snapd/Snap>
#include <Snapd/SystemInformation>

class QSnapdConnectRequestPrivate;
class Q_DECL_EXPORT QSnapdConnectRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdConnectRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdConnectRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdConnectRequest)
};

class QSnapdLoginRequestPrivate;
class Q_DECL_EXPORT QSnapdLoginRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY (QSnapdAuthData* authData READ authData)

public:
    explicit QSnapdLoginRequest (void *snapd_client, const QString& username, const QString& password, const QString& otp, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdAuthData *authData ();
    void handleResult (void *, void *);

private:
    QSnapdLoginRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdLoginRequest)
};

class QSnapdGetSystemInformationRequestPrivate;
class Q_DECL_EXPORT QSnapdGetSystemInformationRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(QSnapdSystemInformation* systemInformation READ systemInformation)

public:
    explicit QSnapdGetSystemInformationRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    QSnapdSystemInformation *systemInformation ();
    void handleResult (void *, void *);

private:
    QSnapdGetSystemInformationRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetSystemInformationRequest)
};

class QSnapdListRequestPrivate;
class Q_DECL_EXPORT QSnapdListRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int snapCount READ snapCount)

public:
    explicit QSnapdListRequest (void *snapd_client, QObject *parent = 0);

    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int snapCount () const;
    Q_INVOKABLE QSnapdSnap *snap (int) const;
    void handleResult (void *, void *);

private:
    QSnapdListRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdListRequest)
};

class QSnapdListOneRequestPrivate;
class Q_DECL_EXPORT QSnapdListOneRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdListOneRequest (const QString& name, void *snapd_client, QObject *parent = 0);

    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdSnap *snap () const;
    void handleResult (void *, void *);

private:
    QSnapdListOneRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdListOneRequest)
};

class QSnapdGetIconRequestPrivate;
class Q_DECL_EXPORT QSnapdGetIconRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdGetIconRequest (const QString& name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    QSnapdIcon *icon () const;
    void handleResult (void *, void *);

private:
    QSnapdGetIconRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetIconRequest)
};

class QSnapdGetAssertionsRequestPrivate;
class Q_DECL_EXPORT QSnapdGetAssertionsRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(QStringList assertions READ assertions)

public:
    explicit QSnapdGetAssertionsRequest (const QString& type, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QStringList assertions () const;
    void handleResult (void *, void *);

private:
    QSnapdGetAssertionsRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetAssertionsRequest)
};

class QSnapdAddAssertionsRequestPrivate;
class Q_DECL_EXPORT QSnapdAddAssertionsRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdAddAssertionsRequest (const QStringList& assertions, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdAddAssertionsRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdAddAssertionsRequest)
};

class QSnapdGetInterfacesRequestPrivate;
class Q_DECL_EXPORT QSnapdGetInterfacesRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int plugCount READ plugCount)
    Q_PROPERTY(int slotCount READ slotCount)      

public:
    explicit QSnapdGetInterfacesRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int plugCount () const;
    Q_INVOKABLE QSnapdConnection *plug (int) const;
    Q_INVOKABLE int slotCount () const;
    Q_INVOKABLE QSnapdConnection *slot (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetInterfacesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetInterfacesRequest)
};

class QSnapdConnectInterfaceRequestPrivate;
class Q_DECL_EXPORT QSnapdConnectInterfaceRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdConnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);  

private:
    QSnapdConnectInterfaceRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdConnectInterfaceRequest)
};

class QSnapdDisconnectInterfaceRequestPrivate;
class Q_DECL_EXPORT QSnapdDisconnectInterfaceRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdDisconnectInterfaceRequest (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdDisconnectInterfaceRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdDisconnectInterfaceRequest)
};

class QSnapdFindRequestPrivate;
class Q_DECL_EXPORT QSnapdFindRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int snapCount READ snapCount)
    Q_PROPERTY(QString suggestedCurrency READ suggestedCurrency)

public:
    explicit QSnapdFindRequest (int flags, const QString& section, const QString& name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int snapCount () const;
    Q_INVOKABLE QSnapdSnap *snap (int) const;
    const QString suggestedCurrency () const;
    void handleResult (void *, void *);

private:
    QSnapdFindRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdFindRequest)
};

class QSnapdFindRefreshableRequestPrivate;
class Q_DECL_EXPORT QSnapdFindRefreshableRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int snapCount READ snapCount)

public:
    explicit QSnapdFindRefreshableRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int snapCount () const;
    Q_INVOKABLE QSnapdSnap *snap (int) const;
    void handleResult (void *, void *);

private:
    QSnapdFindRefreshableRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdFindRefreshableRequest)
};

class QSnapdInstallRequestPrivate;
class Q_DECL_EXPORT QSnapdInstallRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdInstallRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdInstallRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdInstallRequest)
};

class QSnapdRefreshRequestPrivate;
class Q_DECL_EXPORT QSnapdRefreshRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdRefreshRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdRefreshRequest)
};

class QSnapdRefreshAllRequestPrivate;
class Q_DECL_EXPORT QSnapdRefreshAllRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(QStringList snapNames READ snapNames)

public:
    explicit QSnapdRefreshAllRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QStringList snapNames () const;
    void handleResult (void *, void *);

private:
    QSnapdRefreshAllRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdRefreshAllRequest)
};

class QSnapdRemoveRequestPrivate;
class Q_DECL_EXPORT QSnapdRemoveRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRemoveRequest (const QString& name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdRemoveRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdRemoveRequest)
};

class QSnapdEnableRequestPrivate;
class Q_DECL_EXPORT QSnapdEnableRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdEnableRequest (const QString& name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdEnableRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdEnableRequest)
};

class QSnapdDisableRequestPrivate;
class Q_DECL_EXPORT QSnapdDisableRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdDisableRequest (const QString& name, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdDisableRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdDisableRequest)
};

class QSnapdCheckBuyRequestPrivate;
class Q_DECL_EXPORT QSnapdCheckBuyRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(bool canBuy READ canBuy)

public:
    explicit QSnapdCheckBuyRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE bool canBuy () const;
    void handleResult (void *, void *);

private:
    QSnapdCheckBuyRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdCheckBuyRequest)
};

class QSnapdBuyRequestPrivate;
class Q_DECL_EXPORT QSnapdBuyRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdBuyRequest (const QString& id, double amount, const QString& currency, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdBuyRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdBuyRequest)
};

class QSnapdGetSectionsRequestPrivate;
class Q_DECL_EXPORT QSnapdGetSectionsRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdGetSectionsRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QStringList sections () const;
    void handleResult (void *, void *);

private:
    QSnapdGetSectionsRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetSectionsRequest)
};

class QSnapdGetAliasesRequestPrivate;
class Q_DECL_EXPORT QSnapdGetAliasesRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int aliasCount READ aliasCount)    

public:
    explicit QSnapdGetAliasesRequest (void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int aliasCount () const;
    Q_INVOKABLE QSnapdAlias *alias (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetAliasesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetAliasesRequest)
};

class QSnapdEnableAliasesRequestPrivate;
class Q_DECL_EXPORT QSnapdEnableAliasesRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdEnableAliasesRequest (const QString& snap, const QStringList& aliases, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdEnableAliasesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdEnableAliasesRequest)
};

class QSnapdDisableAliasesRequestPrivate;
class Q_DECL_EXPORT QSnapdDisableAliasesRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdDisableAliasesRequest (const QString& snap, const QStringList& aliases, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdDisableAliasesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdDisableAliasesRequest)
};

class QSnapdResetAliasesRequestPrivate;
class Q_DECL_EXPORT QSnapdResetAliasesRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdResetAliasesRequest (const QString& snap, const QStringList& aliases, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdResetAliasesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdResetAliasesRequest)
};

class QSnapdRunSnapCtlRequestPrivate;
class Q_DECL_EXPORT QSnapdRunSnapCtlRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRunSnapCtlRequest (const QString& contextId, const QStringList& args, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QString stdout () const;
    Q_INVOKABLE QString stderr () const;  
    void handleResult (void *, void *);

private:
    QSnapdRunSnapCtlRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdRunSnapCtlRequest)
};

Q_INVOKABLE QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp);

class QSnapdClientPrivate;
class Q_DECL_EXPORT QSnapdClient : public QObject
{
    Q_OBJECT
    Q_FLAGS(FindFlags)

public:
    enum FindFlag
    {
        None          = 0,
        MatchName     = 1 << 0,
        SelectPrivate = 1 << 1
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag);
    explicit QSnapdClient (QObject* parent=0);
    explicit QSnapdClient (int fd, QObject* parent=0);
    Q_INVOKABLE QSnapdConnectRequest *connect ();
    Q_INVOKABLE QSnapdLoginRequest *login (const QString& username, const QString& password, const QString& otp);
    Q_INVOKABLE void setAuthData (QSnapdAuthData *authData);
    Q_INVOKABLE QSnapdAuthData *authData ();
    Q_INVOKABLE QSnapdGetSystemInformationRequest *getSystemInformation ();
    Q_INVOKABLE QSnapdListRequest *list ();
    Q_INVOKABLE QSnapdListOneRequest *listOne (const QString &name);
    Q_INVOKABLE QSnapdGetIconRequest *getIcon (const QString &name);
    Q_INVOKABLE QSnapdGetAssertionsRequest *getAssertions (const QString &type);
    Q_INVOKABLE QSnapdAddAssertionsRequest *addAssertions (const QStringList &assertions);  
    Q_INVOKABLE QSnapdGetInterfacesRequest *getInterfaces ();
    Q_INVOKABLE QSnapdConnectInterfaceRequest *connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    Q_INVOKABLE QSnapdDisconnectInterfaceRequest *disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    Q_INVOKABLE QSnapdFindRequest *find (FindFlags flags, const QString &query);
    Q_INVOKABLE QSnapdFindRequest *findSection (FindFlags flags, const QString &section, const QString &query);
    Q_INVOKABLE QSnapdFindRefreshableRequest *findRefreshable ();  
    Q_INVOKABLE QSnapdInstallRequest *install (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdRefreshRequest *refresh (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdRefreshAllRequest *refreshAll ();  
    Q_INVOKABLE QSnapdRemoveRequest *remove (const QString &name);
    Q_INVOKABLE QSnapdEnableRequest *enable (const QString &name);
    Q_INVOKABLE QSnapdDisableRequest *disable (const QString &name);
    Q_INVOKABLE QSnapdCheckBuyRequest *checkBuy ();
    Q_INVOKABLE QSnapdBuyRequest *buy (const QString& id, double amount, const QString& currency);
    Q_INVOKABLE QSnapdGetSectionsRequest *getSections ();
    Q_INVOKABLE QSnapdGetAliasesRequest *getAliases ();
    Q_INVOKABLE QSnapdEnableAliasesRequest *enableAliases (const QString snap, const QStringList &aliases);
    Q_INVOKABLE QSnapdDisableAliasesRequest *disableAliases (const QString snap, const QStringList &aliases);
    Q_INVOKABLE QSnapdResetAliasesRequest *resetAliases (const QString snap, const QStringList &aliases);  
    Q_INVOKABLE QSnapdRunSnapCtlRequest *runSnapCtl (const QString contextId, const QStringList &args);    

private:
    QSnapdClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdClient)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSnapdClient::FindFlags)

#endif
