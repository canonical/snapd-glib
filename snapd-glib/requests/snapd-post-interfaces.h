/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_INTERFACES_H__
#define __SNAPD_POST_INTERFACES_H__

#include "snapd-request-async.h"

#include "snapd-client.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostInterfaces, snapd_post_interfaces, SNAPD, POST_INTERFACES, SnapdRequestAsync)

SnapdPostInterfaces *_snapd_post_interfaces_new (const gchar           *action,
                                                 const gchar           *plug_snap,
                                                 const gchar           *plug_name,
                                                 const gchar           *slot_snap,
                                                 const gchar           *slot_name,
                                                 SnapdProgressCallback  progress_callback,
                                                 gpointer               progress_callback_data,
                                                 GCancellable          *cancellable,
                                                 GAsyncReadyCallback    callback,
                                                 gpointer               user_data);

G_END_DECLS

#endif /* __SNAPD_POST_INTERFACES_H__ */
