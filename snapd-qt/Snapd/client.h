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
#include <QIODevice>
#include <QLocalSocket>
#include <Snapd/Alias>
#include <Snapd/AuthData>
#include <Snapd/Connection>
#include <Snapd/Icon>
#include <Snapd/Plug>
#include <Snapd/Request>
#include <Snapd/Slot>
#include <Snapd/Snap>
#include <Snapd/SystemInformation>
#include <Snapd/UserInformation>

class QSnapdConnectRequestPrivate;
class Q_DECL_EXPORT QSnapdConnectRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdConnectRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdConnectRequest ();
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
    Q_PROPERTY (QSnapdUserInformation* userInformation READ userInformation)
    Q_PROPERTY (QSnapdAuthData* authData READ authData)

public:
    explicit QSnapdLoginRequest (void *snapd_client, const QString& email, const QString& password, const QString& otp, QObject *parent = 0);
    ~QSnapdLoginRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdUserInformation *userInformation ();
    Q_INVOKABLE QSnapdAuthData *authData ();
    void handleResult (void *, void *);

private:
    QSnapdLoginRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdLoginRequest)
};

class QSnapdGetChangesRequestPrivate;
class Q_DECL_EXPORT QSnapdGetChangesRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int changeCount READ changeCount)

public:
    explicit QSnapdGetChangesRequest (int filter, const QString& snapName, void *snapd_client, QObject *parent = 0);
    ~QSnapdGetChangesRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int changeCount () const;
    Q_INVOKABLE QSnapdChange *change (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetChangesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetChangesRequest)
};

class QSnapdGetChangeRequestPrivate;
class Q_DECL_EXPORT QSnapdGetChangeRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdGetChangeRequest (const QString& id, void *snapd_client, QObject *parent = 0);
    ~QSnapdGetChangeRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdChange *change () const;
    void handleResult (void *, void *);

private:
    QSnapdGetChangeRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetChangeRequest)
};

class QSnapdGetSystemInformationRequestPrivate;
class Q_DECL_EXPORT QSnapdGetSystemInformationRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(QSnapdSystemInformation* systemInformation READ systemInformation)

public:
    explicit QSnapdGetSystemInformationRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdGetSystemInformationRequest ();
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
    ~QSnapdListRequest ();
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
    ~QSnapdListOneRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdSnap *snap () const;
    void handleResult (void *, void *);

private:
    QSnapdListOneRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdListOneRequest)
};

class QSnapdGetAppsRequestPrivate;
class Q_DECL_EXPORT QSnapdGetAppsRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int appCount READ appCount)

public:
    explicit QSnapdGetAppsRequest (int flags, void *snapd_client, QObject *parent = 0);
    ~QSnapdGetAppsRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int appCount () const;
    Q_INVOKABLE QSnapdApp *app (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetAppsRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetAppsRequest)
};

class QSnapdGetIconRequestPrivate;
class Q_DECL_EXPORT QSnapdGetIconRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdGetIconRequest (const QString& name, void *snapd_client, QObject *parent = 0);
    ~QSnapdGetIconRequest ();
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
    ~QSnapdGetAssertionsRequest ();
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
    ~QSnapdAddAssertionsRequest ();
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
    ~QSnapdGetInterfacesRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int plugCount () const;
    Q_INVOKABLE QSnapdPlug *plug (int) const;
    Q_INVOKABLE int slotCount () const;
    Q_INVOKABLE QSnapdSlot *slot (int) const;
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
    ~QSnapdConnectInterfaceRequest ();
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
    ~QSnapdDisconnectInterfaceRequest ();
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
    ~QSnapdFindRequest ();
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
    ~QSnapdFindRefreshableRequest ();
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
    explicit QSnapdInstallRequest (int flags, const QString& name, const QString& channel, const QString& revision, QIODevice *ioDevice, void *snapd_client, QObject *parent = 0);
    ~QSnapdInstallRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdInstallRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdInstallRequest)
};

class QSnapdTryRequestPrivate;
class Q_DECL_EXPORT QSnapdTryRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdTryRequest (const QString& path, void *snapd_client, QObject *parent = 0);
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdTryRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdTryRequest)
};

