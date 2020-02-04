/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_CLIENT_PRIVATE_H
#define SNAPD_CLIENT_PRIVATE_H

#include <snapd-glib/snapd-glib.h>

#include "stream-wrapper.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QIODevice>

class QSnapdConnectRequestPrivate
{
};

class QSnapdLoginRequestPrivate
{
public:
    QSnapdLoginRequestPrivate (const QString& email, const QString& password, const QString& otp) :
        email(email), password(password), otp(otp) {}
    ~QSnapdLoginRequestPrivate ()
    {
        if (user_information != NULL)
            g_object_unref (user_information);
        if (auth_data != NULL)
            g_object_unref (auth_data);
    }
    QString email;
    QString password;
    QString otp;
    SnapdUserInformation *user_information = NULL;
    SnapdAuthData *auth_data = NULL;
};

class QSnapdLogoutRequestPrivate
{
public:
    QSnapdLogoutRequestPrivate (qint64 id) :
        id (id) {}
    qint64 id;
};

class QSnapdGetChangesRequestPrivate
{
public:
    QSnapdGetChangesRequestPrivate (int filter, const QString& snapName) :
        filter(filter), snapName(snapName) {}
    ~QSnapdGetChangesRequestPrivate ()
    {
        if (changes != NULL)
            g_ptr_array_unref (changes);
    }
    int filter;
    QString snapName;
    GPtrArray *changes = NULL;
};

class QSnapdGetChangeRequestPrivate
{
public:
    QSnapdGetChangeRequestPrivate (const QString& id) :
        id(id) {}
    ~QSnapdGetChangeRequestPrivate ()
    {
        if (change != NULL)
            g_object_unref (change);
    }
    QString id;
    SnapdChange *change = NULL;
};

class QSnapdAbortChangeRequestPrivate
{
public:
    QSnapdAbortChangeRequestPrivate (const QString& id) :
        id(id) {}
    ~QSnapdAbortChangeRequestPrivate ()
    {
        if (change != NULL)
            g_object_unref (change);
    }
    QString id;
    SnapdChange *change = NULL;
};

class QSnapdGetSystemInformationRequestPrivate
{
public:
    ~QSnapdGetSystemInformationRequestPrivate ()
    {
        if (info != NULL)
            g_object_unref (info);
    }
    SnapdSystemInformation *info = NULL;
};

