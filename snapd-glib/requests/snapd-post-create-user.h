/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"
#include "snapd-user-information.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdPostCreateUser, snapd_post_create_user, SNAPD,
                     POST_CREATE_USER, SnapdRequest)

SnapdPostCreateUser *_snapd_post_create_user_new(const gchar *email,
                                                 GCancellable *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer user_data);

void _snapd_post_create_user_set_sudoer(SnapdPostCreateUser *request,
                                        gboolean sudoer);

void _snapd_post_create_user_set_known(SnapdPostCreateUser *request,
                                       gboolean known);

SnapdUserInformation *
_snapd_post_create_user_get_user_information(SnapdPostCreateUser *request);

G_END_DECLS
