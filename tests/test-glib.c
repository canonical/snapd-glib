/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>
#include <snapd-glib/snapd-glib.h>

#include "config.h"
#include "mock-snapd.h"

typedef struct
{
    GMainLoop *loop;
    MockSnapd *snapd;
    int counter;
} AsyncData;

static AsyncData *
async_data_new (GMainLoop *loop, MockSnapd *snapd)
{
    AsyncData *data = g_slice_new0 (AsyncData);
    data->loop = g_main_loop_ref (loop);
    data->snapd = g_object_ref (snapd);
    return data;
}

static void
async_data_free (AsyncData *data)
{
    g_main_loop_unref (data->loop);
    g_object_unref (data->snapd);
    g_slice_free (AsyncData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(AsyncData, async_data_free)

static void
test_socket_closed_before_request (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    mock_snapd_stop (snapd);

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_null (info);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED);
}

static void
test_socket_closed_after_request (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_close_on_request (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_null (info);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED);
}

static void
test_user_agent_default (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_cmpstr (snapd_client_get_user_agent (client), ==, "snapd-glib/" VERSION);

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, "snapd-glib/" VERSION);
}

static void
test_user_agent_custom (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_set_user_agent (client, "Foo/1.0");
    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, "Foo/1.0");
}

static void
test_user_agent_null (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_set_user_agent (client, NULL);
    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, NULL);
}

static void
test_accept_language (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    g_setenv ("LANG", "en_US.UTF-8", TRUE);
    g_setenv ("LANGUAGE", "en_US:fr", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en-us, en;q=0.9, fr;q=0.8");
}

static void
test_accept_language_empty (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    g_setenv ("LANG", "", TRUE);
    g_setenv ("LANGUAGE", "", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en");
}

static void
test_allow_interaction (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    /* By default, interaction is allowed */
    g_assert_true (snapd_client_get_allow_interaction (client));

    /* ... which sends the X-Allow-Interaction header with requests */
    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, "true");

    /* If interaction is not allowed, the header is not sent */
    snapd_client_set_allow_interaction (client, FALSE);
    g_assert_false (snapd_client_get_allow_interaction (client));
    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, NULL);
}

static void
test_get_system_information_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpint (snapd_system_information_get_confinement (info), ==, SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
    g_assert_cmpstr (snapd_system_information_get_kernel_version (info), ==, "KERNEL-VERSION");
    g_assert_cmpstr (snapd_system_information_get_os_id (info), ==, "OS-ID");
    g_assert_cmpstr (snapd_system_information_get_os_version (info), ==, "OS-VERSION");
    g_assert_cmpstr (snapd_system_information_get_series (info), ==, "SERIES");
    g_assert_cmpstr (snapd_system_information_get_version (info), ==, "VERSION");
    g_assert_true (snapd_system_information_get_managed (info));
    g_assert_true (snapd_system_information_get_on_classic (info));
    g_assert_cmpstr (snapd_system_information_get_mount_directory (info), ==, "/snap");
    g_assert_cmpstr (snapd_system_information_get_binaries_directory (info), ==, "/snap/bin");
    g_assert_null (snapd_system_information_get_store (info));
}

static void
system_information_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(SnapdSystemInformation) info = NULL;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    info = snapd_client_get_system_information_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpint (snapd_system_information_get_confinement (info), ==, SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
    g_assert_cmpstr (snapd_system_information_get_kernel_version (info), ==, "KERNEL-VERSION");
    g_assert_cmpstr (snapd_system_information_get_os_id (info), ==, "OS-ID");
    g_assert_cmpstr (snapd_system_information_get_os_version (info), ==, "OS-VERSION");
    g_assert_cmpstr (snapd_system_information_get_series (info), ==, "SERIES");
    g_assert_cmpstr (snapd_system_information_get_version (info), ==, "VERSION");
    g_assert_true (snapd_system_information_get_managed (info));
    g_assert_true (snapd_system_information_get_on_classic (info));
    g_assert_cmpstr (snapd_system_information_get_mount_directory (info), ==, "/snap");
    g_assert_cmpstr (snapd_system_information_get_binaries_directory (info), ==, "/snap/bin");
    g_assert_null (snapd_system_information_get_store (info));

    g_main_loop_quit (data->loop);
}

static void
test_get_system_information_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_get_system_information_async (client, NULL, system_information_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
test_get_system_information_store (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_store (snapd, "store");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (snapd_system_information_get_store (info), ==, "store");
}

static void
test_get_system_information_confinement_strict (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "strict");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpint (snapd_system_information_get_confinement (info), ==, SNAPD_SYSTEM_CONFINEMENT_STRICT);
}

static void
test_get_system_information_confinement_none (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "partial");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpint (snapd_system_information_get_confinement (info), ==, SNAPD_SYSTEM_CONFINEMENT_PARTIAL);
}

static void
test_get_system_information_confinement_unknown (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "NOT_DEFINED");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpint (snapd_system_information_get_confinement (info), ==, SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
}

static void
test_login_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_auto(GStrv) keys = NULL;
    gchar **ssh_keys = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    SnapdAuthData *auth_data;
    int i;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    keys = g_strsplit ("KEY1;KEY2", ";", -1);
    mock_account_set_ssh_keys (a, keys);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    g_assert_cmpint (snapd_user_information_get_id (user_information), ==, 1);
    g_assert_cmpstr (snapd_user_information_get_email (user_information), ==, "test@example.com");
    g_assert_cmpstr (snapd_user_information_get_username (user_information), ==, "test");
    ssh_keys = snapd_user_information_get_ssh_keys (user_information);
    g_assert_nonnull (ssh_keys);
    g_assert_cmpint (g_strv_length (ssh_keys), ==, 0);
    auth_data = snapd_user_information_get_auth_data (user_information);
    g_assert_nonnull (auth_data);
    g_assert_cmpstr (snapd_auth_data_get_macaroon (auth_data), ==, mock_account_get_macaroon (a));
    g_assert (g_strv_length (snapd_auth_data_get_discharges (auth_data)) == g_strv_length (mock_account_get_discharges (a)));
    for (i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert_cmpstr (snapd_auth_data_get_discharges (auth_data)[i], ==, mock_account_get_discharges (a)[i]);
}

static void
test_login_invalid_email (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "not-an-email", "secret", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_INVALID);
    g_assert_null (user_information);
}

static void
test_login_invalid_password (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "invalid", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert_null (user_information);
}

static void
test_login_otp_missing (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_otp (a, "1234");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_REQUIRED);
    g_assert_null (user_information);
}

static void
test_login_otp_invalid (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_otp (a, "1234");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", "0000", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_INVALID);
    g_assert_null (user_information);
}

static void
test_login_legacy (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    int i;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
G_GNUC_END_IGNORE_DEPRECATIONS
    g_assert_no_error (error);
    g_assert_nonnull (auth_data);
    g_assert_cmpstr (snapd_auth_data_get_macaroon (auth_data), ==, mock_account_get_macaroon (a));
    g_assert (g_strv_length (snapd_auth_data_get_discharges (auth_data)) == g_strv_length (mock_account_get_discharges (a)));
    for (i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert_cmpstr (snapd_auth_data_get_discharges (auth_data)[i], ==, mock_account_get_discharges (a)[i]);
}

static void
test_list_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_list_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 3);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap1");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[1]), ==, "snap2");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[2]), ==, "snap3");
}

static void
list_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_list_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 3);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap1");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[1]), ==, "snap2");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[2]), ==, "snap3");

    g_main_loop_quit (data->loop);
}