class QSnapdRefreshRequestPrivate;
class Q_DECL_EXPORT QSnapdRefreshRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdRefreshRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent = 0);
    ~QSnapdRefreshRequest ();
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
    ~QSnapdRefreshAllRequest ();
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
    ~QSnapdRemoveRequest ();
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
    ~QSnapdEnableRequest ();
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
    ~QSnapdDisableRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdDisableRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdDisableRequest)
};

class QSnapdSwitchChannelRequestPrivate;
class Q_DECL_EXPORT QSnapdSwitchChannelRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdSwitchChannelRequest (const QString& name, const QString& channel, void *snapd_client, QObject *parent = 0);
    ~QSnapdSwitchChannelRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdSwitchChannelRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdSwitchChannelRequest)
};

class QSnapdCheckBuyRequestPrivate;
class Q_DECL_EXPORT QSnapdCheckBuyRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(bool canBuy READ canBuy)

public:
    explicit QSnapdCheckBuyRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdCheckBuyRequest ();
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
    ~QSnapdBuyRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdBuyRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdBuyRequest)
};

class QSnapdCreateUserRequestPrivate;
class Q_DECL_EXPORT QSnapdCreateUserRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdCreateUserRequest (const QString& email, int flags, void *snapd_client, QObject *parent = 0);
    ~QSnapdCreateUserRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QSnapdUserInformation *userInformation () const;
    void handleResult (void *, void *);

private:
    QSnapdCreateUserRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdCreateUserRequest)
};

class QSnapdCreateUsersRequestPrivate;
class Q_DECL_EXPORT QSnapdCreateUsersRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int userInformationCount READ userInformationCount)

public:
    explicit QSnapdCreateUsersRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdCreateUsersRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int userInformationCount () const;
    Q_INVOKABLE QSnapdUserInformation *userInformation (int) const;
    void handleResult (void *, void *);

private:
    QSnapdCreateUsersRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdCreateUsersRequest)
};

class QSnapdGetUsersRequestPrivate;
class Q_DECL_EXPORT QSnapdGetUsersRequest : public QSnapdRequest
{
    Q_OBJECT
    Q_PROPERTY(int userInformationCount READ userInformationCount)

public:
    explicit QSnapdGetUsersRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdGetUsersRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int userInformationCount () const;
    Q_INVOKABLE QSnapdUserInformation *userInformation (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetUsersRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetUsersRequest)
};

class QSnapdGetSectionsRequestPrivate;
class Q_DECL_EXPORT QSnapdGetSectionsRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdGetSectionsRequest (void *snapd_client, QObject *parent = 0);
    ~QSnapdGetSectionsRequest ();
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
    ~QSnapdGetAliasesRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE int aliasCount () const;
    Q_INVOKABLE QSnapdAlias *alias (int) const;
    void handleResult (void *, void *);

private:
    QSnapdGetAliasesRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdGetAliasesRequest)
};

class QSnapdAliasRequestPrivate;
class Q_DECL_EXPORT QSnapdAliasRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdAliasRequest (const QString& snap, const QString& app, const QString& alias, void *snapd_client, QObject *parent = 0);
    ~QSnapdAliasRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdAliasRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdAliasRequest)
};

class QSnapdUnaliasRequestPrivate;
class Q_DECL_EXPORT QSnapdUnaliasRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdUnaliasRequest (const QString& snap, const QString& alias, void *snapd_client, QObject *parent = 0);
    ~QSnapdUnaliasRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdUnaliasRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdUnaliasRequest)
};

class QSnapdPreferRequestPrivate;
class Q_DECL_EXPORT QSnapdPreferRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdPreferRequest (const QString& snap, void *snapd_client, QObject *parent = 0);
    ~QSnapdPreferRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    void handleResult (void *, void *);

private:
    QSnapdPreferRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdPreferRequest)
};

