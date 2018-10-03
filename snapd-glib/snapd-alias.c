/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-alias.h"
#include "snapd-enum-types.h"

/**
 * SECTION: snapd-alias
 * @short_description: App aliases.
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdAlias represents an optional alias that can be used for an app.
 * Aliases can be queried using snapd_client_get_aliases_sync() and are used in
 * snapd_client_enable_aliases_sync(), snapd_client_disable_aliases_sync() and
 * snapd_client_reset_aliases_sync().
 */

/**
 * SnapdAlias:
 *
 * #SnapdAlias contains alias information for a Snap.
 *
 * Aliases are used to provide alternative binary names for Snap apps.
 *
 * Since: 1.8
 */

struct _SnapdAlias
{
    GObject parent_instance;

    gchar *command;
    gchar *name;
    gchar *snap;
    gchar *app_auto;
    gchar *app_manual;
    SnapdAliasStatus status;
};

enum
{
    PROP_NAME = 1,
    PROP_SNAP,
    PROP_STATUS,
    PROP_COMMAND,
    PROP_APP_AUTO,
    PROP_APP_MANUAL,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdAlias, snapd_alias, G_TYPE_OBJECT)

/**
 * snapd_alias_get_app:
 * @alias: a #SnapdAlias.
 *
 * Get the app this is an alias for.
 *
 * Returns: (allow-none): an app name or %NULL.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_alias_get_app_manual() or snapd_alias_get_app_auto().
 */
const gchar *
snapd_alias_get_app (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return NULL;
}

/**
 * snapd_alias_get_app_auto:
 * @alias: a #SnapdAlias.
 *
 * Get the app this alias has been automatically set to (status is %SNAPD_ALIAS_STATUS_AUTO).
 * Can be overridden when status is %SNAPD_ALIAS_STATUS_MANUAL.
 *
 * Returns: (allow-none): an app name or %NULL.
 *
 * Since: 1.25
 */
const gchar *
snapd_alias_get_app_auto (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return alias->app_auto;
}

/**
 * snapd_alias_get_app_manual:
 * @alias: a #SnapdAlias.
 *
 * Get the app this alias has been manually set to (status is %SNAPD_ALIAS_STATUS_MANUAL).
 * This overrides the app from snapd_alias_get_app_auto().
 *
 * Returns: (allow-none): an app name or %NULL.
 *
 * Since: 1.25
 */
const gchar *
snapd_alias_get_app_manual (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return alias->app_manual;
}

/**
 * snapd_alias_get_command:
 * @alias: a #SnapdAlias.
 *
 * Get the command this alias runs.
 *
 * Returns: a command.
 *
 * Since: 1.25
 */
const gchar *
snapd_alias_get_command (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return alias->command;
}

/**
 * snapd_alias_get_name:
 * @alias: a #SnapdAlias.
 *
 * Get the name of this alias.
 *
 * Returns: an alias name.
 *
 * Since: 1.8
 */
const gchar *
snapd_alias_get_name (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return alias->name;
}

/**
 * snapd_alias_get_snap:
 * @alias: a #SnapdAlias.
 *
 * Get the snap this alias is for.
 *
 * Returns: a snap name.
 *
 * Since: 1.8
 */
const gchar *
snapd_alias_get_snap (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), NULL);
    return alias->snap;
}

/**
 * snapd_alias_get_status:
 * @alias: a #SnapdAlias.
 *
 * Get the status of this alias.
 *
 * Return: a #SnapdAliasStatus.
 *
 * Since: 1.8
 */
SnapdAliasStatus
snapd_alias_get_status (SnapdAlias *alias)
{
    g_return_val_if_fail (SNAPD_IS_ALIAS (alias), SNAPD_ALIAS_STATUS_UNKNOWN);
    return alias->status;
}

static void
snapd_alias_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdAlias *alias = SNAPD_ALIAS (object);

    switch (prop_id) {
    case PROP_APP_AUTO:
        g_free (alias->app_auto);
        alias->app_auto = g_strdup (g_value_get_string (value));
        break;
    case PROP_APP_MANUAL:
        g_free (alias->app_manual);
        alias->app_manual = g_strdup (g_value_get_string (value));
        break;
    case PROP_COMMAND:
        g_free (alias->command);
        alias->command = g_strdup (g_value_get_string (value));
        break;
    case PROP_NAME:
        g_free (alias->name);
        alias->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (alias->snap);
        alias->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_STATUS:
        alias->status = g_value_get_enum (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_alias_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdAlias *alias = SNAPD_ALIAS (object);

    switch (prop_id) {
    case PROP_APP_AUTO:
        g_value_set_string (value, alias->app_auto);
        break;
    case PROP_APP_MANUAL:
        g_value_set_string (value, alias->app_manual);
        break;
    case PROP_COMMAND:
        g_value_set_string (value, alias->command);
        break;
    case PROP_NAME:
        g_value_set_string (value, alias->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, alias->snap);
        break;
    case PROP_STATUS:
        g_value_set_enum (value, alias->status);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_alias_finalize (GObject *object)
{
    SnapdAlias *alias = SNAPD_ALIAS (object);

    g_clear_pointer (&alias->app_auto, g_free);
    g_clear_pointer (&alias->app_manual, g_free);
    g_clear_pointer (&alias->command, g_free);
    g_clear_pointer (&alias->name, g_free);
    g_clear_pointer (&alias->snap, g_free);

    G_OBJECT_CLASS (snapd_alias_parent_class)->finalize (object);
}

static void
snapd_alias_class_init (SnapdAliasClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_alias_set_property;
    gobject_class->get_property = snapd_alias_get_property;
    gobject_class->finalize = snapd_alias_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_APP_AUTO,
                                     g_param_spec_string ("app-auto",
                                                          "app-auto",
                                                          "App this alias is for (when status is SNAPD_ALIAS_STATUS_AUTO)",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_APP_MANUAL,
                                     g_param_spec_string ("app-manual",
                                                          "app-manual",
                                                          "App this alias is for (when status is SNAPD_ALIAS_STATUS_MANUAL)",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_COMMAND,
                                     g_param_spec_string ("command",
                                                          "command",
                                                          "Command this alias runs",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Name of alias",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this alias is for",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_STATUS,
                                     g_param_spec_enum ("status",
                                                        "status",
                                                        "Alias status",
                                                        SNAPD_TYPE_ALIAS_STATUS, SNAPD_ALIAS_STATUS_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_alias_init (SnapdAlias *alias)
{
}
