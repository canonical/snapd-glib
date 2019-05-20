/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_PUT_SNAP_CONF_H__
#define __SNAPD_PUT_SNAP_CONF_H__

#include "snapd-request-async.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPutSnapConf, snapd_put_snap_conf, SNAPD, PUT_SNAP_CONF, SnapdRequestAsync)

SnapdPutSnapConf *_snapd_put_snap_conf_new (const gchar         *name,
                                            GHashTable          *key_values,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data);

G_END_DECLS

#endif /* __SNAPD_PUT_SNAP_CONF_H__ */