class QSnapdEnableAliasesRequestPrivate;
class Q_DECL_EXPORT QSnapdEnableAliasesRequest : public QSnapdRequest
{
    Q_OBJECT

public:
    explicit QSnapdEnableAliasesRequest (const QString& snap, const QStringList& aliases, void *snapd_client, QObject *parent = 0);
    ~QSnapdEnableAliasesRequest ();
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
    ~QSnapdDisableAliasesRequest ();
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
    ~QSnapdResetAliasesRequest ();
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
    ~QSnapdRunSnapCtlRequest ();
    virtual void runSync ();
    virtual void runAsync ();
    Q_INVOKABLE QString stdout () const;
    Q_INVOKABLE QString stderr () const;
    void handleResult (void *, void *);

private:
    QSnapdRunSnapCtlRequestPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdRunSnapCtlRequest)
};

Q_INVOKABLE QSnapdLoginRequest *login (const QString& email, const QString& password);
Q_INVOKABLE QSnapdLoginRequest *login (const QString& email, const QString& password, const QString& otp);

class QSnapdClientPrivate;
class Q_DECL_EXPORT QSnapdClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString socketPath READ socketPath WRITE setSocketPath)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent)
    Q_PROPERTY(bool allowInteraction READ allowInteraction WRITE setAllowInteraction)
    Q_FLAGS(FindFlags)

