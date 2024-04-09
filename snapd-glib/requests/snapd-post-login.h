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

G_DECLARE_FINAL_TYPE (SnapdPostLogin, snapd_post_login, SNAPD, POST_LOGIN, SnapdRequest)

SnapdPostLogin       *_snapd_post_login_new                  (const gchar         *email,
                                                              const gchar         *password,
                                                              const gchar         *otp,
                                                              GCancellable        *cancellable,
                                                              GAsyncReadyCallback  callback,
                                                              gpointer             user_data);

SnapdUserInformation *_snapd_post_login_get_user_information (SnapdPostLogin *request);

G_END_DECLS
