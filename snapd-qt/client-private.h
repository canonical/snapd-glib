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
#include <QIODevice>

struct QSnapdConnectRequestPrivate
{
};

struct QSnapdLoginRequestPrivate
{
    QSnapdLoginRequestPrivate (const QString& username, const QString& password, const QString& otp) :
        username(username), password(password), otp(otp) {}
    ~QSnapdLoginRequestPrivate ()
    {
        if (auth_data != NULL)
            g_object_unref (auth_data);
    }
    QString username;
    QString password;
    QString otp;
    SnapdAuthData *auth_data = NULL;
};

struct QSnapdGetSystemInformationRequestPrivate
{
    ~QSnapdGetSystemInformationRequestPrivate ()
    {
        if (info != NULL)
            g_object_unref (info);
    }
    SnapdSystemInformation *info = NULL;
};

struct QSnapdListRequestPrivate
{
    ~QSnapdListRequestPrivate ()
    {
        if (snaps != NULL)
            g_ptr_array_unref (snaps);
    }
    GPtrArray *snaps = NULL;
};

struct QSnapdListOneRequestPrivate
{
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

struct QSnapdGetIconRequestPrivate
{
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

struct QSnapdGetAssertionsRequestPrivate
{
    QSnapdGetAssertionsRequestPrivate (const QString& type) :
        type (type) {}
    ~QSnapdGetAssertionsRequestPrivate ()
    {
        if (assertions != NULL)
            g_strfreev (assertions);
    }
    QString type;
    gchar **assertions = NULL;
};

struct QSnapdAddAssertionsRequestPrivate
{
    QSnapdAddAssertionsRequestPrivate (const QStringList& assertions) :
        assertions (assertions) {}
    QStringList assertions;
};

struct QSnapdGetInterfacesRequestPrivate
{
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

struct QSnapdConnectInterfaceRequestPrivate
{
    QSnapdConnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

struct QSnapdDisconnectInterfaceRequestPrivate
{
    QSnapdDisconnectInterfaceRequestPrivate (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name) :
        plug_snap (plug_snap), plug_name (plug_name), slot_snap (slot_snap), slot_name (slot_name) {}
    QString plug_snap;
    QString plug_name;
    QString slot_snap;
    QString slot_name;
};

struct QSnapdFindRequestPrivate
{
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

struct QSnapdFindRefreshableRequestPrivate
{
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
    Q_OBJECT

public:    
    QSnapdInstallRequestPrivate (int flags, const QString& name, const QString& channel, QIODevice *ioDevice, QObject *parent = NULL) :
        QObject (parent),
        flags(flags), name(name), channel(channel)
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
    StreamWrapper *wrapper = NULL;
};

struct QSnapdTryRequestPrivate
{
    QSnapdTryRequestPrivate (const QString& path) :
        path(path) {}
    QString path;
};

struct QSnapdRefreshRequestPrivate
{
    QSnapdRefreshRequestPrivate (const QString& name, const QString& channel) :
        name(name), channel(channel) {}
    QString name;
    QString channel;
};

struct QSnapdRefreshAllRequestPrivate
{
    QSnapdRefreshAllRequestPrivate () {}
    ~QSnapdRefreshAllRequestPrivate ()
    {
        if (snap_names != NULL)
            g_strfreev (snap_names);
    }  
    gchar **snap_names = NULL;
};

struct QSnapdRemoveRequestPrivate
{
    QSnapdRemoveRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdEnableRequestPrivate
{
    QSnapdEnableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdDisableRequestPrivate
{
    QSnapdDisableRequestPrivate (const QString& name) :
        name(name) {}
    QString name;
};

struct QSnapdCheckBuyRequestPrivate
{
    bool canBuy;
};

struct QSnapdBuyRequestPrivate
{
    QSnapdBuyRequestPrivate (const QString& id, double amount, const QString& currency) :
      id(id), amount(amount), currency(currency) {}
    QString id;
    double amount;
    QString currency;
};

struct QSnapdGetSectionsRequestPrivate
{
    ~QSnapdGetSectionsRequestPrivate ()
    {
        if (sections != NULL)
            g_strfreev (sections);
    }
    gchar **sections = NULL;
};

struct QSnapdGetAliasesRequestPrivate
{
    ~QSnapdGetAliasesRequestPrivate ()
    {
        if (aliases != NULL)
            g_ptr_array_unref (aliases);
    }
    GPtrArray *aliases = NULL;
};

struct QSnapdEnableAliasesRequestPrivate
{
    QSnapdEnableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdDisableAliasesRequestPrivate
{
    QSnapdDisableAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdResetAliasesRequestPrivate
{
    QSnapdResetAliasesRequestPrivate (const QString &snap, const QStringList& aliases) :
        snap (snap), aliases (aliases) {}
    QString snap;
    QStringList aliases;
};

struct QSnapdRunSnapCtlRequestPrivate
{
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

#endif
