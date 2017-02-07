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

#include "mock-snapd.h"

static void
test_get_system_information (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    info = snapd_client_get_system_information_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert (info != NULL);
    g_assert_cmpstr (snapd_system_information_get_os_id (info), ==, "OS-ID");
    g_assert_cmpstr (snapd_system_information_get_os_version (info), ==, "OS-VERSION");
    g_assert_cmpstr (snapd_system_information_get_series (info), ==, "SERIES");
    g_assert_cmpstr (snapd_system_information_get_version (info), ==, "VERSION");
}

static void
test_login (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    int i;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    g_assert_cmpstr (snapd_auth_data_get_macaroon (auth_data), ==, a->macaroon);
    g_assert (g_strv_length (snapd_auth_data_get_discharges (auth_data)) == g_strv_length (a->discharges));
    for (i = 0; a->discharges[i]; i++)
        g_assert_cmpstr (snapd_auth_data_get_discharges (auth_data)[i], ==, a->discharges[i]);
}

static void
test_login_invalid_email (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "not-an-email", "secret", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_INVALID);
    g_assert (auth_data == NULL);
}

static void
test_login_invalid_password (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "invalid", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert (auth_data == NULL);
}

static void
test_login_otp_missing (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", "1234");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_REQUIRED);
    g_assert (auth_data == NULL);
}

static void
test_login_otp_invalid (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", "1234");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", "0000", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_INVALID);
    g_assert (auth_data == NULL);
}

static void
test_list (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_list_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 3);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap1");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[1]), ==, "snap2");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[2]), ==, "snap3");
}

static void
test_list_one (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    MockApp *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdSnap) snap = NULL;
    g_autoptr(GError) error = NULL;
    g_autoptr(GDateTime) date = NULL;
    SnapdApp *app;
    gchar **aliases;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    a = mock_snap_add_app (s, "app");
    mock_app_add_alias (a, "app2");
    mock_app_add_alias (a, "app3");  
    s->devmode = TRUE;
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    s->installed_size = 1024;
    s->trymode = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert (snap != NULL);
    g_assert_cmpint (snapd_snap_get_apps (snap)->len, ==, 1);
    app = snapd_snap_get_apps (snap)->pdata[0];
    g_assert_cmpstr (snapd_app_get_name (app), ==, "app");
    aliases = snapd_app_get_aliases (app);
    g_assert_cmpint (g_strv_length (aliases), ==, 2);
    g_assert_cmpstr (aliases[0], ==, "app2");
    g_assert_cmpstr (aliases[1], ==, "app3");
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, "CHANNEL");
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, "DESCRIPTION");
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert (snapd_snap_get_devmode (snap) == TRUE);
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    date = g_date_time_new_utc (2017, 1, 2, 11, 23, 58);
    g_assert (g_date_time_compare (snapd_snap_get_install_date (snap), date) == 0);
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 1024);
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "snap");
    g_assert_cmpint (snapd_snap_get_prices (snap)->len, ==, 0);
    g_assert (snapd_snap_get_private (snap) == FALSE);
    g_assert_cmpstr (snapd_snap_get_revision (snap), ==, "REVISION");
    g_assert_cmpint (snapd_snap_get_screenshots (snap)->len, ==, 0);
    g_assert_cmpint (snapd_snap_get_snap_type (snap), ==, SNAPD_SNAP_TYPE_APP);
    g_assert_cmpint (snapd_snap_get_status (snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
    g_assert_cmpstr (snapd_snap_get_summary (snap), ==, "SUMMARY");
    g_assert (snapd_snap_get_trymode (snap) == TRUE);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snap = snapd_client_list_one_sync (client, "snap", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert (snap == NULL);
}

static void
test_icon (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdIcon) icon = NULL;
    GBytes *data;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    icon = snapd_client_get_icon_sync (client, "snap", NULL, &error);
    g_assert_no_error (error);
    g_assert (icon != NULL);
    g_assert_cmpstr (snapd_icon_get_mime_type (icon), ==, "image/png");
    data = snapd_icon_get_data (icon);
    g_assert_cmpmem (g_bytes_get_data (data, NULL), g_bytes_get_size (data), "ICON", 4);
}

static void
test_icon_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdIcon) icon = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    icon = snapd_client_get_icon_sync (client, "snap", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert (icon == NULL);
}