static void
test_list_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_list_async (client, NULL, list_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
test_list_one_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_apps (snap)->len, ==, 0);
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, NULL);
    g_assert_cmpint (g_strv_length (snapd_snap_get_tracks (snap)), ==, 0);
    g_assert_cmpint (snapd_snap_get_channels (snap)->len, ==, 0);
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_snap_get_contact (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert_false (snapd_snap_get_devmode (snap));
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    g_assert_null (snapd_snap_get_install_date (snap));
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 0);
    g_assert_false (snapd_snap_get_jailmode (snap));
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "snap");
    g_assert_cmpint (snapd_snap_get_prices (snap)->len, ==, 0);
    g_assert_false (snapd_snap_get_private (snap));
    g_assert_cmpstr (snapd_snap_get_revision (snap), ==, "REVISION");
    g_assert_cmpint (snapd_snap_get_screenshots (snap)->len, ==, 0);
    g_assert_cmpint (snapd_snap_get_snap_type (snap), ==, SNAPD_SNAP_TYPE_APP);
    g_assert_cmpint (snapd_snap_get_status (snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
    g_assert_cmpstr (snapd_snap_get_summary (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_tracking_channel (snap), ==, NULL);
    g_assert_false (snapd_snap_get_trymode (snap));
    g_assert_cmpstr (snapd_snap_get_version (snap), ==, "VERSION");
}

static void
list_one_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    snap = snapd_client_list_one_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_apps (snap)->len, ==, 0);
    g_assert_cmpstr (snapd_snap_get_broken (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, NULL);
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_snap_get_contact (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert_false (snapd_snap_get_devmode (snap));
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    g_assert_null (snapd_snap_get_install_date (snap));
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 0);
    g_assert_false (snapd_snap_get_jailmode (snap));
    g_assert_null (snapd_snap_get_license (snap));
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "snap");
    g_assert_cmpint (snapd_snap_get_prices (snap)->len, ==, 0);
    g_assert_false (snapd_snap_get_private (snap));
    g_assert_cmpstr (snapd_snap_get_revision (snap), ==, "REVISION");
    g_assert_cmpint (snapd_snap_get_screenshots (snap)->len, ==, 0);
    g_assert_cmpint (snapd_snap_get_snap_type (snap), ==, SNAPD_SNAP_TYPE_APP);
    g_assert_cmpint (snapd_snap_get_status (snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
    g_assert_cmpstr (snapd_snap_get_summary (snap), ==, NULL);
    g_assert_cmpstr (snapd_snap_get_tracking_channel (snap), ==, NULL);
    g_assert_false (snapd_snap_get_trymode (snap));
    g_assert_cmpstr (snapd_snap_get_version (snap), ==, "VERSION");

    g_main_loop_quit (data->loop);
}

static void
test_list_one_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_list_one_async (client, "snap", NULL, list_one_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
test_list_one_optional_fields (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(GDateTime) date = NULL;
    SnapdApp *app;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");
    mock_app_add_auto_alias (a, "app2");
    mock_app_add_auto_alias (a, "app3");
    mock_app_set_desktop_file (a, "/var/lib/snapd/desktop/applications/app.desktop");
    mock_snap_set_broken (s, "BROKEN");
    mock_snap_set_confinement (s, "classic");
    mock_snap_set_devmode (s, TRUE);
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    mock_snap_set_installed_size (s, 1024);
    mock_snap_set_jailmode (s, TRUE);
    mock_snap_set_trymode (s, TRUE);
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_license (s, "LICENSE");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_tracking_channel (s, "CHANNEL");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_apps (snap)->len, ==, 1);
    app = snapd_snap_get_apps (snap)->pdata[0];
    g_assert_cmpstr (snapd_app_get_name (app), ==, "app");
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_NONE);
    g_assert_cmpstr (snapd_app_get_snap (app), ==, "snap");
    g_assert_false (snapd_app_get_active (app));
    g_assert_false (snapd_app_get_enabled (app));
    g_assert_cmpstr (snapd_app_get_desktop_file (app), ==, "/var/lib/snapd/desktop/applications/app.desktop");
    g_assert_cmpstr (snapd_snap_get_broken (snap), ==, "BROKEN");
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, "CHANNEL");
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_CLASSIC);
    g_assert_cmpstr (snapd_snap_get_contact (snap), ==, "CONTACT");
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, "DESCRIPTION");
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert_true (snapd_snap_get_devmode (snap));
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    date = g_date_time_new_utc (2017, 1, 2, 11, 23, 58);
    g_assert (g_date_time_compare (snapd_snap_get_install_date (snap), date) == 0);
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 1024);
    g_assert_true (snapd_snap_get_jailmode (snap));
    g_assert_cmpstr (snapd_snap_get_license (snap), ==, "LICENSE");
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "snap");
    g_assert_cmpint (snapd_snap_get_prices (snap)->len, ==, 0);
    g_assert_false (snapd_snap_get_private (snap));
    g_assert_cmpstr (snapd_snap_get_revision (snap), ==, "REVISION");
    g_assert_cmpint (snapd_snap_get_screenshots (snap)->len, ==, 0);
    g_assert_cmpint (snapd_snap_get_snap_type (snap), ==, SNAPD_SNAP_TYPE_APP);
    g_assert_cmpint (snapd_snap_get_status (snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
    g_assert_cmpstr (snapd_snap_get_summary (snap), ==, "SUMMARY");
    g_assert_cmpstr (snapd_snap_get_tracking_channel (snap), ==, "CHANNEL");
    g_assert_true (snapd_snap_get_trymode (snap));
    g_assert_cmpstr (snapd_snap_get_version (snap), ==, "VERSION");
}

static void
test_list_one_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert_null (snap);
}

static void
test_list_one_classic_confinement (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_CLASSIC);
}

static void
test_list_one_devmode_confinement (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_DEVMODE);
}

static void
test_list_one_daemons (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;
    SnapdApp *app;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app1");
    mock_app_set_daemon (a, "simple");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_daemon (a, "forking");
    a = mock_snap_add_app (s, "app3");
    mock_app_set_daemon (a, "oneshot");
    a = mock_snap_add_app (s, "app4");
    mock_app_set_daemon (a, "notify");
    a = mock_snap_add_app (s, "app5");
    mock_app_set_daemon (a, "dbus");
    a = mock_snap_add_app (s, "app6");
    mock_app_set_daemon (a, "INVALID");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snap);
    g_assert_cmpint (snapd_snap_get_apps (snap)->len, ==, 6);
    app = snapd_snap_get_apps (snap)->pdata[0];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_SIMPLE);
    app = snapd_snap_get_apps (snap)->pdata[1];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_FORKING);
    app = snapd_snap_get_apps (snap)->pdata[2];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_ONESHOT);
    app = snapd_snap_get_apps (snap)->pdata[3];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_NOTIFY);
    app = snapd_snap_get_apps (snap)->pdata[4];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_DBUS);
    app = snapd_snap_get_apps (snap)->pdata[5];
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_UNKNOWN);
}

static void
test_get_apps_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(GPtrArray) apps = NULL;
    SnapdApp *app;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app1");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_desktop_file (a, "foo.desktop");
    a = mock_snap_add_app (s, "app3");
    mock_app_set_daemon (a, "simple");
    mock_app_set_active (a, TRUE);
    mock_app_set_enabled (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    apps = snapd_client_get_apps_sync (client, SNAPD_GET_APPS_FLAGS_NONE, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (apps);
    g_assert_cmpint (apps->len, ==, 3);
    app = apps->pdata[0];
    g_assert_cmpstr (snapd_app_get_name (app), ==, "app1");
    g_assert_cmpstr (snapd_app_get_snap (app), ==, "snap");
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_NONE);
    g_assert_false (snapd_app_get_active (app));
    g_assert_false (snapd_app_get_enabled (app));
    app = apps->pdata[1];
    g_assert_cmpstr (snapd_app_get_name (app), ==, "app2");
    g_assert_cmpstr (snapd_app_get_snap (app), ==, "snap");
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_NONE);
    g_assert_false (snapd_app_get_active (app));
    g_assert_false (snapd_app_get_enabled (app));
    app = apps->pdata[2];
    g_assert_cmpstr (snapd_app_get_name (app), ==, "app3");
    g_assert_cmpstr (snapd_app_get_snap (app), ==, "snap");
    g_assert_cmpint (snapd_app_get_daemon_type (app), ==, SNAPD_DAEMON_TYPE_SIMPLE);
    g_assert_true (snapd_app_get_active (app));
    g_assert_true (snapd_app_get_enabled (app));
}

static void
test_get_apps_services (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(GPtrArray) apps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_add_app (s, "app1");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_daemon (a, "simple");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    apps = snapd_client_get_apps_sync (client, SNAPD_GET_APPS_FLAGS_SELECT_SERVICES, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (apps);
    g_assert_cmpint (apps->len, ==, 1);
    g_assert_cmpstr (snapd_app_get_name (apps->pdata[0]), ==, "app2");
}

static void
test_icon_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(GBytes) icon_data = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdIcon) icon = NULL;
    GBytes *data;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    icon_data = g_bytes_new ("ICON-DATA", 9);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    icon = snapd_client_get_icon_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (icon);
    g_assert_cmpstr (snapd_icon_get_mime_type (icon), ==, "image/png");
    data = snapd_icon_get_data (icon);
    g_assert_cmpmem (g_bytes_get_data (data, NULL), g_bytes_get_size (data), "ICON-DATA", 9);
}

static void
icon_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(SnapdIcon) icon = NULL;
    GBytes *icon_data;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    icon = snapd_client_get_icon_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_nonnull (icon);
    g_assert_cmpstr (snapd_icon_get_mime_type (icon), ==, "image/png");
    icon_data = snapd_icon_get_data (icon);
    g_assert_cmpmem (g_bytes_get_data (icon_data, NULL), g_bytes_get_size (icon_data), "ICON-DATA", 9);

    g_main_loop_quit (data->loop);
}

static void
test_icon_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(GBytes) icon_data = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    icon_data = g_bytes_new ("ICON-DATA", 9);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snapd_client_get_icon_async (client, "snap", NULL, icon_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
test_icon_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdIcon) icon = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    icon = snapd_client_get_icon_sync (client, "snap", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert_null (icon);
}

