/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <string.h>

#include "snapd-app.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-app
 * @short_description: Application metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdApp contains information about an application that snapd provides.
 * Apps are retrieved using snapd_snap_get_apps().
 */

/**
 * SnapdApp:
 *
 * #SnapdApp contains information about an app in a Snap.
 *
 * Snaps can contain apps which is a single binary executable.
 *
 * Since: 1.0
 */
struct _SnapdApp
{
    GObject parent_instance;

    SnapdDaemonType daemon_type;
    gchar *name;
    gchar *snap;
    gchar *common_id;
    gchar *desktop_file;
    gboolean enabled;
    gboolean active;
};

enum
{
    PROP_NAME = 1,
    PROP_ALIASES,
    PROP_DAEMON_TYPE,
    PROP_DESKTOP_FILE,
    PROP_SNAP,
    PROP_ACTIVE,
    PROP_ENABLED,
    PROP_COMMON_ID,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdApp, snapd_app, G_TYPE_OBJECT)

/**
 * snapd_app_get_name:
 * @app: a #SnapdApp.
 *
 * Get the name of this app.
 *
 * Returns: a name.
 *
 * Since: 1.0
 */
const gchar *
snapd_app_get_name (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), NULL);
    return self->name;
}

/**
 * snapd_app_get_active:
 * @app: a #SnapdApp.
 *
 * Get if this service is active.
 *
 * Returns: %TRUE if active.
 *
 * Since: 1.25
 */
gboolean
snapd_app_get_active (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), FALSE);
    return self->active;
}

/**
 * snapd_app_get_aliases:
 * @app: a #SnapdApp.
 *
 * Get the aliases for this app.
 *
 * Returns: (transfer none) (array zero-terminated=1): the alias names.
 *
 * Since: 1.7
 * Deprecated: 1.25
 */
GStrv
snapd_app_get_aliases (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), NULL);
    return NULL;
}

/**
 * snapd_app_get_common_id:
 * @app: a #SnapdApp.
 *
 * Get the common ID associated with this app.
 *
 * Returns: (allow-none): an ID or %NULL.
 *
 * Since: 1.41
 */
const gchar *
snapd_app_get_common_id (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), NULL);
    return self->common_id;
}

/**
 * snapd_app_get_daemon_type:
 * @app: a #SnapdApp.
 *
 * Get the daemon type for this app.
 *
 * Returns: (allow-none): the daemon type or %NULL.
 *
 * Since: 1.9
 */
SnapdDaemonType
snapd_app_get_daemon_type (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), SNAPD_DAEMON_TYPE_NONE);
    return self->daemon_type;
}

/**
 * snapd_app_get_desktop_file:
 * @app: a #SnapdApp.
 *
 * Get the path to the desktop file for this app.
 *
 * Returns: (allow-none): a path or %NULL.
 *
 * Since: 1.14
 */
const gchar *
snapd_app_get_desktop_file (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), NULL);
    return self->desktop_file;
}

/**
 * snapd_app_get_enabled:
 * @app: a #SnapdApp.
 *
 * Get if this service is enabled.
 *
 * Returns: %TRUE if enabled.
 *
 * Since: 1.25
 */
gboolean
snapd_app_get_enabled (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), FALSE);
    return self->enabled;
}

/**
 * snapd_app_get_snap:
 * @app: a #SnapdApp.
 *
 * Get the snap this app is associated with.
 *
 * Returns: a snap name.
 *
 * Since: 1.25
 */
const gchar *
snapd_app_get_snap (SnapdApp *self)
{
    g_return_val_if_fail (SNAPD_IS_APP (self), NULL);
    return self->snap;
}

static void
snapd_app_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdApp *self = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (self->name);
        self->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_ALIASES:
        break;
    case PROP_COMMON_ID:
        g_free (self->common_id);
        self->common_id = g_strdup (g_value_get_string (value));
        break;
    case PROP_DAEMON_TYPE:
        self->daemon_type = g_value_get_enum (value);
        break;
    case PROP_DESKTOP_FILE:
        g_free (self->desktop_file);
        self->desktop_file = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (self->snap);
        self->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_ACTIVE:
        self->active = g_value_get_boolean (value);
        break;
    case PROP_ENABLED:
        self->enabled = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdApp *self = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_ALIASES:
        g_value_set_boxed (value, NULL);
        break;
    case PROP_COMMON_ID:
        g_value_set_string (value, self->common_id);
        break;
    case PROP_DAEMON_TYPE:
        g_value_set_enum (value, self->daemon_type);
        break;
    case PROP_DESKTOP_FILE:
        g_value_set_string (value, self->desktop_file);
        break;
    case PROP_SNAP:
        g_value_set_string (value, self->snap);
        break;
    case PROP_ACTIVE:
        g_value_set_boolean (value, self->active);
        break;
    case PROP_ENABLED:
        g_value_set_boolean (value, self->enabled);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_finalize (GObject *object)
{
    SnapdApp *self = SNAPD_APP (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->common_id, g_free);
    g_clear_pointer (&self->desktop_file, g_free);
    g_clear_pointer (&self->snap, g_free);

    G_OBJECT_CLASS (snapd_app_parent_class)->finalize (object);
}

static void
snapd_app_class_init (SnapdAppClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_app_set_property;
    gobject_class->get_property = snapd_app_get_property;
    gobject_class->finalize = snapd_app_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "App name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ALIASES,
                                     g_param_spec_boxed ("aliases",
                                                         "aliases",
                                                         "App aliases",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_COMMON_ID,
                                     g_param_spec_string ("common-id",
                                                          "common-id",
                                                          "Common ID",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DAEMON_TYPE,
                                     g_param_spec_enum ("daemon-type",
                                                        "daemon-type",
                                                        "Daemon type",
                                                        SNAPD_TYPE_DAEMON_TYPE, SNAPD_DAEMON_TYPE_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DESKTOP_FILE,
                                     g_param_spec_string ("desktop-file",
                                                          "desktop-file",
                                                          "App desktop file path",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ACTIVE,
                                     g_param_spec_boolean ("active",
                                                           "active",
                                                           "TRUE if active",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ENABLED,
                                     g_param_spec_boolean ("enabled",
                                                           "enabled",
                                                           "TRUE if enabled",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_app_init (SnapdApp *self)
{
}
