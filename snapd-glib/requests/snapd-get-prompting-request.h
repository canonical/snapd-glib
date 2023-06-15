/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_PROMPTING_REQUEST_H__
#define __SNAPD_GET_PROMPTING_REQUEST_H__

#include "snapd-request.h"
#include "snapd-prompting-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetPromptingRequest, snapd_get_prompting_request, SNAPD, GET_PROMPTING_REQUEST, SnapdRequest)

SnapdGetPromptingRequest *_snapd_get_prompting_request_new         (const char          *id,
                                                                    GCancellable        *cancellable,
                                                                    GAsyncReadyCallback  callback,
                                                                    gpointer             user_data);

SnapdPromptingRequest    *_snapd_get_prompting_request_get_request (SnapdGetPromptingRequest *request);

G_END_DECLS

#endif /* __SNAPD_GET_PROMPTING_REQUEST_H__ */
