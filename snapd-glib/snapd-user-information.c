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

    gint64 id;
    gchar *username;
    gchar *email;
    GStrv ssh_keys;
    SnapdAuthData *auth_data;
};

enum
{
    PROP_ID = 1,
    PROP_USERNAME,
    PROP_EMAIL,
    PROP_SSH_KEYS,
    PROP_AUTH_DATA,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdUserInformation, snapd_user_information, G_TYPE_OBJECT)

/**
 * snapd_user_information_get_id:
 * @user_information: a #SnapdUserInformation.
 *
 * Get the id for this account.
 *
 * Returns: a user id.
 *
 * Since: 1.26
 */
gint64
snapd_user_information_get_id (SnapdUserInformation *self)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (self), -1);
    return self->id;
}

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
snapd_user_information_get_username (SnapdUserInformation *self)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (self), NULL);
    return self->username;
}

/**
 * snapd_user_information_get_email:
 * @user_information: a #SnapdUserInformation.
 *
 * Get the email address for this account.
 *
 * Returns: a email address.
 *
 * Since: 1.26
 */
const gchar *
snapd_user_information_get_email (SnapdUserInformation *self)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (self), NULL);
    return self->email;
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
GStrv
snapd_user_information_get_ssh_keys (SnapdUserInformation *self)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (self), NULL);
    return self->ssh_keys;
}

/**
 * snapd_user_information_get_auth_data:
 * @user_information: a #SnapdUserInformation.
 *
 * Get the email address for this account.
 *
 * Returns: (transfer none) (allow-none): a #SnapdAuthData or %NULL if not set.
 *
 * Since: 1.26
 */
SnapdAuthData *
snapd_user_information_get_auth_data (SnapdUserInformation *self)
{
    g_return_val_if_fail (SNAPD_IS_USER_INFORMATION (self), NULL);
    return self->auth_data;
}

static void
snapd_user_information_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdUserInformation *self = SNAPD_USER_INFORMATION (object);

    switch (prop_id) {
    case PROP_ID:
        self->id = g_value_get_int64 (value);
        break;
    case PROP_USERNAME:
        g_free (self->username);
        self->username = g_strdup (g_value_get_string (value));
        break;
    case PROP_EMAIL:
        g_free (self->email);
        self->email = g_strdup (g_value_get_string (value));
        break;
    case PROP_SSH_KEYS:
        g_strfreev (self->ssh_keys);
        self->ssh_keys = g_strdupv (g_value_get_boxed (value));
        break;
    case PROP_AUTH_DATA:
        g_clear_object (&self->auth_data);
        self->auth_data = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_user_information_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdUserInformation *self = SNAPD_USER_INFORMATION (object);

    switch (prop_id) {
    case PROP_ID:
        g_value_set_int64 (value, self->id);
        break;
    case PROP_USERNAME:
        g_value_set_string (value, self->username);
        break;
    case PROP_EMAIL:
        g_value_set_string (value, self->email);
        break;
    case PROP_SSH_KEYS:
        g_value_set_boxed (value, self->ssh_keys);
        break;
    case PROP_AUTH_DATA:
        g_value_set_object (value, self->auth_data);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_user_information_finalize (GObject *object)
{
    SnapdUserInformation *self = SNAPD_USER_INFORMATION (object);

    g_clear_pointer (&self->username, g_free);
    g_clear_pointer (&self->email, g_free);
    g_clear_pointer (&self->ssh_keys, g_strfreev);
    g_clear_object (&self->auth_data);

    G_OBJECT_CLASS (snapd_user_information_parent_class)->finalize (object);
}

static void
snapd_user_information_class_init (SnapdUserInformationClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_user_information_set_property;
    gobject_class->get_property = snapd_user_information_get_property;
    gobject_class->finalize = snapd_user_information_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_int64 ("id",
                                                         "id",
                                                         "Account id",
                                                         G_MININT64, G_MAXINT64, -1,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_USERNAME,
                                     g_param_spec_string ("username",
                                                          "username",
                                                          "Unix username",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_EMAIL,
                                     g_param_spec_string ("email",
                                                          "email",
                                                          "Email address",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SSH_KEYS,
                                     g_param_spec_boxed ("ssh-keys",
                                                         "ssh-keys",
                                                         "SSH keys",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_AUTH_DATA,
                                     g_param_spec_object ("auth-data",
                                                          "auth-data",
                                                          "Authorization data",
                                                          SNAPD_TYPE_AUTH_DATA,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_user_information_init (SnapdUserInformation *self)
{
    self->id = -1;
}
