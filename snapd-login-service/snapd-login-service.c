/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdlib.h>
#include <gio/gio.h>
#include <snapd-glib/snapd-glib.h>
#include <polkit/polkit.h>

#include "login-service.h"

#if !defined(POLKIT_HAS_AUTOPTR_MACROS)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PolkitAuthorizationResult, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PolkitSubject, g_object_unref)
#endif

static GMainLoop *loop;
static int result = EXIT_SUCCESS;

static PolkitAuthority *authority = NULL;
static IoSnapcraftSnapdLoginService *service = NULL;

typedef struct
{
    GDBusMethodInvocation *invocation;
    gchar *username;
    gchar *password;
    gchar *otp;
    GPermission *permission;
    SnapdClient *client;
} LoginRequest;

static void
free_login_request (LoginRequest *request)
{
    g_object_unref (request->invocation);
    g_free (request->username);
    g_free (request->password);
    g_free (request->otp);
    if (request->permission != NULL)
        g_object_unref (request->permission);
    if (request->client != NULL)
        g_object_unref (request->client);
    g_slice_free (LoginRequest, request);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (LoginRequest, free_login_request);

static void
return_error (LoginRequest *request, GError *error)
{
    if (error->domain != SNAPD_ERROR) {
        g_dbus_method_invocation_return_gerror (request->invocation, error);
        return;
    }

    switch (error->code) {
    case SNAPD_ERROR_CONNECTION_FAILED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.ConnectionFailed", error->message);
        break;
    case SNAPD_ERROR_WRITE_FAILED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.WriteFailed", error->message);
        break;
    case SNAPD_ERROR_READ_FAILED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.ReadFailed", error->message);
        break;
    case SNAPD_ERROR_BAD_REQUEST:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.BadRequest", error->message);
        break;
    case SNAPD_ERROR_BAD_RESPONSE:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.BadResponse", error->message);
        break;
    case SNAPD_ERROR_AUTH_DATA_REQUIRED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.AuthDataRequired", error->message);
        break;
    case SNAPD_ERROR_AUTH_DATA_INVALID:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.AuthDataInvalid", error->message);
        break;
    case SNAPD_ERROR_TWO_FACTOR_REQUIRED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.TwoFactorRequired", error->message);
        break;
    case SNAPD_ERROR_TWO_FACTOR_INVALID:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.TwoFactorInvalid", error->message);
        break;
    case SNAPD_ERROR_PERMISSION_DENIED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.PermissionDenied", error->message);
        break;
    case SNAPD_ERROR_FAILED:
        g_dbus_method_invocation_return_dbus_error (request->invocation, "io.snapcraft.SnapdLoginService.Error.Failed", error->message);
        break;
    default:
        g_dbus_method_invocation_return_gerror (request->invocation, error);
        break;
    }
}

static void
login_result_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(LoginRequest) request = user_data;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    auth_data = snapd_client_login_finish (request->client, result, &error);
    if (auth_data == NULL) {
        return_error (request, error);
        return;
    }

    io_snapcraft_snapd_login_service_complete_login (service, request->invocation,
                                                     snapd_auth_data_get_macaroon (auth_data),
                                                     (const gchar *const *) snapd_auth_data_get_discharges (auth_data));
}

static void
auth_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(LoginRequest) request = user_data;
    g_autoptr(PolkitAuthorizationResult) r = NULL;
    g_autoptr(GError) error = NULL;

    r = polkit_authority_check_authorization_finish (authority, result, &error);
    if (r == NULL) {
        g_dbus_method_invocation_return_error (request->invocation,
                                               SNAPD_ERROR,
                                               SNAPD_ERROR_PERMISSION_DENIED,
                                               "Failed to get permission from Polkit: %s", error->message);
        return;
    }

    if (!polkit_authorization_result_get_is_authorized (r)) {
        g_dbus_method_invocation_return_error (request->invocation,
                                               SNAPD_ERROR,
                                               SNAPD_ERROR_PERMISSION_DENIED,
                                               "Permission denied by Polkit");
        return;
    }

    g_debug ("Requesting login from snapd...");

    request->client = snapd_client_new ();
    if (!snapd_client_connect_sync (request->client, NULL, &error)) {
        return_error (request, error);
        return;
    }

    snapd_client_login_async (request->client,
                              request->username, request->password, request->otp, NULL,
                              login_result_cb, request);
    g_steal_pointer (&request);
}

static gboolean
login_cb (IoSnapcraftSnapdLoginService *service,
          GDBusMethodInvocation        *invocation,
          const gchar                  *username,
          const gchar                  *password,
          const gchar                  *otp,
          gpointer                      user_data)
{
    g_autoptr(LoginRequest) request = NULL;
    g_autoptr(PolkitSubject) subject = NULL;

    request = g_slice_new0 (LoginRequest);
    request->invocation = g_object_ref (invocation);
    request->username = g_strdup (username);
    request->password = g_strdup (password);
    if (otp[0] != '\0')
        request->otp = g_strdup (otp);

    g_debug ("Processing login request...");

    subject = polkit_system_bus_name_new (g_dbus_method_invocation_get_sender (invocation));
    polkit_authority_check_authorization (authority,
                                          subject, "io.snapcraft.login", NULL,
                                          POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                                          NULL,
                                          auth_cb, g_steal_pointer (&request));

    return TRUE;
}

static void
bus_acquired_cb (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
    g_autoptr(GError) error = NULL;

    g_debug ("Connected to D-Bus");

    service = io_snapcraft_snapd_login_service_skeleton_new ();
    g_signal_connect (service, "handle-login", G_CALLBACK (login_cb), NULL);
    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service),
                                           connection,
                                           "/io/snapcraft/SnapdLoginService",
                                           &error)) {
        g_warning ("Failed to register object: %s", error->message);
        result = EXIT_FAILURE;
        g_main_loop_quit (loop);
    }
}

static void
name_acquired_cb (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
    g_debug ("Acquired bus name %s", name);
}

static void
name_lost_cb (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
    if (connection == NULL) {
        g_warning ("Failed to connect to bus");
        result = EXIT_FAILURE;
    }

    g_info ("Lost bus name %s", name);
    g_main_loop_quit (loop);
}


int main (int argc, char **argv)
{
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);
  
    authority = polkit_authority_get_sync (NULL, &error);
    if (authority == NULL) {
        g_warning ("Failed to get Polkit authority: %s", error->message);
        return EXIT_FAILURE;
    }
    g_debug ("Connected to Polkit");

    g_bus_own_name (G_BUS_TYPE_SYSTEM,
                    "io.snapcraft.SnapdLoginService",
                    G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT | G_BUS_NAME_OWNER_FLAGS_REPLACE,
                    bus_acquired_cb,
                    name_acquired_cb,
                    name_lost_cb,
                    NULL, NULL);

    g_main_loop_run (loop);

    return result;
}
