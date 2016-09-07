/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_LOGIN_H
#define SNAPD_LOGIN_H

#include <Snapd/AuthData>

namespace Snapd
{

Snapd::AuthData loginSync (const QString &username, const QString &password, const QString &otp = "");

}

#endif
