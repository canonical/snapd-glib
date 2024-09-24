/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdGetModelSerial, snapd_get_model_serial, SNAPD,
                     GET_MODEL_SERIAL, SnapdRequest)

SnapdGetModelSerial *_snapd_get_model_serial_new(GCancellable *cancellable,
                                                 GAsyncReadyCallback callback,
                                                 gpointer user_data);

const gchar *
_snapd_get_model_serial_get_serial_assertion(SnapdGetModelSerial *request);

G_END_DECLS