static void
test_get_interfaces (void)
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
    p->connection = sl;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_get_interfaces_sync (client, &plugs, &slots, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);

    g_assert (plugs != NULL);
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

    g_assert (slots != NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_get_interfaces_sync (client, &plugs, &slots, NULL, &error);
    g_assert_no_error (error);
    g_assert (plugs != NULL);
    g_assert_cmpint (plugs->len, ==, 0);
    g_assert (slots != NULL);
    g_assert_cmpint (slots->len, ==, 0);
    g_assert (result);
}

static void
test_connect_interface (void)
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (plug->connection == slot);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    connect_interface_progress_data.progress_done = 0;
    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", connect_interface_progress_cb, &connect_interface_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (plug->connection == slot);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_connect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_disconnect_interface (void)
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
    plug->connection = slot;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (plug->connection == NULL);
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
    plug->connection = slot;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    disconnect_interface_progress_data.progress_done = 0;
    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", disconnect_interface_progress_cb, &disconnect_interface_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (plug->connection == NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_disconnect_interface_sync (client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
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
    GPtrArray *prices, *screenshots;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    mock_snapd_add_store_snap (snapd, "apple");
    mock_snapd_add_store_snap (snapd, "banana");
    mock_snapd_add_store_snap (snapd, "carrot1");
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    s->devmode = TRUE;
    s->download_size = 1024;
    mock_snap_add_price (s, 1.20, "NZD");
    mock_snap_add_price (s, 0.87, "USD");
    mock_snap_add_screenshot (s, "screenshot0.png", 0, 0);
    mock_snap_add_screenshot (s, "screenshot1.png", 1024, 1024);
    s->trymode = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_NONE, "carrot", &suggested_currency, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 2);
    g_assert_cmpstr (suggested_currency, ==, "NZD");
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "carrot1");
    snap = snaps->pdata[1];
    g_assert_cmpstr (snapd_snap_get_channel (snap), ==, "CHANNEL");
    g_assert_cmpint (snapd_snap_get_confinement (snap), ==, SNAPD_CONFINEMENT_STRICT);
    g_assert_cmpstr (snapd_snap_get_description (snap), ==, "DESCRIPTION");
    g_assert_cmpstr (snapd_snap_get_developer (snap), ==, "DEVELOPER");
    g_assert (snapd_snap_get_devmode (snap) == TRUE);
    g_assert_cmpint (snapd_snap_get_download_size (snap), ==, 1024);
    g_assert_cmpstr (snapd_snap_get_icon (snap), ==, "ICON");
    g_assert_cmpstr (snapd_snap_get_id (snap), ==, "ID");
    g_assert (snapd_snap_get_install_date (snap) == NULL);
    g_assert_cmpint (snapd_snap_get_installed_size (snap), ==, 0);
    g_assert_cmpstr (snapd_snap_get_name (snap), ==, "carrot2");
    prices = snapd_snap_get_prices (snap);
    g_assert_cmpint (prices->len, ==, 2);
    g_assert_cmpfloat (snapd_price_get_amount (prices->pdata[0]), ==, 1.20);
    g_assert_cmpstr (snapd_price_get_currency (prices->pdata[0]), ==, "NZD");
    g_assert_cmpfloat (snapd_price_get_amount (prices->pdata[1]), ==, 0.87);
    g_assert_cmpstr (snapd_price_get_currency (prices->pdata[1]), ==, "USD");
    g_assert (snapd_snap_get_private (snap) == FALSE);
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
    g_assert (snapd_snap_get_trymode (snap) == TRUE);
    g_assert_cmpstr (snapd_snap_get_version (snap), ==, "VERSION");
}

static void
test_find_query_private (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_account_add_private_snap (a, "snap2");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap2");
    g_assert (snapd_snap_get_private (snaps->pdata[0]) == TRUE);
}

static void
test_find_query_private_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert (snaps == NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap");
}

static void
test_find_name_private (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    mock_account_add_private_snap (a, "snap");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 1);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap");
    g_assert (snapd_snap_get_private (snaps->pdata[0]) == TRUE);
}

static void
test_find_name_private_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    g_autoptr(GPtrArray) snaps = NULL;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert (snaps == NULL);
}

