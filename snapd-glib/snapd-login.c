/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-login.h"
#include "snapd-client.h"
#include "snapd-error.h"

/**
 * SECTION: snapd-login
 * @short_description: Client authorization
 * @include: snapd-glib/snapd-glib.h
 *
 * Deprecated methods of authenticating with snapd.
 */

/**
 * snapd_login_sync:
 * @username: username to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * This call used to contact a D-Bus service to perform snapd authentication using
 * Polkit. This now just creates a #SnapdClient and does the call directly.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 *
 * Since: 1.0
 * Deprecated: 1.34: Use snapd_client_login2_sync()
 */
SnapdAuthData *
snapd_login_sync (const gchar *username, const gchar *password, const gchar *otp,
                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (username != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync (client, username, password, otp, cancellable, error);
    if (user_information == NULL)
        return NULL;

    return g_object_ref (snapd_user_information_get_auth_data (user_information));
}

static void
login_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    GTask *task = user_data;

    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_finish (SNAPD_CLIENT (object), result, &error);
    if (user_information != NULL)
        g_task_return_pointer (task, g_object_ref (snapd_user_information_get_auth_data (user_information)), g_object_unref);
    else
        g_task_return_error (task, error);
}

/**
 * snapd_login_async:
 * @username: username to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get authorization to install/remove snaps.
 * See snapd_login_sync() for more information.
 *
 * Since: 1.0
 * Deprecated: 1.34: Use snapd_client_login2_async()
 */
void
snapd_login_async (const gchar *username, const gchar *password, const gchar *otp,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (username != NULL);
    g_return_if_fail (password != NULL);

    g_autoptr(GTask) task = g_task_new (NULL, cancellable, callback, user_data);
    g_autoptr(SnapdClient) client = snapd_client_new ();
    snapd_client_login2_async (client, username, password, otp, cancellable, login_cb, task);
}

/**
 * snapd_login_finish:
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete login started with snapd_login_async().
 * See snapd_login_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 *
 * Since: 1.0
 * Deprecated: 1.34: Use snapd_client_login2_finish()
 */
SnapdAuthData *
snapd_login_finish (GAsyncResult *result, GError **error)
{
    return g_task_propagate_pointer (G_TASK (result), error);
}
