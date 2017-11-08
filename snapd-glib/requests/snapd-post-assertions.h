/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_ASSERTIONS_H__
#define __SNAPD_POST_ASSERTIONS_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostAssertions, snapd_post_assertions, SNAPD, POST_ASSERTIONS, SnapdRequest)

SnapdPostAssertions *_snapd_post_assertions_new (gchar              **assertions,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);

G_END_DECLS

#endif /* __SNAPD_POST_ASSERTIONS_H__ */
