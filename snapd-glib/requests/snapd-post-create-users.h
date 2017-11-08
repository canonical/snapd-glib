/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_CREATE_USERS_H__
#define __SNAPD_POST_CREATE_USERS_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostCreateUsers, snapd_post_create_users, SNAPD, POST_CREATE_USERS, SnapdRequest)

SnapdPostCreateUsers *_snapd_post_create_users_new                   (GCancellable         *cancellable,
                                                                       GAsyncReadyCallback   callback,
                                                                       gpointer              user_data);

GPtrArray            *_snapd_post_create_users_get_users_information (SnapdPostCreateUsers  *request);

G_END_DECLS

#endif /* __SNAPD_POST_CREATE_USERS_H__ */
