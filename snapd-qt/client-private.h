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

G_DECLARE_FINAL_TYPE (CallbackData, callback_data, SNAPD, CALLBACK_DATA, GObject)

struct _CallbackData
{
    GObject parent_instance;
    gpointer request;
};

CallbackData *callback_data_new (gpointer request);

class QSnapdConnectRequestPrivate
{
public:
    QSnapdConnectRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdConnectRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    CallbackData *callback_data;
};

class QSnapdLoginRequestPrivate
{
public:
    QSnapdLoginRequestPrivate (gpointer request, const QString& email, const QString& password, const QString& otp) :
        email(email), password(password), otp(otp) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdLoginRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (user_information != NULL)
            g_object_unref (user_information);
        if (auth_data != NULL)
            g_object_unref (auth_data);
    }
    QString email;
    QString password;
    QString otp;
    CallbackData *callback_data;
    SnapdUserInformation *user_information = NULL;
    SnapdAuthData *auth_data = NULL;
};

class QSnapdLogoutRequestPrivate
{
public:
    QSnapdLogoutRequestPrivate (gpointer request, qint64 id) :
        id (id) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdLogoutRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    qint64 id;
    CallbackData *callback_data;
};

class QSnapdGetChangesRequestPrivate
{
public:
    QSnapdGetChangesRequestPrivate (gpointer request, int filter, const QString& snapName) :
        filter(filter), snapName(snapName) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetChangesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (changes != NULL)
            g_ptr_array_unref (changes);
    }
    int filter;
    QString snapName;
    CallbackData *callback_data;
    GPtrArray *changes = NULL;
};

class QSnapdGetChangeRequestPrivate
{
public:
    QSnapdGetChangeRequestPrivate (gpointer request, const QString& id) :
        id(id) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetChangeRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (change != NULL)
            g_object_unref (change);
    }
    QString id;
    CallbackData *callback_data;
    SnapdChange *change = NULL;
};

class QSnapdAbortChangeRequestPrivate
{
public:
    QSnapdAbortChangeRequestPrivate (gpointer request, const QString& id) :
        id(id) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdAbortChangeRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (change != NULL)
            g_object_unref (change);
    }
    QString id;
    CallbackData *callback_data;
    SnapdChange *change = NULL;
};

class QSnapdGetSystemInformationRequestPrivate
{
public:
    QSnapdGetSystemInformationRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetSystemInformationRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (info != NULL)
            g_object_unref (info);
    }
    CallbackData *callback_data;
    SnapdSystemInformation *info = NULL;
};

class QSnapdListRequestPrivate
{
public:
    QSnapdListRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdListRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    CallbackData *callback_data;
    GPtrArray *snaps = NULL;
};

class QSnapdGetSnapsRequestPrivate
{
public:
    QSnapdGetSnapsRequestPrivate (gpointer request, int flags, const QStringList& snaps) :
        flags(flags), filter_snaps(snaps) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetSnapsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    int flags;
    QStringList filter_snaps;
    CallbackData *callback_data;
    GPtrArray *snaps = NULL;
};

class QSnapdListOneRequestPrivate
{
public:
    QSnapdListOneRequestPrivate (gpointer request, const QString& name) :
        name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdListOneRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snap != NULL)
            g_object_unref (snap);
    }
    QString name;
    CallbackData *callback_data;
    SnapdSnap *snap = NULL;
};

class QSnapdGetSnapRequestPrivate
{
public:
    QSnapdGetSnapRequestPrivate (gpointer request, const QString& name) :
        name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetSnapRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snap != NULL)
            g_object_unref (snap);
    }
    QString name;
    CallbackData *callback_data;
    SnapdSnap *snap = NULL;
};

class QSnapdGetSnapConfRequestPrivate
{
public:
    QSnapdGetSnapConfRequestPrivate (gpointer request, const QString& name, const QStringList& keys) :
        name(name), keys(keys) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetSnapConfRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (configuration != NULL)
            g_hash_table_unref (configuration);
    }
    QString name;
    QStringList keys;
    CallbackData *callback_data;
    GHashTable *configuration = NULL;
};

class QSnapdSetSnapConfRequestPrivate
{
public:
    QSnapdSetSnapConfRequestPrivate (gpointer request, const QString& name, const QHash<QString, QVariant>& configuration) :
        name(name), configuration(configuration) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdSetSnapConfRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString name;
    QHash<QString, QVariant> configuration;
    CallbackData *callback_data;
};

class QSnapdGetAppsRequestPrivate
{
public:
    QSnapdGetAppsRequestPrivate (gpointer request, int flags, const QStringList& snaps) :
        flags(flags), filter_snaps(snaps) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetAppsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (apps != NULL)
            g_ptr_array_unref (apps);
    }
    int flags;
    QStringList filter_snaps;
    CallbackData *callback_data;
    GPtrArray *apps = NULL;
};

class QSnapdGetIconRequestPrivate
{
public:
    QSnapdGetIconRequestPrivate (gpointer request, const QString& name) :
        name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetIconRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (icon != NULL)
            g_object_unref (icon);
    }
    QString name;
    CallbackData *callback_data;
    SnapdIcon *icon = NULL;
};