class QSnapdListRequestPrivate
{
public:
    ~QSnapdListRequestPrivate ()
    {
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    GPtrArray *snaps = NULL;
};

class QSnapdGetSnapsRequestPrivate
{
public:
    QSnapdGetSnapsRequestPrivate (int flags, const QStringList& snaps) :
        flags(flags), filter_snaps(snaps) {}
    ~QSnapdGetSnapsRequestPrivate ()
    {
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    int flags;
    QStringList filter_snaps;
    GPtrArray *snaps = NULL;
};

class QSnapdListOneRequestPrivate
{
public:
    QSnapdListOneRequestPrivate (const QString& name) :
        name(name) {}
    ~QSnapdListOneRequestPrivate ()
    {
        if (snap != NULL)
            g_object_unref (snap);
    }
    QString name;
    SnapdSnap *snap = NULL;
};

class QSnapdGetSnapRequestPrivate
{
public:
    QSnapdGetSnapRequestPrivate (const QString& name) :
        name(name) {}
    ~QSnapdGetSnapRequestPrivate ()
    {
        if (snap != NULL)
            g_object_unref (snap);
    }
    QString name;
    SnapdSnap *snap = NULL;
};

class QSnapdGetSnapConfRequestPrivate
{
public:
    QSnapdGetSnapConfRequestPrivate (const QString& name, const QStringList& keys) :
        name(name), keys(keys) {}
    ~QSnapdGetSnapConfRequestPrivate ()
    {
        if (configuration != NULL)
            g_hash_table_unref (configuration);
    }
    QString name;
    QStringList keys;
    GHashTable *configuration = NULL;
};

class QSnapdSetSnapConfRequestPrivate
{
public:
    QSnapdSetSnapConfRequestPrivate (const QString& name, const QHash<QString, QVariant>& configuration) :
        name(name), configuration(configuration) {}
    QString name;
    QHash<QString, QVariant> configuration;
};

class QSnapdGetAppsRequestPrivate
{
public:
    QSnapdGetAppsRequestPrivate (int flags, const QStringList& snaps) :
        flags(flags), filter_snaps(snaps) {}
    ~QSnapdGetAppsRequestPrivate ()
    {
        if (apps != NULL)
            g_ptr_array_unref (apps);
    }
    int flags;
    QStringList filter_snaps;
    GPtrArray *apps = NULL;
};

class QSnapdGetIconRequestPrivate
{
public:
    QSnapdGetIconRequestPrivate (const QString& name) :
        name(name) {}
    ~QSnapdGetIconRequestPrivate ()
    {
        if (icon != NULL)
            g_object_unref (icon);
    }
    QString name;
    SnapdIcon *icon = NULL;
};

class QSnapdGetAssertionsRequestPrivate
{
public:
    QSnapdGetAssertionsRequestPrivate (const QString& type) :
        type (type) {}
    ~QSnapdGetAssertionsRequestPrivate ()
    {
        if (assertions != NULL)
            g_strfreev (assertions);
    }
    QString type;
    GStrv assertions = NULL;
};

class QSnapdAddAssertionsRequestPrivate
{
public:
    QSnapdAddAssertionsRequestPrivate (const QStringList& assertions) :
        assertions (assertions) {}
    QStringList assertions;
};

class QSnapdGetConnectionsRequestPrivate
{
public:
    QSnapdGetConnectionsRequestPrivate (int flags, const QString &snap, const QString &interface) :
        flags (flags), snap (snap), interface (interface) {};
    ~QSnapdGetConnectionsRequestPrivate ()
    {
        if (established != NULL)
            g_ptr_array_unref (established);
        if (undesired != NULL)
            g_ptr_array_unref (undesired);
        if (plugs != NULL)
            g_ptr_array_unref (plugs);
        if (slots_ != NULL)
            g_ptr_array_unref (slots_);
    }
    int flags;
    QString snap;
    QString interface;
    GPtrArray *established = NULL;
    GPtrArray *undesired = NULL;
    GPtrArray *plugs = NULL;
    GPtrArray *slots_ = NULL;
};

class QSnapdGetInterfacesRequestPrivate
{
public:
    ~QSnapdGetInterfacesRequestPrivate ()
    {
        if (plugs != NULL)
            g_ptr_array_unref (plugs);
        if (slots_ != NULL)
            g_ptr_array_unref (slots_);
    }
    GPtrArray *plugs = NULL;
    GPtrArray *slots_ = NULL;
};

class QSnapdGetInterfaces2RequestPrivate
{
public:
    QSnapdGetInterfaces2RequestPrivate (int flags, const QStringList &names) :
        flags (flags), names (names) {};
    ~QSnapdGetInterfaces2RequestPrivate ()
    {
        if (interfaces != NULL)
            g_ptr_array_unref (interfaces);
    }
    int flags;
    QStringList names;
    GPtrArray *interfaces = NULL;
};

class QSnapdConnectInterfaceRequestPrivate
{
public:
    QSnapdConnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

class QSnapdDisconnectInterfaceRequestPrivate
{
public:
    QSnapdDisconnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

class QSnapdFindRequestPrivate
{
public:
    QSnapdFindRequestPrivate (int flags, const QString& section, const QString& name) :
        flags (flags), section (section), name (name) {}
    ~QSnapdFindRequestPrivate ()
    {
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    int flags;
    QString section;
    QString name;
    GPtrArray *snaps = NULL;
    QString suggestedCurrency;
};

class QSnapdFindRefreshableRequestPrivate
{
public:
    QSnapdFindRefreshableRequestPrivate () {}
    ~QSnapdFindRefreshableRequestPrivate ()
    {
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    GPtrArray *snaps = NULL;
};

class QSnapdInstallRequestPrivate : public QObject
{
public:
    Q_OBJECT

public:
    QSnapdInstallRequestPrivate (int flags, const QString& name, const QString& channel, const QString& revision, QIODevice *ioDevice, QObject *parent = NULL) :
        QObject (parent),
        flags(flags), name(name), channel(channel), revision(revision)
    {
        if (ioDevice != NULL) {
            wrapper = (StreamWrapper *) g_object_new (stream_wrapper_get_type (), NULL);
            wrapper->ioDevice = ioDevice;
        }
    }

    ~QSnapdInstallRequestPrivate ()
    {
        g_clear_object (&wrapper);
    }

