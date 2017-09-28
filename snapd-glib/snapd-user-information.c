/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-user-information.h"

/**
 * SECTION:snapd-user-information
 * @short_description: User information class
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdUserInformation object contains the information about local users
 * created using snapd_client_create_user_sync() and
 * snapd_client_create_users_sync().
 */

/**
 * SnapdUserInformation:
 *
 * #SnapdUserInformation contains information about a user account on the system
 * snapd is running on.
 *
 * Since: 1.3
 */

struct _SnapdUserInformation
{
    GObject parent_instance;

    gchar *username;
    gchar **ssh_keys;
};

enum
{
    PROP_USERNAME = 1,
    PROP_SSH_KEYS,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdUserInformation, snapd_user_information, G_TYPE_OBJECT)

/**
 * snapd_user_information_get_username:
 * @user_information: a #SnapdUserInformation.
 *
 * Get the local username for this account.
 *
 * Returns: a username.
 *
 * Since: 1.3
 */
const gchar *
snapd_user_information_get_username (SnapdUserInformation *user_information)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (user_information), NULL);
    return user_information->username;
}

/**
 * snapd_user_information_get_ssh_keys:
 * @user_information: a #SnapdUserInformation.
 *
 * Get the SSH keys added to this account.
 *
 * Returns: (transfer none) (array zero-terminated=1): the names of the SSH keys.
 *
 * Since: 1.3
 */
gchar **
snapd_user_information_get_ssh_keys (SnapdUserInformation *user_information)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (user_information), NULL);
    return user_information->ssh_keys;
}

static void
snapd_user_information_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdUserInformation *user_information = SNAPD_USER_INFORMATION (object);

    switch (prop_id) {
    case PROP_USERNAME:
        g_free (user_information->username);
        user_information->username = g_strdup (g_value_get_string (value));
        break;
    case PROP_SSH_KEYS:
        g_strfreev (user_information->ssh_keys);
        user_information->ssh_keys = g_strdupv (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_user_information_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdUserInformation *user_information = SNAPD_USER_INFORMATION (object);

    switch (prop_id) {
    case PROP_USERNAME:
        g_value_set_string (value, user_information->username);
        break;
    case PROP_SSH_KEYS:
        g_value_set_boxed (value, user_information->ssh_keys);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_user_information_finalize (GObject *object)
{
    SnapdUserInformation *user_information = SNAPD_USER_INFORMATION (object);

    g_clear_pointer (&user_information->username, g_free);
    g_clear_pointer (&user_information->ssh_keys, g_strfreev);
}

static void
snapd_user_information_class_init (SnapdUserInformationClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_user_information_set_property;
    gobject_class->get_property = snapd_user_information_get_property;
    gobject_class->finalize = snapd_user_information_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_USERNAME,
                                     g_param_spec_string ("username",
                                                          "username",
                                                          "Unix username",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SSH_KEYS,
                                     g_param_spec_boxed ("ssh-keys",
                                                         "ssh-keys",
                                                         "SSH keys",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_user_information_init (SnapdUserInformation *user_information)
{
}
