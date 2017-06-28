/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_LOGIN_PRIVATE_H__
#define __SNAPD_LOGIN_PRIVATE_H__

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdLoginRequest, snapd_login_request, SNAPD, LOGIN_REQUEST, GObject)

G_END_DECLS

#endif /* __SNAPD_LOGIN_PRIVATE_H__ */
