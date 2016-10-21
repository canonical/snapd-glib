/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <QtQml/QtQml>
#include <Snapd/Client>
#include "qml-plugin.h"

void SnapdQmlPlugin::registerTypes(const char *uri) 
{
    Q_ASSERT(uri == QLatin1String("Snapd"));
    qmlRegisterType<Snapd::Client>(uri, 1, 0, "SnapdClient");
    qmlRegisterType<Snapd::AuthData>(uri, 1, 0, "AuthData");
    qmlRegisterUncreatableType<Snapd::Icon>(uri, 1, 0, "Icon", "Can't create");
    qmlRegisterUncreatableType<Snapd::Snap>(uri, 1, 0, "Snap", "Can't create");
    qmlRegisterUncreatableType<Snapd::SystemInformation>(uri, 1, 0, "SystemInformation", "Can't create");
    qmlRegisterUncreatableType<Snapd::Request>(uri, 1, 0, "Request", "Can't create");
    qmlRegisterUncreatableType<Snapd::ConnectRequest>(uri, 1, 0, "ConnectRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::LoginRequest>(uri, 1, 0, "LoginRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::SystemInformationRequest>(uri, 1, 0, "SystemInformationRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::ListRequest>(uri, 1, 0, "ListRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::ListOneRequest>(uri, 1, 0, "ListOneRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::IconRequest>(uri, 1, 0, "IconRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::FindRequest>(uri, 1, 0, "FindRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::InstallRequest>(uri, 1, 0, "InstallRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::RefreshRequest>(uri, 1, 0, "RefreshRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::RemoveRequest>(uri, 1, 0, "RemoveRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::EnableRequest>(uri, 1, 0, "EnableRequest", "Can't create");
    qmlRegisterUncreatableType<Snapd::DisableRequest>(uri, 1, 0, "DisableRequest", "Can't create");
}
