/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_PROMPTING_REQUEST_H__
#define __SNAPD_PROMPTING_REQUEST_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PROMPTING_REQUEST  (snapd_prompting_request_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPromptingRequest, snapd_prompting_request, SNAPD, PROMPTING_REQUEST, GObject)

const gchar *snapd_prompting_request_get_id            (SnapdPromptingRequest *request);

const gchar *snapd_prompting_request_get_snap          (SnapdPromptingRequest *request);

const gchar *snapd_prompting_request_get_app           (SnapdPromptingRequest *request);

const gchar *snapd_prompting_request_get_path          (SnapdPromptingRequest *request);

const gchar *snapd_prompting_request_get_resource_type (SnapdPromptingRequest *request);

const gchar *snapd_prompting_request_get_permission    (SnapdPromptingRequest *request);

G_END_DECLS

#endif /* __SNAPD_PROMPTING_REQUEST_H__ */