static void
test_find_refreshable (void)
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_refreshable_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 2);
    g_assert_cmpstr (snapd_snap_get_name (snaps->pdata[0]), ==, "snap1");
    g_assert_cmpstr (snapd_snap_get_revision (snaps->pdata[1]), ==, "1");
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snaps = snapd_client_find_refreshable_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert (snaps != NULL);
    g_assert_cmpint (snaps->len, ==, 0);
}

static void
test_install (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    result = snapd_client_install_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
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
        g_assert (snapd_change_get_ready (change));
    else
        g_assert (!snapd_change_get_ready (change));
    g_assert_cmpstr (spawn_time, ==, data->spawn_time);
    if (snapd_change_get_ready (change))
        g_assert_cmpstr (ready_time, ==, data->ready_time);
    else
        g_assert (ready_time == NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_install_sync (client, "snap", NULL, install_progress_cb, &install_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert_cmpint (install_progress_data.progress_done, >, 0);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_install_sync (client, "snap", "channel2", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->channel, ==, "channel2");
}

static void
test_install_not_available (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_install_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_refresh (void)
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    refresh_progress_data.progress_done = 0;
    result = snapd_client_refresh_sync (client, "snap", NULL, refresh_progress_cb, &refresh_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_refresh_sync (client, "snap", "channel2", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->channel, ==, "channel2");
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NO_UPDATE_AVAILABLE);
    g_assert (!result);
}

static void
test_refresh_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_refresh_sync (client, "snap", NULL, NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_refresh_all (void)
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    snap_names = snapd_client_refresh_all_sync (client, NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (g_strv_length (snap_names), ==, 0);
}

static void
test_remove (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    result = snapd_client_remove_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    remove_progress_data.progress_done = 0;
    result = snapd_client_remove_sync (client, "snap", remove_progress_cb, &remove_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
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

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_remove_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert (!result);
}

static void
test_enable (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->disabled);
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
    s->disabled = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    enable_progress_data.progress_done = 0;
    result = snapd_client_enable_sync (client, "snap", enable_progress_cb, &enable_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->disabled);
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
    s->disabled = FALSE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_enable_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_enable_sync (client, "snap", NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_disable (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = FALSE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap")->disabled);
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
    s->disabled = FALSE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    disable_progress_data.progress_done = 0;
    result = snapd_client_disable_sync (client, "snap", disable_progress_cb, &disable_progress_data, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
    g_assert (mock_snapd_find_snap (snapd, "snap")->disabled);
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
    s->disabled = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_disable_not_installed (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_disable_sync (client, "snap", NULL, NULL, NULL, &error);
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
    g_assert (!result);
}

static void
test_check_buy (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
}

static void
test_check_buy_terms_not_accepted (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = FALSE;
    a->has_payment_methods = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
    g_assert (!result);
}

static void
test_check_buy_no_payment_methods (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = FALSE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
    g_assert (!result);
}

static void
test_check_buy_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    g_autoptr(SnapdClient) client = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_check_buy_sync (client, NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert (!result);
}

static void
test_buy (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_no_error (error);
    g_assert (result);
}

static void
test_buy_not_logged_in (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
    g_assert (!result);
}

static void
test_buy_not_available (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
    g_assert (!result);
}

static void
test_buy_terms_not_accepted (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = FALSE;
    a->has_payment_methods = FALSE;
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
    g_assert (!result);
}

static void
test_buy_no_payment_methods (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = FALSE;
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_buy_sync (client, "ABCDEF", 1.20, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
    g_assert (!result);
}

static void
test_buy_invalid_price (void)
{
    g_autoptr(MockSnapd) snapd = NULL;
    MockAccount *a;
    MockSnap *s;
    g_autoptr(SnapdClient) client = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    gboolean result;
    g_autoptr(GError) error = NULL;

    snapd = mock_snapd_new ();
    a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    client = snapd_client_new_from_socket (mock_snapd_get_client_socket (snapd));
    snapd_client_connect_sync (client, NULL, &error);
    g_assert_no_error (error);

    auth_data = snapd_client_login_sync (client, "test@example.com", "secret", NULL, NULL, &error);
    g_assert_no_error (error);
    g_assert (auth_data != NULL);
    snapd_client_set_auth_data (client, auth_data);

    result = snapd_client_buy_sync (client, "ABCDEF", 0.6, "NZD", NULL, &error);
    g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_DECLINED);
    g_assert (!result);
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/get-system-information/basic", test_get_system_information);
    g_test_add_func ("/login/basic", test_login);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/list/basic", test_list);
    g_test_add_func ("/list-one/basic", test_list_one);
    g_test_add_func ("/list-one/not-installed", test_list_one_not_installed);
    g_test_add_func ("/icon/basic", test_icon);
    g_test_add_func ("/icon/not-installed", test_icon_not_installed);
    g_test_add_func ("/get-interfaces/basic", test_get_interfaces);
    g_test_add_func ("/get-interfaces/no-snaps", test_get_interfaces_no_snaps);
    g_test_add_func ("/connect-interface/basic", test_connect_interface);
    g_test_add_func ("/connect-interface/progress", test_connect_interface_progress);
    g_test_add_func ("/connect-interface/invalid", test_connect_interface_invalid);
    g_test_add_func ("/disconnect-interface/basic", test_disconnect_interface);
    g_test_add_func ("/disconnect-interface/progress", test_disconnect_interface_progress);
    g_test_add_func ("/disconnect-interface/invalid", test_disconnect_interface_invalid);
    g_test_add_func ("/find/query", test_find_query);
    g_test_add_func ("/find/query-private", test_find_query_private);
    g_test_add_func ("/find/query-private/not-logged-in", test_find_query_private_not_logged_in);
    g_test_add_func ("/find/name", test_find_name);
    g_test_add_func ("/find/name-private", test_find_name_private);
    g_test_add_func ("/find/name-private/not-logged-in", test_find_name_private_not_logged_in);
    g_test_add_func ("/find-refreshable/basic", test_find_refreshable);
    g_test_add_func ("/find-refreshable/no-updates", test_find_refreshable_no_updates);
    g_test_add_func ("/install/basic", test_install);
    g_test_add_func ("/install/progress", test_install_progress);
    g_test_add_func ("/install/channel", test_install_channel);
    g_test_add_func ("/install/not-available", test_install_not_available);
    g_test_add_func ("/refresh/basic", test_refresh);
    g_test_add_func ("/refresh/progress", test_refresh_progress);
    g_test_add_func ("/refresh/channel", test_refresh_channel);
    g_test_add_func ("/refresh/no-updates", test_refresh_no_updates);
    g_test_add_func ("/refresh/not-installed", test_refresh_not_installed);
    g_test_add_func ("/refresh-all/basic", test_refresh_all);
    g_test_add_func ("/refresh-all/progress", test_refresh_all_progress);
    g_test_add_func ("/refresh-all/no-updates", test_refresh_all_no_updates);
    g_test_add_func ("/remove/basic", test_remove);
    g_test_add_func ("/remove/progress", test_remove_progress);
    g_test_add_func ("/remove/not-installed", test_remove_not_installed);
    g_test_add_func ("/enable/basic", test_enable);
    g_test_add_func ("/enable/progress", test_enable_progress);
    g_test_add_func ("/enable/already-enabled", test_enable_already_enabled);
    g_test_add_func ("/enable/not-installed", test_enable_not_installed);
    g_test_add_func ("/disable/basic", test_disable);
    g_test_add_func ("/disable/progress", test_disable_progress);
    g_test_add_func ("/disable/already-disabled", test_disable_already_disabled);
    g_test_add_func ("/disable/not-installed", test_disable_not_installed);
    g_test_add_func ("/check-buy/basic", test_check_buy);
    g_test_add_func ("/check-buy/no-terms-not-accepted", test_check_buy_terms_not_accepted);
    g_test_add_func ("/check-buy/no-payment-methods", test_check_buy_no_payment_methods);
    g_test_add_func ("/check-buy/not-logged-in", test_check_buy_not_logged_in);
    g_test_add_func ("/buy/basic", test_buy);
    g_test_add_func ("/buy/not-logged-in", test_buy_not_logged_in);
    g_test_add_func ("/buy/not-available", test_buy_not_available);
    g_test_add_func ("/buy/terms-not-accepted", test_buy_terms_not_accepted);
    g_test_add_func ("/buy/no-payment-methods", test_buy_no_payment_methods);
    g_test_add_func ("/buy/invalid-price", test_buy_invalid_price);
    //FIXMEg_test_add_func ("/create-user/basic", test_create_user);
    //FIXMEg_test_add_func ("/create-users/basic", test_create_user);

    return g_test_run ();
}
