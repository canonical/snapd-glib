/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_USER_INFORMATION_H__
#define __SNAPD_USER_INFORMATION_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_USER_INFORMATION  (snapd_user_information_get_type ())

G_DECLARE_FINAL_TYPE (SnapdUserInformation, snapd_user_information, SNAPD, USER_INFORMATION, GObject)

const gchar  *snapd_user_information_get_username (SnapdUserInformation *user_information);

gchar       **snapd_user_information_get_ssh_keys (SnapdUserInformation *user_information);

G_END_DECLS

#endif /* __SNAPD_USER_INFORMATION_H__ */
