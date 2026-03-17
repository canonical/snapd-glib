/*
 * Copyright (C) 2026 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdPostInterfacesRequests,
                     snapd_post_interfaces_requests, SNAPD,
                     POST_INTERFACES_REQUESTS, SnapdRequest)

SnapdPostInterfacesRequests *_snapd_post_interfaces_requests_new(
    const gchar *interface, GPid pid, GCancellable *cancellable,
    GAsyncReadyCallback callback, gpointer user_data);

gboolean _snapd_post_interfaces_requests_get_allowed(
    SnapdPostInterfacesRequests *request);

G_END_DECLS