class QSnapdGetAssertionsRequestPrivate
{
public:
    QSnapdGetAssertionsRequestPrivate (gpointer request, const QString& type) :
        type (type) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetAssertionsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (assertions != NULL)
            g_strfreev (assertions);
    }
    QString type;
    CallbackData *callback_data;
    GStrv assertions = NULL;
};

class QSnapdAddAssertionsRequestPrivate
{
public:
    QSnapdAddAssertionsRequestPrivate (gpointer request, const QStringList& assertions) :
        assertions (assertions) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdAddAssertionsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QStringList assertions;
    CallbackData *callback_data;
};

class QSnapdGetConnectionsRequestPrivate
{
public:
    QSnapdGetConnectionsRequestPrivate (gpointer request, int flags, const QString &snap, const QString &interface) :
        flags (flags), snap (snap), interface (interface) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetConnectionsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
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
    CallbackData *callback_data;
    GPtrArray *established = NULL;
    GPtrArray *undesired = NULL;
    GPtrArray *plugs = NULL;
    GPtrArray *slots_ = NULL;
};

class QSnapdGetInterfacesRequestPrivate
{
public:
    QSnapdGetInterfacesRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetInterfacesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (plugs != NULL)
            g_ptr_array_unref (plugs);
        if (slots_ != NULL)
            g_ptr_array_unref (slots_);
    }
    CallbackData *callback_data;
    GPtrArray *plugs = NULL;
    GPtrArray *slots_ = NULL;
};

class QSnapdGetInterfaces2RequestPrivate
{
public:
    QSnapdGetInterfaces2RequestPrivate (gpointer request, int flags, const QStringList &names) :
        flags (flags), names (names) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetInterfaces2RequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (interfaces != NULL)
            g_ptr_array_unref (interfaces);
    }
    int flags;
    QStringList names;
    CallbackData *callback_data;
    GPtrArray *interfaces = NULL;
};

class QSnapdConnectInterfaceRequestPrivate
{
public:
    QSnapdConnectInterfaceRequestPrivate (gpointer request, const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdConnectInterfaceRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
    CallbackData *callback_data;
};

class QSnapdDisconnectInterfaceRequestPrivate
{
public:
    QSnapdDisconnectInterfaceRequestPrivate (gpointer request, const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdDisconnectInterfaceRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
    CallbackData *callback_data;
};

class QSnapdFindRequestPrivate
{
public:
    QSnapdFindRequestPrivate (gpointer request, int flags, const QString& section, const QString& name) :
        flags (flags), section (section), name (name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdFindRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    int flags;
    QString section;
    QString name;
    CallbackData *callback_data;
    GPtrArray *snaps = NULL;
    QString suggestedCurrency;
};

class QSnapdFindRefreshableRequestPrivate
{
public:
    QSnapdFindRefreshableRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdFindRefreshableRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    CallbackData *callback_data;
    GPtrArray *snaps = NULL;
};

class QSnapdInstallRequestPrivate : public QObject
{
public:
    QSnapdInstallRequestPrivate (gpointer request, int flags, const QString& name, const QString& channel, const QString& revision, QIODevice *ioDevice, QObject *parent = NULL) :
        QObject (parent),
        flags(flags), name(name), channel(channel), revision(revision)
    {
        callback_data = callback_data_new (request);
        if (ioDevice != NULL) {
            wrapper = (StreamWrapper *) g_object_new (stream_wrapper_get_type (), NULL);
            wrapper->ioDevice = ioDevice;
        }
    }
    ~QSnapdInstallRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        g_clear_object (&wrapper);
    }
    int flags;
    QString name;
    QString channel;
    QString revision;
    CallbackData *callback_data;
    StreamWrapper *wrapper = NULL;
};

class QSnapdTryRequestPrivate
{
public:
    QSnapdTryRequestPrivate (gpointer request, const QString& path) :
        path(path) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdTryRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString path;
    CallbackData *callback_data;
};

class QSnapdRefreshRequestPrivate
{
public:
    QSnapdRefreshRequestPrivate (gpointer request, const QString& name, const QString& channel) :
        name(name), channel(channel) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdRefreshRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString name;
    QString channel;
    CallbackData *callback_data;
};

class QSnapdRefreshAllRequestPrivate
{
public:
    QSnapdRefreshAllRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdRefreshAllRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (snap_names != NULL)
            g_strfreev (snap_names);
    }
    CallbackData *callback_data;
    GStrv snap_names = NULL;
};

class QSnapdRemoveRequestPrivate
{
public:
    QSnapdRemoveRequestPrivate (gpointer request, int flags, const QString& name) :
        flags(flags), name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdRemoveRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    int flags;
    QString name;
    CallbackData *callback_data;
};

class QSnapdEnableRequestPrivate
{
public:
    QSnapdEnableRequestPrivate (gpointer request, const QString& name) :
        name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdEnableRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString name;
    CallbackData *callback_data;
};

class QSnapdDisableRequestPrivate
{
public:
    QSnapdDisableRequestPrivate (gpointer request, const QString& name) :
        name(name) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdDisableRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString name;
    CallbackData *callback_data;
};

class QSnapdSwitchChannelRequestPrivate
{
public:
    QSnapdSwitchChannelRequestPrivate (gpointer request, const QString& name, const QString& channel) :
        name(name), channel(channel) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdSwitchChannelRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString name;
    QString channel;
    CallbackData *callback_data;
};

class QSnapdCheckBuyRequestPrivate
{
public:
    QSnapdCheckBuyRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdCheckBuyRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    CallbackData *callback_data;
    bool canBuy;
};

class QSnapdBuyRequestPrivate
{
public:
    QSnapdBuyRequestPrivate (gpointer request, const QString& id, double amount, const QString& currency) :
      id(id), amount(amount), currency(currency) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdBuyRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString id;
    double amount;
    QString currency;
    CallbackData *callback_data;
};

class QSnapdCreateUserRequestPrivate
{
public:
    QSnapdCreateUserRequestPrivate (gpointer request, const QString& email, int flags) :
      email(email), flags(flags) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdCreateUserRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (info != NULL)
            g_object_unref (info);
    }
    QString email;
    int flags;
    CallbackData *callback_data;
    SnapdUserInformation *info = NULL;
};