    int flags;
    QString name;
    QString channel;
    QString revision;
    StreamWrapper *wrapper = NULL;
};

class QSnapdTryRequestPrivate
{
public:
    QSnapdTryRequestPrivate (const QString& path) :
        path(path) {}
    QString path;
};

class QSnapdRefreshRequestPrivate
{
public:
    QSnapdRefreshRequestPrivate (const QString& name, const QString& channel) :
        name(name), channel(channel) {}
    QString name;
    QString channel;
};

class QSnapdRefreshAllRequestPrivate
{
public:
    QSnapdRefreshAllRequestPrivate () {}
    ~QSnapdRefreshAllRequestPrivate ()
    {
        if (snap_names != NULL)
            g_strfreev (snap_names);
    }
    GStrv snap_names = NULL;
};

class QSnapdRemoveRequestPrivate
{
public:
    QSnapdRemoveRequestPrivate (int flags, const QString& name) :
        flags(flags), name(name) {}
    int flags;
    QString name;
};

class QSnapdEnableRequestPrivate
{
public:
    QSnapdEnableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

class QSnapdDisableRequestPrivate
{
public:
    QSnapdDisableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

class QSnapdSwitchChannelRequestPrivate
{
public:
    QSnapdSwitchChannelRequestPrivate (const QString& name, const QString& channel) :
        name(name), channel(channel) {}
    QString name;
    QString channel;
};

class QSnapdCheckBuyRequestPrivate
{
public:
    bool canBuy;
};

class QSnapdBuyRequestPrivate
{
public:
    QSnapdBuyRequestPrivate (const QString& id, double amount, const QString& currency) :
      id(id), amount(amount), currency(currency) {}
    QString id;
    double amount;
    QString currency;
};

class QSnapdCreateUserRequestPrivate
{
public:
    QSnapdCreateUserRequestPrivate (const QString& email, int flags) :
      email(email), flags(flags) {}
    ~QSnapdCreateUserRequestPrivate ()
    {
        if (info != NULL)
            g_object_unref (info);
    }
    QString email;
    int flags;
    SnapdUserInformation *info = NULL;
};

class QSnapdCreateUsersRequestPrivate
{
public:
    ~QSnapdCreateUsersRequestPrivate ()
    {
        if (info != NULL)
            g_ptr_array_unref (info);
    }
    GPtrArray *info = NULL;
};

class QSnapdGetUsersRequestPrivate
{
public:
    ~QSnapdGetUsersRequestPrivate ()
    {
        if (info != NULL)
            g_ptr_array_unref (info);
    }
    GPtrArray *info = NULL;
};

class QSnapdGetSectionsRequestPrivate
{
public:
    ~QSnapdGetSectionsRequestPrivate ()
    {
        if (sections != NULL)
            g_strfreev (sections);
    }
    GStrv sections = NULL;
};

class QSnapdGetAliasesRequestPrivate
{
public:
    ~QSnapdGetAliasesRequestPrivate ()
    {
        if (aliases != NULL)
            g_ptr_array_unref (aliases);
    }
    GPtrArray *aliases = NULL;
};

class QSnapdAliasRequestPrivate
{
public:
    QSnapdAliasRequestPrivate (const QString &snap, const QString &app, const QString &alias) :
        snap (snap), app (app), alias (alias) {}
    QString snap;
    QString app;
    QString alias;
};

class QSnapdUnaliasRequestPrivate
{
public:
    QSnapdUnaliasRequestPrivate (const QString &snap, const QString &alias) :
        snap (snap), alias (alias) {}
    QString snap;
    QString alias;
};

class QSnapdPreferRequestPrivate
{
public:
    QSnapdPreferRequestPrivate (const QString &snap) :
        snap (snap) {}
    QString snap;
    QString app;
    QString alias;
};

class QSnapdEnableAliasesRequestPrivate
{
public:
    QSnapdEnableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

class QSnapdDisableAliasesRequestPrivate
{
public:
    QSnapdDisableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

class QSnapdResetAliasesRequestPrivate
{
public:
    QSnapdResetAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

class QSnapdRunSnapCtlRequestPrivate
{
public:
    QSnapdRunSnapCtlRequestPrivate (const QString &contextId, const QStringList& args) :
        contextId (contextId), args (args) {}
    ~QSnapdRunSnapCtlRequestPrivate ()
    {
        if (stdout_output != NULL)
            g_free (stdout_output);
        if (stderr_output != NULL)
            g_free (stderr_output);
    }
    QString contextId;
    QStringList args;
    gchar *stdout_output = NULL;
    gchar *stderr_output = NULL;
};

class QSnapdDownloadRequestPrivate
{
public:
    QSnapdDownloadRequestPrivate (const QString &name, const QString& channel, const QString& revision) :
        name (name), channel (channel), revision (revision) {}
    ~QSnapdDownloadRequestPrivate ()
    {
        if (data != NULL)
            g_bytes_unref (data);
    }
    QString name;
    QString channel;
    QString revision;
    GBytes *data = NULL;
};

#endif