static void
test_icon_large (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autofree gchar *icon_buffer = NULL;
    gsize i, icon_buffer_length = 1048576;
    g_autoptr(GBytes) icon_data = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdIcon) icon = NULL;
    GBytes *data;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    icon_buffer = g_malloc (icon_buffer_length);
    for (i = 0; i < icon_buffer_length; i++)
        icon_buffer[i] = i % 255;
    icon_data = g_bytes_new (icon_buffer, icon_buffer_length);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    icon = snapd_client_get_icon_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (icon);
    g_assert_cmpstr (snapd_icon_get_mime_type (icon), ==, "image/png");
    data = snapd_icon_get_data (icon);
    g_assert_cmpmem (g_bytes_get_data (data, NULL), g_bytes_get_size (data), icon_buffer, icon_buffer_length);
}

static void
test_get_assertions_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) assertions;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_assertion (snapd,
                              "type: account\n"
                              "list-header:\n"
                              "  - list-value\n"
                              "map-header:\n"
                              "  map-value: foo\n"
                              "\n"
                              "SIGNATURE");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    assertions = snapd_client_get_assertions_sync (client, "account", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (assertions);
    g_assert_cmpint (g_strv_length (assertions), ==, 1);
    g_assert_cmpstr (assertions[0], ==, "type: account\n"
                                        "list-header:\n"
                                        "  - list-value\n"
                                        "map-header:\n"
                                        "  map-value: foo\n"
                                        "\n"
                                        "SIGNATURE");
}

static void
test_get_assertions_body (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) assertions = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_assertion (snapd,
                              "type: account\n"
                              "body-length: 4\n"
                              "\n"
                              "BODY\n"
                              "\n"
                              "SIGNATURE");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    assertions = snapd_client_get_assertions_sync (client, "account", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (assertions);
    g_assert_cmpint (g_strv_length (assertions), ==, 1);
    g_assert_cmpstr (assertions[0], ==, "type: account\n"
                                        "body-length: 4\n"
                                        "\n"
                                        "BODY\n"
                                        "\n"
                                        "SIGNATURE");
}

static void
test_get_assertions_multiple (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) assertions = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_assertion (snapd,
                              "type: account\n"
                              "\n"
                              "SIGNATURE1\n"
                              "\n"
                              "type: account\n"
                              "body-length: 4\n"
                              "\n"
                              "BODY\n"
                              "\n"
                              "SIGNATURE2\n"
                              "\n"
                              "type: account\n"
                              "\n"
                              "SIGNATURE3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    assertions = snapd_client_get_assertions_sync (client, "account", NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (assertions);
    g_assert_cmpint (g_strv_length (assertions), ==, 3);
    g_assert_cmpstr (assertions[0], ==, "type: account\n"
                                        "\n"
                                        "SIGNATURE1");
    g_assert_cmpstr (assertions[1], ==, "type: account\n"
                                        "body-length: 4\n"
                                        "\n"
                                        "BODY\n"
                                        "\n"
                                        "SIGNATURE2");
    g_assert_cmpstr (assertions[2], ==, "type: account\n"
                                        "\n"
                                        "SIGNATURE3");
}

static void
test_get_assertions_invalid (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) assertions = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    assertions = snapd_client_get_assertions_sync (client, "account", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_null (assertions);
}

static void
test_add_assertions_sync (void)
{
    gchar *assertions[2];
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_get_assertions (snapd));
    assertions[0] = "type: account\n"
                    "\n"
                    "SIGNATURE";
    assertions[1] = NULL;
    result = snapd_client_add_assertions_sync (client,
                                               assertions,
                                               NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpint (g_list_length (mock_snapd_get_assertions (snapd)), == , 1);
    g_assert_cmpstr (mock_snapd_get_assertions (snapd)->data, == , "type: account\n\nSIGNATURE");
}

static void
test_assertions_sync (void)
{
    g_autoptr(SnapdAssertion) assertion = NULL;
    g_auto(GStrv) headers = NULL;
    g_autofree gchar *type = NULL;
    g_autofree gchar *authority_id = NULL;
    g_autofree gchar *invalid_header = NULL;
    g_autofree gchar *body = NULL;
    g_autofree gchar *signature = NULL;

    assertion = snapd_assertion_new ("type: account\n"
                                     "authority-id: canonical\n"
                                     "\n"
                                     "SIGNATURE");
    headers = snapd_assertion_get_headers (assertion);
    g_assert_cmpint (g_strv_length (headers), ==, 2);
    g_assert_cmpstr (headers[0], ==, "type");
    type = snapd_assertion_get_header (assertion, "type");
    g_assert_cmpstr (type, ==, "account");
    g_assert_cmpstr (headers[1], ==, "authority-id");
    authority_id = snapd_assertion_get_header (assertion, "authority-id");
    g_assert_cmpstr (authority_id, ==, "canonical");
    invalid_header = snapd_assertion_get_header (assertion, "invalid");
    g_assert_cmpstr (invalid_header, ==, NULL);
    body = snapd_assertion_get_body (assertion);
    g_assert_cmpstr (body, ==, NULL);
    signature = snapd_assertion_get_signature (assertion);
    g_assert_cmpstr (signature, ==, "SIGNATURE");
}

static void
test_assertions_body (void)
{
    g_autoptr(SnapdAssertion) assertion = NULL;
    g_auto(GStrv) headers = NULL;
    g_autofree gchar *type = NULL;
    g_autofree gchar *body_length = NULL;
    g_autofree gchar *invalid_header = NULL;
    g_autofree gchar *body = NULL;
    g_autofree gchar *signature = NULL;

    assertion = snapd_assertion_new ("type: account\n"
                                     "body-length: 4\n"
                                     "\n"
                                     "BODY\n"
                                     "\n"
                                     "SIGNATURE");
    headers = snapd_assertion_get_headers (assertion);
    g_assert_cmpint (g_strv_length (headers), ==, 2);
    g_assert_cmpstr (headers[0], ==, "type");
    type = snapd_assertion_get_header (assertion, "type");
    g_assert_cmpstr (type, ==, "account");
    g_assert_cmpstr (headers[1], ==, "body-length");
    body_length = snapd_assertion_get_header (assertion, "body-length");
    g_assert_cmpstr (body_length, ==, "4");
    invalid_header = snapd_assertion_get_header (assertion, "invalid");
    g_assert_cmpstr (invalid_header, ==, NULL);
    body = snapd_assertion_get_body (assertion);
    g_assert_cmpstr (body, ==, "BODY");
    signature = snapd_assertion_get_signature (assertion);
    g_assert_cmpstr (signature, ==, "SIGNATURE");
}

static void
test_get_interfaces_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockSlot *sl;
    MockPlug *p;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots = NULL;
    SnapdPlug *plug;
    SnapdSlot *slot;
    GPtrArray *connections;
    SnapdConnection *connection;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    sl = mock_snap_add_slot (s, "slot1");
    mock_snap_add_slot (s, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    p = mock_snap_add_plug (s, "plug1");
    mock_plug_set_connection (p, sl);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_get_interfaces_sync (client, &plugs, &slots, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);

    g_assert_nonnull (plugs);
    g_assert_cmpint (plugs->len, ==, 1);

    plug = plugs->pdata[0];
    g_assert_cmpstr (snapd_plug_get_name (plug), ==, "plug1");
    g_assert_cmpstr (snapd_plug_get_snap (plug), ==, "snap2");
    g_assert_cmpstr (snapd_plug_get_interface (plug), ==, "INTERFACE");
    // FIXME: Attributes
    g_assert_cmpstr (snapd_plug_get_label (plug), ==, "LABEL");
    connections = snapd_plug_get_connections (plug);
    g_assert_cmpint (connections->len, ==, 1);
    connection = connections->pdata[0];
    g_assert_cmpstr (snapd_connection_get_snap (connection), ==, "snap1");
    g_assert_cmpstr (snapd_connection_get_name (connection), ==, "slot1");

    g_assert_nonnull (slots);
    g_assert_cmpint (slots->len, ==, 2);

    slot = slots->pdata[0];
    g_assert_cmpstr (snapd_slot_get_name (slot), ==, "slot1");
    g_assert_cmpstr (snapd_slot_get_snap (slot), ==, "snap1");
    g_assert_cmpstr (snapd_slot_get_interface (slot), ==, "INTERFACE");
    // FIXME: Attributes
    g_assert_cmpstr (snapd_slot_get_label (slot), ==, "LABEL");
    connections = snapd_slot_get_connections (slot);
    g_assert_cmpint (connections->len, ==, 1);
    connection = connections->pdata[0];
    g_assert_cmpstr (snapd_connection_get_snap (connection), ==, "snap2");
    g_assert_cmpstr (snapd_connection_get_name (connection), ==, "plug1");

    slot = slots->pdata[1];
    g_assert_cmpstr (snapd_slot_get_name (slot), ==, "slot2");
    g_assert_cmpstr (snapd_slot_get_snap (slot), ==, "snap1");
    connections = snapd_slot_get_connections (slot);
    g_assert_cmpint (connections->len, ==, 0);
}

static void
test_get_interfaces_no_snaps (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) plugs = NULL;
    g_autoptr(GPtrArray) slots = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_get_interfaces_sync (client, &plugs, &slots, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (plugs);
    g_assert_cmpint (plugs->len, ==, 0);
    g_assert_nonnull (slots);
    g_assert_cmpint (slots->len, ==, 0);
    g_assert_true (result);
}

