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

/**
 * SnapdPromptingPermissionFlags:
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_NONE: no permissions requested.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_EXECUTE: execute.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_WRITE: write.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_READ: read.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_APPEND: append.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CREATE: create.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_DELETE: delete.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_OPEN: open.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_RENAME: rename.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_SET_ATTR: set attribute.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_GET_ATTR: get attribute.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_SET_CRED: set credential.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_GET_CRED: get credential.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_MODE: change mode.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_OWNER: change owner.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_GROUP: change group.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_LOCK: lock.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_EXECUTE_MAP: execute map.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_LINK: link.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_PROFILE: change profile.
 * @SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_PROFILE_ON_EXEC: change profile in execution.
 *
 * Permissions requested in a prompt.
 *
 * Since: 1.65
 */
typedef enum
{
    SNAPD_PROMPTING_PERMISSION_FLAGS_NONE = 0,
    SNAPD_PROMPTING_PERMISSION_FLAGS_EXECUTE = 1 << 0,
    SNAPD_PROMPTING_PERMISSION_FLAGS_WRITE = 1 << 1,
    SNAPD_PROMPTING_PERMISSION_FLAGS_READ = 1 << 2,
    SNAPD_PROMPTING_PERMISSION_FLAGS_APPEND = 1 << 3,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CREATE = 1 << 4,
    SNAPD_PROMPTING_PERMISSION_FLAGS_DELETE = 1 << 5,
    SNAPD_PROMPTING_PERMISSION_FLAGS_OPEN = 1 << 6,
    SNAPD_PROMPTING_PERMISSION_FLAGS_RENAME = 1 << 7,
    SNAPD_PROMPTING_PERMISSION_FLAGS_SET_ATTR = 1 << 8,
    SNAPD_PROMPTING_PERMISSION_FLAGS_GET_ATTR = 1 << 9,
    SNAPD_PROMPTING_PERMISSION_FLAGS_SET_CRED = 1 << 10,
    SNAPD_PROMPTING_PERMISSION_FLAGS_GET_CRED = 1 << 11,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_MODE = 1 << 12,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_OWNER = 1 << 13,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_GROUP = 1 << 14,
    SNAPD_PROMPTING_PERMISSION_FLAGS_LOCK = 1 << 15,
    SNAPD_PROMPTING_PERMISSION_FLAGS_EXECUTE_MAP = 1 << 16,
    SNAPD_PROMPTING_PERMISSION_FLAGS_LINK = 1 << 17,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_PROFILE = 1 << 18,
    SNAPD_PROMPTING_PERMISSION_FLAGS_CHANGE_PROFILE_ON_EXEC = 1 << 19
} SnapdPromptingPermissionFlags;

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PROMPTING_REQUEST  (snapd_prompting_request_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPromptingRequest, snapd_prompting_request, SNAPD, PROMPTING_REQUEST, GObject)

const gchar                  *snapd_prompting_request_get_id            (SnapdPromptingRequest *request);

const gchar                  *snapd_prompting_request_get_snap          (SnapdPromptingRequest *request);

const gchar                  *snapd_prompting_request_get_app           (SnapdPromptingRequest *request);

const gchar                  *snapd_prompting_request_get_path          (SnapdPromptingRequest *request);

const gchar                  *snapd_prompting_request_get_resource_type (SnapdPromptingRequest *request);

SnapdPromptingPermissionFlags snapd_prompting_request_get_permissions   (SnapdPromptingRequest *request);

G_END_DECLS

#endif /* __SNAPD_PROMPTING_REQUEST_H__ */
