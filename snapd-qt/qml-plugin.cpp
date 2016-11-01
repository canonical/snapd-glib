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
    qmlRegisterType<QSnapdClient>(uri, 1, 0, "SnapdClient");
    qmlRegisterType<QSnapdAuthData>(uri, 1, 0, "AuthData");
    qmlRegisterUncreatableType<QSnapdIcon>(uri, 1, 0, "Icon", "Can't create");
    qmlRegisterUncreatableType<QSnapdSnap>(uri, 1, 0, "Snap", "Can't create");
    qmlRegisterUncreatableType<QSnapdSystemInformation>(uri, 1, 0, "SystemInformation", "Can't create");
    qmlRegisterUncreatableType<QSnapdRequest>(uri, 1, 0, "Request", "Can't create");
    qmlRegisterUncreatableType<QSnapdConnectRequest>(uri, 1, 0, "ConnectRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdLoginRequest>(uri, 1, 0, "LoginRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdSystemInformationRequest>(uri, 1, 0, "SystemInformationRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdListRequest>(uri, 1, 0, "ListRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdListOneRequest>(uri, 1, 0, "ListOneRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdIconRequest>(uri, 1, 0, "IconRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdFindRequest>(uri, 1, 0, "FindRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdInstallRequest>(uri, 1, 0, "InstallRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdRefreshRequest>(uri, 1, 0, "RefreshRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdRemoveRequest>(uri, 1, 0, "RemoveRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdEnableRequest>(uri, 1, 0, "EnableRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdDisableRequest>(uri, 1, 0, "DisableRequest", "Can't create");
}