static void
test_connect_interface_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockSlot *slot;
    MockPlug *plug;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    plug = mock_snap_add_plug (s, "plug");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert (mock_plug_get_connection (plug) == slot);
}

typedef struct
{
    int progress_done;
} ConnectInterfaceProgressData;

static void
connect_interface_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    ConnectInterfaceProgressData *data = user_data;
    data->progress_done++;
}

static void
test_connect_interface_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockSlot *slot;
    MockPlug *plug;
    g_autoptr(SnapdClient) client = NULL;
    ConnectInterfaceProgressData connect_interface_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    plug = mock_snap_add_plug (s, "plug");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    connect_interface_progress_data.progress_done = 0;
    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", connect_interface_progress_cb, &connect_interface_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert (mock_plug_get_connection (plug) == slot);
    g_assert_cmpint (connect_interface_progress_data.progress_done, >, 0);
}

static void
test_connect_interface_invalid (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_disconnect_interface_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockSlot *slot;
    MockPlug *plug;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    plug = mock_snap_add_plug (s, "plug");
    mock_plug_set_connection (plug, slot);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_plug_get_connection (plug));
}

typedef struct
{
    int progress_done;
} DisconnectInterfaceProgressData;

static void
disconnect_interface_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    DisconnectInterfaceProgressData *data = user_data;
    data->progress_done++;
}

static void
test_disconnect_interface_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockSlot *slot;
    MockPlug *plug;
    g_autoptr(SnapdClient) client = NULL;
    DisconnectInterfaceProgressData disconnect_interface_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    plug = mock_snap_add_plug (s, "plug");
    mock_plug_set_connection (plug, slot);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    disconnect_interface_progress_data.progress_done = 0;
    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", disconnect_interface_progress_cb, &disconnect_interface_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_plug_get_connection (plug));
    g_assert_cmpint (disconnect_interface_progress_data.progress_done, >, 0);
}

static void
test_disconnect_interface_invalid (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_find_query (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    SnapdSnap *snap;
    gchar **tracks;
    GPtrArray *channels, *prices, *screenshots;
    SnapdChannel *channel;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    mock_snapd_add_store_snap (snapd, "apple");
    mock_snapd_add_store_snap (snapd, "banana");
    mock_snapd_add_store_snap (snapd, "carrot1");
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_download_size (s, 1024);
    mock_snap_add_price (s, 1.20, "NZD");
    mock_snap_add_price (s, 0.87, "USD");
    mock_snap_add_screenshot (s, "screenshot0.png", 0, 0);
    mock_snap_add_screenshot (s, "screenshot1.png", 1024, 1024);
    mock_snap_set_trymode (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_NONE, "carrot", &suggested_currency, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 2);
    g_assert_cmpstr (suggested_currency, ==, "NZD");
    snap = snaps->pdata[0];
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "carrot1");
    g_assert_null (snapd_snap_get_channel (snap));
    tracks = snapd_snap_get_tracks (snap);
    g_assert_cmpint (g_strv_length (tracks), ==, 1);
    g_assert_cmpstr (tracks[0], ==, "latest");
    channels = snapd_snap_get_channels (snap);
    g_assert_cmpint (channels->len, ==, 1);
    channel = channels->pdata[0];
    g_assert_cmpstr (snapd_channel_get_name (channel), ==, "stable");
    g_assert_cmpint (snapd_channel_get_confinement (channel), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_channel_get_revision (channel), ==, "REVISION");
    g_assert_cmpstr (snapd_channel_get_version (channel), ==, "VERSION");
    g_assert_cmpstr (snapd_channel_get_epoch (channel), ==, "0");
    g_assert_cmpint (snapd_channel_get_size (channel), ==, 65535);
    g_assert_null (snapd_snap_get_contact (snap));
    g_assert_null (snapd_snap_get_description (snap));
    g_assert_null (snapd_snap_get_summary (snap));
    snap = snaps->pdata[1];
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, "CHANNEL");
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_snap_get_contact (snap), ==, "CONTACT");
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, "DESCRIPTION");
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 1024);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    g_assert_null (snapd_snap_get_install_date (snap));
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "carrot2");
    prices = snapd_snap_get_prices (snap);
    g_assert_cmpint (prices->len, ==, 2);
    g_assert_cmpfloat (snapd_price_get_amount (prices->pdata[0]), ==, 1.20);
    g_assert_cmpstr (snapd_price_get_currency (prices->pdata[0]), ==, "NZD");
    g_assert_cmpfloat (snapd_price_get_amount (prices->pdata[1]), ==, 0.87);
    g_assert_cmpstr (snapd_price_get_currency (prices->pdata[1]), ==, "USD");
    g_assert_false (snapd_snap_get_private (snap));
    g_assert_cmpstr (snapd_snap_get_revision (snap), ==, "REVISION");
    screenshots = snapd_snap_get_screenshots (snap);
    g_assert_cmpint (screenshots->len, ==, 2);
    g_assert_cmpstr (snapd_screenshot_get_url (screenshots->pdata[0]), ==, "screenshot0.png");
    g_assert_cmpstr (snapd_screenshot_get_url (screenshots->pdata[1]), ==, "screenshot1.png");
    g_assert_cmpint (snapd_screenshot_get_width (screenshots->pdata[1]), ==, 1024);
    g_assert_cmpint (snapd_screenshot_get_height (screenshots->pdata[1]), ==, 1024);
    g_assert_cmpint (snapd_snap_get_snap_type (snap), ==, SNAPD_SNAP_TYPE_APP);
    g_assert_cmpint (snapd_snap_get_status (snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
    g_assert_cmpstr (snapd_snap_get_summary (snap), ==, "SUMMARY");
    g_assert_true (snapd_snap_get_trymode (snap));
    g_assert_cmpstr (snapd_snap_get_version (snap), ==, "VERSION");
}

static void
test_find_query_private (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_account_add_private_snap (a, "snap2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap2");
    g_assert_true (snapd_snap_get_private (snaps->pdata[0]));
}

static void
test_find_query_private_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert_null (snaps);
}

static void
test_find_name (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap");
}

static void
test_find_name_private (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_add_private_snap (a, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap");
    g_assert_true (snapd_snap_get_private (snaps->pdata[0]));
}

static void
test_find_name_private_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert_null (snaps);
}

static void
test_find_channels (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockTrack *t;
    MockChannel *c;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    SnapdSnap *snap;
    gchar **tracks;
    GPtrArray *channels;
    SnapdChannel *channel;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    t = mock_snap_add_track (s, "latest");
    c = mock_track_add_channel (t, "beta", NULL);
    mock_channel_set_revision (c, "BETA-REVISION");
    mock_channel_set_version (c, "BETA-VERSION");
    mock_channel_set_epoch (c, "1");
    mock_channel_set_confinement (c, "classic");
    mock_channel_set_size (c, 10000);
    mock_snap_add_track (s, "TRACK");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    snap = snaps->pdata[0];
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "snap");
    tracks = snapd_snap_get_tracks (snap);
    g_assert_cmpint (g_strv_length (tracks), ==, 2);
    g_assert_cmpstr (tracks[0], ==, "latest");
    g_assert_cmpstr (tracks[1], ==, "TRACK");
    channels = snapd_snap_get_channels (snap);
    g_assert_cmpint (channels->len, ==, 2);
    channel = snapd_snap_match_channel (snap, "stable");
    g_assert_nonnull (channel);
    g_assert_cmpstr (snapd_channel_get_name (channel), ==, "stable");
    g_assert_cmpstr (snapd_channel_get_revision (channel), ==, "REVISION");
    g_assert_cmpstr (snapd_channel_get_version (channel), ==, "VERSION");
    g_assert_cmpstr (snapd_channel_get_epoch (channel), ==, "0");
    g_assert_cmpint (snapd_channel_get_confinement (channel), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpint (snapd_channel_get_size (channel), ==, 65535);
    channel = snapd_snap_match_channel (snap, "beta");
    g_assert_nonnull (channel);
    g_assert_cmpstr (snapd_channel_get_name (channel), ==, "beta");
    g_assert_cmpstr (snapd_channel_get_revision (channel), ==, "BETA-REVISION");
    g_assert_cmpstr (snapd_channel_get_version (channel), ==, "BETA-VERSION");
    g_assert_cmpstr (snapd_channel_get_epoch (channel), ==, "1");
    g_assert_cmpint (snapd_channel_get_confinement (channel), ==, SNAPD_CONFINEMENT_CLASSIC);
    g_assert_cmpint (snapd_channel_get_size (channel), ==, 10000);
}

static gboolean
cancel_cb (gpointer user_data)
{
    GCancellable *cancellable = user_data;
    g_cancellable_cancel (cancellable);
    return G_SOURCE_REMOVE;
}

static void
find_cancel_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    snaps = snapd_client_find_finish (SNAPD_CLIENT (object), result, NULL, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_null (snaps);

    g_main_loop_quit (data->loop);
}

