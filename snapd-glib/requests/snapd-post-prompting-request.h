/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_POST_PROMPTING_REQUEST_H__
#define __SNAPD_POST_PROMPTING_REQUEST_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdPostPromptingRequest, snapd_post_prompting_request, SNAPD, POST_PROMPTING_REQUEST, SnapdRequest)

SnapdPostPromptingRequest *_snapd_post_prompting_request_new (const gchar         *id,
                                                              const gchar         *outcome,
                                                              const gchar         *lifespan,
                                                              gint64               duration,
                                                              const gchar         *path_pattern,
                                                              GStrv                permissions,
                                                              GCancellable        *cancellable,
                                                              GAsyncReadyCallback  callback,
                                                              gpointer             user_data);

G_END_DECLS

#endif /* __SNAPD_POST_PROMPTING_REQUEST_H__ */
