/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "config.h"

#include "snapd-login.h"
#include "snapd-login-private.h"
#include "snapd-error.h"

/**
 * SECTION: snapd-login
 * @short_description: Client authorization
 * @include: snapd-glib/snapd-glib.h
 *
 * To allow non-root users to authorize with snapd as D-Bus service called
 * snapd-login-service is provided. This service uses Polkit to allow privileded
 * users to install and remove snaps. snapd_login_sync() calls this service to
 * get a #SnapdAuthData that can be passed to a snapd client with
 * snapd_client_set_auth_data().
 */

struct _SnapdLoginRequest
{
    GObject parent_instance;

    GCancellable *cancellable;
    GAsyncReadyCallback ready_callback;
    gpointer ready_callback_data;

    gchar *username;
    gchar *password;
    gchar *otp;
    SnapdAuthData *auth_data;
    GError *error;
};

static void
snapd_login_request_async_result_init (GAsyncResultIface *iface)
{
}

G_DEFINE_TYPE_WITH_CODE (SnapdLoginRequest, snapd_login_request, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_RESULT, snapd_login_request_async_result_init))

static void
snapd_login_request_finalize (GObject *object)
{
    SnapdLoginRequest *request = SNAPD_LOGIN_REQUEST (object);

    g_clear_object (&request->cancellable);
    g_free (request->username);
    g_free (request->password);
    g_free (request->otp);
    g_clear_object (&request->auth_data);
    g_clear_object (&request->error);
}

static void
snapd_login_request_class_init (SnapdLoginRequestClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = snapd_login_request_finalize;
}

static void
snapd_login_request_init (SnapdLoginRequest *request)
{
}

static GError *
convert_dbus_error (GError *dbus_error)
{
    const gchar *remote_error;
    GError *error;

    remote_error = g_dbus_error_get_remote_error (dbus_error);
    if (remote_error == NULL)
        return g_error_copy (dbus_error);

    if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.ConnectionFailed") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.WriteFailed") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_WRITE_FAILED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.ReadFailed") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.BadRequest") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.BadResponse") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.AuthDataRequired") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.AuthDataInvalid") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_INVALID, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.TwoFactorRequired") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_REQUIRED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.TwoFactorInvalid") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_INVALID, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.PermissionDenied") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_PERMISSION_DENIED, dbus_error->message);
    else if (strcmp (remote_error, "io.snapcraft.SnapdLoginService.Error.Failed") == 0)
        error = g_error_new_literal (SNAPD_ERROR, SNAPD_ERROR_FAILED, dbus_error->message);
    else
        return g_error_copy (dbus_error);

    g_dbus_error_strip_remote_error (error);
    return error;
}

/**
 * snapd_login_sync:
 * @username: usename to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Log in to snapd and get authorization to install/remove snaps.
 * This call contacts the snapd-login-service that will authenticate the user
 * using Polkit and contact snapd on their behalf.
 * If you are root, you can get this authentication directly from snapd using
 * snapd_client_login_sync().
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 */
SnapdAuthData *
snapd_login_sync (const gchar *username, const gchar *password, const gchar *otp,
                  GCancellable *cancellable, GError **error)
{
    g_autoptr(GDBusConnection) c = NULL;
    g_autoptr(GVariant) result = NULL;
    const gchar *macaroon;
    g_auto(GStrv) discharges = NULL;
    g_autoptr(GError) dbus_error = NULL;

    g_return_val_if_fail (username != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    if (otp == NULL)
        otp = "";

    c = g_bus_get_sync (G_BUS_TYPE_SYSTEM, cancellable, error);
    if (c == NULL)
        return NULL;
    result = g_dbus_connection_call_sync (c,
                                          "io.snapcraft.SnapdLoginService",
                                          "/io/snapcraft/SnapdLoginService",
                                          "io.snapcraft.SnapdLoginService",
                                          "Login",
                                          g_variant_new ("(sss)", username, password, otp),
                                          G_VARIANT_TYPE ("(sas)"),
                                          G_DBUS_CALL_FLAGS_NONE,
                                          -1,
                                          cancellable,
                                          &dbus_error);
    if (result == NULL) {
        if (error != NULL)
            *error = convert_dbus_error (dbus_error);
        return NULL;
    }

    g_variant_get (result, "(&s^as)", &macaroon, &discharges);

    return snapd_auth_data_new (macaroon, discharges);
}

static gboolean
login_complete_cb (gpointer user_data)
{
    g_autoptr(SnapdLoginRequest) request = user_data;

    if (request->ready_callback != NULL)
        request->ready_callback (NULL, G_ASYNC_RESULT (request), request->ready_callback_data);

    return G_SOURCE_REMOVE;
}

static void
login_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(SnapdLoginRequest) request = user_data;
    g_autoptr(GVariant) r = NULL;
    const gchar *macaroon;
    g_auto(GStrv) discharges = NULL;
    g_autoptr(GError) dbus_error = NULL;

    r = g_dbus_connection_call_finish (G_DBUS_CONNECTION (object), result, &dbus_error);
    if (r == NULL) {
        request->error = convert_dbus_error (dbus_error);
        return;
    }

    g_variant_get (r, "(&s^as)", &macaroon, &discharges);
    request->auth_data = snapd_auth_data_new (macaroon, discharges);

    g_idle_add (login_complete_cb, g_steal_pointer (&request));
}

static void
bus_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(SnapdLoginRequest) request = user_data;
    g_autoptr(GDBusConnection) c = NULL;
    g_autoptr(GError) error = NULL;

    c = g_bus_get_finish (result, &error);
    if (c == NULL) {
        request->error = g_error_new (SNAPD_ERROR,
                                      SNAPD_ERROR_CONNECTION_FAILED,
                                      "Failed to get system bus: %s", error->message);
        g_idle_add (login_complete_cb, g_steal_pointer (&request));
        return;
    }

    g_dbus_connection_call (c,
                            "io.snapcraft.SnapdLoginService",
                            "/io/snapcraft/SnapdLoginService",
                            "io.snapcraft.SnapdLoginService",
                            "Login",
                            g_variant_new ("(sss)", request->username, request->password, request->otp),
                            G_VARIANT_TYPE ("(sas)"),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            request->cancellable,
                            login_cb,
                            request);
    g_steal_pointer (&request);
}

/**
 * snapd_login_async:
 * @username: usename to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get authorization to install/remove snaps.
 * See snapd_login_sync() for more information.
 */
void
snapd_login_async (const gchar *username, const gchar *password, const gchar *otp,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdLoginRequest *request;

    g_return_if_fail (username != NULL);
    g_return_if_fail (password != NULL);

    request = g_object_new (snapd_login_request_get_type (), NULL);
    request->ready_callback = callback;
    request->ready_callback_data = user_data;
    request->cancellable = g_object_ref (cancellable);
    request->username = g_strdup (username);
    request->password = g_strdup (password);
    request->otp = otp != NULL ? g_strdup (otp) : "";

    g_bus_get (G_BUS_TYPE_SYSTEM, cancellable, bus_cb, NULL);
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
 */
SnapdAuthData *
snapd_login_finish (GAsyncResult *result, GError **error)
{
    SnapdLoginRequest *request = SNAPD_LOGIN_REQUEST (result);

    if (request->error)
        g_propagate_error (error, request->error);

    return g_steal_pointer (&request->auth_data);
}