static void
test_find_cancel (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GCancellable) cancellable = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    /* Use a special query that never responds */
    cancellable = g_cancellable_new ();
    snapd_client_find_async (client, SNAPD_FIND_FLAGS_NONE, "do-not-respond", cancellable, find_cancel_cb, async_data_new (loop, snapd));
    g_idle_add (cancel_cb, cancellable);

    g_main_loop_run (loop);
}

static void
test_find_section (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_section_sync (client, SNAPD_FIND_FLAGS_NONE, "section", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 2);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "apple");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[1]), ==, "carrot1");
}

static void
test_find_section_query (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_section_sync (client, SNAPD_FIND_FLAGS_NONE, "section", "carrot", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "carrot1");
}

static void
test_find_section_name (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_snap_add_store_section (s, "section");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_section_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME, "section", "carrot1", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "carrot1");
}

static void
test_find_refreshable_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_refreshable_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 2);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap1");
    g_assert_cmpstr (snapd_snap_get_revision (snaps->pdata[0]), ==, "1");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[1]), ==, "snap3");
    g_assert_cmpstr (snapd_snap_get_revision (snaps->pdata[1]), ==, "1");
}

static void
test_find_refreshable_no_updates (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autofree gchar *suggested_currency = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snaps = snapd_client_find_refreshable_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (snaps);
    g_assert_cmpint (snaps->len, ==, 0);
}

static void
test_install_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (snapd, "snap")), ==, "strict");
    g_assert_false (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_sync_multiple (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap3"));
}

static void
install_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_true (r);
    g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (data->snapd, "snap")), ==, "strict");
    g_assert_false (mock_snap_get_devmode (mock_snapd_find_snap (data->snapd, "snap")));
    g_assert_false (mock_snap_get_dangerous (mock_snapd_find_snap (data->snapd, "snap")));
    g_assert_false (mock_snap_get_jailmode (mock_snapd_find_snap (data->snapd, "snap")));

    g_main_loop_quit (data->loop);
}

static void
test_install_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, install_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
install_multiple_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    AsyncData *data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_true (r);

    data->counter--;
    if (data->counter == 0) {
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap2"));
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap3"));

        g_main_loop_quit (data->loop);

        async_data_free (data);
    }
}

static void
test_install_async_multiple (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    AsyncData *data;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));
    data = async_data_new (loop, snapd);
    data->counter = 3;
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL, NULL, NULL, NULL, NULL, install_multiple_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL, NULL, NULL, NULL, NULL, install_multiple_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL, NULL, NULL, NULL, NULL, install_multiple_cb, data);
    g_main_loop_run (loop);
}

static void
install_failure_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert_cmpstr (error->message, ==, "ERROR");
    g_assert_false (r);
    g_assert_null (mock_snapd_find_snap (data->snapd, "snap"));

    g_main_loop_quit (data->loop);
}

static void
test_install_async_failure (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_error (s, "ERROR");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, install_failure_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
install_cancel_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_false (r);
    g_assert_null (mock_snapd_find_snap (data->snapd, "snap"));

    g_main_loop_quit (data->loop);
}

static void
test_install_async_cancel (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GCancellable) cancellable = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    cancellable = g_cancellable_new ();
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, cancellable, install_cancel_cb, async_data_new (loop, snapd));
    g_idle_add (cancel_cb, cancellable);
    g_main_loop_run (loop);
}

static void
complete_async_multiple_cancel_first (AsyncData *data)
{
    data->counter--;
    if (data->counter == 0) {
        g_assert_null (mock_snapd_find_snap (data->snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap2"));
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap3"));

        g_main_loop_quit (data->loop);

        async_data_free (data);
    }
}

static void
install_multiple_cancel_first_snap1_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    AsyncData *data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_false (r);
    g_assert_null (mock_snapd_find_snap (data->snapd, "snap1"));

    complete_async_multiple_cancel_first (data);
}

static void
install_multiple_cancel_first_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    AsyncData *data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_true (r);

    complete_async_multiple_cancel_first (data);
}

static void
test_install_async_multiple_cancel_first (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GCancellable) cancellable = NULL;
    AsyncData *data;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));
    cancellable = g_cancellable_new ();
    data = async_data_new (loop, snapd);
    data->counter = 3;
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL, NULL, NULL, NULL, cancellable, install_multiple_cancel_first_snap1_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL, NULL, NULL, NULL, NULL, install_multiple_cancel_first_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL, NULL, NULL, NULL, NULL, install_multiple_cancel_first_cb, data);
    g_idle_add (cancel_cb, cancellable);
    g_main_loop_run (loop);
}

static void
complete_async_multiple_cancel_last (AsyncData *data)
{
    data->counter--;
    if (data->counter == 0) {
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap2"));
        g_assert_null (mock_snapd_find_snap (data->snapd, "snap3"));

        g_main_loop_quit (data->loop);

        async_data_free (data);
    }
}

static void
install_multiple_cancel_last_snap3_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    AsyncData *data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_false (r);
    g_assert_null (mock_snapd_find_snap (data->snapd, "snap3"));

    complete_async_multiple_cancel_last (data);
}

static void
install_multiple_cancel_last_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    AsyncData *data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_install2_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_true (r);

    complete_async_multiple_cancel_last (data);
}

static void
test_install_async_multiple_cancel_last (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GCancellable) cancellable = NULL;
    AsyncData *data;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));
    cancellable = g_cancellable_new ();
    data = async_data_new (loop, snapd);
    data->counter = 3;
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL, NULL, NULL, NULL, NULL, install_multiple_cancel_last_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL, NULL, NULL, NULL, NULL, install_multiple_cancel_last_cb, data);
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL, NULL, NULL, NULL, cancellable, install_multiple_cancel_last_snap3_cb, data);
    g_idle_add (cancel_cb, cancellable);
    g_main_loop_run (loop);
}

typedef struct
{
    int progress_done;
    const gchar *spawn_time;
    const gchar *ready_time;
} InstallProgressData;

static gchar *
time_to_string (GDateTime *time)
{
    if (time == NULL)
        return NULL;

    return g_date_time_format (time, "%FT%H:%M:%S%Z");
}

static void
install_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    InstallProgressData *data = user_data;
    int i, progress_done, progress_total;
    GPtrArray *tasks;
    g_autofree gchar *spawn_time = NULL, *ready_time = NULL;

    data->progress_done++;

    // Check we've been notified of all tasks
    tasks = snapd_change_get_tasks (change);
    for (progress_done = 0, progress_total = 0, i = 0; i < tasks->len; i++) {
        SnapdTask *task = tasks->pdata[i];
        progress_done += snapd_task_get_progress_done (task);
        progress_total += snapd_task_get_progress_total (task);
    }
    g_assert_cmpint (data->progress_done, ==, progress_done);

    spawn_time = time_to_string (snapd_change_get_spawn_time (change));
    ready_time = time_to_string (snapd_change_get_ready_time (change));

    g_assert_cmpstr (snapd_change_get_kind (change), ==, "KIND");
    g_assert_cmpstr (snapd_change_get_summary (change), ==, "SUMMARY");
    g_assert_cmpstr (snapd_change_get_status (change), ==, "STATUS");
    if (progress_done == progress_total)
        g_assert_true (snapd_change_get_ready (change));
    else
        g_assert_false (snapd_change_get_ready (change));
    g_assert_cmpstr (spawn_time, ==, data->spawn_time);
    if (snapd_change_get_ready (change))
        g_assert_cmpstr (ready_time, ==, data->ready_time);
    else
        g_assert_null (ready_time);
}

static void
test_install_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    InstallProgressData install_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    install_progress_data.progress_done = 0;
    install_progress_data.spawn_time = "2017-01-02T11:23:58Z";
    install_progress_data.ready_time = "2017-01-03T00:00:00Z";

    snapd = mock_snapd_new ();
    mock_snapd_set_spawn_time (snapd, install_progress_data.spawn_time);
    mock_snapd_set_ready_time (snapd, install_progress_data.ready_time);
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, install_progress_cb, &install_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpint (install_progress_data.progress_done, >, 0);
}

static void
test_install_needs_classic (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_CLASSIC);
    g_assert_false (result);
}

static void
test_install_classic (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_on_classic (snapd, TRUE);
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_CLASSIC, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (snapd, "snap")), ==, "classic");
}

static void
test_install_needs_classic_system (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_CLASSIC, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_CLASSIC_SYSTEM);
    g_assert_false (result);
}

static void
test_install_needs_devmode (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_DEVMODE);
    g_assert_false (result);
}

