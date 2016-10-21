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
}
