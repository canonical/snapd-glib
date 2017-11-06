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
    gchar *desktop_file;
};

enum
{
    PROP_NAME = 1,
    PROP_ALIASES,
    PROP_DAEMON_TYPE,
    PROP_DESKTOP_FILE,
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
snapd_app_get_name (SnapdApp *app)
{
    g_return_val_if_fail (SNAPD_IS_APP (app), NULL);
    return app->name;
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
gchar **
snapd_app_get_aliases (SnapdApp *app)
{
    g_return_val_if_fail (SNAPD_IS_APP (app), NULL);
    return NULL;
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
snapd_app_get_daemon_type (SnapdApp *app)
{
    g_return_val_if_fail (SNAPD_IS_APP (app), SNAPD_DAEMON_TYPE_NONE);
    return app->daemon_type;
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
snapd_app_get_desktop_file (SnapdApp *app)
{
    g_return_val_if_fail (SNAPD_IS_APP (app), NULL);
    return app->desktop_file;
}

static void
snapd_app_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdApp *app = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (app->name);
        app->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_ALIASES:
        break;
    case PROP_DAEMON_TYPE:
        app->daemon_type = g_value_get_enum (value);
        break;
    case PROP_DESKTOP_FILE:
        g_free (app->desktop_file);
        app->desktop_file = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdApp *app = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, app->name);
        break;
    case PROP_ALIASES:
        g_value_set_boxed (value, NULL);
        break;
    case PROP_DAEMON_TYPE:
        g_value_set_enum (value, app->daemon_type);
        break;
    case PROP_DESKTOP_FILE:
        g_value_set_string (value, app->desktop_file);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_finalize (GObject *object)
{
    SnapdApp *app = SNAPD_APP (object);

    g_clear_pointer (&app->name, g_free);
    g_clear_pointer (&app->desktop_file, g_free);
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
}

static void
snapd_app_init (SnapdApp *app)
{
}