static void
test_install_devmode (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_DEVMODE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_dangerous (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_DANGEROUS, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_jailmode (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_JAILMODE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_channel (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel1");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", "channel2", NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_channel (mock_snapd_find_snap (snapd, "snap")), ==, "channel2");
}

static void
test_install_revision (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.2");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, "1.1", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_revision (mock_snapd_find_snap (snapd, "snap")), ==, "1.1");
}

static void
test_install_not_available (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_install_snapd_restart (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_restart_required (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
}

static void
test_install_async_snapd_restart (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_restart_required (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, NULL, NULL, NULL, install_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
test_install_stream_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_NONE, stream, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

typedef struct
{
    int progress_done;
} InstallStreamProgressData;

static void
install_stream_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    InstallStreamProgressData *data = user_data;
    data->progress_done++;
}

static void
test_install_stream_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    InstallStreamProgressData install_stream_progress_data;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    install_stream_progress_data.progress_done = 0;
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_NONE, stream, install_stream_progress_cb, &install_stream_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpint (install_stream_progress_data.progress_done, >, 0);
}

static void
test_install_stream_classic (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_CLASSIC, stream, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "classic");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_dangerous (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_DANGEROUS, stream, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_true (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_devmode (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_DEVMODE, stream, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_true (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_jailmode (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    stream = g_memory_input_stream_new_from_data ("SNAP", 4, NULL);
    result = snapd_client_install_stream_sync (client, SNAPD_INSTALL_FLAGS_JAILMODE, stream, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_true (mock_snap_get_jailmode (snap));
}

static void
test_try_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_try_sync (client, "/path/to/snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "try");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_path (snap), ==, "/path/to/snap");
}

typedef struct
{
    int progress_done;
} TryProgressData;

static void
try_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    TryProgressData *data = user_data;
    data->progress_done++;
}

static void
test_try_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    TryProgressData try_progress_data;
    g_autoptr(GInputStream) stream = NULL;
    gboolean result;
    MockSnap *snap;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    try_progress_data.progress_done = 0;
    result = snapd_client_try_sync (client, "/path/to/snap", try_progress_cb, &try_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    snap = mock_snapd_find_snap (snapd, "try");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_path (snap), ==, "/path/to/snap");
    g_assert_cmpint (try_progress_data.progress_done, >, 0);
}

static void
test_refresh_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
}

typedef struct
{
    int progress_done;
} RefreshProgressData;

static void
refresh_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    RefreshProgressData *data = user_data;
    data->progress_done++;
}

static void
test_refresh_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    RefreshProgressData refresh_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    refresh_progress_data.progress_done = 0;
    result = snapd_client_refresh_sync (client, "snap", NULL, refresh_progress_cb, &refresh_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpint (refresh_progress_data.progress_done, >, 0);
}

static void
test_refresh_channel (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    mock_snap_set_channel (s, "channel1");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    mock_snap_set_channel (s, "channel2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_refresh_sync (client, "snap", "channel2", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpstr (mock_snap_get_channel (mock_snapd_find_snap (snapd, "snap")), ==, "channel2");
}

static void
test_refresh_no_updates (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NO_UPDATE_AVAILABLE);
    g_assert_false (result);
}

static void
test_refresh_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_refresh_all_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) snap_names = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap_names = snapd_client_refresh_all_sync (client, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (g_strv_length (snap_names), ==, 2);
    g_assert_cmpstr (snap_names[0], ==, "snap1");
    g_assert_cmpstr (snap_names[1], ==, "snap3");
}

typedef struct
{
    int progress_done;
} RefreshAllProgressData;

static void
refresh_all_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    RefreshAllProgressData *data = user_data;
    data->progress_done++;
}

static void
test_refresh_all_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    RefreshAllProgressData refresh_all_progress_data;
    g_auto(GStrv) snap_names = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    refresh_all_progress_data.progress_done = 0;
    snap_names = snapd_client_refresh_all_sync (client, refresh_all_progress_cb, &refresh_all_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (g_strv_length (snap_names), ==, 2);
    g_assert_cmpstr (snap_names[0], ==, "snap1");
    g_assert_cmpstr (snap_names[1], ==, "snap3");
    g_assert_cmpint (refresh_all_progress_data.progress_done, >, 0);
}

static void
test_refresh_all_no_updates (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) snap_names = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    snap_names = snapd_client_refresh_all_sync (client, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (g_strv_length (snap_names), ==, 0);
}

static void
test_remove_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    result = snapd_client_remove_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
}

static void
remove_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_remove_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_no_error (error);
    g_assert_true (r);
    g_assert_null (mock_snapd_find_snap (data->snapd, "snap"));

    g_main_loop_quit (data->loop);
}

static void
test_remove_async (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    snapd_client_remove_async (client, "snap", NULL, NULL, NULL, remove_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
remove_failure_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_remove_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert_cmpstr (error->message, ==, "ERROR");
    g_assert_false (r);
    g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap"));

    g_main_loop_quit (data->loop);
}

static void
test_remove_async_failure (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_error (s, "ERROR");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    snapd_client_remove_async (client, "snap", NULL, NULL, NULL, remove_failure_cb, async_data_new (loop, snapd));
    g_main_loop_run (loop);
}

static void
remove_cancel_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    gboolean r;
    g_autoptr(AsyncData) data = user_data;
    g_autoptr(GError) error = NULL;

    r = snapd_client_remove_finish (SNAPD_CLIENT (object), result, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_false (r);
    g_assert_nonnull (mock_snapd_find_snap (data->snapd, "snap"));

    g_main_loop_quit (data->loop);
}

static void
test_remove_async_cancel (void)
{
    g_autoptr(GMainLoop) loop = NULL;
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GCancellable) cancellable = NULL;
    g_autoptr(GError) error = NULL;

    loop = g_main_loop_new (NULL, FALSE);

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    cancellable = g_cancellable_new ();
    snapd_client_remove_async (client, "snap", NULL, NULL, cancellable, remove_cancel_cb, async_data_new (loop, snapd));
    g_idle_add (cancel_cb, cancellable);
    g_main_loop_run (loop);
}

typedef struct
{
    int progress_done;
} RemoveProgressData;

static void
remove_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    RemoveProgressData *data = user_data;
    data->progress_done++;
}

static void
test_remove_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    RemoveProgressData remove_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    remove_progress_data.progress_done = 0;
    result = snapd_client_remove_sync (client, "snap", remove_progress_cb, &remove_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpint (remove_progress_data.progress_done, >, 0);
}

static void
test_remove_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_remove_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_false (result);
}

static void
test_enable_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_false (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
}

typedef struct
{
    int progress_done;
} EnableProgressData;

static void
enable_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    EnableProgressData *data = user_data;
    data->progress_done++;
}

static void
test_enable_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    EnableProgressData enable_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    enable_progress_data.progress_done = 0;
    result = snapd_client_enable_sync (client, "snap", enable_progress_cb, &enable_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_false (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
    g_assert_cmpint (enable_progress_data.progress_done, >, 0);
}

static void
test_enable_already_enabled (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_enable_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_disable_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_true (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
}

typedef struct
{
    int progress_done;
} DisableProgressData;

static void
disable_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    DisableProgressData *data = user_data;
    data->progress_done++;
}

static void
test_disable_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    DisableProgressData disable_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    disable_progress_data.progress_done = 0;
    result = snapd_client_disable_sync (client, "snap", disable_progress_cb, &disable_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_true (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
    g_assert_cmpint (disable_progress_data.progress_done, >, 0);
}

static void
test_disable_already_disabled (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_disable_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_switch_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_tracking_channel (s, "stable");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_switch_sync (client, "snap", "beta", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpstr (mock_snap_get_tracking_channel (mock_snapd_find_snap (snapd, "snap")), ==, "beta");
}

typedef struct
{
    int progress_done;
} SwitchProgressData;

static void
switch_progress_cb (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data)
{
    SwitchProgressData *data = user_data;
    data->progress_done++;
}

static void
test_switch_progress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    SwitchProgressData switch_progress_data;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_tracking_channel (s, "stable");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    switch_progress_data.progress_done = 0;
    result = snapd_client_switch_sync (client, "snap", "beta", switch_progress_cb, &switch_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpstr (mock_snap_get_tracking_channel (mock_snapd_find_snap (snapd, "snap")), ==, "beta");
    g_assert_cmpint (switch_progress_data.progress_done, >, 0);
}

static void
test_switch_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_switch_sync (client, "snap", "beta", NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert_false (result);
}

static void
test_check_buy_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
}

static void
test_check_buy_terms_not_accepted (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, FALSE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
    g_assert_false (result);
}

static void
test_check_buy_no_payment_methods (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, FALSE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
    g_assert_false (result);
}

static void
test_check_buy_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert_false (result);
}

static void
test_buy_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
}

static void
test_buy_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert_false (result);
}

static void
test_buy_not_available (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert_false (result);
}

static void
test_buy_terms_not_accepted (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, FALSE);
    mock_account_set_has_payment_methods (a, FALSE);
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
    g_assert_false (result);
}

static void
test_buy_no_payment_methods (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, FALSE);
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
    g_assert_false (result);
}

static void
test_buy_invalid_price (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    user_information = snapd_client_login2_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (user_information);
    snapd_client_set_auth_data (client, snapd_user_information_get_auth_data (user_information));

    result = snapd_client_buy_sync (client, "ABCDEF", 0.6, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_DECLINED);
    g_assert_false (result);
}

