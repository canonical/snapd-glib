/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_PROMPTING_REQUESTS_H__
#define __SNAPD_GET_PROMPTING_REQUESTS_H__

#include "snapd-request.h"
#include "snapd-prompting-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetPromptingRequests, snapd_get_prompting_requests, SNAPD, GET_PROMPTING_REQUESTS, SnapdRequest)

typedef void (*SnapdGetPromptingRequestsRequestCallback) (SnapdGetPromptingRequests *request, SnapdPromptingRequest *prompting_request, gpointer user_data);

SnapdGetPromptingRequests *_snapd_get_prompting_requests_new          (gboolean                                 follow,
                                                                       SnapdGetPromptingRequestsRequestCallback request_callback,
                                                                       gpointer                                 request_callback_data,
                                                                       GDestroyNotify                           request_callback_destroy_notify,
                                                                       GCancellable                            *cancellable,
                                                                       GAsyncReadyCallback                      callback,
                                                                       gpointer                                 user_data);

GPtrArray                 *_snapd_get_prompting_requests_get_requests (SnapdGetPromptingRequests *request);

G_END_DECLS

#endif /* __SNAPD_GET_PROMPTING_REQUESTS_H__ */
