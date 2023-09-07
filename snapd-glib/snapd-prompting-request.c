/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-prompting-request.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-prompting-request
 * @short_description: Snap prompting request metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPromptingRequest contains the metadata for a prompting request as returned
 * using snapd_snap_get_prompting_requests().
 */

/**
 * SnapdPromptingRequest:
 *
 * #SnapdPromptingRequest is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.65
 */

struct _SnapdPromptingRequest
{
    GObject parent_instance;

    gchar *id;
    gchar *snap;
    gchar *app;
    gchar *path;
    gchar *resource_type; // FIXME: enum
    SnapdPromptingPermissionFlags permissions;
};

enum
{
    PROP_ID = 1,
    PROP_SNAP,
    PROP_APP,
    PROP_PATH,
    PROP_RESOURCE_TYPE,
    PROP_PERMISSIONS,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdPromptingRequest, snapd_prompting_request, G_TYPE_OBJECT)

/**
 * snapd_prompting_request_get_id:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the id of this prompt request, e.g. "123".
 *
 * Returns: a request ID.
 *
 * Since: 1.65
 */
const gchar *
snapd_prompting_request_get_id (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), NULL);
    return self->id;
}

/**
 * snapd_prompting_request_get_snap:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the snap this prompt request is for, e.g. "firefox".
 *
 * Returns: a snap name.
 *
 * Since: 1.65
 */
const gchar *
snapd_prompting_request_get_snap (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), NULL);
    return self->snap;
}

/**
 * snapd_prompting_request_get_app:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the app this prompt request is for, e.g. "firefox".
 *
 * Returns: an app name.
 *
 * Since: 1.65
 */
const gchar *
snapd_prompting_request_get_app (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), NULL);
    return self->app;
}

/**
 * snapd_prompting_request_get_path:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the path that is being requested, e.g. "/home/foo/somefile.txt".
 *
 * Returns: a file path.
 *
 * Since: 1.65
 */
const gchar *
snapd_prompting_request_get_path (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), NULL);
    return self->path;
}

/**
 * snapd_prompting_request_get_resource_type:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the resource type of this prompt request, e.g. "file".
 *
 * Returns: a request resource type.
 *
 * Since: 1.65
 */
const gchar *
snapd_prompting_request_get_resource_type (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), NULL);
    return self->resource_type;
}

/**
 * snapd_prompting_request_get_permissions:
 * @request: a #SnapdPromptingRequest.
 *
 * Get the permissions requested in this prompt request, e.g. SNAPD_PROMPTING_PERMISSION_FLAGS_READ.
 *
 * Returns: permissions.
 *
 * Since: 1.65
 */
SnapdPromptingPermissionFlags
snapd_prompting_request_get_permissions (SnapdPromptingRequest *self)
{
    g_return_val_if_fail (SNAPD_IS_PROMPTING_REQUEST (self), SNAPD_PROMPTING_PERMISSION_FLAGS_NONE);
    return self->permissions;
}

static void
snapd_prompting_request_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPromptingRequest *self = SNAPD_PROMPTING_REQUEST (object);

    switch (prop_id) {
    case PROP_ID:
        g_free (self->id);
        self->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (self->snap);
        self->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_APP:
        g_free (self->app);
        self->app = g_strdup (g_value_get_string (value));
        break;
    case PROP_PATH:
        g_free (self->path);
        self->path = g_strdup (g_value_get_string (value));
        break;
    case PROP_RESOURCE_TYPE:
        g_free (self->resource_type);
        self->resource_type = g_strdup (g_value_get_string (value));
        break;
    case PROP_PERMISSIONS:
        self->permissions = g_value_get_flags (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_prompting_request_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPromptingRequest *self = SNAPD_PROMPTING_REQUEST (object);

    switch (prop_id) {
    case PROP_ID:
        g_value_set_string (value, self->id);
        break;
    case PROP_SNAP:
        g_value_set_string (value, self->snap);
        break;
    case PROP_APP:
        g_value_set_string (value, self->app);
        break;
    case PROP_PATH:
        g_value_set_string (value, self->path);
        break;
    case PROP_RESOURCE_TYPE:
        g_value_set_string (value, self->resource_type);
        break;
    case PROP_PERMISSIONS:
        g_value_set_flags (value, self->permissions);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_prompting_request_finalize (GObject *object)
{
    SnapdPromptingRequest *self = SNAPD_PROMPTING_REQUEST (object);

    g_clear_pointer (&self->id, g_free);
    g_clear_pointer (&self->snap, g_free);
    g_clear_pointer (&self->app, g_free);
    g_clear_pointer (&self->path, g_free);
    g_clear_pointer (&self->resource_type, g_free);

    G_OBJECT_CLASS (snapd_prompting_request_parent_class)->finalize (object);
}

static void
snapd_prompting_request_class_init (SnapdPromptingRequestClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_prompting_request_set_property;
    gobject_class->get_property = snapd_prompting_request_get_property;
    gobject_class->finalize = snapd_prompting_request_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_APP,
                                     g_param_spec_string ("app",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PATH,
                                     g_param_spec_string ("path",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_RESOURCE_TYPE,
                                     g_param_spec_string ("resource-type",
                                                          NULL,
                                                          NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PERMISSIONS,
                                     g_param_spec_flags ("permissions",
                                                         NULL,
                                                         NULL,
                                                         SNAPD_TYPE_PROMPTING_PERMISSION_FLAGS, SNAPD_PROMPTING_PERMISSION_FLAGS_NONE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_prompting_request_init (SnapdPromptingRequest *self)
{
}
