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
    qmlRegisterType<QSnapdAuthData>(uri, 1, 0, "SnapdAuthData");
    qmlRegisterUncreatableType<QSnapdIcon>(uri, 1, 0, "SnapdIcon", "Can't create");
    qmlRegisterUncreatableType<QSnapdSnap>(uri, 1, 0, "SnapdSnap", "Can't create");
    qmlRegisterUncreatableType<QSnapdSystemInformation>(uri, 1, 0, "SnapdSystemInformation", "Can't create");
    qmlRegisterUncreatableType<QSnapdRequest>(uri, 1, 0, "SnapdRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdConnectRequest>(uri, 1, 0, "SnapdConnectRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdLoginRequest>(uri, 1, 0, "SnapdLoginRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdSystemInformationRequest>(uri, 1, 0, "SnapdSystemInformationRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdListRequest>(uri, 1, 0, "SnapdListRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdListOneRequest>(uri, 1, 0, "SnapdListOneRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdIconRequest>(uri, 1, 0, "SnapdIconRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdFindRequest>(uri, 1, 0, "SnapdFindRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdInstallRequest>(uri, 1, 0, "SnapdInstallRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdRefreshRequest>(uri, 1, 0, "SnapdRefreshRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdRemoveRequest>(uri, 1, 0, "SnapdRemoveRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdEnableRequest>(uri, 1, 0, "SnapdEnableRequest", "Can't create");
    qmlRegisterUncreatableType<QSnapdDisableRequest>(uri, 1, 0, "SnapdDisableRequest", "Can't create");
}