static void
test_create_user_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) info = NULL;
    gchar **ssh_keys;
    MockAccount *account;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    info = snapd_client_create_user_sync (client, "user@example.com", SNAPD_CREATE_USER_FLAGS_NONE, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "user");
    ssh_keys = snapd_user_information_get_ssh_keys (info);
    g_assert_nonnull (ssh_keys);
    g_assert_cmpint (g_strv_length (ssh_keys), ==, 2);
    g_assert_cmpstr (ssh_keys[0], ==, "KEY1");
    g_assert_cmpstr (ssh_keys[1], ==, "KEY2");
    account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_false (mock_account_get_sudoer (account));
    g_assert_false (mock_account_get_known (account));
}

static void
test_create_user_sudo (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) info = NULL;
    MockAccount *account;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    info = snapd_client_create_user_sync (client, "user@example.com", SNAPD_CREATE_USER_FLAGS_SUDO, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_true (mock_account_get_sudoer (account));
}

static void
test_create_user_known (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdUserInformation) info = NULL;
    MockAccount *account;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    info = snapd_client_create_user_sync (client, "user@example.com", SNAPD_CREATE_USER_FLAGS_KNOWN, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (info);
    account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_true (mock_account_get_known (account));
}

static void
test_create_users_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) users_info = NULL;
    SnapdUserInformation *info;
    gchar **ssh_keys;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    users_info = snapd_client_create_users_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (users_info);
    g_assert_cmpint (users_info->len, ==, 3);
    info = users_info->pdata[0];
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "admin");
    ssh_keys = snapd_user_information_get_ssh_keys (info);
    g_assert_nonnull (ssh_keys);
    g_assert_cmpint (g_strv_length (ssh_keys), ==, 2);
    g_assert_cmpstr (ssh_keys[0], ==, "KEY1");
    g_assert_cmpstr (ssh_keys[1], ==, "KEY2");
    info = users_info->pdata[1];
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "alice");
    info = users_info->pdata[2];
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "bob");
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "admin"));
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "alice"));
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "bob"));
}

static void
test_get_users_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) users_info = NULL;
    SnapdUserInformation *info;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "alice@example.com", "alice", "secret");
    mock_snapd_add_account (snapd, "bob@example.com", "bob", "secret");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    users_info = snapd_client_get_users_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (users_info);
    g_assert_cmpint (users_info->len, ==, 2);
    info = users_info->pdata[0];
    g_assert_cmpint (snapd_user_information_get_id (info), ==, 1);
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "alice");
    g_assert_cmpstr (snapd_user_information_get_email (info), ==, "alice@example.com");
    info = users_info->pdata[1];
    g_assert_cmpint (snapd_user_information_get_id (info), ==, 2);
    g_assert_cmpstr (snapd_user_information_get_username (info), ==, "bob");
    g_assert_cmpstr (snapd_user_information_get_email (info), ==, "bob@example.com");
}

static void
test_get_sections_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) sections = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_section (snapd, "SECTION1");
    mock_snapd_add_store_section (snapd, "SECTION2");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    sections = snapd_client_get_sections_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (sections);
    g_assert_cmpint (g_strv_length (sections), ==, 2);
    g_assert_cmpstr (sections[0], ==, "SECTION1");
    g_assert_cmpstr (sections[1], ==, "SECTION2");
}

static void
test_aliases_get_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) aliases = NULL;
    SnapdAlias *alias;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");

    mock_app_add_auto_alias (a, "alias1");

    mock_app_add_manual_alias (a, "alias2", TRUE);

    mock_app_add_auto_alias (a, "alias3");
    mock_app_add_manual_alias (a, "alias3", FALSE);

    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    aliases = snapd_client_get_aliases_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (aliases);
    g_assert_cmpint (aliases->len, ==, 3);
    alias = aliases->pdata[0];
    g_assert_cmpstr (snapd_alias_get_name (alias), ==, "alias1");
    g_assert_cmpstr (snapd_alias_get_snap (alias), ==, "snap");
    g_assert_cmpstr (snapd_alias_get_command (alias), ==, "snap.app");
    g_assert_cmpint (snapd_alias_get_status (alias), ==, SNAPD_ALIAS_STATUS_AUTO);
    g_assert_cmpstr (snapd_alias_get_app_auto (alias), ==, "app");
    g_assert_null (snapd_alias_get_app_manual (alias));
    alias = aliases->pdata[1];
    g_assert_cmpstr (snapd_alias_get_name (alias), ==, "alias2");
    g_assert_cmpstr (snapd_alias_get_snap (alias), ==, "snap");
    g_assert_cmpstr (snapd_alias_get_command (alias), ==, "snap.app");
    g_assert_cmpint (snapd_alias_get_status (alias), ==, SNAPD_ALIAS_STATUS_MANUAL);
    g_assert_null (snapd_alias_get_app_auto (alias));
    g_assert_cmpstr (snapd_alias_get_app_manual (alias), ==, "app");
    alias = aliases->pdata[2];
    g_assert_cmpstr (snapd_alias_get_name (alias), ==, "alias3");
    g_assert_cmpstr (snapd_alias_get_snap (alias), ==, "snap");
    g_assert_cmpstr (snapd_alias_get_command (alias), ==, "snap.app");
    g_assert_cmpint (snapd_alias_get_status (alias), ==, SNAPD_ALIAS_STATUS_DISABLED);
    g_assert_cmpstr (snapd_alias_get_app_auto (alias), ==, "app");
    g_assert_null (snapd_alias_get_app_manual (alias));
}

static void
test_aliases_get_empty (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) aliases = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    aliases = snapd_client_get_aliases_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert_nonnull (aliases);
    g_assert_cmpint (aliases->len, ==, 0);
}

static void
test_aliases_alias_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    MockApp *a;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_app_find_alias (a, "foo"));
    result = snapd_client_alias_sync (client, "snap", "app", "foo", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_nonnull (mock_app_find_alias (a, "foo"));
}

static void
test_aliases_unalias_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    MockApp *a;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");
    mock_app_add_manual_alias (a, "foo", TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_unalias_sync (client, "snap", "foo", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_app_find_alias (a, "foo"));
}

static void
test_aliases_unalias_no_snap_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    MockApp *a;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");
    mock_app_add_manual_alias (a, "foo", TRUE);
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    result = snapd_client_unalias_sync (client, NULL, "foo", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_null (mock_app_find_alias (a, "foo"));
}

static void
test_aliases_prefer_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    MockSnap *s;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    g_assert_false (mock_snap_get_preferred (s));
    result = snapd_client_prefer_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_true (mock_snap_get_preferred (s));
}

static void
test_run_snapctl_sync (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_auto(GStrv) args = NULL;
    gboolean result;
    g_autofree gchar *stdout_output = NULL;
    g_autofree gchar *stderr_output = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    args = g_strsplit ("arg1;arg2", ";", -1);
    result = snapd_client_run_snapctl_sync (client, "ABC", args, &stdout_output, &stderr_output, NULL, &error);
    g_assert_no_error (error);
    g_assert_true (result);
    g_assert_cmpstr (stdout_output, ==, "STDOUT:ABC:arg1:arg2");
    g_assert_cmpstr (stderr_output, ==, "STDERR");
}