public:
    enum ChangeFilter
    {
        FilterAll,
        FilterInProgress,
        FilterReady
    };
    Q_ENUM(ChangeFilter)
    enum GetAppsFlag
    {
        SelectServices = 1 << 0
    };
    Q_DECLARE_FLAGS(GetAppsFlags, GetAppsFlag);
    enum FindFlag
    {
        None          = 0,
        MatchName     = 1 << 0,
        SelectPrivate = 1 << 1
    };
    Q_DECLARE_FLAGS(FindFlags, FindFlag);
    enum InstallFlag
    {
        Classic        = 1 << 0,
        Dangerous      = 1 << 1,
        Devmode        = 1 << 2,
        Jailmode       = 1 << 3
    };
    Q_DECLARE_FLAGS(InstallFlags, InstallFlag);
    enum CreateUserFlag
    {
        Sudo           = 1 << 0,
        Known          = 1 << 1
    };
    Q_DECLARE_FLAGS(CreateUserFlags, CreateUserFlag);
    explicit QSnapdClient (QObject* parent=0);
    explicit QSnapdClient (int fd, QObject* parent=0);
    virtual ~QSnapdClient ();
    Q_INVOKABLE Q_DECL_DEPRECATED QSnapdConnectRequest *connect ();
    Q_INVOKABLE QSnapdLoginRequest *login (const QString& email, const QString& password);
    Q_INVOKABLE QSnapdLoginRequest *login (const QString& email, const QString& password, const QString& otp);
    Q_INVOKABLE void setSocketPath (const QString &socketPath);
    Q_INVOKABLE QString socketPath () const;
    Q_INVOKABLE void setUserAgent (const QString &userAgent);
    Q_INVOKABLE QString userAgent () const;
    Q_INVOKABLE void setAllowInteraction (bool allowInteraction);
    Q_INVOKABLE bool allowInteraction () const;
    Q_INVOKABLE void setAuthData (QSnapdAuthData *authData);
    Q_INVOKABLE QSnapdAuthData *authData ();
    Q_INVOKABLE QSnapdGetChangesRequest *getChanges ();
    Q_INVOKABLE QSnapdGetChangesRequest *getChanges (ChangeFilter filter);
    Q_INVOKABLE QSnapdGetChangesRequest *getChanges (const QString &snapName);
    Q_INVOKABLE QSnapdGetChangesRequest *getChanges (ChangeFilter filter, const QString &snapName);
    Q_INVOKABLE QSnapdGetChangeRequest *getChange (const QString &id);
    Q_INVOKABLE QSnapdGetSystemInformationRequest *getSystemInformation ();
    Q_INVOKABLE QSnapdListRequest *list ();
    Q_INVOKABLE QSnapdListOneRequest *listOne (const QString &name);
    Q_INVOKABLE QSnapdGetAppsRequest *getApps ();
    Q_INVOKABLE QSnapdGetAppsRequest *getApps (GetAppsFlags flags);
    Q_INVOKABLE QSnapdGetIconRequest *getIcon (const QString &name);
    Q_INVOKABLE QSnapdGetAssertionsRequest *getAssertions (const QString &type);
    Q_INVOKABLE QSnapdAddAssertionsRequest *addAssertions (const QStringList &assertions);
    Q_INVOKABLE QSnapdGetInterfacesRequest *getInterfaces ();
    Q_INVOKABLE QSnapdConnectInterfaceRequest *connectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    Q_INVOKABLE QSnapdDisconnectInterfaceRequest *disconnectInterface (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name);
    Q_INVOKABLE QSnapdFindRequest *find (FindFlags flags);
    Q_INVOKABLE QSnapdFindRequest *find (FindFlags flags, const QString &query);
    Q_INVOKABLE QSnapdFindRequest *findSection (FindFlags flags, const QString &section, const QString &query);
    Q_INVOKABLE QSnapdFindRefreshableRequest *findRefreshable ();
    Q_INVOKABLE QSnapdInstallRequest *install (const QString &name);
    Q_INVOKABLE QSnapdInstallRequest *install (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdInstallRequest *install (const QString &name, const QString &channel, const QString &revision);
    Q_INVOKABLE QSnapdInstallRequest *install (InstallFlags flags, const QString &name);
    Q_INVOKABLE QSnapdInstallRequest *install (InstallFlags flags, const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdInstallRequest *install (InstallFlags flags, const QString &name, const QString &channel, const QString &revision);
    Q_INVOKABLE QSnapdInstallRequest *install (QIODevice *ioDevice);
    Q_INVOKABLE QSnapdInstallRequest *install (InstallFlags flags, QIODevice *ioDevice);
    Q_INVOKABLE QSnapdTryRequest *trySnap (const QString &path);
    Q_INVOKABLE QSnapdRefreshRequest *refresh (const QString &name);
    Q_INVOKABLE QSnapdRefreshRequest *refresh (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdRefreshAllRequest *refreshAll ();
    Q_INVOKABLE QSnapdRemoveRequest *remove (const QString &name);
    Q_INVOKABLE QSnapdEnableRequest *enable (const QString &name);
    Q_INVOKABLE QSnapdDisableRequest *disable (const QString &name);
    Q_INVOKABLE QSnapdSwitchChannelRequest *switchChannel (const QString &name, const QString &channel);
    Q_INVOKABLE QSnapdCheckBuyRequest *checkBuy ();
    Q_INVOKABLE QSnapdBuyRequest *buy (const QString& id, double amount, const QString& currency);
    Q_INVOKABLE QSnapdCreateUserRequest *createUser (const QString& email);
    Q_INVOKABLE QSnapdCreateUserRequest *createUser (const QString& email, CreateUserFlags flags);
    Q_INVOKABLE QSnapdCreateUsersRequest *createUsers ();
    Q_INVOKABLE QSnapdGetUsersRequest *getUsers ();
    Q_INVOKABLE QSnapdGetSectionsRequest *getSections ();
    Q_INVOKABLE QSnapdGetAliasesRequest *getAliases ();
    Q_INVOKABLE QSnapdAliasRequest *alias (const QString &snap, const QString &app, const QString &alias);
    Q_INVOKABLE QSnapdUnaliasRequest *unalias (const QString &snap, const QString &alias);
    Q_INVOKABLE QSnapdUnaliasRequest *unalias (const QString &alias);
    Q_INVOKABLE QSnapdPreferRequest *prefer (const QString &snap);
    Q_INVOKABLE Q_DECL_DEPRECATED QSnapdEnableAliasesRequest *enableAliases (const QString snap, const QStringList &aliases);
    Q_INVOKABLE Q_DECL_DEPRECATED QSnapdDisableAliasesRequest *disableAliases (const QString snap, const QStringList &aliases);
    Q_INVOKABLE Q_DECL_DEPRECATED QSnapdResetAliasesRequest *resetAliases (const QString snap, const QStringList &aliases);
    Q_INVOKABLE QSnapdRunSnapCtlRequest *runSnapCtl (const QString contextId, const QStringList &args);

private:
    QSnapdClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QSnapdClient)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSnapdClient::GetAppsFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSnapdClient::FindFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSnapdClient::InstallFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSnapdClient::CreateUserFlags)

#endif
