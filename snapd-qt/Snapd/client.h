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
class ClientPrivate;

class Q_DECL_EXPORT Client : public QObject
{
    Q_OBJECT

public:
    explicit Client (QObject* parent=0);
    void connectSync ();
    Snapd::AuthData loginSync (const QString &username, const QString &password, const QString &otp = "");
    void setAuthData (const Snapd::AuthData& auth_data);
    Snapd::AuthData authData ();
    Snapd::SystemInformation getSystemInformationSync ();
    QList<Snapd::Snap> listSync ();
    Snapd::Snap listOneSync (const QString &name);
    Snapd::Icon getIconSync (const QString &name);
    //FIXMEvoid getInterfacesSync (GPtrArray **plugs, GPtrArray **slots);
    /*void connectInterfaceSync (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    void disconnectInterfaceSync (const QString &plug_snap, const QString &plug_name, const QString &slot_snap, const QString &slot_name, SnapdProgressCallback progress_callback, gpointer progress_callback_data);
    QList<Snapd::Snap> findSync (SnapdFindFlags flags, const QString &query, gchar **suggested_currency);
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
