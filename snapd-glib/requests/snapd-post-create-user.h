/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_CREATE_USER_H__
#define __SNAPD_POST_CREATE_USER_H__

#include "snapd-request.h"

#include "snapd-client.h"
#include "snapd-user-information.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostCreateUser, snapd_post_create_user, SNAPD, POST_CREATE_USER, SnapdRequest)

SnapdPostCreateUser  *_snapd_post_create_user_new                  (const gchar          *email,
                                                                    SnapdCreateUserFlags  flags,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);

SnapdUserInformation *_snapd_post_create_user_get_user_information (SnapdPostCreateUser  *request);

G_END_DECLS

#endif /* __SNAPD_POST_CREATE_USER_H__ */