class QSnapdCreateUsersRequestPrivate
{
public:
    QSnapdCreateUsersRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdCreateUsersRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (info != NULL)
            g_ptr_array_unref (info);
    }
    CallbackData *callback_data;
    GPtrArray *info = NULL;
};

class QSnapdGetUsersRequestPrivate
{
public:
    QSnapdGetUsersRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetUsersRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (info != NULL)
            g_ptr_array_unref (info);
    }
    CallbackData *callback_data;
    GPtrArray *info = NULL;
};

class QSnapdGetSectionsRequestPrivate
{
public:
    QSnapdGetSectionsRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetSectionsRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (sections != NULL)
            g_strfreev (sections);
    }
    CallbackData *callback_data;
    GStrv sections = NULL;
};

class QSnapdGetAliasesRequestPrivate
{
public:
    QSnapdGetAliasesRequestPrivate (gpointer request) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdGetAliasesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (aliases != NULL)
            g_ptr_array_unref (aliases);
    }
    CallbackData *callback_data;
    GPtrArray *aliases = NULL;
};

class QSnapdAliasRequestPrivate
{
public:
    QSnapdAliasRequestPrivate (gpointer request, const QString &snap, const QString &app, const QString &alias) :
        snap (snap), app (app), alias (alias) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdAliasRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    QString app;
    QString alias;
    CallbackData *callback_data;
};

class QSnapdUnaliasRequestPrivate
{
public:
    QSnapdUnaliasRequestPrivate (gpointer request, const QString &snap, const QString &alias) :
        snap (snap), alias (alias) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdUnaliasRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    QString alias;
    CallbackData *callback_data;
};

class QSnapdPreferRequestPrivate
{
public:
    QSnapdPreferRequestPrivate (gpointer request, const QString &snap) :
        snap (snap) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdPreferRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    CallbackData *callback_data;
    QString app;
    QString alias;
};

class QSnapdEnableAliasesRequestPrivate
{
public:
    QSnapdEnableAliasesRequestPrivate (gpointer request, const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdEnableAliasesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    QStringList aliases;
    CallbackData *callback_data;
};

class QSnapdDisableAliasesRequestPrivate
{
public:
    QSnapdDisableAliasesRequestPrivate (gpointer request, const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdDisableAliasesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    QStringList aliases;
    CallbackData *callback_data;
};

class QSnapdResetAliasesRequestPrivate
{
public:
    QSnapdResetAliasesRequestPrivate (gpointer request, const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdResetAliasesRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
    }
    QString snap;
    QStringList aliases;
    CallbackData *callback_data;
};

class QSnapdRunSnapCtlRequestPrivate
{
public:
    QSnapdRunSnapCtlRequestPrivate (gpointer request, const QString &contextId, const QStringList& args) :
        contextId (contextId), args (args) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdRunSnapCtlRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (stdout_output != NULL)
            g_free (stdout_output);
        if (stderr_output != NULL)
            g_free (stderr_output);
    }
    QString contextId;
    QStringList args;
    CallbackData *callback_data;
    gchar *stdout_output = NULL;
    gchar *stderr_output = NULL;
    int exit_code = 0;
};

class QSnapdDownloadRequestPrivate
{
public:
    QSnapdDownloadRequestPrivate (gpointer request, const QString &name, const QString& channel, const QString& revision) :
        name (name), channel (channel), revision (revision) {
        callback_data = callback_data_new (request);
    }
    ~QSnapdDownloadRequestPrivate ()
    {
        callback_data->request = NULL;
        g_object_unref (callback_data);
        if (data != NULL)
            g_bytes_unref (data);
    }
    QString name;
    QString channel;
    QString revision;
    CallbackData *callback_data;
    GBytes *data = NULL;
};

#endif