static void
test_stress (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gint i;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, &error));

    client = snapd_client_new ();
    snapd_client_set_socket_path (client, mock_snapd_get_socket_path (snapd));

    for (i = 0; i < 10000; i++) {
        g_autoptr(SnapdSystemInformation) info = NULL;
        info = snapd_client_get_system_information_sync (client, NULL, &error);
        g_assert_no_error (error);
        g_assert_nonnull (info);
        g_assert_cmpstr (snapd_system_information_get_version (info), ==, "VERSION");
    }
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/socket-closed/before-request", test_socket_closed_before_request);
    g_test_add_func ("/socket-closed/after-request", test_socket_closed_after_request);
    g_test_add_func ("/user-agent/default", test_user_agent_default);
    g_test_add_func ("/user-agent/custom", test_user_agent_custom);
    g_test_add_func ("/user-agent/null", test_user_agent_null);
    g_test_add_func ("/accept-language/basic", test_accept_language);
    g_test_add_func ("/accept-language/empty", test_accept_language_empty);
    g_test_add_func ("/allow-interaction/basic", test_allow_interaction);
    g_test_add_func ("/get-system-information/sync", test_get_system_information_sync);
    g_test_add_func ("/get-system-information/async", test_get_system_information_async);
    g_test_add_func ("/get-system-information/store", test_get_system_information_store);
    g_test_add_func ("/get-system-information/confinement_strict", test_get_system_information_confinement_strict);
    g_test_add_func ("/get-system-information/confinement_none", test_get_system_information_confinement_none);
    g_test_add_func ("/get-system-information/confinement_unknown", test_get_system_information_confinement_unknown);
    g_test_add_func ("/login/sync", test_login_sync);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/login/legacy", test_login_legacy);
    g_test_add_func ("/list/sync", test_list_sync);
    g_test_add_func ("/list/async", test_list_async);
    g_test_add_func ("/list-one/sync", test_list_one_sync);
    g_test_add_func ("/list-one/async", test_list_one_async);
    g_test_add_func ("/list-one/optional-fields", test_list_one_optional_fields);
    g_test_add_func ("/list-one/not-installed", test_list_one_not_installed);
    g_test_add_func ("/list-one/classic-confinement", test_list_one_classic_confinement);
    g_test_add_func ("/list-one/devmode-confinement", test_list_one_devmode_confinement);
    g_test_add_func ("/list-one/daemons", test_list_one_daemons);
    g_test_add_func ("/get-apps/sync", test_get_apps_sync);
    g_test_add_func ("/get-apps/services", test_get_apps_services);
    g_test_add_func ("/icon/sync", test_icon_sync);
    g_test_add_func ("/icon/async", test_icon_async);
    g_test_add_func ("/icon/not-installed", test_icon_not_installed);
    g_test_add_func ("/icon/large", test_icon_large);
    g_test_add_func ("/get-assertions/sync", test_get_assertions_sync);
    g_test_add_func ("/get-assertions/body", test_get_assertions_body);
    g_test_add_func ("/get-assertions/multiple", test_get_assertions_multiple);
    g_test_add_func ("/get-assertions/invalid", test_get_assertions_invalid);
    g_test_add_func ("/add-assertions/sync", test_add_assertions_sync);
    g_test_add_func ("/assertions/sync", test_assertions_sync);
    g_test_add_func ("/assertions/body", test_assertions_body);
    g_test_add_func ("/get-interfaces/sync", test_get_interfaces_sync);
    g_test_add_func ("/get-interfaces/no-snaps", test_get_interfaces_no_snaps);
    g_test_add_func ("/connect-interface/sync", test_connect_interface_sync);
    g_test_add_func ("/connect-interface/progress", test_connect_interface_progress);
    g_test_add_func ("/connect-interface/invalid", test_connect_interface_invalid);
    g_test_add_func ("/disconnect-interface/sync", test_disconnect_interface_sync);
    g_test_add_func ("/disconnect-interface/progress", test_disconnect_interface_progress);
    g_test_add_func ("/disconnect-interface/invalid", test_disconnect_interface_invalid);
    g_test_add_func ("/find/query", test_find_query);
    g_test_add_func ("/find/query-private", test_find_query_private);
    g_test_add_func ("/find/query-private/not-logged-in", test_find_query_private_not_logged_in);
    g_test_add_func ("/find/name", test_find_name);
    g_test_add_func ("/find/name-private", test_find_name_private);
    g_test_add_func ("/find/name-private/not-logged-in", test_find_name_private_not_logged_in);
    g_test_add_func ("/find/channels", test_find_channels);
    g_test_add_func ("/find/cancel", test_find_cancel);
    g_test_add_func ("/find/section", test_find_section);
    g_test_add_func ("/find/section_query", test_find_section_query);
    g_test_add_func ("/find/section_name", test_find_section_name);
    g_test_add_func ("/find-refreshable/sync", test_find_refreshable_sync);
    g_test_add_func ("/find-refreshable/no-updates", test_find_refreshable_no_updates);
    g_test_add_func ("/install/sync", test_install_sync);
    g_test_add_func ("/install/sync-multiple", test_install_sync_multiple);
    g_test_add_func ("/install/async", test_install_async);
    g_test_add_func ("/install/async-multiple", test_install_async_multiple);
    g_test_add_func ("/install/async-failure", test_install_async_failure);
    g_test_add_func ("/install/async-cancel", test_install_async_cancel);
    g_test_add_func ("/install/async-multiple-cancel-first", test_install_async_multiple_cancel_first);
    g_test_add_func ("/install/async-multiple-cancel-last", test_install_async_multiple_cancel_last);
    g_test_add_func ("/install/progress", test_install_progress);
    g_test_add_func ("/install/needs-classic", test_install_needs_classic);
    g_test_add_func ("/install/classic", test_install_classic);
    g_test_add_func ("/install/needs-classic-system", test_install_needs_classic_system);
    g_test_add_func ("/install/needs-devmode", test_install_needs_devmode);
    g_test_add_func ("/install/devmode", test_install_devmode);
    g_test_add_func ("/install/dangerous", test_install_dangerous);
    g_test_add_func ("/install/jailmode", test_install_jailmode);
    g_test_add_func ("/install/channel", test_install_channel);
    g_test_add_func ("/install/revision", test_install_revision);
    g_test_add_func ("/install/not-available", test_install_not_available);
    g_test_add_func ("/install/snapd-restart", test_install_snapd_restart);
    g_test_add_func ("/install/async-snapd-restart", test_install_async_snapd_restart);
    g_test_add_func ("/install-stream/sync", test_install_stream_sync);
    g_test_add_func ("/install-stream/progress", test_install_stream_progress);
    g_test_add_func ("/install-stream/classic", test_install_stream_classic);
    g_test_add_func ("/install-stream/dangerous", test_install_stream_dangerous);
    g_test_add_func ("/install-stream/devmode", test_install_stream_devmode);
    g_test_add_func ("/install-stream/jailmode", test_install_stream_jailmode);
    g_test_add_func ("/try/sync", test_try_sync);
    g_test_add_func ("/try/progress", test_try_progress);
    g_test_add_func ("/refresh/sync", test_refresh_sync);
    g_test_add_func ("/refresh/progress", test_refresh_progress);
    g_test_add_func ("/refresh/channel", test_refresh_channel);
    g_test_add_func ("/refresh/no-updates", test_refresh_no_updates);
    g_test_add_func ("/refresh/not-installed", test_refresh_not_installed);
    g_test_add_func ("/refresh-all/sync", test_refresh_all_sync);
    g_test_add_func ("/refresh-all/progress", test_refresh_all_progress);
    g_test_add_func ("/refresh-all/no-updates", test_refresh_all_no_updates);
    g_test_add_func ("/remove/sync", test_remove_sync);
    g_test_add_func ("/remove/async", test_remove_async);
    g_test_add_func ("/remove/async-failure", test_remove_async_failure);
    g_test_add_func ("/remove/async-cancel", test_remove_async_cancel);
    g_test_add_func ("/remove/progress", test_remove_progress);
    g_test_add_func ("/remove/not-installed", test_remove_not_installed);
    g_test_add_func ("/enable/sync", test_enable_sync);
    g_test_add_func ("/enable/progress", test_enable_progress);
    g_test_add_func ("/enable/already-enabled", test_enable_already_enabled);
    g_test_add_func ("/enable/not-installed", test_enable_not_installed);
    g_test_add_func ("/disable/sync", test_disable_sync);
    g_test_add_func ("/disable/progress", test_disable_progress);
    g_test_add_func ("/disable/already-disabled", test_disable_already_disabled);
    g_test_add_func ("/disable/not-installed", test_disable_not_installed);
    g_test_add_func ("/switch/sync", test_switch_sync);
    g_test_add_func ("/switch/progress", test_switch_progress);
    g_test_add_func ("/switch/not-installed", test_switch_not_installed);
    g_test_add_func ("/check-buy/sync", test_check_buy_sync);
    g_test_add_func ("/check-buy/no-terms-not-accepted", test_check_buy_terms_not_accepted);
    g_test_add_func ("/check-buy/no-payment-methods", test_check_buy_no_payment_methods);
    g_test_add_func ("/check-buy/not-logged-in", test_check_buy_not_logged_in);
    g_test_add_func ("/buy/sync", test_buy_sync);
    g_test_add_func ("/buy/not-logged-in", test_buy_not_logged_in);
    g_test_add_func ("/buy/not-available", test_buy_not_available);
    g_test_add_func ("/buy/terms-not-accepted", test_buy_terms_not_accepted);
    g_test_add_func ("/buy/no-payment-methods", test_buy_no_payment_methods);
    g_test_add_func ("/buy/invalid-price", test_buy_invalid_price);
    g_test_add_func ("/create-user/sync", test_create_user_sync);
    g_test_add_func ("/create-user/sudo", test_create_user_sudo);
    g_test_add_func ("/create-user/known", test_create_user_known);
    g_test_add_func ("/create-users/sync", test_create_users_sync);
    g_test_add_func ("/get-users/sync", test_get_users_sync);
    g_test_add_func ("/get-sections/sync", test_get_sections_sync);
    g_test_add_func ("/aliases/get-sync", test_aliases_get_sync);
    g_test_add_func ("/aliases/get-empty", test_aliases_get_empty);
    g_test_add_func ("/aliases/alias-sync", test_aliases_alias_sync);
    g_test_add_func ("/aliases/unalias-sync", test_aliases_unalias_sync);
    g_test_add_func ("/aliases/unalias-no-snap-sync", test_aliases_unalias_no_snap_sync);
    g_test_add_func ("/aliases/prefer-sync", test_aliases_prefer_sync);
    g_test_add_func ("/run-snapctl/sync", test_run_snapctl_sync);
    g_test_add_func ("/stress/basic", test_stress);

    return g_test_run ();
}
