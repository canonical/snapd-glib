/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <json-glib/json-glib.h>
#include <snapd-glib/snapd-glib.h>
#include <string.h>

#include "mock-snapd.h"

typedef struct {
  GMainLoop *loop;
  MockSnapd *snapd;
  int counter;
  gint64 id;
} AsyncData;

static AsyncData *async_data_new(GMainLoop *loop, MockSnapd *snapd) {
  AsyncData *data = g_slice_new0(AsyncData);
  data->loop = g_main_loop_ref(loop);
  data->snapd = g_object_ref(snapd);
  return data;
}

static void async_data_free(AsyncData *data) {
  g_main_loop_unref(data->loop);
  g_object_unref(data->snapd);
  g_slice_free(AsyncData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(AsyncData, async_data_free)

static void test_socket_closed_before_request(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  mock_snapd_stop(snapd);

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_null(info);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED);
}

static void test_socket_closed_after_request(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_close_on_request(snapd, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_null(info);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED);
}

static void test_socket_closed_reconnect(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_clear_object(&info);

  mock_snapd_stop(snapd);
  g_assert_true(mock_snapd_start(snapd, &error));

  info = snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_clear_object(&info);
}

static void test_socket_closed_reconnect_after_failure(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_clear_object(&info);

  mock_snapd_stop(snapd);

  info = snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_null(info);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED);
  g_clear_error(&error);

  g_assert_true(mock_snapd_start(snapd, &error));

  info = snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_clear_object(&info);
}

static void test_client_set_socket_path(void) {
  g_autoptr(SnapdClient) client = snapd_client_new();
  g_autofree gchar *default_path =
      g_strdup(snapd_client_get_socket_path(client));

  snapd_client_set_socket_path(client, "first.sock");
  g_assert_cmpstr(snapd_client_get_socket_path(client), ==, "first.sock");

  snapd_client_set_socket_path(client, "second.sock");
  g_assert_cmpstr(snapd_client_get_socket_path(client), ==, "second.sock");

  snapd_client_set_socket_path(client, NULL);
  g_assert_cmpstr(snapd_client_get_socket_path(client), ==, default_path);
}

static void test_user_agent_default(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_cmpstr(snapd_client_get_user_agent(client), ==,
                  "snapd-glib/" VERSION);

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(mock_snapd_get_last_user_agent(snapd), ==,
                  "snapd-glib/" VERSION);
}

static void test_user_agent_custom(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_set_user_agent(client, "Foo/1.0");
  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(mock_snapd_get_last_user_agent(snapd), ==, "Foo/1.0");
}

static void test_user_agent_null(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_set_user_agent(client, NULL);
  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(mock_snapd_get_last_user_agent(snapd), ==, NULL);
}

static void test_accept_language(void) {
  g_setenv("LANG", "en_US.UTF-8", TRUE);
  g_setenv("LANGUAGE", "en_US:fr", TRUE);
  g_setenv("LC_ALL", "", TRUE);
  g_setenv("LC_MESSAGES", "", TRUE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(mock_snapd_get_last_accept_language(snapd), ==,
                  "en-us, en;q=0.9, fr;q=0.8");
}

static void test_accept_language_empty(void) {
  g_setenv("LANG", "", TRUE);
  g_setenv("LANGUAGE", "", TRUE);
  g_setenv("LC_ALL", "", TRUE);
  g_setenv("LC_MESSAGES", "", TRUE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(mock_snapd_get_last_accept_language(snapd), ==, "en");
}

static void test_allow_interaction(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  /* By default, interaction is allowed */
  g_assert_true(snapd_client_get_allow_interaction(client));

  /* ... which sends the X-Allow-Interaction header with requests */
  g_autoptr(SnapdSystemInformation) info1 =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info1);
  g_assert_cmpstr(mock_snapd_get_last_allow_interaction(snapd), ==, "true");

  /* If interaction is not allowed, the header is not sent */
  snapd_client_set_allow_interaction(client, FALSE);
  g_assert_false(snapd_client_get_allow_interaction(client));
  g_autoptr(SnapdSystemInformation) info2 =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info2);
  g_assert_cmpstr(mock_snapd_get_last_allow_interaction(snapd), ==, NULL);
}

static void test_maintenance_none(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);

  SnapdMaintenance *maintenance = snapd_client_get_maintenance(client);
  g_assert_null(maintenance);
}

static void test_maintenance_daemon_restart(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_maintenance(snapd, "daemon-restart", "daemon is restarting");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);

  SnapdMaintenance *maintenance = snapd_client_get_maintenance(client);
  g_assert_nonnull(maintenance);
  g_assert_cmpint(snapd_maintenance_get_kind(maintenance), ==,
                  SNAPD_MAINTENANCE_KIND_DAEMON_RESTART);
  g_assert_cmpstr(snapd_maintenance_get_message(maintenance), ==,
                  "daemon is restarting");
}

static void test_maintenance_system_restart(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_maintenance(snapd, "system-restart", "system is restarting");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);

  SnapdMaintenance *maintenance = snapd_client_get_maintenance(client);
  g_assert_nonnull(maintenance);
  g_assert_cmpint(snapd_maintenance_get_kind(maintenance), ==,
                  SNAPD_MAINTENANCE_KIND_SYSTEM_RESTART);
  g_assert_cmpstr(snapd_maintenance_get_message(maintenance), ==,
                  "system is restarting");
}

static void test_maintenance_unknown(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_maintenance(snapd, "no-such-kind", "MESSAGE");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);

  SnapdMaintenance *maintenance = snapd_client_get_maintenance(client);
  g_assert_nonnull(maintenance);
  g_assert_cmpint(snapd_maintenance_get_kind(maintenance), ==,
                  SNAPD_MAINTENANCE_KIND_UNKNOWN);
  g_assert_cmpstr(snapd_maintenance_get_message(maintenance), ==, "MESSAGE");
}

static gboolean date_matches(GDateTime *date, int year, int month, int day,
                             int hour, int minute, int second) {
  g_autoptr(GDateTime) d =
      g_date_time_new_utc(year, month, day, hour, minute, second);
  if (date == NULL)
    return FALSE;
  return g_date_time_compare(date, d) == 0;
}

static void test_get_system_information_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_managed(snapd, TRUE);
  mock_snapd_set_on_classic(snapd, TRUE);
  mock_snapd_set_architecture(snapd, "amd64");
  mock_snapd_set_build_id(snapd, "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
  mock_snapd_add_sandbox_feature(snapd, "backend", "feature1");
  mock_snapd_add_sandbox_feature(snapd, "backend", "feature2");
  mock_snapd_set_refresh_timer(snapd, "00:00~24:00/4");
  mock_snapd_set_refresh_next(snapd, "2018-01-19T13:14:15Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(snapd_system_information_get_architecture(info), ==, "amd64");
  g_assert_cmpstr(snapd_system_information_get_build_id(info), ==,
                  "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
  g_assert_cmpint(snapd_system_information_get_confinement(info), ==,
                  SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
  g_assert_cmpstr(snapd_system_information_get_kernel_version(info), ==,
                  "KERNEL-VERSION");
  g_assert_cmpstr(snapd_system_information_get_os_id(info), ==, "OS-ID");
  g_assert_cmpstr(snapd_system_information_get_os_version(info), ==,
                  "OS-VERSION");
  g_assert_cmpstr(snapd_system_information_get_series(info), ==, "SERIES");
  g_assert_cmpstr(snapd_system_information_get_version(info), ==, "VERSION");
  g_assert_true(snapd_system_information_get_managed(info));
  g_assert_true(snapd_system_information_get_on_classic(info));
  g_assert_cmpstr(snapd_system_information_get_mount_directory(info), ==,
                  "/snap");
  g_assert_cmpstr(snapd_system_information_get_binaries_directory(info), ==,
                  "/snap/bin");
  g_assert_null(snapd_system_information_get_refresh_schedule(info));
  g_assert_cmpstr(snapd_system_information_get_refresh_timer(info), ==,
                  "00:00~24:00/4");
  g_assert_null(snapd_system_information_get_refresh_hold(info));
  g_assert_null(snapd_system_information_get_refresh_last(info));
  g_assert_true(date_matches(snapd_system_information_get_refresh_next(info),
                             2018, 1, 19, 13, 14, 15));
  g_assert_null(snapd_system_information_get_store(info));
  GHashTable *sandbox_features =
      snapd_system_information_get_sandbox_features(info);
  g_assert_nonnull(sandbox_features);
  GStrv backend_features = g_hash_table_lookup(sandbox_features, "backend");
  g_assert_nonnull(backend_features);
  g_assert_cmpint(g_strv_length(backend_features), ==, 2);
  g_assert_cmpstr(backend_features[0], ==, "feature1");
  g_assert_cmpstr(backend_features[1], ==, "feature2");
}

static void system_information_cb(GObject *object, GAsyncResult *result,
                                  gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_finish(SNAPD_CLIENT(object), result,
                                                 &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpint(snapd_system_information_get_confinement(info), ==,
                  SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
  g_assert_cmpstr(snapd_system_information_get_kernel_version(info), ==,
                  "KERNEL-VERSION");
  g_assert_cmpstr(snapd_system_information_get_os_id(info), ==, "OS-ID");
  g_assert_cmpstr(snapd_system_information_get_os_version(info), ==,
                  "OS-VERSION");
  g_assert_cmpstr(snapd_system_information_get_series(info), ==, "SERIES");
  g_assert_cmpstr(snapd_system_information_get_version(info), ==, "VERSION");
  g_assert_true(snapd_system_information_get_managed(info));
  g_assert_true(snapd_system_information_get_on_classic(info));
  g_assert_cmpstr(snapd_system_information_get_mount_directory(info), ==,
                  "/snap");
  g_assert_cmpstr(snapd_system_information_get_binaries_directory(info), ==,
                  "/snap/bin");
  g_assert_null(snapd_system_information_get_store(info));

  g_main_loop_quit(data->loop);
}

static void test_get_system_information_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_managed(snapd, TRUE);
  mock_snapd_set_on_classic(snapd, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_system_information_async(client, NULL, system_information_cb,
                                            async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_system_information_store(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_store(snapd, "store");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(snapd_system_information_get_store(info), ==, "store");
}

static void test_get_system_information_refresh(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_refresh_timer(snapd, "00:00~24:00/4");
  mock_snapd_set_refresh_hold(snapd, "2018-01-20T01:02:03Z");
  mock_snapd_set_refresh_last(snapd, "2018-01-19T01:02:03Z");
  mock_snapd_set_refresh_next(snapd, "2018-01-19T13:14:15Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_null(snapd_system_information_get_refresh_schedule(info));
  g_assert_cmpstr(snapd_system_information_get_refresh_timer(info), ==,
                  "00:00~24:00/4");
  g_assert_true(date_matches(snapd_system_information_get_refresh_hold(info),
                             2018, 1, 20, 1, 2, 3));
  g_assert_true(date_matches(snapd_system_information_get_refresh_last(info),
                             2018, 1, 19, 1, 2, 3));
  g_assert_true(date_matches(snapd_system_information_get_refresh_next(info),
                             2018, 1, 19, 13, 14, 15));
}

static void test_get_system_information_refresh_schedule(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_refresh_schedule(
      snapd, "00:00-04:59/5:00-10:59/11:00-16:59/17:00-23:59");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(snapd_system_information_get_refresh_schedule(info), ==,
                  "00:00-04:59/5:00-10:59/11:00-16:59/17:00-23:59");
  g_assert_null(snapd_system_information_get_refresh_timer(info));
}

static void test_get_system_information_confinement_strict(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_confinement(snapd, "strict");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpint(snapd_system_information_get_confinement(info), ==,
                  SNAPD_SYSTEM_CONFINEMENT_STRICT);
}

static void test_get_system_information_confinement_none(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_confinement(snapd, "partial");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpint(snapd_system_information_get_confinement(info), ==,
                  SNAPD_SYSTEM_CONFINEMENT_PARTIAL);
}

static void test_get_system_information_confinement_unknown(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_confinement(snapd, "NOT_DEFINED");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSystemInformation) info =
      snapd_client_get_system_information_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpint(snapd_system_information_get_confinement(info), ==,
                  SNAPD_SYSTEM_CONFINEMENT_UNKNOWN);
}

static void test_login_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  g_auto(GStrv) keys = g_strsplit("KEY1;KEY2", ";", -1);
  mock_account_set_ssh_keys(a, keys);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  g_assert_cmpint(snapd_user_information_get_id(user_information), ==, 1);
  g_assert_cmpstr(snapd_user_information_get_email(user_information), ==,
                  "test@example.com");
  g_assert_cmpstr(snapd_user_information_get_username(user_information), ==,
                  "test");
  GStrv ssh_keys = snapd_user_information_get_ssh_keys(user_information);
  g_assert_nonnull(ssh_keys);
  g_assert_cmpint(g_strv_length(ssh_keys), ==, 0);
  SnapdAuthData *auth_data =
      snapd_user_information_get_auth_data(user_information);
  g_assert_nonnull(auth_data);
  g_assert_cmpstr(snapd_auth_data_get_macaroon(auth_data), ==,
                  mock_account_get_macaroon(a));
  g_assert_true(g_strv_length(snapd_auth_data_get_discharges(auth_data)) ==
                g_strv_length(mock_account_get_discharges(a)));
  for (int i = 0; mock_account_get_discharges(a)[i]; i++)
    g_assert_cmpstr(snapd_auth_data_get_discharges(auth_data)[i], ==,
                    mock_account_get_discharges(a)[i]);
}

static void login_cb(GObject *object, GAsyncResult *result,
                     gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockAccount *a = mock_snapd_find_account_by_username(data->snapd, "test");

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdUserInformation) user_information =
      snapd_client_login2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  g_assert_cmpint(snapd_user_information_get_id(user_information), ==, 1);
  g_assert_cmpstr(snapd_user_information_get_email(user_information), ==,
                  "test@example.com");
  g_assert_cmpstr(snapd_user_information_get_username(user_information), ==,
                  "test");
  GStrv ssh_keys = snapd_user_information_get_ssh_keys(user_information);
  g_assert_nonnull(ssh_keys);
  g_assert_cmpint(g_strv_length(ssh_keys), ==, 0);
  SnapdAuthData *auth_data =
      snapd_user_information_get_auth_data(user_information);
  g_assert_nonnull(auth_data);
  g_assert_cmpstr(snapd_auth_data_get_macaroon(auth_data), ==,
                  mock_account_get_macaroon(a));
  g_assert_true(g_strv_length(snapd_auth_data_get_discharges(auth_data)) ==
                g_strv_length(mock_account_get_discharges(a)));
  for (int i = 0; mock_account_get_discharges(a)[i]; i++)
    g_assert_cmpstr(snapd_auth_data_get_discharges(auth_data)[i], ==,
                    mock_account_get_discharges(a)[i]);

  g_main_loop_quit(data->loop);
}

static void test_login_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  g_auto(GStrv) keys = g_strsplit("KEY1;KEY2", ";", -1);
  mock_account_set_ssh_keys(a, keys);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_login2_async(client, "test@example.com", "secret", NULL, NULL,
                            login_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_login_invalid_email(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "not-an-email", "secret", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_INVALID);
  g_assert_null(user_information);
}

static void test_login_invalid_password(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "test@example.com", "test", "secret");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "invalid", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
  g_assert_null(user_information);
}

static void test_login_otp_missing(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_otp(a, "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_REQUIRED);
  g_assert_null(user_information);
}

static void test_login_otp_invalid(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_otp(a, "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", "0000", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_TWO_FACTOR_INVALID);
  g_assert_null(user_information);
}

static void test_login_legacy(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(SnapdAuthData) auth_data = snapd_client_login_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(auth_data);
  g_assert_cmpstr(snapd_auth_data_get_macaroon(auth_data), ==,
                  mock_account_get_macaroon(a));
  g_assert_true(g_strv_length(snapd_auth_data_get_discharges(auth_data)) ==
                g_strv_length(mock_account_get_discharges(a)));
  for (int i = 0; mock_account_get_discharges(a)[i]; i++)
    g_assert_cmpstr(snapd_auth_data_get_discharges(auth_data)[i], ==,
                    mock_account_get_discharges(a)[i]);
}

static void test_logout_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "test1@example.com", "test1", "secret");
  MockAccount *a =
      mock_snapd_add_account(snapd, "test2@example.com", "test2", "secret");
  mock_snapd_add_account(snapd, "test3@example.com", "test3", "secret");
  gint64 id = mock_account_get_id(a);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdAuthData) auth_data = snapd_auth_data_new(
      mock_account_get_macaroon(a), mock_account_get_discharges(a));
  snapd_client_set_auth_data(client, auth_data);

  g_assert_nonnull(mock_snapd_find_account_by_id(snapd, id));
  gboolean result = snapd_client_logout_sync(client, id, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_account_by_id(snapd, id));
}

static void logout_cb(GObject *object, GAsyncResult *result,
                      gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_logout_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_null(mock_snapd_find_account_by_id(data->snapd, data->id));

  g_main_loop_quit(data->loop);
}

static void test_logout_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "test1@example.com", "test1", "secret");
  MockAccount *a =
      mock_snapd_add_account(snapd, "test2@example.com", "test2", "secret");
  mock_snapd_add_account(snapd, "test3@example.com", "test3", "secret");
  gint64 id = mock_account_get_id(a);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdAuthData) auth_data = snapd_auth_data_new(
      mock_account_get_macaroon(a), mock_account_get_discharges(a));
  snapd_client_set_auth_data(client, auth_data);

  g_assert_nonnull(mock_snapd_find_account_by_id(snapd, id));
  AsyncData *data = async_data_new(loop, snapd);
  data->id = id;
  snapd_client_logout_async(client, id, NULL, logout_cb, data);
  g_main_loop_run(loop);
}

static void test_logout_no_auth(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "test1@example.com", "test1", "secret");
  MockAccount *a =
      mock_snapd_add_account(snapd, "test2@example.com", "test2", "secret");
  mock_snapd_add_account(snapd, "test3@example.com", "test3", "secret");
  gint64 id = mock_account_get_id(a);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_account_by_id(snapd, id));
  gboolean result = snapd_client_logout_sync(client, id, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_false(result);
  g_assert_nonnull(mock_snapd_find_account_by_id(snapd, id));
}

static void test_get_changes_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:00:00Z");
  MockTask *t = mock_change_add_task(c, "download");
  mock_task_set_progress(t, 65535, 65535);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:00Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:10Z");
  t = mock_change_add_task(c, "install");
  mock_task_set_progress(t, 1, 1);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:10Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:30Z");
  mock_change_set_ready_time(c, "2017-01-02T11:00:30Z");

  c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:15:00Z");
  t = mock_change_add_task(c, "remove");
  mock_task_set_progress(t, 0, 1);
  mock_task_set_spawn_time(t, "2017-01-02T11:15:00Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_ALL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 2);

  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "1");
  g_assert_cmpstr(snapd_change_get_kind(changes->pdata[0]), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(changes->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(changes->pdata[0]), ==, "Done");
  g_assert_true(snapd_change_get_ready(changes->pdata[0]));
  g_assert_true(date_matches(snapd_change_get_spawn_time(changes->pdata[0]),
                             2017, 1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_change_get_ready_time(changes->pdata[0]),
                             2017, 1, 2, 11, 0, 30));
  g_assert_null(snapd_change_get_error(changes->pdata[0]));
  GPtrArray *tasks = snapd_change_get_tasks(changes->pdata[0]);
  g_assert_cmpint(tasks->len, ==, 2);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "100");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "download");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 65535);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 65535);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 10));

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[1]), ==, "101");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[1]), ==, "install");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[1]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[1]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[1]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[1]), ==, 1);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[1]), ==, 1);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[1]), 2017,
                             1, 2, 11, 0, 10));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[1]), 2017,
                             1, 2, 11, 0, 30));

  g_assert_cmpstr(snapd_change_get_id(changes->pdata[1]), ==, "2");
  g_assert_cmpstr(snapd_change_get_kind(changes->pdata[1]), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(changes->pdata[1]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(changes->pdata[1]), ==, "Do");
  g_assert_false(snapd_change_get_ready(changes->pdata[1]));
  g_assert_true(date_matches(snapd_change_get_spawn_time(changes->pdata[1]),
                             2017, 1, 2, 11, 15, 0));
  g_assert_null(snapd_change_get_ready_time(changes->pdata[1]));
  g_assert_null(snapd_change_get_error(changes->pdata[1]));
  tasks = snapd_change_get_tasks(changes->pdata[1]);
  g_assert_cmpint(tasks->len, ==, 1);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "200");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "remove");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Do");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 0);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 1);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 15, 0));
  g_assert_null(snapd_task_get_ready_time(tasks->pdata[0]));
}

static void get_changes_cb(GObject *object, GAsyncResult *result,
                           gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) changes =
      snapd_client_get_changes_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 2);

  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "1");
  g_assert_cmpstr(snapd_change_get_kind(changes->pdata[0]), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(changes->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(changes->pdata[0]), ==, "Done");
  g_assert_true(snapd_change_get_ready(changes->pdata[0]));
  g_assert_true(date_matches(snapd_change_get_spawn_time(changes->pdata[0]),
                             2017, 1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_change_get_ready_time(changes->pdata[0]),
                             2017, 1, 2, 11, 0, 30));
  g_assert_null(snapd_change_get_error(changes->pdata[0]));
  GPtrArray *tasks = snapd_change_get_tasks(changes->pdata[0]);
  g_assert_cmpint(tasks->len, ==, 2);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "100");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "download");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 65535);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 65535);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 10));

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[1]), ==, "101");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[1]), ==, "install");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[1]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[1]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[1]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[1]), ==, 1);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[1]), ==, 1);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[1]), 2017,
                             1, 2, 11, 0, 10));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[1]), 2017,
                             1, 2, 11, 0, 30));

  g_assert_cmpstr(snapd_change_get_id(changes->pdata[1]), ==, "2");
  g_assert_cmpstr(snapd_change_get_kind(changes->pdata[1]), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(changes->pdata[1]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(changes->pdata[1]), ==, "Do");
  g_assert_false(snapd_change_get_ready(changes->pdata[1]));
  g_assert_true(date_matches(snapd_change_get_spawn_time(changes->pdata[1]),
                             2017, 1, 2, 11, 15, 0));
  g_assert_null(snapd_change_get_ready_time(changes->pdata[1]));
  g_assert_null(snapd_change_get_error(changes->pdata[1]));
  tasks = snapd_change_get_tasks(changes->pdata[1]);
  g_assert_cmpint(tasks->len, ==, 1);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "200");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "remove");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Do");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 0);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 1);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 15, 0));
  g_assert_null(snapd_task_get_ready_time(tasks->pdata[0]));

  g_main_loop_quit(data->loop);
}

static void test_get_changes_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:00:00Z");
  MockTask *t = mock_change_add_task(c, "download");
  mock_task_set_progress(t, 65535, 65535);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:00Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:10Z");
  t = mock_change_add_task(c, "install");
  mock_task_set_progress(t, 1, 1);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:10Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:30Z");
  mock_change_set_ready_time(c, "2017-01-02T11:00:30Z");

  c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:15:00Z");
  t = mock_change_add_task(c, "remove");
  mock_task_set_progress(t, 0, 1);
  mock_task_set_spawn_time(t, "2017-01-02T11:15:00Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_changes_async(client, SNAPD_CHANGE_FILTER_ALL, NULL, NULL,
                                 get_changes_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_changes_filter_in_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  MockTask *t = mock_change_add_task(c, "foo");
  mock_task_set_status(t, "Done");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "foo");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "foo");
  mock_task_set_status(t, "Done");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_IN_PROGRESS, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 1);
  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "2");
}

static void test_get_changes_filter_ready(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  MockTask *t = mock_change_add_task(c, "foo");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "foo");
  mock_task_set_status(t, "Done");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "foo");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_READY, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 1);
  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "2");
}

static void test_get_changes_filter_snap(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  MockTask *t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap1");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap2");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_ALL, "snap2", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 1);
  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "2");
}

static void test_get_changes_filter_ready_snap(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  MockTask *t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap1");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap2");
  mock_task_set_status(t, "Done");

  c = mock_snapd_add_change(snapd);
  t = mock_change_add_task(c, "install");
  mock_task_set_snap_name(t, "snap2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_READY, "snap2", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 1);
  g_assert_cmpstr(snapd_change_get_id(changes->pdata[0]), ==, "2");
}

static void test_get_change_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:00:00Z");
  MockTask *t = mock_change_add_task(c, "download");
  mock_task_set_progress(t, 65535, 65535);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:00Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:10Z");
  t = mock_change_add_task(c, "install");
  mock_task_set_progress(t, 1, 1);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:10Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:30Z");
  mock_change_set_ready_time(c, "2017-01-02T11:00:30Z");

  c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:15:00Z");
  t = mock_change_add_task(c, "remove");
  mock_task_set_progress(t, 0, 1);
  mock_task_set_spawn_time(t, "2017-01-02T11:15:00Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdChange) change =
      snapd_client_get_change_sync(client, "1", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(change);

  g_assert_cmpstr(snapd_change_get_id(change), ==, "1");
  g_assert_cmpstr(snapd_change_get_kind(change), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(change), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(change), ==, "Done");
  g_assert_true(snapd_change_get_ready(change));
  g_assert_true(
      date_matches(snapd_change_get_spawn_time(change), 2017, 1, 2, 11, 0, 0));
  g_assert_true(
      date_matches(snapd_change_get_ready_time(change), 2017, 1, 2, 11, 0, 30));
  g_assert_null(snapd_change_get_error(change));
  GPtrArray *tasks = snapd_change_get_tasks(change);
  g_assert_cmpint(tasks->len, ==, 2);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "100");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "download");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 65535);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 65535);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 10));
}

static void get_change_cb(GObject *object, GAsyncResult *result,
                          gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdChange) change =
      snapd_client_get_change_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(change);

  g_assert_cmpstr(snapd_change_get_id(change), ==, "1");
  g_assert_cmpstr(snapd_change_get_kind(change), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(change), ==, "SUMMARY");
  g_assert_cmpstr(snapd_change_get_status(change), ==, "Done");
  g_assert_true(snapd_change_get_ready(change));
  g_assert_true(
      date_matches(snapd_change_get_spawn_time(change), 2017, 1, 2, 11, 0, 0));
  g_assert_true(
      date_matches(snapd_change_get_ready_time(change), 2017, 1, 2, 11, 0, 30));
  g_assert_null(snapd_change_get_error(change));
  GPtrArray *tasks = snapd_change_get_tasks(change);
  g_assert_cmpint(tasks->len, ==, 2);

  g_assert_cmpstr(snapd_task_get_id(tasks->pdata[0]), ==, "100");
  g_assert_cmpstr(snapd_task_get_kind(tasks->pdata[0]), ==, "download");
  g_assert_cmpstr(snapd_task_get_summary(tasks->pdata[0]), ==, "SUMMARY");
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Done");
  g_assert_cmpstr(snapd_task_get_progress_label(tasks->pdata[0]), ==, "LABEL");
  g_assert_cmpint(snapd_task_get_progress_done(tasks->pdata[0]), ==, 65535);
  g_assert_cmpint(snapd_task_get_progress_total(tasks->pdata[0]), ==, 65535);
  g_assert_true(date_matches(snapd_task_get_spawn_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 0));
  g_assert_true(date_matches(snapd_task_get_ready_time(tasks->pdata[0]), 2017,
                             1, 2, 11, 0, 10));

  g_main_loop_quit(data->loop);
}

static void test_get_change_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:00:00Z");
  MockTask *t = mock_change_add_task(c, "download");
  mock_task_set_progress(t, 65535, 65535);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:00Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:10Z");
  t = mock_change_add_task(c, "install");
  mock_task_set_progress(t, 1, 1);
  mock_task_set_status(t, "Done");
  mock_task_set_spawn_time(t, "2017-01-02T11:00:10Z");
  mock_task_set_ready_time(t, "2017-01-02T11:00:30Z");
  mock_change_set_ready_time(c, "2017-01-02T11:00:30Z");

  c = mock_snapd_add_change(snapd);
  mock_change_set_spawn_time(c, "2017-01-02T11:15:00Z");
  t = mock_change_add_task(c, "remove");
  mock_task_set_progress(t, 0, 1);
  mock_task_set_spawn_time(t, "2017-01-02T11:15:00Z");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_change_async(client, "1", NULL, get_change_cb,
                                async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_abort_change_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_add_task(c, "foo");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdChange) change =
      snapd_client_abort_change_sync(client, "1", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(change);
  g_assert_true(snapd_change_get_ready(change));
  g_assert_cmpstr(snapd_change_get_status(change), ==, "Error");
  g_assert_cmpstr(snapd_change_get_error(change), ==, "cancelled");
  GPtrArray *tasks = snapd_change_get_tasks(change);
  g_assert_cmpint(tasks->len, ==, 1);
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Error");
}

static void abort_change_cb(GObject *object, GAsyncResult *result,
                            gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdChange) change =
      snapd_client_abort_change_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(change);
  g_assert_true(snapd_change_get_ready(change));
  g_assert_cmpstr(snapd_change_get_status(change), ==, "Error");
  g_assert_cmpstr(snapd_change_get_error(change), ==, "cancelled");
  GPtrArray *tasks = snapd_change_get_tasks(change);
  g_assert_cmpint(tasks->len, ==, 1);
  g_assert_cmpstr(snapd_task_get_status(tasks->pdata[0]), ==, "Error");

  g_main_loop_quit(data->loop);
}

static void test_abort_change_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);
  mock_change_add_task(c, "foo");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_abort_change_async(client, "1", NULL, abort_change_cb,
                                  async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_list_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) snaps = snapd_client_list_sync(client, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 3);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap2");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[2]), ==, "snap3");
}

static void list_cb(GObject *object, GAsyncResult *result, gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) snaps =
      snapd_client_list_finish(SNAPD_CLIENT(object), result, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 3);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap2");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[2]), ==, "snap3");

  g_main_loop_quit(data->loop);
}

static void test_list_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  snapd_client_list_async(client, NULL, list_cb, async_data_new(loop, snapd));
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(loop);
}

static void test_get_snaps_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_status(s, "installed");
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_sync(
      client, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 3);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap2");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[2]), ==, "snap3");
}

static void test_get_snaps_inhibited(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_proceed_time(s, "2024-03-13T15:43:32Z");
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_sync(
      client, SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_true(date_matches(snapd_snap_get_proceed_time(snaps->pdata[0]), 2024,
                             3, 13, 15, 43, 32));
}

static void get_snaps_cb(GObject *object, GAsyncResult *result,
                         gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) snaps =
      snapd_client_get_snaps_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 3);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap2");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[2]), ==, "snap3");

  g_main_loop_quit(data->loop);
}

static void test_get_snaps_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_status(s, "installed");
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_snaps_async(client, SNAPD_GET_SNAPS_FLAGS_NONE, NULL, NULL,
                               get_snaps_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_snaps_filter(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_status(s, "installed");
  mock_snapd_add_snap(snapd, "snap1");
  mock_snapd_add_snap(snapd, "snap2");
  mock_snapd_add_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter_snaps[] = {"snap1", NULL};
  g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_sync(
      client, SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE, filter_snaps, NULL,
      &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpint(snapd_snap_get_status(snaps->pdata[0]), ==,
                  SNAPD_SNAP_STATUS_INSTALLED);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap1");
  g_assert_cmpint(snapd_snap_get_status(snaps->pdata[1]), ==,
                  SNAPD_SNAP_STATUS_ACTIVE);
}

static void test_list_one_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(SnapdSnap) snap =
      snapd_client_list_one_sync(client, "snap", NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 0);
  g_assert_cmpint(snapd_snap_get_categories(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, NULL);
  g_assert_cmpint(g_strv_length(snapd_snap_get_tracks(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_channels(snap)->len, ==, 0);
  g_assert_cmpint(g_strv_length(snapd_snap_get_common_ids(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_false(snapd_snap_get_devmode(snap));
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 0);
  g_assert_null(snapd_snap_get_hold(snap));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_null(snapd_snap_get_install_date(snap));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 0);
  g_assert_false(snapd_snap_get_jailmode(snap));
  g_assert_cmpint(snapd_snap_get_media(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  g_assert_cmpint(snapd_snap_get_prices(snap)->len, ==, 0);
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_tracking_channel(snap), ==, NULL);
  g_assert_false(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, NULL);
}

static void list_one_cb(GObject *object, GAsyncResult *result,
                        gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(SnapdSnap) snap =
      snapd_client_list_one_finish(SNAPD_CLIENT(object), result, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_base(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_broken(snap), ==, NULL);
  g_assert_cmpint(snapd_snap_get_categories(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, NULL);
  g_assert_cmpint(g_strv_length(snapd_snap_get_common_ids(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_false(snapd_snap_get_devmode(snap));
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 0);
  g_assert_null(snapd_snap_get_hold(snap));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_null(snapd_snap_get_install_date(snap));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 0);
  g_assert_false(snapd_snap_get_jailmode(snap));
  g_assert_null(snapd_snap_get_license(snap));
  g_assert_cmpint(snapd_snap_get_media(snap)->len, ==, 0);
  g_assert_null(snapd_snap_get_mounted_from(snap));
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  g_assert_cmpint(snapd_snap_get_prices(snap)->len, ==, 0);
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_tracking_channel(snap), ==, NULL);
  g_assert_false(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, NULL);

  g_main_loop_quit(data->loop);
}

static void test_list_one_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  snapd_client_list_one_async(client, "snap", NULL, list_one_cb,
                              async_data_new(loop, snapd));
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(loop);
}

static void test_get_snap_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 0);
  g_assert_cmpint(snapd_snap_get_categories(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, NULL);
  g_assert_cmpint(g_strv_length(snapd_snap_get_tracks(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_channels(snap)->len, ==, 0);
  g_assert_cmpint(g_strv_length(snapd_snap_get_common_ids(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_false(snapd_snap_get_devmode(snap));
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 0);
  g_assert_null(snapd_snap_get_hold(snap));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_null(snapd_snap_get_install_date(snap));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 0);
  g_assert_false(snapd_snap_get_jailmode(snap));
  g_assert_cmpint(snapd_snap_get_media(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  g_assert_cmpint(snapd_snap_get_prices(snap)->len, ==, 0);
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_tracking_channel(snap), ==, NULL);
  g_assert_false(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, NULL);
}

static void get_snap_cb(GObject *object, GAsyncResult *result,
                        gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_base(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_broken(snap), ==, NULL);
  g_assert_cmpint(snapd_snap_get_categories(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, NULL);
  g_assert_cmpint(g_strv_length(snapd_snap_get_common_ids(snap)), ==, 0);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_false(snapd_snap_get_devmode(snap));
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 0);
  g_assert_null(snapd_snap_get_hold(snap));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_null(snapd_snap_get_install_date(snap));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 0);
  g_assert_false(snapd_snap_get_jailmode(snap));
  g_assert_null(snapd_snap_get_license(snap));
  g_assert_cmpint(snapd_snap_get_media(snap)->len, ==, 0);
  g_assert_null(snapd_snap_get_mounted_from(snap));
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  g_assert_cmpint(snapd_snap_get_prices(snap)->len, ==, 0);
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, NULL);
  g_assert_cmpstr(snapd_snap_get_tracking_channel(snap), ==, NULL);
  g_assert_false(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, NULL);

  g_main_loop_quit(data->loop);
}

static void test_get_snap_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_snap_async(client, "snap", NULL, get_snap_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_snap_types(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "kernel");
  mock_snap_set_type(s, "kernel");
  s = mock_snapd_add_snap(snapd, "gadget");
  mock_snap_set_type(s, "gadget");
  s = mock_snapd_add_snap(snapd, "os");
  mock_snap_set_type(s, "os");
  s = mock_snapd_add_snap(snapd, "core");
  mock_snap_set_type(s, "core");
  s = mock_snapd_add_snap(snapd, "base");
  mock_snap_set_type(s, "base");
  s = mock_snapd_add_snap(snapd, "snapd");
  mock_snap_set_type(s, "snapd");
  s = mock_snapd_add_snap(snapd, "app");
  mock_snap_set_type(s, "app");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) kernel_snap =
      snapd_client_get_snap_sync(client, "kernel", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(kernel_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(kernel_snap), ==,
                  SNAPD_SNAP_TYPE_KERNEL);
  g_autoptr(SnapdSnap) gadget_snap =
      snapd_client_get_snap_sync(client, "gadget", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(gadget_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(gadget_snap), ==,
                  SNAPD_SNAP_TYPE_GADGET);
  g_autoptr(SnapdSnap) os_snap =
      snapd_client_get_snap_sync(client, "os", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(os_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(os_snap), ==, SNAPD_SNAP_TYPE_OS);
  g_autoptr(SnapdSnap) core_snap =
      snapd_client_get_snap_sync(client, "core", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(core_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(core_snap), ==,
                  SNAPD_SNAP_TYPE_CORE);
  g_autoptr(SnapdSnap) base_snap =
      snapd_client_get_snap_sync(client, "base", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(base_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(base_snap), ==,
                  SNAPD_SNAP_TYPE_BASE);
  g_autoptr(SnapdSnap) snapd_snap =
      snapd_client_get_snap_sync(client, "snapd", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snapd_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(snapd_snap), ==,
                  SNAPD_SNAP_TYPE_SNAPD);
  g_autoptr(SnapdSnap) app_snap =
      snapd_client_get_snap_sync(client, "app", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(app_snap);
  g_assert_cmpint(snapd_snap_get_snap_type(app_snap), ==, SNAPD_SNAP_TYPE_APP);
}

static void test_get_snap_optional_fields(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");
  mock_app_add_auto_alias(a, "app2");
  mock_app_add_auto_alias(a, "app3");
  mock_app_set_desktop_file(a,
                            "/var/lib/snapd/desktop/applications/app.desktop");
  mock_snap_set_base(s, "BASE");
  mock_snap_set_broken(s, "BROKEN");
  mock_snap_set_confinement(s, "classic");
  mock_snap_set_devmode(s, TRUE);
  mock_snap_set_hold(s, "2315-06-19T13:00:37Z");
  mock_snap_set_install_date(s, "2017-01-02T11:23:58Z");
  mock_snap_set_installed_size(s, 1024);
  mock_snap_set_jailmode(s, TRUE);
  mock_snap_set_trymode(s, TRUE);
  mock_snap_set_contact(s, "CONTACT");
  mock_snap_set_website(s, "WEBSITE");
  mock_snap_set_channel(s, "CHANNEL");
  mock_snap_set_description(s, "DESCRIPTION");
  mock_snap_set_license(s, "LICENSE");
  mock_snap_set_mounted_from(s, "MOUNTED-FROM");
  mock_snap_set_store_url(s, "https://snapcraft.io/snap");
  mock_snap_set_summary(s, "SUMMARY");
  mock_snap_set_tracking_channel(s, "CHANNEL");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 1);
  SnapdApp *app = snapd_snap_get_apps(snap)->pdata[0];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app");
  g_assert_null(snapd_app_get_common_id(app));
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NONE);
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_false(snapd_app_get_active(app));
  g_assert_false(snapd_app_get_enabled(app));
  g_assert_cmpstr(snapd_app_get_desktop_file(app), ==,
                  "/var/lib/snapd/desktop/applications/app.desktop");
  g_assert_cmpstr(snapd_snap_get_base(snap), ==, "BASE");
  g_assert_cmpstr(snapd_snap_get_broken(snap), ==, "BROKEN");
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, "CHANNEL");
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_CLASSIC);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, "CONTACT");
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, "DESCRIPTION");
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_true(snapd_snap_get_devmode(snap));
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 0);
  g_assert_true(
      date_matches(snapd_snap_get_hold(snap), 2315, 6, 19, 13, 00, 37));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_true(
      date_matches(snapd_snap_get_install_date(snap), 2017, 1, 2, 11, 23, 58));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 1024);
  g_assert_true(snapd_snap_get_jailmode(snap));
  g_assert_cmpstr(snapd_snap_get_license(snap), ==, "LICENSE");
  g_assert_cmpint(snapd_snap_get_media(snap)->len, ==, 0);
  g_assert_cmpstr(snapd_snap_get_mounted_from(snap), ==, "MOUNTED-FROM");
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  g_assert_cmpint(snapd_snap_get_prices(snap)->len, ==, 0);
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==,
                  "https://snapcraft.io/snap");
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, "SUMMARY");
  g_assert_cmpstr(snapd_snap_get_tracking_channel(snap), ==, "CHANNEL");
  g_assert_true(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, "WEBSITE");
}

static void test_get_snap_deprecated_fields(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpstr(snapd_snap_get_developer(snap), ==, "PUBLISHER-USERNAME");
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static void test_get_snap_common_ids(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app1");
  mock_app_set_common_id(a, "ID1");
  a = mock_snap_add_app(s, "app2");
  mock_app_set_common_id(a, "ID2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  GStrv common_ids = snapd_snap_get_common_ids(snap);
  g_assert_cmpint(g_strv_length(common_ids), ==, 2);
  g_assert_cmpstr(common_ids[0], ==, "ID1");
  g_assert_cmpstr(common_ids[1], ==, "ID2");
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 2);
  SnapdApp *app = snapd_snap_get_apps(snap)->pdata[0];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app1");
  g_assert_cmpstr(snapd_app_get_common_id(app), ==, "ID1");
  app = snapd_snap_get_apps(snap)->pdata[1];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app2");
  g_assert_cmpstr(snapd_app_get_common_id(app), ==, "ID2");
}

static void test_get_snap_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_FOUND);
  g_assert_null(snap);
}

static void test_get_snap_classic_confinement(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_confinement(s, "classic");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_CLASSIC);
}

static void test_get_snap_devmode_confinement(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_confinement(s, "devmode");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_DEVMODE);
}

static void test_get_snap_daemons(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app1");
  mock_app_set_daemon(a, "simple");
  a = mock_snap_add_app(s, "app2");
  mock_app_set_daemon(a, "forking");
  a = mock_snap_add_app(s, "app3");
  mock_app_set_daemon(a, "oneshot");
  a = mock_snap_add_app(s, "app4");
  mock_app_set_daemon(a, "notify");
  a = mock_snap_add_app(s, "app5");
  mock_app_set_daemon(a, "dbus");
  a = mock_snap_add_app(s, "app6");
  mock_app_set_daemon(a, "INVALID");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_apps(snap)->len, ==, 6);
  SnapdApp *app = snapd_snap_get_apps(snap)->pdata[0];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_SIMPLE);
  app = snapd_snap_get_apps(snap)->pdata[1];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==,
                  SNAPD_DAEMON_TYPE_FORKING);
  app = snapd_snap_get_apps(snap)->pdata[2];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==,
                  SNAPD_DAEMON_TYPE_ONESHOT);
  app = snapd_snap_get_apps(snap)->pdata[3];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NOTIFY);
  app = snapd_snap_get_apps(snap)->pdata[4];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_DBUS);
  app = snapd_snap_get_apps(snap)->pdata[5];
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==,
                  SNAPD_DAEMON_TYPE_UNKNOWN);
}

static void test_get_snap_publisher_starred(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_publisher_validation(s, "starred");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_STARRED);
}

static void test_get_snap_publisher_verified(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_publisher_validation(s, "verified");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_VERIFIED);
}

static void test_get_snap_publisher_unproven(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_publisher_validation(s, "unproven");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNPROVEN);
}

static void test_get_snap_publisher_unknown_validation(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_publisher_validation(s, "NOT-A-VALIDIATION");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdSnap) snap =
      snapd_client_get_snap_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap);
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
}

static void setup_get_snap_conf(MockSnapd *snapd) {
  MockSnap *s = mock_snapd_add_snap(snapd, "core");
  mock_snap_set_conf(s, "string-key", "\"value\"");
  mock_snap_set_conf(s, "int-key", "42");
  mock_snap_set_conf(s, "bool-key", "true");
  mock_snap_set_conf(s, "number-key", "1.25");
  mock_snap_set_conf(s, "array-key", "[ 1, \"two\", 3.25 ]");
  mock_snap_set_conf(s, "object-key", "{\"name\": \"foo\", \"value\": 42}");
}

static void check_get_snap_conf_result(GHashTable *conf) {
  g_assert_cmpint(g_hash_table_size(conf), ==, 6);
  GVariant *value = g_hash_table_lookup(conf, "string-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  value = g_hash_table_lookup(conf, "int-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  value = g_hash_table_lookup(conf, "bool-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = g_hash_table_lookup(conf, "number-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat(g_variant_get_double(value), ==, 1.25);
}

static void test_get_snap_conf_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_snap_conf(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GHashTable) conf =
      snapd_client_get_snap_conf_sync(client, "system", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(conf);

  check_get_snap_conf_result(conf);
}

static void get_snap_conf_cb(GObject *object, GAsyncResult *result,
                             gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GHashTable) conf =
      snapd_client_get_snap_conf_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(conf);

  check_get_snap_conf_result(conf);

  g_main_loop_quit(data->loop);
}

static void test_get_snap_conf_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_snap_conf(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_snap_conf_async(client, "system", NULL, NULL,
                                   get_snap_conf_cb,
                                   async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_snap_conf_key_filter(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "core");
  mock_snap_set_conf(s, "string-key", "\"value\"");
  mock_snap_set_conf(s, "int-key", "42");
  mock_snap_set_conf(s, "bool-key", "true");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter_keys[] = {"int-key", NULL};
  g_autoptr(GHashTable) conf = snapd_client_get_snap_conf_sync(
      client, "system", filter_keys, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(conf);
  g_assert_cmpint(g_hash_table_size(conf), ==, 1);
  GVariant *value = g_hash_table_lookup(conf, "int-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
}

static void test_get_snap_conf_invalid_key(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "core");
  mock_snap_set_conf(s, "string-key", "\"value\"");
  mock_snap_set_conf(s, "int-key", "42");
  mock_snap_set_conf(s, "bool-key", "true");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter_keys[] = {"invalid-key", NULL};
  g_autoptr(GHashTable) conf = snapd_client_get_snap_conf_sync(
      client, "system", filter_keys, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_OPTION_NOT_FOUND);
  g_assert_null(conf);
}

static GHashTable *setup_set_snap_conf(MockSnapd *snapd) {
  mock_snapd_add_snap(snapd, "core");

  g_autoptr(GHashTable) key_values = g_hash_table_new_full(
      g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_variant_unref);
  g_hash_table_insert(key_values, "string-key", g_variant_new_string("value"));
  g_hash_table_insert(key_values, "int-key", g_variant_new_int64(42));
  g_hash_table_insert(key_values, "bool-key", g_variant_new_boolean(TRUE));
  g_hash_table_insert(key_values, "number-key", g_variant_new_double(1.25));
  g_autoptr(GVariantBuilder) array_builder =
      g_variant_builder_new(G_VARIANT_TYPE("av"));
  g_variant_builder_add(array_builder, "v", g_variant_new_int64(1));
  g_variant_builder_add(array_builder, "v", g_variant_new_string("two"));
  g_variant_builder_add(array_builder, "v", g_variant_new_double(3.25));
  g_hash_table_insert(key_values, "array-key",
                      g_variant_builder_end(array_builder));
  g_autoptr(GVariantBuilder) object_builder =
      g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(object_builder, "{sv}", "name",
                        g_variant_new_string("foo"));
  g_variant_builder_add(object_builder, "{sv}", "value",
                        g_variant_new_int64(42));
  g_hash_table_insert(key_values, "object-key",
                      g_variant_builder_end(object_builder));

  return g_steal_pointer(&key_values);
}

static void check_set_snap_conf_result(MockSnapd *snapd) {
  MockSnap *snap = mock_snapd_find_snap(snapd, "core");
  g_assert_cmpint(mock_snap_get_conf_count(snap), ==, 6);
  g_assert_cmpstr(mock_snap_get_conf(snap, "string-key"), ==, "\"value\"");
  g_assert_cmpstr(mock_snap_get_conf(snap, "int-key"), ==, "42");
  g_assert_cmpstr(mock_snap_get_conf(snap, "bool-key"), ==, "true");
  g_assert_cmpstr(mock_snap_get_conf(snap, "number-key"), ==, "1.25");
  g_assert_cmpstr(mock_snap_get_conf(snap, "array-key"), ==,
                  "[1,\"two\",3.25]");
  g_assert_cmpstr(mock_snap_get_conf(snap, "object-key"), ==,
                  "{\"name\":\"foo\",\"value\":42}");
}

static void test_set_snap_conf_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  g_autoptr(GHashTable) key_values = setup_set_snap_conf(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean r = snapd_client_set_snap_conf_sync(client, "system", key_values,
                                               NULL, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  check_set_snap_conf_result(snapd);
}

static void set_snap_conf_cb(GObject *object, GAsyncResult *result,
                             gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_set_snap_conf_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  check_set_snap_conf_result(data->snapd);

  g_main_loop_quit(data->loop);
}

static void test_set_snap_conf_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  g_autoptr(GHashTable) key_values = setup_set_snap_conf(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_set_snap_conf_async(client, "system", key_values, NULL,
                                   set_snap_conf_cb,
                                   async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_set_snap_conf_invalid(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "core");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GHashTable) key_values = g_hash_table_new_full(
      g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_variant_unref);
  g_hash_table_insert(key_values, "string-value",
                      g_variant_new_string("value"));
  gboolean r = snapd_client_set_snap_conf_sync(client, "invalid", key_values,
                                               NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_FOUND);
  g_assert_false(r);
}

static void test_get_apps_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app1");
  a = mock_snap_add_app(s, "app2");
  mock_app_set_desktop_file(a, "foo.desktop");
  a = mock_snap_add_app(s, "app3");
  mock_app_set_daemon(a, "simple");
  mock_app_set_active(a, TRUE);
  mock_app_set_enabled(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) apps = snapd_client_get_apps2_sync(
      client, SNAPD_GET_APPS_FLAGS_NONE, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(apps);
  g_assert_cmpint(apps->len, ==, 3);
  SnapdApp *app = apps->pdata[0];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app1");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NONE);
  g_assert_false(snapd_app_get_active(app));
  g_assert_false(snapd_app_get_enabled(app));
  app = apps->pdata[1];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app2");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NONE);
  g_assert_false(snapd_app_get_active(app));
  g_assert_false(snapd_app_get_enabled(app));
  app = apps->pdata[2];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app3");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_SIMPLE);
  g_assert_true(snapd_app_get_active(app));
  g_assert_true(snapd_app_get_enabled(app));
}

static void get_apps_cb(GObject *object, GAsyncResult *result,
                        gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) apps =
      snapd_client_get_apps2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(apps);
  g_assert_cmpint(apps->len, ==, 3);
  SnapdApp *app = apps->pdata[0];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app1");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NONE);
  g_assert_false(snapd_app_get_active(app));
  g_assert_false(snapd_app_get_enabled(app));
  app = apps->pdata[1];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app2");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_NONE);
  g_assert_false(snapd_app_get_active(app));
  g_assert_false(snapd_app_get_enabled(app));
  app = apps->pdata[2];
  g_assert_cmpstr(snapd_app_get_name(app), ==, "app3");
  g_assert_cmpstr(snapd_app_get_snap(app), ==, "snap");
  g_assert_cmpint(snapd_app_get_daemon_type(app), ==, SNAPD_DAEMON_TYPE_SIMPLE);
  g_assert_true(snapd_app_get_active(app));
  g_assert_true(snapd_app_get_enabled(app));

  g_main_loop_quit(data->loop);
}

static void test_get_apps_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app1");
  a = mock_snap_add_app(s, "app2");
  mock_app_set_desktop_file(a, "foo.desktop");
  a = mock_snap_add_app(s, "app3");
  mock_app_set_daemon(a, "simple");
  mock_app_set_active(a, TRUE);
  mock_app_set_enabled(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_apps2_async(client, SNAPD_GET_APPS_FLAGS_NONE, NULL, NULL,
                               get_apps_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_apps_services(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_add_app(s, "app1");
  MockApp *a = mock_snap_add_app(s, "app2");
  mock_app_set_daemon(a, "simple");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) apps = snapd_client_get_apps2_sync(
      client, SNAPD_GET_APPS_FLAGS_SELECT_SERVICES, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(apps);
  g_assert_cmpint(apps->len, ==, 1);
  g_assert_cmpstr(snapd_app_get_name(apps->pdata[0]), ==, "app2");
}

static void test_get_apps_filter(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_app(s, "app1");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_add_app(s, "app2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter_snaps[] = {"snap1", NULL};
  g_autoptr(GPtrArray) apps = snapd_client_get_apps2_sync(
      client, SNAPD_GET_APPS_FLAGS_NONE, filter_snaps, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(apps);
  g_assert_cmpint(apps->len, ==, 1);
  g_assert_cmpstr(snapd_app_get_snap(apps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_app_get_name(apps->pdata[0]), ==, "app1");
}

static void test_icon_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  g_autoptr(GBytes) icon_data = g_bytes_new("ICON-DATA", 9);
  mock_snap_set_icon_data(s, "image/png", icon_data);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdIcon) icon =
      snapd_client_get_icon_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(icon);
  g_assert_cmpstr(snapd_icon_get_mime_type(icon), ==, "image/png");
  GBytes *data = snapd_icon_get_data(icon);
  g_assert_cmpmem(g_bytes_get_data(data, NULL), g_bytes_get_size(data),
                  "ICON-DATA", 9);
}

static void icon_cb(GObject *object, GAsyncResult *result, gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(SnapdIcon) icon =
      snapd_client_get_icon_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(icon);
  g_assert_cmpstr(snapd_icon_get_mime_type(icon), ==, "image/png");
  GBytes *icon_data = snapd_icon_get_data(icon);
  g_assert_cmpmem(g_bytes_get_data(icon_data, NULL),
                  g_bytes_get_size(icon_data), "ICON-DATA", 9);

  g_main_loop_quit(data->loop);
}

static void test_icon_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  g_autoptr(GBytes) icon_data = g_bytes_new("ICON-DATA", 9);
  mock_snap_set_icon_data(s, "image/png", icon_data);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_icon_async(client, "snap", NULL, icon_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_icon_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdIcon) icon =
      snapd_client_get_icon_sync(client, "snap", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_FOUND);
  g_assert_null(icon);
}

static void test_icon_large(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  gsize icon_buffer_length = 1048576;
  g_autofree gchar *icon_buffer = g_malloc(icon_buffer_length);
  for (gsize i = 0; i < icon_buffer_length; i++)
    icon_buffer[i] = i % 255;
  g_autoptr(GBytes) icon_data = g_bytes_new(icon_buffer, icon_buffer_length);
  mock_snap_set_icon_data(s, "image/png", icon_data);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdIcon) icon =
      snapd_client_get_icon_sync(client, "snap", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(icon);
  g_assert_cmpstr(snapd_icon_get_mime_type(icon), ==, "image/png");
  GBytes *data = snapd_icon_get_data(icon);
  g_assert_cmpmem(g_bytes_get_data(data, NULL), g_bytes_get_size(data),
                  icon_buffer, icon_buffer_length);
}

static void test_get_assertions_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_assertion(snapd, "type: account\n"
                                  "list-header:\n"
                                  "  - list-value\n"
                                  "map-header:\n"
                                  "  map-value: foo\n"
                                  "\n"
                                  "SIGNATURE");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) assertions =
      snapd_client_get_assertions_sync(client, "account", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(assertions);
  g_assert_cmpint(g_strv_length(assertions), ==, 1);
  g_assert_cmpstr(assertions[0], ==,
                  "type: account\n"
                  "list-header:\n"
                  "  - list-value\n"
                  "map-header:\n"
                  "  map-value: foo\n"
                  "\n"
                  "SIGNATURE");
}

static void test_get_assertions_body(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_assertion(snapd, "type: account\n"
                                  "body-length: 4\n"
                                  "\n"
                                  "BODY\n"
                                  "\n"
                                  "SIGNATURE");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) assertions =
      snapd_client_get_assertions_sync(client, "account", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(assertions);
  g_assert_cmpint(g_strv_length(assertions), ==, 1);
  g_assert_cmpstr(assertions[0], ==,
                  "type: account\n"
                  "body-length: 4\n"
                  "\n"
                  "BODY\n"
                  "\n"
                  "SIGNATURE");
}

static void test_get_assertions_multiple(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_assertion(snapd, "type: account\n"
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

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) assertions =
      snapd_client_get_assertions_sync(client, "account", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(assertions);
  g_assert_cmpint(g_strv_length(assertions), ==, 3);
  g_assert_cmpstr(assertions[0], ==,
                  "type: account\n"
                  "\n"
                  "SIGNATURE1");
  g_assert_cmpstr(assertions[1], ==,
                  "type: account\n"
                  "body-length: 4\n"
                  "\n"
                  "BODY\n"
                  "\n"
                  "SIGNATURE2");
  g_assert_cmpstr(assertions[2], ==,
                  "type: account\n"
                  "\n"
                  "SIGNATURE3");
}

static void test_get_assertions_invalid(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) assertions =
      snapd_client_get_assertions_sync(client, "account", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_null(assertions);
}

static void test_add_assertions_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_get_assertions(snapd));
  gchar *assertions[2];
  assertions[0] = "type: account\n"
                  "\n"
                  "SIGNATURE";
  assertions[1] = NULL;
  gboolean result =
      snapd_client_add_assertions_sync(client, assertions, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpint(g_list_length(mock_snapd_get_assertions(snapd)), ==, 1);
  g_assert_cmpstr(mock_snapd_get_assertions(snapd)->data, ==,
                  "type: account\n\nSIGNATURE");
}

static void test_assertions_sync(void) {
  g_autoptr(SnapdAssertion) assertion =
      snapd_assertion_new("type: account\n"
                          "authority-id: canonical\n"
                          "\n"
                          "SIGNATURE");
  g_auto(GStrv) headers = snapd_assertion_get_headers(assertion);
  g_assert_cmpint(g_strv_length(headers), ==, 2);
  g_assert_cmpstr(headers[0], ==, "type");
  g_autofree gchar *type = snapd_assertion_get_header(assertion, "type");
  g_assert_cmpstr(type, ==, "account");
  g_assert_cmpstr(headers[1], ==, "authority-id");
  g_autofree gchar *authority_id =
      snapd_assertion_get_header(assertion, "authority-id");
  g_assert_cmpstr(authority_id, ==, "canonical");
  g_autofree gchar *invalid_header =
      snapd_assertion_get_header(assertion, "invalid");
  g_assert_cmpstr(invalid_header, ==, NULL);
  g_autofree gchar *body = snapd_assertion_get_body(assertion);
  g_assert_cmpstr(body, ==, NULL);
  g_autofree gchar *signature = snapd_assertion_get_signature(assertion);
  g_assert_cmpstr(signature, ==, "SIGNATURE");
}

static void test_assertions_body(void) {
  g_autoptr(SnapdAssertion) assertion = snapd_assertion_new("type: account\n"
                                                            "body-length: 4\n"
                                                            "\n"
                                                            "BODY\n"
                                                            "\n"
                                                            "SIGNATURE");
  g_auto(GStrv) headers = snapd_assertion_get_headers(assertion);
  g_assert_cmpint(g_strv_length(headers), ==, 2);
  g_assert_cmpstr(headers[0], ==, "type");
  g_autofree gchar *type = snapd_assertion_get_header(assertion, "type");
  g_assert_cmpstr(type, ==, "account");
  g_assert_cmpstr(headers[1], ==, "body-length");
  g_autofree gchar *body_length =
      snapd_assertion_get_header(assertion, "body-length");
  g_assert_cmpstr(body_length, ==, "4");
  g_autofree gchar *invalid_header =
      snapd_assertion_get_header(assertion, "invalid");
  g_assert_cmpstr(invalid_header, ==, NULL);
  g_autofree gchar *body = snapd_assertion_get_body(assertion);
  g_assert_cmpstr(body, ==, "BODY");
  g_autofree gchar *signature = snapd_assertion_get_signature(assertion);
  g_assert_cmpstr(signature, ==, "SIGNATURE");
}

static void setup_get_connections(MockSnapd *snapd) {
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *snap1 = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot1 = mock_snap_add_slot(snap1, i, "slot1");
  mock_snap_add_slot(snap1, i, "slot2");
  MockSnap *snap2 = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(snap2, i, "auto-plug");
  mock_snapd_connect(snapd, plug, slot1, FALSE, FALSE);
  plug = mock_snap_add_plug(snap2, i, "manual-plug");
  mock_snapd_connect(snapd, plug, slot1, TRUE, FALSE);
  plug = mock_snap_add_plug(snap2, i, "gadget-plug");
  mock_snapd_connect(snapd, plug, slot1, FALSE, TRUE);
  plug = mock_snap_add_plug(snap2, i, "undesired-plug");
  mock_snapd_connect(snapd, plug, slot1, FALSE, FALSE);
  mock_snapd_connect(snapd, plug, NULL, TRUE, FALSE);
}

static void check_plug_no_attributes(SnapdPlug *plug) {
  guint length;
  g_auto(GStrv) attribute_names = snapd_plug_get_attribute_names(plug, &length);
  g_assert_cmpint(length, ==, 0);
  g_assert_cmpint(g_strv_length(attribute_names), ==, 0);
}

static void check_slot_no_attributes(SnapdSlot *slot) {
  guint length;
  g_auto(GStrv) attribute_names = snapd_slot_get_attribute_names(slot, &length);
  g_assert_cmpint(length, ==, 0);
  g_assert_cmpint(g_strv_length(attribute_names), ==, 0);
}

static void check_connection_no_plug_attributes(SnapdConnection *connection) {
  guint length;
  g_auto(GStrv) attribute_names =
      snapd_connection_get_plug_attribute_names(connection, &length);
  g_assert_cmpint(length, ==, 0);
  g_assert_cmpint(g_strv_length(attribute_names), ==, 0);
}

static void check_connection_no_slot_attributes(SnapdConnection *connection) {
  guint length;
  g_auto(GStrv) attribute_names =
      snapd_connection_get_slot_attribute_names(connection, &length);
  g_assert_cmpint(length, ==, 0);
  g_assert_cmpint(g_strv_length(attribute_names), ==, 0);
}

static void check_get_connections_result(GPtrArray *established,
                                         GPtrArray *undesired, GPtrArray *plugs,
                                         GPtrArray *slots,
                                         gboolean select_all) {
  g_assert_nonnull(established);
  g_assert_cmpint(established->len, ==, 3);

  SnapdConnection *connection = established->pdata[0];
  g_assert_cmpstr(snapd_connection_get_interface(connection), ==, "interface");
  SnapdSlotRef *slot_ref = snapd_connection_get_slot(connection);
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");
  SnapdPlugRef *plug_ref = snapd_connection_get_plug(connection);
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "auto-plug");
  check_connection_no_slot_attributes(connection);
  check_connection_no_plug_attributes(connection);
  g_assert_false(snapd_connection_get_manual(connection));
  g_assert_false(snapd_connection_get_gadget(connection));

  connection = established->pdata[1];
  g_assert_cmpstr(snapd_connection_get_interface(connection), ==, "interface");
  slot_ref = snapd_connection_get_slot(connection);
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");
  plug_ref = snapd_connection_get_plug(connection);
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "manual-plug");
  check_connection_no_slot_attributes(connection);
  check_connection_no_plug_attributes(connection);
  g_assert_true(snapd_connection_get_manual(connection));
  g_assert_false(snapd_connection_get_gadget(connection));

  connection = established->pdata[2];
  g_assert_cmpstr(snapd_connection_get_interface(connection), ==, "interface");
  slot_ref = snapd_connection_get_slot(connection);
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");
  plug_ref = snapd_connection_get_plug(connection);
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "gadget-plug");
  check_connection_no_slot_attributes(connection);
  check_connection_no_plug_attributes(connection);
  g_assert_false(snapd_connection_get_manual(connection));
  g_assert_true(snapd_connection_get_gadget(connection));

  g_assert_nonnull(undesired);

  if (select_all) {
    g_assert_cmpint(undesired->len, ==, 1);

    connection = undesired->pdata[0];
    g_assert_cmpstr(snapd_connection_get_interface(connection), ==,
                    "interface");
    slot_ref = snapd_connection_get_slot(connection);
    g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
    g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");
    plug_ref = snapd_connection_get_plug(connection);
    g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
    g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "undesired-plug");
    check_connection_no_slot_attributes(connection);
    check_connection_no_plug_attributes(connection);
    g_assert_true(snapd_connection_get_manual(connection));
    g_assert_false(snapd_connection_get_gadget(connection));
  } else
    g_assert_cmpint(undesired->len, ==, 0);

  g_assert_nonnull(plugs);
  if (select_all)
    g_assert_cmpint(plugs->len, ==, 4);
  else
    g_assert_cmpint(plugs->len, ==, 3);

  SnapdPlug *plug = plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_get_name(plug), ==, "auto-plug");
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap2");
  g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface");
  check_plug_no_attributes(plug);
  g_assert_cmpstr(snapd_plug_get_label(plug), ==, "LABEL");
  GPtrArray *connected_slots = snapd_plug_get_connected_slots(plug);
  g_assert_cmpint(connected_slots->len, ==, 1);
  slot_ref = connected_slots->pdata[0];
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");

  plug = plugs->pdata[1];
  g_assert_cmpstr(snapd_plug_get_name(plug), ==, "manual-plug");
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap2");
  g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface");
  check_plug_no_attributes(plug);
  g_assert_cmpstr(snapd_plug_get_label(plug), ==, "LABEL");
  connected_slots = snapd_plug_get_connected_slots(plug);
  g_assert_cmpint(connected_slots->len, ==, 1);
  slot_ref = connected_slots->pdata[0];
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");

  plug = plugs->pdata[2];
  g_assert_cmpstr(snapd_plug_get_name(plug), ==, "gadget-plug");
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap2");
  g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface");
  check_plug_no_attributes(plug);
  g_assert_cmpstr(snapd_plug_get_label(plug), ==, "LABEL");
  connected_slots = snapd_plug_get_connected_slots(plug);
  g_assert_cmpint(connected_slots->len, ==, 1);
  slot_ref = connected_slots->pdata[0];
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");

  if (select_all) {
    plug = plugs->pdata[3];
    g_assert_cmpstr(snapd_plug_get_name(plug), ==, "undesired-plug");
    g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap2");
    g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface");
    check_plug_no_attributes(plug);
    g_assert_cmpstr(snapd_plug_get_label(plug), ==, "LABEL");
    connected_slots = snapd_plug_get_connected_slots(plug);
    g_assert_cmpint(connected_slots->len, ==, 0);
  }

  g_assert_nonnull(slots);
  if (select_all)
    g_assert_cmpint(slots->len, ==, 2);
  else
    g_assert_cmpint(slots->len, ==, 1);

  SnapdSlot *slot = slots->pdata[0];
  g_assert_cmpstr(snapd_slot_get_name(slot), ==, "slot1");
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "snap1");
  g_assert_cmpstr(snapd_slot_get_interface(slot), ==, "interface");
  check_slot_no_attributes(slot);
  g_assert_cmpstr(snapd_slot_get_label(slot), ==, "LABEL");
  GPtrArray *connected_plugs = snapd_slot_get_connected_plugs(slot);
  g_assert_cmpint(connected_plugs->len, ==, 3);
  plug_ref = connected_plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "auto-plug");
  plug_ref = connected_plugs->pdata[1];
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "manual-plug");
  plug_ref = connected_plugs->pdata[2];
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "gadget-plug");

  if (select_all) {
    slot = slots->pdata[1];
    g_assert_cmpstr(snapd_slot_get_name(slot), ==, "slot2");
    g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "snap1");
    check_slot_no_attributes(slot);
    connected_plugs = snapd_slot_get_connected_plugs(slot);
    g_assert_cmpint(connected_plugs->len, ==, 0);
  }
}

static void test_get_connections_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_connections(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) established = NULL;
  g_autoptr(GPtrArray) undesired = NULL;
  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_NONE, NULL, NULL, &established,
      &undesired, &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  check_get_connections_result(established, undesired, plugs, slots, FALSE);
}

static void get_connections_cb(GObject *object, GAsyncResult *result,
                               gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GPtrArray) established = NULL;
  g_autoptr(GPtrArray) undesired = NULL;
  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_get_connections2_finish(
      SNAPD_CLIENT(object), result, &established, &undesired, &plugs, &slots,
      &error);
  g_assert_no_error(error);
  g_assert_true(r);

  check_get_connections_result(established, undesired, plugs, slots, FALSE);

  g_main_loop_quit(data->loop);
}

static void test_get_connections_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_connections(snapd);
  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_connections2_async(client, SNAPD_GET_CONNECTIONS_FLAGS_NONE,
                                      NULL, NULL, NULL, get_connections_cb,
                                      async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_connections_empty(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) established = NULL;
  g_autoptr(GPtrArray) undesired = NULL;
  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_NONE, NULL, NULL, &established,
      &undesired, &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_nonnull(established);
  g_assert_cmpint(established->len, ==, 0);

  g_assert_nonnull(undesired);
  g_assert_cmpint(undesired->len, ==, 0);

  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);

  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
}

static void test_get_connections_filter_all(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_connections(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) established = NULL;
  g_autoptr(GPtrArray) undesired = NULL;
  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_SELECT_ALL, NULL, NULL, &established,
      &undesired, &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  check_get_connections_result(established, undesired, plugs, slots, TRUE);
}

static void test_get_connections_filter_snap(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *core = mock_snapd_add_snap(snapd, "core");
  MockSlot *s = mock_snap_add_slot(core, i, "slot");
  MockSnap *snap1 = mock_snapd_add_snap(snapd, "snap1");
  MockPlug *plug1 = mock_snap_add_plug(snap1, i, "plug1");
  mock_snapd_connect(snapd, plug1, s, FALSE, FALSE);
  MockSnap *snap2 = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug2 = mock_snap_add_plug(snap2, i, "plug2");
  mock_snapd_connect(snapd, plug2, s, FALSE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_NONE, "snap1", NULL, NULL, NULL,
      &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_cmpint(plugs->len, ==, 1);
  SnapdPlug *plug = plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap1");
  g_assert_cmpint(slots->len, ==, 1);
  SnapdSlot *slot = slots->pdata[0];
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "core");
}

static void test_get_connections_filter_interface(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i1 = mock_snapd_add_interface(snapd, "interface1");
  MockInterface *i2 = mock_snapd_add_interface(snapd, "interface2");
  MockSnap *core = mock_snapd_add_snap(snapd, "core");
  MockSlot *slot1 = mock_snap_add_slot(core, i1, "slot1");
  MockSlot *slot2 = mock_snap_add_slot(core, i2, "slot2");
  MockSnap *snap = mock_snapd_add_snap(snapd, "snap");
  MockPlug *plug1 = mock_snap_add_plug(snap, i1, "plug1");
  MockPlug *plug2 = mock_snap_add_plug(snap, i2, "plug2");
  mock_snapd_connect(snapd, plug1, slot1, FALSE, FALSE);
  mock_snapd_connect(snapd, plug2, slot2, FALSE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_NONE, NULL, "interface1", NULL, NULL,
      &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_cmpint(plugs->len, ==, 1);
  SnapdPlug *plug = plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap");
  g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface1");
  g_assert_cmpint(slots->len, ==, 1);
  SnapdSlot *slot = slots->pdata[0];
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "core");
  g_assert_cmpstr(snapd_slot_get_interface(slot), ==, "interface1");
}

static gint compare_name(gconstpointer a, gconstpointer b) {
  gchar *name_a = *((gchar **)a);
  gchar *name_b = *((gchar **)b);
  return strcmp(name_a, name_b);
}

static GStrv sort_names(GStrv value) {
  g_autoptr(GPtrArray) result = g_ptr_array_new();
  for (int i = 0; value[i] != NULL; i++)
    g_ptr_array_add(result, g_strdup(value[i]));
  g_ptr_array_sort(result, compare_name);
  g_ptr_array_add(result, NULL);

  return g_steal_pointer((GStrv *)&result->pdata);
}

static void check_names_match(GStrv names, guint names_length,
                              const gchar *expected) {
  g_assert_cmpint(g_strv_length(names), ==, names_length);
  g_auto(GStrv) expected_names = g_strsplit(expected, ",", -1);
  g_auto(GStrv) sorted_expected_names = sort_names(expected_names);
  g_assert_cmpint(g_strv_length(expected_names), ==, names_length);
  g_auto(GStrv) sorted_names = sort_names(names);
  for (guint i = 0; i < names_length; i++)
    g_assert_cmpstr(sorted_names[i], ==, sorted_expected_names[i]);
}

static void test_get_connections_attributes(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *sl = mock_snap_add_slot(s, i, "slot1");
  mock_slot_add_attribute(sl, "slot-string-key", "\"value\"");
  mock_slot_add_attribute(sl, "slot-int-key", "42");
  mock_slot_add_attribute(sl, "slot-bool-key", "true");
  mock_slot_add_attribute(sl, "slot-number-key", "1.25");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *p = mock_snap_add_plug(s, i, "plug1");
  mock_plug_add_attribute(p, "plug-string-key", "\"value\"");
  mock_plug_add_attribute(p, "plug-int-key", "42");
  mock_plug_add_attribute(p, "plug-bool-key", "true");
  mock_plug_add_attribute(p, "plug-number-key", "1.25");
  mock_snapd_connect(snapd, p, sl, FALSE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) established = NULL;
  g_autoptr(GPtrArray) undesired = NULL;
  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  gboolean result = snapd_client_get_connections2_sync(
      client, SNAPD_GET_CONNECTIONS_FLAGS_NONE, NULL, NULL, &established,
      &undesired, &plugs, &slots, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_nonnull(established);
  g_assert_cmpint(established->len, ==, 1);

  SnapdConnection *connection = established->pdata[0];
  guint names_length;
  g_auto(GStrv) plug_attribute_names =
      snapd_connection_get_plug_attribute_names(connection, &names_length);
  check_names_match(
      plug_attribute_names, names_length,
      "plug-string-key,plug-int-key,plug-bool-key,plug-number-key");
  g_assert_true(
      snapd_connection_has_plug_attribute(connection, "plug-string-key"));
  GVariant *value =
      snapd_connection_get_plug_attribute(connection, "plug-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(
      snapd_connection_has_plug_attribute(connection, "plug-int-key"));
  value = snapd_connection_get_plug_attribute(connection, "plug-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(
      snapd_connection_has_plug_attribute(connection, "plug-bool-key"));
  value = snapd_connection_get_plug_attribute(connection, "plug-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = snapd_connection_get_plug_attribute(connection, "plug-number-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat(g_variant_get_double(value), ==, 1.25);
  g_assert_false(
      snapd_connection_has_plug_attribute(connection, "plug-invalid-key"));
  g_assert_null(
      snapd_connection_get_plug_attribute(connection, "plug-invalid-key"));

  g_auto(GStrv) slot_attribute_names =
      snapd_connection_get_slot_attribute_names(connection, &names_length);
  check_names_match(
      slot_attribute_names, names_length,
      "slot-string-key,slot-int-key,slot-bool-key,slot-number-key");
  g_assert_true(
      snapd_connection_has_slot_attribute(connection, "slot-string-key"));
  value = snapd_connection_get_slot_attribute(connection, "slot-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(
      snapd_connection_has_slot_attribute(connection, "slot-int-key"));
  value = snapd_connection_get_slot_attribute(connection, "slot-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(
      snapd_connection_has_slot_attribute(connection, "slot-bool-key"));
  value = snapd_connection_get_slot_attribute(connection, "slot-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  value = snapd_connection_get_slot_attribute(connection, "slot-number-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat(g_variant_get_double(value), ==, 1.25);
  g_assert_false(
      snapd_connection_has_slot_attribute(connection, "slot-invalid-key"));
  g_assert_null(
      snapd_connection_get_slot_attribute(connection, "slot-invalid-key"));

  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 1);

  SnapdPlug *plug = plugs->pdata[0];
  g_auto(GStrv) plug_names =
      snapd_plug_get_attribute_names(plug, &names_length);
  check_names_match(
      plug_names, names_length,
      "plug-string-key,plug-int-key,plug-bool-key,plug-number-key");
  g_assert_true(snapd_plug_has_attribute(plug, "plug-string-key"));
  value = snapd_plug_get_attribute(plug, "plug-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(snapd_plug_has_attribute(plug, "plug-int-key"));
  value = snapd_plug_get_attribute(plug, "plug-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(snapd_plug_has_attribute(plug, "plug-bool-key"));
  value = snapd_plug_get_attribute(plug, "plug-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = snapd_plug_get_attribute(plug, "plug-number-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat(g_variant_get_double(value), ==, 1.25);
  value = snapd_plug_get_attribute(plug, "plug-invalid-key");
  g_assert_false(snapd_plug_has_attribute(plug, "plug-invalid-key"));
  g_assert_null(snapd_plug_get_attribute(plug, "plug-invalid-key"));

  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 1);

  SnapdSlot *slot = slots->pdata[0];
  g_auto(GStrv) slot_names =
      snapd_slot_get_attribute_names(slot, &names_length);
  check_names_match(
      slot_names, names_length,
      "slot-string-key,slot-int-key,slot-bool-key,slot-number-key");
  g_assert_true(snapd_slot_has_attribute(slot, "slot-string-key"));
  value = snapd_slot_get_attribute(slot, "slot-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(snapd_slot_has_attribute(slot, "slot-int-key"));
  value = snapd_slot_get_attribute(slot, "slot-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(snapd_slot_has_attribute(slot, "slot-bool-key"));
  value = snapd_slot_get_attribute(slot, "slot-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = snapd_slot_get_attribute(slot, "slot-number-key");
  g_assert_nonnull(value);
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE));
  g_assert_cmpfloat(g_variant_get_double(value), ==, 1.25);
  value = snapd_slot_get_attribute(slot, "slot-invalid-key");
  g_assert_false(snapd_slot_has_attribute(slot, "slot-invalid-key"));
  g_assert_null(snapd_slot_get_attribute(slot, "slot-invalid-key"));
}

static void setup_get_interfaces(MockSnapd *snapd) {
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *snap1 = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot1 = mock_snap_add_slot(snap1, i, "slot1");
  mock_snap_add_slot(snap1, i, "slot2");
  MockSnap *snap2 = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug1 = mock_snap_add_plug(snap2, i, "plug1");
  mock_snapd_connect(snapd, plug1, slot1, TRUE, FALSE);
}

static void check_get_interfaces_result(GPtrArray *plugs, GPtrArray *slots) {
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 1);

  SnapdPlug *plug = plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_get_name(plug), ==, "plug1");
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap2");
  g_assert_cmpstr(snapd_plug_get_interface(plug), ==, "interface");
  check_plug_no_attributes(plug);
  g_assert_cmpstr(snapd_plug_get_label(plug), ==, "LABEL");
  GPtrArray *connected_slots = snapd_plug_get_connected_slots(plug);
  g_assert_cmpint(connected_slots->len, ==, 1);
  SnapdSlotRef *slot_ref = connected_slots->pdata[0];
  g_assert_cmpstr(snapd_slot_ref_get_snap(slot_ref), ==, "snap1");
  g_assert_cmpstr(snapd_slot_ref_get_slot(slot_ref), ==, "slot1");

  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 2);

  SnapdSlot *slot = slots->pdata[0];
  g_assert_cmpstr(snapd_slot_get_name(slot), ==, "slot1");
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "snap1");
  g_assert_cmpstr(snapd_slot_get_interface(slot), ==, "interface");
  check_slot_no_attributes(slot);
  g_assert_cmpstr(snapd_slot_get_label(slot), ==, "LABEL");
  GPtrArray *connected_plugs = snapd_slot_get_connected_plugs(slot);
  g_assert_cmpint(connected_plugs->len, ==, 1);
  SnapdPlugRef *plug_ref = connected_plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_ref_get_snap(plug_ref), ==, "snap2");
  g_assert_cmpstr(snapd_plug_ref_get_plug(plug_ref), ==, "plug1");

  slot = slots->pdata[1];
  g_assert_cmpstr(snapd_slot_get_name(slot), ==, "slot2");
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "snap1");
  check_slot_no_attributes(slot);
  connected_plugs = snapd_slot_get_connected_plugs(slot);
  g_assert_cmpint(connected_plugs->len, ==, 0);
}

static void test_get_interfaces_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_interfaces(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean result =
      snapd_client_get_interfaces_sync(client, &plugs, &slots, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_true(result);

  check_get_interfaces_result(plugs, slots);
}

static void get_interfaces_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  g_autoptr(GError) error = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean r = snapd_client_get_interfaces_finish(SNAPD_CLIENT(object), result,
                                                  &plugs, &slots, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS

  g_assert_no_error(error);
  g_assert_true(r);

  check_get_interfaces_result(plugs, slots);

  g_main_loop_quit(data->loop);
}

static void test_get_interfaces_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  setup_get_interfaces(snapd);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  snapd_client_get_interfaces_async(client, NULL, get_interfaces_cb,
                                    async_data_new(loop, snapd));
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(loop);
}

static void test_get_interfaces_no_snaps(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean result =
      snapd_client_get_interfaces_sync(client, &plugs, &slots, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
  g_assert_true(result);
}

static void test_get_interfaces_attributes(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *sl = mock_snap_add_slot(s, i, "slot1");
  mock_slot_add_attribute(sl, "slot-string-key", "\"value\"");
  mock_slot_add_attribute(sl, "slot-int-key", "42");
  mock_slot_add_attribute(sl, "slot-bool-key", "true");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *p = mock_snap_add_plug(s, i, "plug1");
  mock_plug_add_attribute(p, "plug-string-key", "\"value\"");
  mock_plug_add_attribute(p, "plug-int-key", "42");
  mock_plug_add_attribute(p, "plug-bool-key", "true");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean result =
      snapd_client_get_interfaces_sync(client, &plugs, &slots, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 1);

  SnapdPlug *plug = plugs->pdata[0];
  guint names_length;
  g_auto(GStrv) plug_names =
      snapd_plug_get_attribute_names(plug, &names_length);
  check_names_match(plug_names, names_length,
                    "plug-string-key,plug-int-key,plug-bool-key");
  g_assert_true(snapd_plug_has_attribute(plug, "plug-string-key"));
  GVariant *value = snapd_plug_get_attribute(plug, "plug-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(snapd_plug_has_attribute(plug, "plug-int-key"));
  value = snapd_plug_get_attribute(plug, "plug-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(snapd_plug_has_attribute(plug, "plug-bool-key"));
  value = snapd_plug_get_attribute(plug, "plug-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = snapd_plug_get_attribute(plug, "plug-invalid-key");
  g_assert_false(snapd_plug_has_attribute(plug, "plug-invalid-key"));
  g_assert_null(snapd_plug_get_attribute(plug, "plug-invalid-key"));

  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 1);

  SnapdSlot *slot = slots->pdata[0];
  g_auto(GStrv) slot_names =
      snapd_slot_get_attribute_names(slot, &names_length);
  check_names_match(slot_names, names_length,
                    "slot-string-key,slot-int-key,slot-bool-key");
  g_assert_true(snapd_slot_has_attribute(slot, "slot-string-key"));
  value = snapd_slot_get_attribute(slot, "slot-string-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_STRING));
  g_assert_cmpstr(g_variant_get_string(value, NULL), ==, "value");
  g_assert_true(snapd_slot_has_attribute(slot, "slot-int-key"));
  value = snapd_slot_get_attribute(slot, "slot-int-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_INT64));
  g_assert_cmpint(g_variant_get_int64(value), ==, 42);
  g_assert_true(snapd_slot_has_attribute(slot, "slot-bool-key"));
  value = snapd_slot_get_attribute(slot, "slot-bool-key");
  g_assert_true(g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN));
  g_assert_true(g_variant_get_boolean(value));
  value = snapd_slot_get_attribute(slot, "slot-invalid-key");
  g_assert_false(snapd_slot_has_attribute(slot, "slot-invalid-key"));
  g_assert_null(snapd_slot_get_attribute(slot, "slot-invalid-key"));
}

static void test_get_interfaces_legacy(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *sl = mock_snap_add_slot(s, i, "slot1");
  mock_snap_add_slot(s, i, "slot2");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *p = mock_snap_add_plug(s, i, "plug1");
  mock_snapd_connect(snapd, p, sl, TRUE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) plugs = NULL;
  g_autoptr(GPtrArray) slots = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean result =
      snapd_client_get_interfaces_sync(client, &plugs, &slots, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_true(result);

  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 1);

  SnapdPlug *plug = plugs->pdata[0];
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GPtrArray *connections = snapd_plug_get_connections(plug);
  g_assert_cmpint(connections->len, ==, 1);
  SnapdConnection *connection = connections->pdata[0];
  g_assert_cmpstr(snapd_connection_get_snap(connection), ==, "snap1");
  g_assert_cmpstr(snapd_connection_get_name(connection), ==, "slot1");
  G_GNUC_END_IGNORE_DEPRECATIONS

  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 2);

  SnapdSlot *slot = slots->pdata[0];
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  connections = snapd_slot_get_connections(slot);
  g_assert_cmpint(connections->len, ==, 1);
  connection = connections->pdata[0];
  g_assert_cmpstr(snapd_connection_get_snap(connection), ==, "snap2");
  g_assert_cmpstr(snapd_connection_get_name(connection), ==, "plug1");
  G_GNUC_END_IGNORE_DEPRECATIONS

  slot = slots->pdata[1];
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  connections = snapd_slot_get_connections(slot);
  g_assert_cmpint(connections->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static void test_get_interfaces2_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i1 = mock_snapd_add_interface(snapd, "interface1");
  mock_interface_set_summary(i1, "summary1");
  mock_interface_set_doc_url(i1, "url1");
  MockInterface *i2 = mock_snapd_add_interface(snapd, "interface2");
  mock_interface_set_summary(i2, "summary2");
  mock_interface_set_doc_url(i2, "url2");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_plug(s, i1, "plug1");
  mock_snap_add_slot(s, i2, "slot1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) ifaces = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_NONE, NULL, NULL, &error);
  g_assert_no_error(error);

  g_assert_nonnull(ifaces);
  g_assert_cmpint(ifaces->len, ==, 2);

  SnapdInterface *iface = ifaces->pdata[0];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface1");
  g_assert_cmpstr(snapd_interface_get_summary(iface), ==, "summary1");
  g_assert_cmpstr(snapd_interface_get_doc_url(iface), ==, "url1");
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);

  iface = ifaces->pdata[1];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface2");
  g_assert_cmpstr(snapd_interface_get_summary(iface), ==, "summary2");
  g_assert_cmpstr(snapd_interface_get_doc_url(iface), ==, "url2");
  plugs = snapd_interface_get_plugs(iface);
  slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
}

static void get_interfaces2_cb(GObject *object, GAsyncResult *result,
                               gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) ifaces =
      snapd_client_get_interfaces2_finish(SNAPD_CLIENT(object), result, &error);

  g_assert_no_error(error);
  g_assert_nonnull(ifaces);

  g_assert_cmpint(ifaces->len, ==, 2);

  SnapdInterface *iface = ifaces->pdata[0];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface1");
  g_assert_cmpstr(snapd_interface_get_summary(iface), ==, "summary1");
  g_assert_cmpstr(snapd_interface_get_doc_url(iface), ==, "url1");
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);

  iface = ifaces->pdata[1];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface2");
  g_assert_cmpstr(snapd_interface_get_summary(iface), ==, "summary2");
  g_assert_cmpstr(snapd_interface_get_doc_url(iface), ==, "url2");
  plugs = snapd_interface_get_plugs(iface);
  slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);

  g_main_loop_quit(data->loop);
}

static void test_get_interfaces2_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i1 = mock_snapd_add_interface(snapd, "interface1");
  mock_interface_set_summary(i1, "summary1");
  mock_interface_set_doc_url(i1, "url1");
  MockInterface *i2 = mock_snapd_add_interface(snapd, "interface2");
  mock_interface_set_summary(i2, "summary2");
  mock_interface_set_doc_url(i2, "url2");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_plug(s, i1, "plug1");
  mock_snap_add_slot(s, i2, "slot1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_interfaces2_async(client, SNAPD_GET_INTERFACES_FLAGS_NONE,
                                     NULL, NULL, get_interfaces2_cb,
                                     async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_interfaces2_only_connected(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface1");
  mock_snapd_add_interface(snapd, "interface2");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *sl = mock_snap_add_slot(s, i, "slot1");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *p = mock_snap_add_plug(s, i, "plug2");
  mock_snapd_connect(snapd, p, sl, TRUE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) ifaces = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_ONLY_CONNECTED, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces);

  g_assert_cmpint(ifaces->len, ==, 1);

  SnapdInterface *iface = ifaces->pdata[0];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface1");
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
}

static void test_get_interfaces2_slots(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_slot(s, i, "slot1");
  mock_snap_add_plug(s, i, "plug1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) ifaces = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_INCLUDE_SLOTS, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces);

  g_assert_cmpint(ifaces->len, ==, 1);

  SnapdInterface *iface = ifaces->pdata[0];
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 1);
  SnapdSlot *slot = slots->pdata[0];
  g_assert_cmpstr(snapd_slot_get_name(slot), ==, "slot1");
  g_assert_cmpstr(snapd_slot_get_snap(slot), ==, "snap1");
}

static void test_get_interfaces2_plugs(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_slot(s, i, "slot1");
  mock_snap_add_plug(s, i, "plug1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) ifaces = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_INCLUDE_PLUGS, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces);

  g_assert_cmpint(ifaces->len, ==, 1);

  SnapdInterface *iface = ifaces->pdata[0];
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 1);
  SnapdPlug *plug = plugs->pdata[0];
  g_assert_cmpstr(snapd_plug_get_name(plug), ==, "plug1");
  g_assert_cmpstr(snapd_plug_get_snap(plug), ==, "snap1");
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
}

static void test_get_interfaces2_filter(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_interface(snapd, "interface1");
  mock_snapd_add_interface(snapd, "interface2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter_names[] = {"interface2", NULL};
  g_autoptr(GPtrArray) ifaces = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_NONE, filter_names, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces);

  g_assert_cmpint(ifaces->len, ==, 1);

  SnapdInterface *iface = ifaces->pdata[0];
  g_assert_cmpstr(snapd_interface_get_name(iface), ==, "interface2");
  GPtrArray *plugs = snapd_interface_get_plugs(iface);
  g_assert_nonnull(plugs);
  g_assert_cmpint(plugs->len, ==, 0);
  GPtrArray *slots = snapd_interface_get_slots(iface);
  g_assert_nonnull(slots);
  g_assert_cmpint(slots->len, ==, 0);
}

static void test_get_interfaces2_make_label(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_interface(snapd, "camera");
  MockInterface *i =
      mock_snapd_add_interface(snapd, "interface-without-translation");
  mock_interface_set_summary(i, "SUMMARY");
  mock_snapd_add_interface(snapd, "interface-without-summary");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gchar *filter1[] = {"camera", NULL};
  g_autoptr(GPtrArray) ifaces1 = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_NONE, filter1, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces1);
  g_assert_cmpint(ifaces1->len, ==, 1);
  SnapdInterface *iface1 = ifaces1->pdata[0];
  g_autofree gchar *label1 = snapd_interface_make_label(iface1);
  g_assert_cmpstr(label1, ==,
                  "Use your camera"); // FIXME: Won't work if translated

  gchar *filter2[] = {"interface-without-translation", NULL};
  g_autoptr(GPtrArray) ifaces2 = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_NONE, filter2, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces2);
  g_assert_cmpint(ifaces2->len, ==, 1);
  SnapdInterface *iface2 = ifaces2->pdata[0];
  g_autofree gchar *label2 = snapd_interface_make_label(iface2);
  g_assert_cmpstr(label2, ==, "interface-without-translation: SUMMARY");

  gchar *filter3[] = {"interface-without-summary", NULL};
  g_autoptr(GPtrArray) ifaces3 = snapd_client_get_interfaces2_sync(
      client, SNAPD_GET_INTERFACES_FLAGS_NONE, filter3, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(ifaces3);
  g_assert_cmpint(ifaces3->len, ==, 1);
  SnapdInterface *iface3 = ifaces3->pdata[0];
  g_autofree gchar *label3 = snapd_interface_make_label(iface3);
  g_assert_cmpstr(label3, ==, "interface-without-summary");
}

static void test_connect_interface_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot = mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(s, i, "plug");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_connect_interface_sync(
      client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_true(mock_snapd_find_plug_connection(snapd, plug) == slot);
}

static void connect_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockSnap *s = mock_snapd_find_snap(data->snapd, "snap1");
  MockSlot *slot = mock_snap_find_slot(s, "slot");
  s = mock_snapd_find_snap(data->snapd, "snap2");
  MockPlug *plug = mock_snap_find_plug(s, "plug");

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_connect_interface_finish(SNAPD_CLIENT(object),
                                                     result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_true(mock_snapd_find_plug_connection(data->snapd, plug) == slot);

  g_main_loop_quit(data->loop);
}

static void test_connect_interface_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_add_plug(s, i, "plug");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_connect_interface_async(client, "snap2", "plug", "snap1", "slot",
                                       NULL, NULL, NULL, connect_cb,
                                       async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} ConnectInterfaceProgressData;

static void connect_interface_progress_cb(SnapdClient *client,
                                          SnapdChange *change,
                                          gpointer deprecated,
                                          gpointer user_data) {
  ConnectInterfaceProgressData *data = user_data;
  data->progress_done++;
}

static void test_connect_interface_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot = mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(s, i, "plug");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  ConnectInterfaceProgressData connect_interface_progress_data;
  connect_interface_progress_data.progress_done = 0;
  gboolean result = snapd_client_connect_interface_sync(
      client, "snap2", "plug", "snap1", "slot", connect_interface_progress_cb,
      &connect_interface_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_true(mock_snapd_find_plug_connection(snapd, plug) == slot);
  g_assert_cmpint(connect_interface_progress_data.progress_done, >, 0);
}

static void test_connect_interface_invalid(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_connect_interface_sync(
      client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_false(result);
}

static void test_disconnect_interface_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot = mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(s, i, "plug");
  mock_snapd_connect(snapd, plug, slot, TRUE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_disconnect_interface_sync(
      client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_plug_connection(snapd, plug));
}

static void disconnect_cb(GObject *object, GAsyncResult *result,
                          gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockSnap *s = mock_snapd_find_snap(data->snapd, "snap2");
  MockPlug *plug = mock_snap_find_plug(s, "plug");

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_connect_interface_finish(SNAPD_CLIENT(object),
                                                     result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_null(mock_snapd_find_plug_connection(data->snapd, plug));

  g_main_loop_quit(data->loop);
}

static void test_disconnect_interface_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot = mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(s, i, "plug");
  mock_snapd_connect(snapd, plug, slot, TRUE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_disconnect_interface_async(
      client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, disconnect_cb,
      async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} DisconnectInterfaceProgressData;

static void disconnect_interface_progress_cb(SnapdClient *client,
                                             SnapdChange *change,
                                             gpointer deprecated,
                                             gpointer user_data) {
  DisconnectInterfaceProgressData *data = user_data;
  data->progress_done++;
}

static void test_disconnect_interface_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockInterface *i = mock_snapd_add_interface(snapd, "interface");
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  MockSlot *slot = mock_snap_add_slot(s, i, "slot");
  s = mock_snapd_add_snap(snapd, "snap2");
  MockPlug *plug = mock_snap_add_plug(s, i, "plug");
  mock_snapd_connect(snapd, plug, slot, TRUE, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  DisconnectInterfaceProgressData disconnect_interface_progress_data;
  disconnect_interface_progress_data.progress_done = 0;
  gboolean result = snapd_client_disconnect_interface_sync(
      client, "snap2", "plug", "snap1", "slot",
      disconnect_interface_progress_cb, &disconnect_interface_progress_data,
      NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_plug_connection(snapd, plug));
  g_assert_cmpint(disconnect_interface_progress_data.progress_done, >, 0);
}

static void test_disconnect_interface_invalid(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_disconnect_interface_sync(
      client, "snap2", "plug", "snap1", "slot", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_false(result);
}

static void test_find_query(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_suggested_currency(snapd, "NZD");
  mock_snapd_add_store_snap(snapd, "apple");
  mock_snapd_add_store_snap(snapd, "banana");
  MockSnap *s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_track_add_channel(mock_snap_add_track(s, "latest"), "stable", NULL);
  s = mock_snapd_add_store_snap(snapd, "carrot2");
  mock_track_add_channel(mock_snap_add_track(s, "latest"), "stable", NULL);
  mock_snap_set_channel(s, "CHANNEL");
  mock_snap_set_contact(s, "CONTACT");
  mock_snap_set_website(s, "WEBSITE");
  mock_snap_set_description(s, "DESCRIPTION");
  mock_snap_set_store_url(s, "https://snapcraft.io/snap");
  mock_snap_set_summary(s, "SUMMARY");
  mock_snap_set_download_size(s, 1024);
  mock_snap_add_price(s, 1.25, "NZD");
  mock_snap_add_price(s, 0.75, "USD");
  mock_snap_add_media(s, "screenshot", "screenshot0.png", 0, 0);
  mock_snap_add_media(s, "screenshot", "screenshot1.png", 1024, 1024);
  mock_snap_add_media(s, "banner", "banner.png", 0, 0);
  mock_snap_set_trymode(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autofree gchar *suggested_currency = NULL;
  g_autoptr(GPtrArray) snaps =
      snapd_client_find_sync(client, SNAPD_FIND_FLAGS_NONE, "carrot",
                             &suggested_currency, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(suggested_currency, ==, "NZD");
  SnapdSnap *snap = snaps->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "carrot1");
  g_assert_null(snapd_snap_get_channel(snap));
  GStrv tracks = snapd_snap_get_tracks(snap);
  g_assert_cmpint(g_strv_length(tracks), ==, 1);
  g_assert_cmpstr(tracks[0], ==, "latest");
  GPtrArray *channels = snapd_snap_get_channels(snap);
  g_assert_cmpint(channels->len, ==, 1);
  SnapdChannel *channel = channels->pdata[0];
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  g_assert_cmpint(snapd_channel_get_confinement(channel), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_channel_get_revision(channel), ==, "REVISION");
  g_assert_cmpstr(snapd_channel_get_version(channel), ==, "VERSION");
  g_assert_cmpstr(snapd_channel_get_epoch(channel), ==, "0");
  g_assert_cmpint(snapd_channel_get_size(channel), ==, 65535);
  g_assert_null(snapd_snap_get_contact(snap));
  g_assert_null(snapd_snap_get_description(snap));
  g_assert_null(snapd_snap_get_store_url(snap));
  g_assert_null(snapd_snap_get_summary(snap));
  snap = snaps->pdata[1];
  g_assert_cmpstr(snapd_snap_get_channel(snap), ==, "CHANNEL");
  g_assert_cmpint(snapd_snap_get_confinement(snap), ==,
                  SNAPD_CONFINEMENT_STRICT);
  g_assert_cmpstr(snapd_snap_get_contact(snap), ==, "CONTACT");
  g_assert_cmpstr(snapd_snap_get_description(snap), ==, "DESCRIPTION");
  g_assert_cmpstr(snapd_snap_get_publisher_display_name(snap), ==,
                  "PUBLISHER-DISPLAY-NAME");
  g_assert_cmpstr(snapd_snap_get_publisher_id(snap), ==, "PUBLISHER-ID");
  g_assert_cmpstr(snapd_snap_get_publisher_username(snap), ==,
                  "PUBLISHER-USERNAME");
  g_assert_cmpint(snapd_snap_get_publisher_validation(snap), ==,
                  SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
  g_assert_cmpint(snapd_snap_get_download_size(snap), ==, 1024);
  g_assert_null(snapd_snap_get_hold(snap));
  g_assert_cmpstr(snapd_snap_get_icon(snap), ==, "ICON");
  g_assert_cmpstr(snapd_snap_get_id(snap), ==, "ID");
  g_assert_null(snapd_snap_get_install_date(snap));
  g_assert_cmpint(snapd_snap_get_installed_size(snap), ==, 0);
  GPtrArray *media = snapd_snap_get_media(snap);
  g_assert_cmpint(media->len, ==, 3);
  g_assert_cmpstr(snapd_media_get_media_type(media->pdata[0]), ==,
                  "screenshot");
  g_assert_cmpstr(snapd_media_get_url(media->pdata[0]), ==, "screenshot0.png");
  g_assert_cmpstr(snapd_media_get_media_type(media->pdata[1]), ==,
                  "screenshot");
  g_assert_cmpstr(snapd_media_get_url(media->pdata[1]), ==, "screenshot1.png");
  g_assert_cmpint(snapd_media_get_width(media->pdata[1]), ==, 1024);
  g_assert_cmpint(snapd_media_get_height(media->pdata[1]), ==, 1024);
  g_assert_cmpstr(snapd_media_get_media_type(media->pdata[2]), ==, "banner");
  g_assert_cmpstr(snapd_media_get_url(media->pdata[2]), ==, "banner.png");
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "carrot2");
  GPtrArray *prices = snapd_snap_get_prices(snap);
  g_assert_cmpint(prices->len, ==, 2);
  g_assert_cmpfloat(snapd_price_get_amount(prices->pdata[0]), ==, 1.25);
  g_assert_cmpstr(snapd_price_get_currency(prices->pdata[0]), ==, "NZD");
  g_assert_cmpfloat(snapd_price_get_amount(prices->pdata[1]), ==, 0.75);
  g_assert_cmpstr(snapd_price_get_currency(prices->pdata[1]), ==, "USD");
  g_assert_false(snapd_snap_get_private(snap));
  g_assert_cmpstr(snapd_snap_get_revision(snap), ==, "REVISION");
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_screenshots(snap)->len, ==, 0);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_cmpint(snapd_snap_get_snap_type(snap), ==, SNAPD_SNAP_TYPE_APP);
  g_assert_cmpint(snapd_snap_get_status(snap), ==, SNAPD_SNAP_STATUS_ACTIVE);
  g_assert_cmpstr(snapd_snap_get_store_url(snap), ==,
                  "https://snapcraft.io/snap");
  g_assert_cmpstr(snapd_snap_get_summary(snap), ==, "SUMMARY");
  g_assert_true(snapd_snap_get_trymode(snap));
  g_assert_cmpstr(snapd_snap_get_version(snap), ==, "VERSION");
  g_assert_cmpstr(snapd_snap_get_website(snap), ==, "WEBSITE");
}

static void test_find_query_private(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_snapd_add_store_snap(snapd, "snap1");
  mock_account_add_private_snap(a, "snap2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap2");
  g_assert_true(snapd_snap_get_private(snaps->pdata[0]));
}

static void test_find_query_private_not_logged_in(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_SELECT_PRIVATE, "snap", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
  g_assert_null(snaps);
}

static void test_find_bad_query(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  // '?' is not allowed in queries
  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_NONE, "snap?", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_QUERY);
  g_assert_null(snaps);
}

static void test_find_network_timeout(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_NONE, "network-timeout", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NETWORK_TIMEOUT);
  g_assert_null(snaps);
}

static void test_find_dns_failure(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_NONE, "dns-failure", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_DNS_FAILURE);
  g_assert_null(snaps);
}

static void test_find_name(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");
  mock_snapd_add_store_snap(snapd, "snap2");
  mock_snapd_add_store_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap");
}

static void test_find_name_private(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_add_private_snap(a, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE,
      "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap");
  g_assert_true(snapd_snap_get_private(snaps->pdata[0]));
}

static void test_find_name_private_not_logged_in(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME | SNAPD_FIND_FLAGS_SELECT_PRIVATE,
      "snap", NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
  g_assert_null(snaps);
}

static void test_find_channels(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");

  MockTrack *t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "stable", NULL);
  MockChannel *c = mock_track_add_channel(t, "beta", NULL);
  mock_channel_set_revision(c, "BETA-REVISION");
  mock_channel_set_version(c, "BETA-VERSION");
  mock_channel_set_epoch(c, "1");
  mock_channel_set_confinement(c, "classic");
  mock_channel_set_size(c, 10000);
  mock_channel_set_released_at(c, "2018-01-19T13:14:15Z");
  c = mock_track_add_channel(t, "stable", "branch");
  t = mock_snap_add_track(s, "insider");
  mock_track_add_channel(t, "stable", NULL);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  SnapdSnap *snap = snaps->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "snap");
  GStrv tracks = snapd_snap_get_tracks(snap);
  g_assert_cmpint(g_strv_length(tracks), ==, 2);
  g_assert_cmpstr(tracks[0], ==, "latest");
  g_assert_cmpstr(tracks[1], ==, "insider");
  GPtrArray *channels = snapd_snap_get_channels(snap);
  g_assert_cmpint(channels->len, ==, 4);

  gboolean matched_stable = FALSE, matched_beta = FALSE, matched_branch = FALSE,
           matched_track = FALSE;
  for (guint i = 0; i < channels->len; i++) {
    SnapdChannel *channel = channels->pdata[i];

    if (strcmp(snapd_channel_get_name(channel), "stable") == 0) {
      g_assert_cmpstr(snapd_channel_get_track(channel), ==, "latest");
      g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
      g_assert_null(snapd_channel_get_branch(channel));
      g_assert_cmpstr(snapd_channel_get_revision(channel), ==, "REVISION");
      g_assert_cmpstr(snapd_channel_get_version(channel), ==, "VERSION");
      g_assert_cmpstr(snapd_channel_get_epoch(channel), ==, "0");
      g_assert_cmpint(snapd_channel_get_confinement(channel), ==,
                      SNAPD_CONFINEMENT_STRICT);
      g_assert_cmpint(snapd_channel_get_size(channel), ==, 65535);
      g_assert_null(snapd_channel_get_released_at(channel));
      matched_stable = TRUE;
    }
    if (strcmp(snapd_channel_get_name(channel), "beta") == 0) {
      g_assert_cmpstr(snapd_channel_get_track(channel), ==, "latest");
      g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "beta");
      g_assert_null(snapd_channel_get_branch(channel));
      g_assert_cmpstr(snapd_channel_get_revision(channel), ==, "BETA-REVISION");
      g_assert_cmpstr(snapd_channel_get_version(channel), ==, "BETA-VERSION");
      g_assert_cmpstr(snapd_channel_get_epoch(channel), ==, "1");
      g_assert_cmpint(snapd_channel_get_confinement(channel), ==,
                      SNAPD_CONFINEMENT_CLASSIC);
      g_assert_cmpint(snapd_channel_get_size(channel), ==, 10000);
      g_assert_true(date_matches(snapd_channel_get_released_at(channel), 2018,
                                 1, 19, 13, 14, 15));
      matched_beta = TRUE;
    }
    if (strcmp(snapd_channel_get_name(channel), "stable/branch") == 0) {
      g_assert_cmpstr(snapd_channel_get_track(channel), ==, "latest");
      g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
      g_assert_cmpstr(snapd_channel_get_branch(channel), ==, "branch");
      g_assert_null(snapd_channel_get_released_at(channel));
      matched_branch = TRUE;
    }
    if (strcmp(snapd_channel_get_name(channel), "insider/stable") == 0) {
      g_assert_cmpstr(snapd_channel_get_track(channel), ==, "insider");
      g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
      g_assert_null(snapd_channel_get_branch(channel));
      g_assert_null(snapd_channel_get_released_at(channel));
      matched_track = TRUE;
    }
  }
  g_assert_true(matched_stable);
  g_assert_true(matched_beta);
  g_assert_true(matched_branch);
  g_assert_true(matched_track);
}

static void test_find_channels_match(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockSnap *s = mock_snapd_add_store_snap(snapd, "stable-snap");
  MockTrack *t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "stable", NULL);

  s = mock_snapd_add_store_snap(snapd, "full-snap");
  t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "stable", NULL);
  mock_track_add_channel(t, "candidate", NULL);
  mock_track_add_channel(t, "beta", NULL);
  mock_track_add_channel(t, "edge", NULL);

  s = mock_snapd_add_store_snap(snapd, "beta-snap");
  t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "beta", NULL);

  s = mock_snapd_add_store_snap(snapd, "branch-snap");
  t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "stable", NULL);
  mock_track_add_channel(t, "stable", "branch");

  s = mock_snapd_add_store_snap(snapd, "track-snap");
  t = mock_snap_add_track(s, "latest");
  mock_track_add_channel(t, "stable", NULL);
  t = mock_snap_add_track(s, "insider");
  mock_track_add_channel(t, "stable", NULL);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  /* All channels match to stable if only stable defined */
  g_autoptr(GPtrArray) snaps1 = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "stable-snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps1);
  g_assert_cmpint(snaps1->len, ==, 1);
  SnapdSnap *snap = snaps1->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "stable-snap");
  SnapdChannel *channel = snapd_snap_match_channel(snap, "stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "candidate");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "beta");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "edge");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "UNDEFINED");
  g_assert_null(channel);

  /* All channels match if all defined */
  g_autoptr(GPtrArray) snaps2 = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "full-snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps2);
  g_assert_cmpint(snaps2->len, ==, 1);
  snap = snaps2->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "full-snap");
  channel = snapd_snap_match_channel(snap, "stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "candidate");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "candidate");
  channel = snapd_snap_match_channel(snap, "beta");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "beta");
  channel = snapd_snap_match_channel(snap, "edge");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "edge");
  channel = snapd_snap_match_channel(snap, "UNDEFINED");
  g_assert_null(channel);

  /* Only match with more stable channels */
  g_autoptr(GPtrArray) snaps3 = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "beta-snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps3);
  g_assert_cmpint(snaps3->len, ==, 1);
  snap = snaps3->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "beta-snap");
  channel = snapd_snap_match_channel(snap, "stable");
  g_assert_null(channel);
  channel = snapd_snap_match_channel(snap, "candidate");
  g_assert_null(channel);
  channel = snapd_snap_match_channel(snap, "beta");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "beta");
  channel = snapd_snap_match_channel(snap, "edge");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "beta");
  channel = snapd_snap_match_channel(snap, "UNDEFINED");
  g_assert_null(channel);

  /* Match branches */
  g_autoptr(GPtrArray) snaps4 = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "branch-snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps4);
  g_assert_cmpint(snaps4->len, ==, 1);
  snap = snaps4->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "branch-snap");
  channel = snapd_snap_match_channel(snap, "stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "stable/branch");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable/branch");
  channel = snapd_snap_match_channel(snap, "candidate");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "beta");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "edge");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "UNDEFINED");
  g_assert_null(channel);

  /* Match correct tracks */
  g_autoptr(GPtrArray) snaps5 = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "track-snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps5);
  g_assert_cmpint(snaps5->len, ==, 1);
  snap = snaps5->pdata[0];
  g_assert_cmpstr(snapd_snap_get_name(snap), ==, "track-snap");
  channel = snapd_snap_match_channel(snap, "stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  g_assert_cmpstr(snapd_channel_get_track(channel), ==, "latest");
  g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "latest/stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "stable");
  g_assert_cmpstr(snapd_channel_get_track(channel), ==, "latest");
  g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
  channel = snapd_snap_match_channel(snap, "insider/stable");
  g_assert_nonnull(channel);
  g_assert_cmpstr(snapd_channel_get_name(channel), ==, "insider/stable");
  g_assert_cmpstr(snapd_channel_get_track(channel), ==, "insider");
  g_assert_cmpstr(snapd_channel_get_risk(channel), ==, "stable");
}

static gboolean cancel_cb(gpointer user_data) {
  GCancellable *cancellable = user_data;
  g_cancellable_cancel(cancellable);
  return G_SOURCE_REMOVE;
}

static void find_cancel_cb(GObject *object, GAsyncResult *result,
                           gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) snaps =
      snapd_client_find_finish(SNAPD_CLIENT(object), result, NULL, &error);
  g_assert_error(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null(snaps);

  g_main_loop_quit(data->loop);
}

static void test_find_cancel(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  /* Use a special query that never responds */
  g_autoptr(GCancellable) cancellable = g_cancellable_new();
  snapd_client_find_async(client, SNAPD_FIND_FLAGS_NONE, "do-not-respond",
                          cancellable, find_cancel_cb,
                          async_data_new(loop, snapd));
  g_idle_add(cancel_cb, cancellable);

  g_main_loop_run(loop);
}

static void test_find_section(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "section", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "section", FALSE);
  mock_snapd_add_store_snap(snapd, "carrot2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) snaps = snapd_client_find_section_sync(
      client, SNAPD_FIND_FLAGS_NONE, "section", NULL, NULL, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "apple");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "carrot1");
}

static void test_find_section_query(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "section", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "section", FALSE);
  mock_snapd_add_store_snap(snapd, "carrot2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) snaps = snapd_client_find_section_sync(
      client, SNAPD_FIND_FLAGS_NONE, "section", "carrot", NULL, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "carrot1");
}

static void test_find_section_name(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "section", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "section", FALSE);
  s = mock_snapd_add_store_snap(snapd, "carrot2");
  mock_snap_add_store_category(s, "section", FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) snaps =
      snapd_client_find_section_sync(client, SNAPD_FIND_FLAGS_MATCH_NAME,
                                     "section", "carrot1", NULL, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "carrot1");
}

static void test_find_category(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "category", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "category", FALSE);
  mock_snapd_add_store_snap(snapd, "carrot2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_category_sync(
      client, SNAPD_FIND_FLAGS_NONE, "category", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "apple");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "carrot1");
}

static void test_find_category_query(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "category", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "category", FALSE);
  mock_snapd_add_store_snap(snapd, "carrot2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_category_sync(
      client, SNAPD_FIND_FLAGS_NONE, "category", "carrot", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "carrot1");
}

static void test_find_category_name(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_store_category(s, "category", FALSE);
  mock_snapd_add_store_snap(snapd, "banana");
  s = mock_snapd_add_store_snap(snapd, "carrot1");
  mock_snap_add_store_category(s, "category", FALSE);
  s = mock_snapd_add_store_snap(snapd, "carrot2");
  mock_snap_add_store_category(s, "category", FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_category_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "category", "carrot1", NULL, NULL,
      &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "carrot1");
}

static void test_find_scope_narrow(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap1");
  s = mock_snapd_add_store_snap(snapd, "snap2");
  mock_snap_set_scope_is_wide(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_NONE, "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
}

static void test_find_scope_wide(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap1");
  s = mock_snapd_add_store_snap(snapd, "snap2");
  mock_snap_set_scope_is_wide(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_SCOPE_WIDE, "snap", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap2");
}

static void test_find_common_id(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap1");
  MockApp *a = mock_snap_add_app(s, "snap1");
  mock_app_set_common_id(a, "com.example.snap1");
  s = mock_snapd_add_store_snap(snapd, "snap2");
  a = mock_snap_add_app(s, "snap2");
  mock_app_set_common_id(a, "com.example.snap2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps =
      snapd_client_find_sync(client, SNAPD_FIND_FLAGS_MATCH_COMMON_ID,
                             "com.example.snap2", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap2");
}

static void test_find_categories(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "apple");
  mock_snap_add_category(s, "fruit", TRUE);
  mock_snap_add_category(s, "food", FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps = snapd_client_find_sync(
      client, SNAPD_FIND_FLAGS_MATCH_NAME, "apple", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 1);
  SnapdSnap *snap = snaps->pdata[0];
  g_assert_cmpint(snapd_snap_get_categories(snap)->len, ==, 2);
  SnapdCategory *category = snapd_snap_get_categories(snap)->pdata[0];
  g_assert_cmpstr(snapd_category_get_name(category), ==, "fruit");
  g_assert_true(snapd_category_get_featured(category));
  category = snapd_snap_get_categories(snap)->pdata[1];
  g_assert_cmpstr(snapd_category_get_name(category), ==, "food");
  g_assert_false(snapd_category_get_featured(category));
}

static void test_find_refreshable_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap3");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap1");
  mock_snap_set_revision(s, "1");
  s = mock_snapd_add_store_snap(snapd, "snap3");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps =
      snapd_client_find_refreshable_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_revision(snaps->pdata[0]), ==, "1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap3");
  g_assert_cmpstr(snapd_snap_get_revision(snaps->pdata[1]), ==, "1");
}

static void find_refreshable_cb(GObject *object, GAsyncResult *result,
                                gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) snaps = snapd_client_find_refreshable_finish(
      SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 2);
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[0]), ==, "snap1");
  g_assert_cmpstr(snapd_snap_get_revision(snaps->pdata[0]), ==, "1");
  g_assert_cmpstr(snapd_snap_get_name(snaps->pdata[1]), ==, "snap3");
  g_assert_cmpstr(snapd_snap_get_revision(snaps->pdata[1]), ==, "1");

  g_main_loop_quit(data->loop);
}

static void test_find_refreshable_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap3");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap1");
  mock_snap_set_revision(s, "1");
  s = mock_snapd_add_store_snap(snapd, "snap3");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_find_refreshable_async(client, NULL, find_refreshable_cb,
                                      async_data_new(loop, snapd));

  g_main_loop_run(loop);
}

static void test_find_refreshable_no_updates(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) snaps =
      snapd_client_find_refreshable_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snaps);
  g_assert_cmpint(snaps->len, ==, 0);
}

static void test_install_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_cmpstr(
      mock_snap_get_confinement(mock_snapd_find_snap(snapd, "snap")), ==,
      "strict");
  g_assert_false(mock_snap_get_devmode(mock_snapd_find_snap(snapd, "snap")));
  g_assert_false(mock_snap_get_dangerous(mock_snapd_find_snap(snapd, "snap")));
  g_assert_false(mock_snap_get_jailmode(mock_snapd_find_snap(snapd, "snap")));
}

static void test_install_sync_multiple(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap1");
  mock_snapd_add_store_snap(snapd, "snap2");
  mock_snapd_add_store_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap1"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap2"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap3"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap1",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  result = snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap2",
                                      NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  result = snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap3",
                                      NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap1"));
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap2"));
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap3"));
}

static void install_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap"));
  g_assert_cmpstr(
      mock_snap_get_confinement(mock_snapd_find_snap(data->snapd, "snap")), ==,
      "strict");
  g_assert_false(
      mock_snap_get_devmode(mock_snapd_find_snap(data->snapd, "snap")));
  g_assert_false(
      mock_snap_get_dangerous(mock_snapd_find_snap(data->snapd, "snap")));
  g_assert_false(
      mock_snap_get_jailmode(mock_snapd_find_snap(data->snapd, "snap")));

  g_main_loop_quit(data->loop);
}

static void test_install_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                              NULL, NULL, NULL, NULL, install_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void install_multiple_cb(GObject *object, GAsyncResult *result,
                                gpointer user_data) {
  AsyncData *data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  data->counter--;
  if (data->counter == 0) {
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap1"));
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap2"));
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap3"));

    g_main_loop_quit(data->loop);

    async_data_free(data);
  }
}

static void test_install_async_multiple(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap1");
  mock_snapd_add_store_snap(snapd, "snap2");
  mock_snapd_add_store_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap1"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap2"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap3"));
  AsyncData *data = async_data_new(loop, snapd);
  data->counter = 3;
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL,
                              NULL, NULL, NULL, NULL, install_multiple_cb,
                              data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL,
                              NULL, NULL, NULL, NULL, install_multiple_cb,
                              data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL,
                              NULL, NULL, NULL, NULL, install_multiple_cb,
                              data);
  g_main_loop_run(loop);
}

static void install_failure_cb(GObject *object, GAsyncResult *result,
                               gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
  g_assert_cmpstr(error->message, ==, "ERROR");
  g_assert_false(r);
  g_assert_null(mock_snapd_find_snap(data->snapd, "snap"));

  g_main_loop_quit(data->loop);
}

static void test_install_async_failure(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_error(s, "ERROR");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                              NULL, NULL, NULL, NULL, install_failure_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void install_cancel_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false(r);
  g_assert_null(mock_snapd_find_snap(data->snapd, "snap"));

  g_main_loop_quit(data->loop);
}

static void test_install_async_cancel(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  g_autoptr(GCancellable) cancellable = g_cancellable_new();
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                              NULL, NULL, NULL, cancellable, install_cancel_cb,
                              async_data_new(loop, snapd));
  g_idle_add(cancel_cb, cancellable);
  g_main_loop_run(loop);
}

static void complete_async_multiple_cancel_first(AsyncData *data) {
  data->counter--;
  if (data->counter == 0) {
    g_assert_null(mock_snapd_find_snap(data->snapd, "snap1"));
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap2"));
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap3"));

    g_main_loop_quit(data->loop);

    async_data_free(data);
  }
}

static void install_multiple_cancel_first_snap1_cb(GObject *object,
                                                   GAsyncResult *result,
                                                   gpointer user_data) {
  AsyncData *data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false(r);
  g_assert_null(mock_snapd_find_snap(data->snapd, "snap1"));

  complete_async_multiple_cancel_first(data);
}

static void install_multiple_cancel_first_cb(GObject *object,
                                             GAsyncResult *result,
                                             gpointer user_data) {
  AsyncData *data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  complete_async_multiple_cancel_first(data);
}

static void test_install_async_multiple_cancel_first(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap1");
  mock_snapd_add_store_snap(snapd, "snap2");
  mock_snapd_add_store_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap1"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap2"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap3"));
  g_autoptr(GCancellable) cancellable = g_cancellable_new();
  AsyncData *data = async_data_new(loop, snapd);
  data->counter = 3;
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL,
                              NULL, NULL, NULL, cancellable,
                              install_multiple_cancel_first_snap1_cb, data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL,
                              NULL, NULL, NULL, NULL,
                              install_multiple_cancel_first_cb, data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL,
                              NULL, NULL, NULL, NULL,
                              install_multiple_cancel_first_cb, data);
  g_idle_add(cancel_cb, cancellable);
  g_main_loop_run(loop);
}

static void complete_async_multiple_cancel_last(AsyncData *data) {
  data->counter--;
  if (data->counter == 0) {
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap1"));
    g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap2"));
    g_assert_null(mock_snapd_find_snap(data->snapd, "snap3"));

    g_main_loop_quit(data->loop);

    async_data_free(data);
  }
}

static void install_multiple_cancel_last_snap3_cb(GObject *object,
                                                  GAsyncResult *result,
                                                  gpointer user_data) {
  AsyncData *data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false(r);
  g_assert_null(mock_snapd_find_snap(data->snapd, "snap3"));

  complete_async_multiple_cancel_last(data);
}

static void install_multiple_cancel_last_cb(GObject *object,
                                            GAsyncResult *result,
                                            gpointer user_data) {
  AsyncData *data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  complete_async_multiple_cancel_last(data);
}

static void test_install_async_multiple_cancel_last(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap1");
  mock_snapd_add_store_snap(snapd, "snap2");
  mock_snapd_add_store_snap(snapd, "snap3");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap1"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap2"));
  g_assert_null(mock_snapd_find_snap(snapd, "snap3"));
  g_autoptr(GCancellable) cancellable = g_cancellable_new();
  AsyncData *data = async_data_new(loop, snapd);
  data->counter = 3;
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap1", NULL,
                              NULL, NULL, NULL, NULL,
                              install_multiple_cancel_last_cb, data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap2", NULL,
                              NULL, NULL, NULL, NULL,
                              install_multiple_cancel_last_cb, data);
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap3", NULL,
                              NULL, NULL, NULL, cancellable,
                              install_multiple_cancel_last_snap3_cb, data);
  g_idle_add(cancel_cb, cancellable);
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
  const gchar *spawn_time;
  const gchar *ready_time;
} InstallProgressData;

static gchar *time_to_string(GDateTime *time) {
  if (time == NULL)
    return NULL;

  return g_date_time_format(time, "%FT%H:%M:%S%Z");
}

static void install_progress_cb(SnapdClient *client, SnapdChange *change,
                                gpointer deprecated, gpointer user_data) {
  InstallProgressData *data = user_data;

  data->progress_done++;

  // Check we've been notified of all tasks
  GPtrArray *tasks = snapd_change_get_tasks(change);
  int progress_done = 0, progress_total = 0;
  for (guint i = 0; i < tasks->len; i++) {
    SnapdTask *task = tasks->pdata[i];
    progress_done += snapd_task_get_progress_done(task);
    progress_total += snapd_task_get_progress_total(task);
  }
  g_assert_cmpint(data->progress_done, ==, progress_done);

  g_autofree gchar *spawn_time =
      time_to_string(snapd_change_get_spawn_time(change));
  g_autofree gchar *ready_time =
      time_to_string(snapd_change_get_ready_time(change));

  g_assert_cmpstr(snapd_change_get_kind(change), ==, "KIND");
  g_assert_cmpstr(snapd_change_get_summary(change), ==, "SUMMARY");
  if (progress_done == progress_total) {
    g_assert_cmpstr(snapd_change_get_status(change), ==, "Done");
    g_assert_true(snapd_change_get_ready(change));
  } else {
    g_assert_cmpstr(snapd_change_get_status(change), ==, "Do");
    g_assert_false(snapd_change_get_ready(change));
  }
  g_assert_cmpstr(spawn_time, ==, data->spawn_time);
  if (snapd_change_get_ready(change))
    g_assert_cmpstr(ready_time, ==, data->ready_time);
  else
    g_assert_null(ready_time);
}

static void test_install_progress(void) {
  InstallProgressData install_progress_data;
  install_progress_data.progress_done = 0;
  install_progress_data.spawn_time = "2017-01-02T11:23:58Z";
  install_progress_data.ready_time = "2017-01-03T00:00:00Z";

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_spawn_time(snapd, install_progress_data.spawn_time);
  mock_snapd_set_ready_time(snapd, install_progress_data.ready_time);
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_install2_sync(
      client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL, NULL, install_progress_cb,
      &install_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpint(install_progress_data.progress_done, >, 0);
}

static void test_install_needs_classic(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_confinement(s, "classic");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_CLASSIC);
  g_assert_false(result);
}

static void test_install_classic(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_on_classic(snapd, TRUE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_confinement(s, "classic");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_CLASSIC, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_cmpstr(
      mock_snap_get_confinement(mock_snapd_find_snap(snapd, "snap")), ==,
      "classic");
}

static void test_install_not_classic(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_on_classic(snapd, TRUE);
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_CLASSIC, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_CLASSIC);
  g_assert_false(result);
}

static void test_install_needs_classic_system(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_confinement(s, "classic");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_CLASSIC, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_CLASSIC_SYSTEM);
  g_assert_false(result);
}

static void test_install_needs_devmode(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_confinement(s, "devmode");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NEEDS_DEVMODE);
  g_assert_false(result);
}

static void test_install_devmode(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_confinement(s, "devmode");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_DEVMODE, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_true(mock_snap_get_devmode(mock_snapd_find_snap(snapd, "snap")));
}

static void test_install_dangerous(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_DANGEROUS, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_true(mock_snap_get_dangerous(mock_snapd_find_snap(snapd, "snap")));
}

static void test_install_jailmode(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_JAILMODE, "snap",
                                 NULL, NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_true(mock_snap_get_jailmode(mock_snapd_find_snap(snapd, "snap")));
}

static void test_install_channel(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_channel(s, "channel1");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_channel(s, "channel2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap",
                                 "channel2", NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_cmpstr(mock_snap_get_channel(mock_snapd_find_snap(snapd, "snap")),
                  ==, "channel2");
}

static void test_install_revision(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1.2");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1.1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 "1.1", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_assert_cmpstr(mock_snap_get_revision(mock_snapd_find_snap(snapd, "snap")),
                  ==, "1.1");
}

static void test_install_not_available(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_FOUND);
  g_assert_false(result);
}

static void test_install_channel_not_available(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap",
                                 "channel", NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_CHANNEL_NOT_AVAILABLE);
  g_assert_false(result);
}

static void test_install_revision_not_available(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 "1.1", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_REVISION_NOT_AVAILABLE);
  g_assert_false(result);
}

static void test_install_snapd_restart(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_restart_required(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
}

static void test_install_async_snapd_restart(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_restart_required(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  snapd_client_install2_async(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                              NULL, NULL, NULL, NULL, install_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_install_auth_cancelled(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");
  mock_snapd_set_decline_auth(snapd, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_install2_sync(client, SNAPD_INSTALL_FLAGS_NONE, "snap", NULL,
                                 NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_CANCELLED);
  g_assert_false(result);
}

static void test_install_stream_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_NONE, stream, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "strict");
  g_assert_false(mock_snap_get_dangerous(snap));
  g_assert_false(mock_snap_get_devmode(snap));
  g_assert_false(mock_snap_get_jailmode(snap));
}

static void install_stream_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_install_stream_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  MockSnap *snap = mock_snapd_find_snap(data->snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "strict");
  g_assert_false(mock_snap_get_dangerous(snap));
  g_assert_false(mock_snap_get_devmode(snap));
  g_assert_false(mock_snap_get_jailmode(snap));

  g_main_loop_quit(data->loop);
}

static void test_install_stream_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  snapd_client_install_stream_async(client, SNAPD_INSTALL_FLAGS_NONE, stream,
                                    NULL, NULL, NULL, install_stream_cb,
                                    async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} InstallStreamProgressData;

static void install_stream_progress_cb(SnapdClient *client, SnapdChange *change,
                                       gpointer deprecated,
                                       gpointer user_data) {
  InstallStreamProgressData *data = user_data;
  data->progress_done++;
}

static void test_install_stream_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  InstallStreamProgressData install_stream_progress_data;
  install_stream_progress_data.progress_done = 0;
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_NONE, stream, install_stream_progress_cb,
      &install_stream_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpint(install_stream_progress_data.progress_done, >, 0);
}

static void test_install_stream_classic(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_CLASSIC, stream, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "classic");
  g_assert_false(mock_snap_get_dangerous(snap));
  g_assert_false(mock_snap_get_devmode(snap));
  g_assert_false(mock_snap_get_jailmode(snap));
}

static void test_install_stream_dangerous(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_DANGEROUS, stream, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "strict");
  g_assert_true(mock_snap_get_dangerous(snap));
  g_assert_false(mock_snap_get_devmode(snap));
  g_assert_false(mock_snap_get_jailmode(snap));
}

static void test_install_stream_devmode(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_DEVMODE, stream, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "strict");
  g_assert_false(mock_snap_get_dangerous(snap));
  g_assert_true(mock_snap_get_devmode(snap));
  g_assert_false(mock_snap_get_jailmode(snap));
}

static void test_install_stream_jailmode(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_snap(snapd, "sideload"));
  g_autoptr(GInputStream) stream =
      g_memory_input_stream_new_from_data("SNAP", 4, NULL);
  gboolean result = snapd_client_install_stream_sync(
      client, SNAPD_INSTALL_FLAGS_JAILMODE, stream, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "sideload");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_data(snap), ==, "SNAP");
  g_assert_cmpstr(mock_snap_get_confinement(snap), ==, "strict");
  g_assert_false(mock_snap_get_dangerous(snap));
  g_assert_false(mock_snap_get_devmode(snap));
  g_assert_true(mock_snap_get_jailmode(snap));
}

static void test_try_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_try_sync(client, "/path/to/snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "try");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_path(snap), ==, "/path/to/snap");
}

static void try_cb(GObject *object, GAsyncResult *result, gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_try_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  MockSnap *snap = mock_snapd_find_snap(data->snapd, "try");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_path(snap), ==, "/path/to/snap");

  g_main_loop_quit(data->loop);
}

static void test_try_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_try_async(client, "/path/to/snap", NULL, NULL, NULL, try_cb,
                         async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} TryProgressData;

static void try_progress_cb(SnapdClient *client, SnapdChange *change,
                            gpointer deprecated, gpointer user_data) {
  TryProgressData *data = user_data;
  data->progress_done++;
}

static void test_try_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  TryProgressData try_progress_data;
  try_progress_data.progress_done = 0;
  gboolean result =
      snapd_client_try_sync(client, "/path/to/snap", try_progress_cb,
                            &try_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  MockSnap *snap = mock_snapd_find_snap(snapd, "try");
  g_assert_nonnull(snap);
  g_assert_cmpstr(mock_snap_get_path(snap), ==, "/path/to/snap");
  g_assert_cmpint(try_progress_data.progress_done, >, 0);
}

static void test_try_not_a_snap(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_try_sync(client, "*", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_A_SNAP);
  g_assert_false(result);
}

static void test_refresh_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_refresh_sync(client, "snap", NULL, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
}

static void refresh_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_refresh_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  g_main_loop_quit(data->loop);
}

static void test_refresh_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_refresh_async(client, "snap", NULL, NULL, NULL, NULL, refresh_cb,
                             async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} RefreshProgressData;

static void refresh_progress_cb(SnapdClient *client, SnapdChange *change,
                                gpointer deprecated, gpointer user_data) {
  RefreshProgressData *data = user_data;
  data->progress_done++;
}

static void test_refresh_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  RefreshProgressData refresh_progress_data;
  refresh_progress_data.progress_done = 0;
  gboolean result =
      snapd_client_refresh_sync(client, "snap", NULL, refresh_progress_cb,
                                &refresh_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpint(refresh_progress_data.progress_done, >, 0);
}

static void test_refresh_channel(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1");
  mock_snap_set_channel(s, "channel1");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "1");
  mock_snap_set_channel(s, "channel2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_refresh_sync(client, "snap", "channel2", NULL,
                                              NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(mock_snap_get_channel(mock_snapd_find_snap(snapd, "snap")),
                  ==, "channel2");
}

static void test_refresh_no_updates(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_refresh_sync(client, "snap", NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NO_UPDATE_AVAILABLE);
  g_assert_false(result);
}

static void test_refresh_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_refresh_sync(client, "snap", NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
  g_assert_false(result);
}

static void test_refresh_not_in_store(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_revision(s, "0");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_refresh_sync(client, "snap", NULL, NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_IN_STORE);
  g_assert_false(result);
}

static void test_refresh_all_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap3");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap1");
  mock_snap_set_revision(s, "1");
  s = mock_snapd_add_store_snap(snapd, "snap3");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) snap_names =
      snapd_client_refresh_all_sync(client, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_cmpint(g_strv_length(snap_names), ==, 2);
  g_assert_cmpstr(snap_names[0], ==, "snap1");
  g_assert_cmpstr(snap_names[1], ==, "snap3");
}

static void refresh_all_cb(GObject *object, GAsyncResult *result,
                           gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_auto(GStrv) snap_names =
      snapd_client_refresh_all_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_cmpint(g_strv_length(snap_names), ==, 2);
  g_assert_cmpstr(snap_names[0], ==, "snap1");
  g_assert_cmpstr(snap_names[1], ==, "snap3");

  g_main_loop_quit(data->loop);
}

static void test_refresh_all_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap3");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap1");
  mock_snap_set_revision(s, "1");
  s = mock_snapd_add_store_snap(snapd, "snap3");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_refresh_all_async(client, NULL, NULL, NULL, refresh_all_cb,
                                 async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} RefreshAllProgressData;

static void refresh_all_progress_cb(SnapdClient *client, SnapdChange *change,
                                    gpointer deprecated, gpointer user_data) {
  RefreshAllProgressData *data = user_data;
  data->progress_done++;
}

static void test_refresh_all_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap1");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap2");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_snap(snapd, "snap3");
  mock_snap_set_revision(s, "0");
  s = mock_snapd_add_store_snap(snapd, "snap1");
  mock_snap_set_revision(s, "1");
  s = mock_snapd_add_store_snap(snapd, "snap3");
  mock_snap_set_revision(s, "1");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  RefreshAllProgressData refresh_all_progress_data;
  refresh_all_progress_data.progress_done = 0;
  g_auto(GStrv) snap_names =
      snapd_client_refresh_all_sync(client, refresh_all_progress_cb,
                                    &refresh_all_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_cmpint(g_strv_length(snap_names), ==, 2);
  g_assert_cmpstr(snap_names[0], ==, "snap1");
  g_assert_cmpstr(snap_names[1], ==, "snap3");
  g_assert_cmpint(refresh_all_progress_data.progress_done, >, 0);
}

static void test_refresh_all_no_updates(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) snap_names =
      snapd_client_refresh_all_sync(client, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_cmpint(g_strv_length(snap_names), ==, 0);
}

static void test_remove_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  gboolean result = snapd_client_remove2_sync(client, SNAPD_REMOVE_FLAGS_NONE,
                                              "snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  g_assert_nonnull(mock_snapd_find_snapshot(snapd, "snap"));
}

static void remove_cb(GObject *object, GAsyncResult *result,
                      gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_remove2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_null(mock_snapd_find_snap(data->snapd, "snap"));
  g_assert_nonnull(mock_snapd_find_snapshot(data->snapd, "snap"));

  g_main_loop_quit(data->loop);
}

static void test_remove_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  snapd_client_remove2_async(client, SNAPD_REMOVE_FLAGS_NONE, "snap", NULL,
                             NULL, NULL, remove_cb,
                             async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void remove_failure_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_remove2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_FAILED);
  g_assert_cmpstr(error->message, ==, "ERROR");
  g_assert_false(r);
  g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap"));

  g_main_loop_quit(data->loop);
}

static void test_remove_async_failure(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_error(s, "ERROR");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  snapd_client_remove2_async(client, SNAPD_REMOVE_FLAGS_NONE, "snap", NULL,
                             NULL, NULL, remove_failure_cb,
                             async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void remove_cancel_cb(GObject *object, GAsyncResult *result,
                             gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_remove2_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_error(error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_false(r);
  g_assert_nonnull(mock_snapd_find_snap(data->snapd, "snap"));

  g_main_loop_quit(data->loop);
}

static void test_remove_async_cancel(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  g_autoptr(GCancellable) cancellable = g_cancellable_new();
  snapd_client_remove2_async(client, SNAPD_REMOVE_FLAGS_NONE, "snap", NULL,
                             NULL, cancellable, remove_cancel_cb,
                             async_data_new(loop, snapd));
  g_idle_add(cancel_cb, cancellable);
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} RemoveProgressData;

static void remove_progress_cb(SnapdClient *client, SnapdChange *change,
                               gpointer deprecated, gpointer user_data) {
  RemoveProgressData *data = user_data;
  data->progress_done++;
}

static void test_remove_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_nonnull(mock_snapd_find_snap(snapd, "snap"));
  RemoveProgressData remove_progress_data;
  remove_progress_data.progress_done = 0;
  gboolean result = snapd_client_remove2_sync(
      client, SNAPD_REMOVE_FLAGS_NONE, "snap", remove_progress_cb,
      &remove_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  g_assert_cmpint(remove_progress_data.progress_done, >, 0);
}

static void test_remove_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_remove2_sync(client, SNAPD_REMOVE_FLAGS_NONE,
                                              "snap", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
  g_assert_false(result);
}

static void test_remove_purge(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_remove2_sync(client, SNAPD_REMOVE_FLAGS_PURGE,
                                              "snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_snapd_find_snap(snapd, "snap"));
  g_assert_null(mock_snapd_find_snapshot(snapd, "snap"));
}

static void test_enable_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_enable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_false(mock_snap_get_disabled(mock_snapd_find_snap(snapd, "snap")));
}

static void enable_cb(GObject *object, GAsyncResult *result,
                      gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_enable_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_false(
      mock_snap_get_disabled(mock_snapd_find_snap(data->snapd, "snap")));

  g_main_loop_quit(data->loop);
}

static void test_enable_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_enable_async(client, "snap", NULL, NULL, NULL, enable_cb,
                            async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} EnableProgressData;

static void enable_progress_cb(SnapdClient *client, SnapdChange *change,
                               gpointer deprecated, gpointer user_data) {
  EnableProgressData *data = user_data;
  data->progress_done++;
}

static void test_enable_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  EnableProgressData enable_progress_data;
  enable_progress_data.progress_done = 0;
  gboolean result = snapd_client_enable_sync(
      client, "snap", enable_progress_cb, &enable_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_false(mock_snap_get_disabled(mock_snapd_find_snap(snapd, "snap")));
  g_assert_cmpint(enable_progress_data.progress_done, >, 0);
}

static void test_enable_already_enabled(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_enable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_false(result);
}

static void test_enable_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_enable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
  g_assert_false(result);
}

static void test_disable_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_disable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_true(mock_snap_get_disabled(mock_snapd_find_snap(snapd, "snap")));
}

static void disable_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_disable_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_true(
      mock_snap_get_disabled(mock_snapd_find_snap(data->snapd, "snap")));

  g_main_loop_quit(data->loop);
}

static void test_disable_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_disable_async(client, "snap", NULL, NULL, NULL, disable_cb,
                             async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} DisableProgressData;

static void disable_progress_cb(SnapdClient *client, SnapdChange *change,
                                gpointer deprecated, gpointer user_data) {
  DisableProgressData *data = user_data;
  data->progress_done++;
}

static void test_disable_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  DisableProgressData disable_progress_data;
  disable_progress_data.progress_done = 0;
  gboolean result =
      snapd_client_disable_sync(client, "snap", disable_progress_cb,
                                &disable_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_true(mock_snap_get_disabled(mock_snapd_find_snap(snapd, "snap")));
  g_assert_cmpint(disable_progress_data.progress_done, >, 0);
}

static void test_disable_already_disabled(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_disabled(s, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_disable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
  g_assert_false(result);
}

static void test_disable_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_disable_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
  g_assert_false(result);
}

static void test_switch_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_tracking_channel(s, "stable");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_switch_sync(client, "snap", "beta", NULL, NULL,
                                             NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(
      mock_snap_get_tracking_channel(mock_snapd_find_snap(snapd, "snap")), ==,
      "beta");
}

static void switch_cb(GObject *object, GAsyncResult *result,
                      gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_switch_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_cmpstr(
      mock_snap_get_tracking_channel(mock_snapd_find_snap(data->snapd, "snap")),
      ==, "beta");

  g_main_loop_quit(data->loop);
}

static void test_switch_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_tracking_channel(s, "stable");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_switch_async(client, "snap", "beta", NULL, NULL, NULL, switch_cb,
                            async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

typedef struct {
  int progress_done;
} SwitchProgressData;

static void switch_progress_cb(SnapdClient *client, SnapdChange *change,
                               gpointer deprecated, gpointer user_data) {
  SwitchProgressData *data = user_data;
  data->progress_done++;
}

static void test_switch_progress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_set_tracking_channel(s, "stable");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  SwitchProgressData switch_progress_data;
  switch_progress_data.progress_done = 0;
  gboolean result =
      snapd_client_switch_sync(client, "snap", "beta", switch_progress_cb,
                               &switch_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(
      mock_snap_get_tracking_channel(mock_snapd_find_snap(snapd, "snap")), ==,
      "beta");
  g_assert_cmpint(switch_progress_data.progress_done, >, 0);
}

static void test_switch_not_installed(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_switch_sync(client, "snap", "beta", NULL, NULL,
                                             NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
  g_assert_false(result);
}

static void test_check_buy_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result = snapd_client_check_buy_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
}

static void check_buy_cb(GObject *object, GAsyncResult *result,
                         gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_check_buy_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  g_main_loop_quit(data->loop);
}

static void test_check_buy_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  snapd_client_check_buy_async(client, NULL, check_buy_cb,
                               async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_check_buy_terms_not_accepted(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, FALSE);
  mock_account_set_has_payment_methods(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result = snapd_client_check_buy_sync(client, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
  g_assert_false(result);
}

static void test_check_buy_no_payment_methods(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result = snapd_client_check_buy_sync(client, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
  g_assert_false(result);
}

static void test_check_buy_not_logged_in(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_check_buy_sync(client, NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
  g_assert_false(result);
}

static void test_buy_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 1.25, "NZD", NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
}

static void buy_cb(GObject *object, GAsyncResult *result, gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_buy_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);

  g_main_loop_quit(data->loop);
}

static void test_buy_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  snapd_client_buy_async(client, "ABCDEF", 1.25, "NZD", NULL, buy_cb,
                         async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_buy_not_logged_in(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 1.25, "NZD", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_AUTH_DATA_REQUIRED);
  g_assert_false(result);
}

static void test_buy_not_available(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 1.25, "NZD", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_NOT_FOUND);
  g_assert_false(result);
}

static void test_buy_terms_not_accepted(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, FALSE);
  mock_account_set_has_payment_methods(a, FALSE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 1.25, "NZD", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_TERMS_NOT_ACCEPTED);
  g_assert_false(result);
}

static void test_buy_no_payment_methods(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, FALSE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 1.25, "NZD", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_NOT_SETUP);
  g_assert_false(result);
}

static void test_buy_invalid_price(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockAccount *a =
      mock_snapd_add_account(snapd, "test@example.com", "test", "secret");
  mock_account_set_terms_accepted(a, TRUE);
  mock_account_set_has_payment_methods(a, TRUE);
  MockSnap *s = mock_snapd_add_store_snap(snapd, "snap");
  mock_snap_set_id(s, "ABCDEF");
  mock_snap_add_price(s, 1.25, "NZD");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdUserInformation) user_information = snapd_client_login2_sync(
      client, "test@example.com", "secret", NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(user_information);
  snapd_client_set_auth_data(
      client, snapd_user_information_get_auth_data(user_information));

  gboolean result =
      snapd_client_buy_sync(client, "ABCDEF", 0.75, "NZD", NULL, &error);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_PAYMENT_DECLINED);
  g_assert_false(result);
}

static void test_create_user_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_account_by_username(snapd, "user"));
  g_autoptr(SnapdUserInformation) info = snapd_client_create_user_sync(
      client, "user@example.com", SNAPD_CREATE_USER_FLAGS_NONE, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "user");
  GStrv ssh_keys = snapd_user_information_get_ssh_keys(info);
  g_assert_nonnull(ssh_keys);
  g_assert_cmpint(g_strv_length(ssh_keys), ==, 2);
  g_assert_cmpstr(ssh_keys[0], ==, "KEY1");
  g_assert_cmpstr(ssh_keys[1], ==, "KEY2");
  MockAccount *account = mock_snapd_find_account_by_username(snapd, "user");
  g_assert_nonnull(account);
  g_assert_false(mock_account_get_sudoer(account));
  g_assert_false(mock_account_get_known(account));
}

static void test_create_user_sudo(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_account_by_username(snapd, "user"));
  g_autoptr(SnapdUserInformation) info = snapd_client_create_user_sync(
      client, "user@example.com", SNAPD_CREATE_USER_FLAGS_SUDO, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  MockAccount *account = mock_snapd_find_account_by_username(snapd, "user");
  g_assert_nonnull(account);
  g_assert_true(mock_account_get_sudoer(account));
}

static void test_create_user_known(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_snapd_find_account_by_username(snapd, "user"));
  g_autoptr(SnapdUserInformation) info = snapd_client_create_user_sync(
      client, "user@example.com", SNAPD_CREATE_USER_FLAGS_KNOWN, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(info);
  MockAccount *account = mock_snapd_find_account_by_username(snapd, "user");
  g_assert_nonnull(account);
  g_assert_true(mock_account_get_known(account));
}

static void test_create_users_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) users_info =
      snapd_client_create_users_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(users_info);
  g_assert_cmpint(users_info->len, ==, 3);
  SnapdUserInformation *info = users_info->pdata[0];
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "admin");
  GStrv ssh_keys = snapd_user_information_get_ssh_keys(info);
  g_assert_nonnull(ssh_keys);
  g_assert_cmpint(g_strv_length(ssh_keys), ==, 2);
  g_assert_cmpstr(ssh_keys[0], ==, "KEY1");
  g_assert_cmpstr(ssh_keys[1], ==, "KEY2");
  info = users_info->pdata[1];
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "alice");
  info = users_info->pdata[2];
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "bob");
  g_assert_nonnull(mock_snapd_find_account_by_username(snapd, "admin"));
  g_assert_nonnull(mock_snapd_find_account_by_username(snapd, "alice"));
  g_assert_nonnull(mock_snapd_find_account_by_username(snapd, "bob"));
}

static void test_get_users_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "alice@example.com", "alice", "secret");
  mock_snapd_add_account(snapd, "bob@example.com", "bob", "secret");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) users_info =
      snapd_client_get_users_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(users_info);
  g_assert_cmpint(users_info->len, ==, 2);
  SnapdUserInformation *info = users_info->pdata[0];
  g_assert_cmpint(snapd_user_information_get_id(info), ==, 1);
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "alice");
  g_assert_cmpstr(snapd_user_information_get_email(info), ==,
                  "alice@example.com");
  info = users_info->pdata[1];
  g_assert_cmpint(snapd_user_information_get_id(info), ==, 2);
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "bob");
  g_assert_cmpstr(snapd_user_information_get_email(info), ==,
                  "bob@example.com");
}

static void get_users_cb(GObject *object, GAsyncResult *result,
                         gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) users_info =
      snapd_client_get_users_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(users_info);
  g_assert_cmpint(users_info->len, ==, 2);
  SnapdUserInformation *info = users_info->pdata[0];
  g_assert_cmpint(snapd_user_information_get_id(info), ==, 1);
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "alice");
  g_assert_cmpstr(snapd_user_information_get_email(info), ==,
                  "alice@example.com");
  info = users_info->pdata[1];
  g_assert_cmpint(snapd_user_information_get_id(info), ==, 2);
  g_assert_cmpstr(snapd_user_information_get_username(info), ==, "bob");
  g_assert_cmpstr(snapd_user_information_get_email(info), ==,
                  "bob@example.com");

  g_main_loop_quit(data->loop);
}

static void test_get_users_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_account(snapd, "alice@example.com", "alice", "secret");
  mock_snapd_add_account(snapd, "bob@example.com", "bob", "secret");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_users_async(client, NULL, get_users_cb,
                               async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_sections_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_category(snapd, "SECTION1");
  mock_snapd_add_store_category(snapd, "SECTION2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_auto(GStrv) sections = snapd_client_get_sections_sync(client, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(sections);
  g_assert_cmpint(g_strv_length(sections), ==, 2);
  g_assert_cmpstr(sections[0], ==, "SECTION1");
  g_assert_cmpstr(sections[1], ==, "SECTION2");
}

static void get_sections_cb(GObject *object, GAsyncResult *result,
                            gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_auto(GStrv) sections =
      snapd_client_get_sections_finish(SNAPD_CLIENT(object), result, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(sections);
  g_assert_cmpint(g_strv_length(sections), ==, 2);
  g_assert_cmpstr(sections[0], ==, "SECTION1");
  g_assert_cmpstr(sections[1], ==, "SECTION2");

  g_main_loop_quit(data->loop);
}

static void test_get_sections_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_category(snapd, "SECTION1");
  mock_snapd_add_store_category(snapd, "SECTION2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  snapd_client_get_sections_async(client, NULL, get_sections_cb,
                                  async_data_new(loop, snapd));
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(loop);
}

static void test_get_categories_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_category(snapd, "CATEGORY1");
  mock_snapd_add_store_category(snapd, "CATEGORY2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) categories =
      snapd_client_get_categories_sync(client, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(categories);
  g_assert_cmpint(categories->len, ==, 2);
  SnapdCategoryDetails *category_details = categories->pdata[0];
  g_assert_cmpstr(snapd_category_details_get_name(category_details), ==,
                  "CATEGORY1");
  category_details = categories->pdata[1];
  g_assert_cmpstr(snapd_category_details_get_name(category_details), ==,
                  "CATEGORY2");
}

static void get_categories_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_autoptr(GPtrArray) categories =
      snapd_client_get_categories_finish(SNAPD_CLIENT(object), result, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_nonnull(categories);
  g_assert_cmpint(categories->len, ==, 2);
  SnapdCategoryDetails *category_details = categories->pdata[0];
  g_assert_cmpstr(snapd_category_details_get_name(category_details), ==,
                  "CATEGORY1");
  category_details = categories->pdata[1];
  g_assert_cmpstr(snapd_category_details_get_name(category_details), ==,
                  "CATEGORY2");

  g_main_loop_quit(data->loop);
}

static void test_get_categories_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_store_category(snapd, "CATEGORY1");
  mock_snapd_add_store_category(snapd, "CATEGORY2");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  snapd_client_get_categories_async(client, NULL, get_categories_cb,
                                    async_data_new(loop, snapd));
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_main_loop_run(loop);
}

static gint compare_alias_name(gconstpointer a, gconstpointer b) {
  SnapdAlias *alias_a = *((SnapdAlias **)a);
  SnapdAlias *alias_b = *((SnapdAlias **)b);
  return strcmp(snapd_alias_get_name(alias_a), snapd_alias_get_name(alias_b));
}

static void test_aliases_get_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");

  mock_app_add_auto_alias(a, "alias1");

  mock_app_add_manual_alias(a, "alias2", TRUE);

  mock_app_add_auto_alias(a, "alias3");
  mock_app_add_manual_alias(a, "alias3", FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) aliases =
      snapd_client_get_aliases_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(aliases);
  g_assert_cmpint(aliases->len, ==, 3);
  g_ptr_array_sort(aliases, compare_alias_name);
  SnapdAlias *alias = aliases->pdata[0];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias1");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==, SNAPD_ALIAS_STATUS_AUTO);
  g_assert_cmpstr(snapd_alias_get_app_auto(alias), ==, "app");
  g_assert_null(snapd_alias_get_app_manual(alias));
  alias = aliases->pdata[1];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias2");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==, SNAPD_ALIAS_STATUS_MANUAL);
  g_assert_null(snapd_alias_get_app_auto(alias));
  g_assert_cmpstr(snapd_alias_get_app_manual(alias), ==, "app");
  alias = aliases->pdata[2];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias3");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==,
                  SNAPD_ALIAS_STATUS_DISABLED);
  g_assert_cmpstr(snapd_alias_get_app_auto(alias), ==, "app");
  g_assert_null(snapd_alias_get_app_manual(alias));
}

static void get_aliases_cb(GObject *object, GAsyncResult *result,
                           gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) aliases =
      snapd_client_get_aliases_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(aliases);
  g_assert_cmpint(aliases->len, ==, 3);
  g_ptr_array_sort(aliases, compare_alias_name);
  SnapdAlias *alias = aliases->pdata[0];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias1");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==, SNAPD_ALIAS_STATUS_AUTO);
  g_assert_cmpstr(snapd_alias_get_app_auto(alias), ==, "app");
  g_assert_null(snapd_alias_get_app_manual(alias));
  alias = aliases->pdata[1];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias2");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==, SNAPD_ALIAS_STATUS_MANUAL);
  g_assert_null(snapd_alias_get_app_auto(alias));
  g_assert_cmpstr(snapd_alias_get_app_manual(alias), ==, "app");
  alias = aliases->pdata[2];
  g_assert_cmpstr(snapd_alias_get_name(alias), ==, "alias3");
  g_assert_cmpstr(snapd_alias_get_snap(alias), ==, "snap");
  g_assert_cmpstr(snapd_alias_get_command(alias), ==, "snap.app");
  g_assert_cmpint(snapd_alias_get_status(alias), ==,
                  SNAPD_ALIAS_STATUS_DISABLED);
  g_assert_cmpstr(snapd_alias_get_app_auto(alias), ==, "app");
  g_assert_null(snapd_alias_get_app_manual(alias));

  g_main_loop_quit(data->loop);
}

static void test_aliases_get_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");

  mock_app_add_auto_alias(a, "alias1");

  mock_app_add_manual_alias(a, "alias2", TRUE);

  mock_app_add_auto_alias(a, "alias3");
  mock_app_add_manual_alias(a, "alias3", FALSE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_aliases_async(client, NULL, get_aliases_cb,
                                 async_data_new(loop, snapd));
}

static void test_aliases_get_empty(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) aliases =
      snapd_client_get_aliases_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(aliases);
  g_assert_cmpint(aliases->len, ==, 0);
}

static void test_aliases_alias_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_app_find_alias(a, "foo"));
  gboolean result = snapd_client_alias_sync(client, "snap", "app", "foo", NULL,
                                            NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(mock_app_find_alias(a, "foo"));
}

static void alias_cb(GObject *object, GAsyncResult *result,
                     gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockSnap *s = mock_snapd_find_snap(data->snapd, "snap");
  MockApp *a = mock_snap_find_app(s, "app");

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_alias_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_nonnull(mock_app_find_alias(a, "foo"));

  g_main_loop_quit(data->loop);
}

static void test_aliases_alias_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_null(mock_app_find_alias(a, "foo"));
  snapd_client_alias_async(client, "snap", "app", "foo", NULL, NULL, NULL,
                           alias_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_aliases_unalias_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");
  mock_app_add_manual_alias(a, "foo", TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result = snapd_client_unalias_sync(client, "snap", "foo", NULL, NULL,
                                              NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_app_find_alias(a, "foo"));
}

static void unalias_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockSnap *s = mock_snapd_find_snap(data->snapd, "snap");
  MockApp *a = mock_snap_find_app(s, "app");

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_unalias_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_null(mock_app_find_alias(a, "foo"));

  g_main_loop_quit(data->loop);
}

static void test_aliases_unalias_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  mock_snap_add_app(s, "app");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_unalias_async(client, "snap", "foo", NULL, NULL, NULL,
                             unalias_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_aliases_unalias_no_snap_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");
  MockApp *a = mock_snap_add_app(s, "app");
  mock_app_add_manual_alias(a, "foo", TRUE);

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  gboolean result =
      snapd_client_unalias_sync(client, NULL, "foo", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_null(mock_app_find_alias(a, "foo"));
}

static void test_aliases_prefer_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_false(mock_snap_get_preferred(s));
  gboolean result =
      snapd_client_prefer_sync(client, "snap", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_true(mock_snap_get_preferred(s));
}

static void prefer_cb(GObject *object, GAsyncResult *result,
                      gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  MockSnap *s = mock_snapd_find_snap(data->snapd, "snap");

  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_prefer_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_true(mock_snap_get_preferred(s));

  g_main_loop_quit(data->loop);
}

static void test_aliases_prefer_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  MockSnap *s = mock_snapd_add_snap(snapd, "snap");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_assert_false(mock_snap_get_preferred(s));
  snapd_client_prefer_async(client, "snap", NULL, NULL, NULL, prefer_cb,
                            async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_run_snapctl_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) args = g_strsplit("arg1;arg2", ";", -1);
  g_autofree gchar *stdout_output = NULL;
  g_autofree gchar *stderr_output = NULL;
  int exit_code = 0;
  gboolean result =
      snapd_client_run_snapctl2_sync(client, "ABC", args, &stdout_output,
                                     &stderr_output, &exit_code, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(stdout_output, ==, "STDOUT:ABC:arg1:arg2");
  g_assert_cmpstr(stderr_output, ==, "STDERR");
  g_assert_cmpint(exit_code, ==, 0);
}

static void snapctl_cb(GObject *object, GAsyncResult *result,
                       gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autofree gchar *stdout_output = NULL;
  g_autofree gchar *stderr_output = NULL;
  int exit_code = 0;
  g_autoptr(GError) error = NULL;
  gboolean r = snapd_client_run_snapctl2_finish(SNAPD_CLIENT(object), result,
                                                &stdout_output, &stderr_output,
                                                &exit_code, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_cmpstr(stdout_output, ==, "STDOUT:ABC:arg1:arg2");
  g_assert_cmpstr(stderr_output, ==, "STDERR");
  g_assert_cmpint(exit_code, ==, 0);

  g_main_loop_quit(data->loop);
}

static void test_run_snapctl_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) args = g_strsplit("arg1;arg2", ";", -1);
  snapd_client_run_snapctl2_async(client, "ABC", args, NULL, snapctl_cb,
                                  async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_run_snapctl_unsuccessful(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) args = g_strsplit("arg1;arg2", ";", -1);
  g_autofree gchar *stdout_output = NULL;
  g_autofree gchar *stderr_output = NULL;
  int exit_code = 0;
  gboolean result = snapd_client_run_snapctl2_sync(
      client, "return-error", args, &stdout_output, &stderr_output, &exit_code,
      NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(stdout_output, ==, "STDOUT:return-error:arg1:arg2");
  g_assert_cmpstr(stderr_output, ==, "STDERR");
  g_assert_cmpint(exit_code, ==, 1);
}

static void test_run_snapctl_legacy(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) args = g_strsplit("arg1;arg2", ";", -1);
  g_autofree gchar *stdout_output = NULL;
  g_autofree gchar *stderr_output = NULL;
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gboolean result = snapd_client_run_snapctl_sync(
      client, "ABC", args, &stdout_output, &stderr_output, NULL, &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpstr(stdout_output, ==, "STDOUT:ABC:arg1:arg2");
  g_assert_cmpstr(stderr_output, ==, "STDERR");

  /* Unsuccessful exit codes are still reported as errors by the old API */
  g_clear_pointer(&stdout_output, g_free);
  g_clear_pointer(&stderr_output, g_free);
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  result = snapd_client_run_snapctl_sync(client, "return-error", args,
                                         &stdout_output, &stderr_output, NULL,
                                         &error);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert_false(result);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_UNSUCCESSFUL);
}

static void test_download_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GBytes) snap_data =
      snapd_client_download_sync(client, "test", NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap_data);

  g_assert_cmpmem(g_bytes_get_data(snap_data, NULL),
                  g_bytes_get_size(snap_data), "SNAP:name=test", 14);
}

static void download_cb(GObject *object, GAsyncResult *result,
                        gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GBytes) snap_data =
      snapd_client_download_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap_data);

  g_assert_cmpmem(g_bytes_get_data(snap_data, NULL),
                  g_bytes_get_size(snap_data), "SNAP:name=test", 14);

  g_main_loop_quit(data->loop);
}

static void test_download_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_download_async(client, "test", NULL, NULL, NULL, download_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_download_channel_revision(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GBytes) snap_data = snapd_client_download_sync(
      client, "test", "CHANNEL", "REVISION", NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(snap_data);

  g_assert_cmpmem(g_bytes_get_data(snap_data, NULL),
                  g_bytes_get_size(snap_data),
                  "SNAP:name=test:channel=CHANNEL:revision=REVISION", 48);
}

static void test_themes_check_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "installed");
  mock_snapd_set_gtk_theme_status(snapd, "gtktheme2", "available");
  mock_snapd_set_gtk_theme_status(snapd, "gtktheme3", "unavailable");
  mock_snapd_set_icon_theme_status(snapd, "icontheme1", "installed");
  mock_snapd_set_icon_theme_status(snapd, "icontheme2", "available");
  mock_snapd_set_icon_theme_status(snapd, "icontheme3", "unavailable");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme1", "installed");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme2", "available");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme3", "unavailable");

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", "gtktheme2", "gtktheme3", NULL};
  char *icon_themes[] = {"icontheme1", "icontheme2", "icontheme3", NULL};
  char *sound_themes[] = {"soundtheme1", "soundtheme2", "soundtheme3", NULL};
  g_autoptr(GHashTable) gtk_status = NULL;
  g_autoptr(GHashTable) icon_status = NULL;
  g_autoptr(GHashTable) sound_status = NULL;

  gboolean result = snapd_client_check_themes_sync(
      client, gtk_themes, icon_themes, sound_themes, &gtk_status, &icon_status,
      &sound_status, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_nonnull(gtk_status);
  g_assert_nonnull(icon_status);
  g_assert_nonnull(sound_status);

  g_assert_cmpint(g_hash_table_size(gtk_status), ==, 3);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme1")),
                  ==, SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme2")),
                  ==, SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme3")),
                  ==, SNAPD_THEME_STATUS_UNAVAILABLE);

  g_assert_cmpint(g_hash_table_size(icon_status), ==, 3);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme1")), ==,
      SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme2")), ==,
      SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme3")), ==,
      SNAPD_THEME_STATUS_UNAVAILABLE);

  g_assert_cmpint(g_hash_table_size(sound_status), ==, 3);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme1")), ==,
      SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme2")), ==,
      SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme3")), ==,
      SNAPD_THEME_STATUS_UNAVAILABLE);
}

static void check_themes_cb(GObject *object, GAsyncResult *result,
                            gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GHashTable) gtk_status = NULL;
  g_autoptr(GHashTable) icon_status = NULL;
  g_autoptr(GHashTable) sound_status = NULL;
  gboolean res = snapd_client_check_themes_finish(SNAPD_CLIENT(object), result,
                                                  &gtk_status, &icon_status,
                                                  &sound_status, &error);
  g_assert_no_error(error);
  g_assert_true(res);
  g_assert_nonnull(gtk_status);
  g_assert_nonnull(icon_status);
  g_assert_nonnull(sound_status);

  g_assert_cmpint(g_hash_table_size(gtk_status), ==, 3);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme1")),
                  ==, SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme2")),
                  ==, SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(GPOINTER_TO_INT(g_hash_table_lookup(gtk_status, "gtktheme3")),
                  ==, SNAPD_THEME_STATUS_UNAVAILABLE);

  g_assert_cmpint(g_hash_table_size(icon_status), ==, 3);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme1")), ==,
      SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme2")), ==,
      SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(icon_status, "icontheme3")), ==,
      SNAPD_THEME_STATUS_UNAVAILABLE);

  g_assert_cmpint(g_hash_table_size(sound_status), ==, 3);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme1")), ==,
      SNAPD_THEME_STATUS_INSTALLED);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme2")), ==,
      SNAPD_THEME_STATUS_AVAILABLE);
  g_assert_cmpint(
      GPOINTER_TO_INT(g_hash_table_lookup(sound_status, "soundtheme3")), ==,
      SNAPD_THEME_STATUS_UNAVAILABLE);

  g_main_loop_quit(data->loop);
}

static void test_themes_check_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "installed");
  mock_snapd_set_gtk_theme_status(snapd, "gtktheme2", "available");
  mock_snapd_set_gtk_theme_status(snapd, "gtktheme3", "unavailable");
  mock_snapd_set_icon_theme_status(snapd, "icontheme1", "installed");
  mock_snapd_set_icon_theme_status(snapd, "icontheme2", "available");
  mock_snapd_set_icon_theme_status(snapd, "icontheme3", "unavailable");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme1", "installed");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme2", "available");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme3", "unavailable");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", "gtktheme2", "gtktheme3", NULL};
  char *icon_themes[] = {"icontheme1", "icontheme2", "icontheme3", NULL};
  char *sound_themes[] = {"soundtheme1", "soundtheme2", "soundtheme3", NULL};
  snapd_client_check_themes_async(client, gtk_themes, icon_themes, sound_themes,
                                  NULL, check_themes_cb,
                                  async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_themes_install_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "available");
  mock_snapd_set_icon_theme_status(snapd, "icontheme1", "available");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme1", "available");

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", NULL};
  char *icon_themes[] = {"icontheme1", NULL};
  char *sound_themes[] = {"soundtheme1", NULL};
  gboolean result = snapd_client_install_themes_sync(
      client, gtk_themes, icon_themes, sound_themes, NULL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
}

static void install_themes_cb(GObject *object, GAsyncResult *result,
                              gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean res =
      snapd_client_install_themes_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(res);

  g_main_loop_quit(data->loop);
}

static void test_themes_install_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "available");
  mock_snapd_set_icon_theme_status(snapd, "icontheme1", "available");
  mock_snapd_set_sound_theme_status(snapd, "soundtheme1", "available");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", NULL};
  char *icon_themes[] = {"icontheme1", NULL};
  char *sound_themes[] = {"soundtheme1", NULL};
  snapd_client_install_themes_async(
      client, gtk_themes, icon_themes, sound_themes, NULL, NULL, NULL,
      install_themes_cb, async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_themes_install_no_snaps(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "installed");
  mock_snapd_set_icon_theme_status(snapd, "icontheme1", "unavailable");

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", NULL};
  char *icon_themes[] = {"icontheme1", NULL};
  char *sound_themes[] = {NULL};
  gboolean result = snapd_client_install_themes_sync(
      client, gtk_themes, icon_themes, sound_themes, NULL, NULL, NULL, &error);
  g_assert_false(result);
  g_assert_error(error, SNAPD_ERROR, SNAPD_ERROR_BAD_REQUEST);
}

static void test_themes_install_progress(void) {
  InstallProgressData install_progress_data;
  install_progress_data.progress_done = 0;
  install_progress_data.spawn_time = "2017-01-02T11:23:58Z";
  install_progress_data.ready_time = "2017-01-03T00:00:00Z";

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_set_spawn_time(snapd, install_progress_data.spawn_time);
  mock_snapd_set_ready_time(snapd, install_progress_data.ready_time);
  mock_snapd_set_gtk_theme_status(snapd, "gtktheme1", "available");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  char *gtk_themes[] = {"gtktheme1", NULL};
  char *icon_themes[] = {"icontheme1", NULL};
  char *sound_themes[] = {NULL};
  gboolean result = snapd_client_install_themes_sync(
      client, gtk_themes, icon_themes, sound_themes, install_progress_cb,
      &install_progress_data, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpint(install_progress_data.progress_done, >, 0);
}

static void test_get_logs_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third",
                     "cups.cups-browsed", "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) logs =
      snapd_client_get_logs_sync(client, NULL, 0, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(logs);
  g_assert_cmpint(logs->len, ==, 3);
  SnapdLog *log = logs->pdata[0];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 15, 23, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "first");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");
  log = logs->pdata[1];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 16, 0, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "second");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");
  log = logs->pdata[2];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 16, 3, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "third");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");
}

static void get_logs_cb(GObject *object, GAsyncResult *result,
                        gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) logs =
      snapd_client_get_logs_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(logs);
  g_assert_cmpint(logs->len, ==, 3);
  SnapdLog *log = logs->pdata[0];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 15, 23, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "first");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");
  log = logs->pdata[1];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 16, 0, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "second");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");
  log = logs->pdata[2];
  g_assert_true(
      date_matches(snapd_log_get_timestamp(log), 2023, 6, 16, 3, 20, 40));
  g_assert_cmpstr(snapd_log_get_message(log), ==, "third");
  g_assert_cmpstr(snapd_log_get_sid(log), ==, "cups.cups-browsed");
  g_assert_cmpstr(snapd_log_get_pid(log), ==, "1234");

  g_main_loop_quit(data->loop);
}

static void test_get_logs_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third",
                     "cups.cups-browsed", "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_logs_async(client, NULL, 0, NULL, get_logs_cb,
                              async_data_new(loop, snapd));
  g_main_loop_run(loop);
}

static void test_get_logs_names(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first", "snap1.app1",
                     "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second", "snap2.app2",
                     "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third", "snap3.app3",
                     "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_auto(GStrv) names = g_strsplit("snap1.app1;snap3.app3", ";", -1);
  g_autoptr(GPtrArray) logs =
      snapd_client_get_logs_sync(client, names, 0, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(logs);
  g_assert_cmpint(logs->len, ==, 2);
  g_assert_cmpstr(snapd_log_get_sid(logs->pdata[0]), ==, "snap1.app1");
  g_assert_cmpstr(snapd_log_get_sid(logs->pdata[1]), ==, "snap3.app3");
}

static void test_get_logs_limit(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third",
                     "cups.cups-browsed", "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) logs =
      snapd_client_get_logs_sync(client, NULL, 1, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(logs);
  g_assert_cmpint(logs->len, ==, 1);
}

static void test_stress(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  for (gint i = 0; i < 10000; i++) {
    g_autoptr(GError) error = NULL;
    g_autoptr(SnapdSystemInformation) info =
        snapd_client_get_system_information_sync(client, NULL, &error);
    g_assert_no_error(error);
    g_assert_nonnull(info);
    g_assert_cmpstr(snapd_system_information_get_version(info), ==, "VERSION");
  }
}

static void sync_log_cb(SnapdClient *client, SnapdLog *log,
                        gpointer user_data) {
  int *counter = user_data;
  (*counter)++;
}

static void test_follow_logs_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third",
                     "cups.cups-browsed", "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  int counter = 0;
  gboolean result = snapd_client_follow_logs_sync(client, NULL, sync_log_cb,
                                                  &counter, NULL, &error);
  g_assert_no_error(error);
  g_assert_true(result);
  g_assert_cmpint(counter, ==, 3);
}

static void async_log_cb(SnapdClient *client, SnapdLog *log,
                         gpointer user_data) {
  AsyncData *data = user_data;
  data->counter++;
}

static void follow_logs_cb(GObject *object, GAsyncResult *result,
                           gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  gboolean r =
      snapd_client_follow_logs_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_true(r);
  g_assert_cmpint(data->counter, ==, 3);

  g_main_loop_quit(data->loop);
}

static void test_follow_logs_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();
  mock_snapd_add_log(snapd, "2023-06-15T23:20:40Z", "first",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T00:20:40Z", "second",
                     "cups.cups-browsed", "1234");
  mock_snapd_add_log(snapd, "2023-06-16T03:20:40Z", "third",
                     "cups.cups-browsed", "1234");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  AsyncData *data = async_data_new(loop, snapd);
  snapd_client_follow_logs_async(client, NULL, async_log_cb, data, NULL,
                                 follow_logs_cb, data);
  g_main_loop_run(loop);
}

static void test_get_changes_data(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  MockChange *c = mock_snapd_add_change(snapd);

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "snap-names");
  json_builder_begin_array(builder);
  json_builder_add_string_value(builder, "snap1");
  json_builder_add_string_value(builder, "snap2");
  json_builder_add_string_value(builder, "snap3");
  json_builder_end_array(builder);
  json_builder_set_member_name(builder, "refresh-forced");
  json_builder_begin_array(builder);
  json_builder_add_string_value(builder, "snap_forced1");
  json_builder_add_string_value(builder, "snap_forced2");
  json_builder_end_array(builder);
  json_builder_end_object(builder);

  JsonNode *node = json_builder_get_root(builder);

  mock_change_add_data(c, node);
  mock_change_set_kind(c, "auto-refresh");

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GPtrArray) changes = snapd_client_get_changes_sync(
      client, SNAPD_CHANGE_FILTER_ALL, NULL, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(changes);
  g_assert_cmpint(changes->len, ==, 1);
  SnapdAutorefreshChangeData *data = SNAPD_AUTOREFRESH_CHANGE_DATA(
      snapd_change_get_data(SNAPD_CHANGE(changes->pdata[0])));
  g_assert_nonnull(data);
  GStrv snap_names = snapd_autorefresh_change_data_get_snap_names(data);
  g_assert_nonnull(snap_names);
  g_assert_cmpint(g_strv_length(snap_names), ==, 3);
  g_assert_true(g_str_equal(snap_names[0], "snap1"));
  g_assert_true(g_str_equal(snap_names[1], "snap2"));
  g_assert_true(g_str_equal(snap_names[2], "snap3"));

  GStrv refresh_forced = snapd_autorefresh_change_data_get_refresh_forced(data);
  g_assert_nonnull(refresh_forced);
  g_assert_cmpint(g_strv_length(refresh_forced), ==, 2);
  g_assert_true(g_str_equal(refresh_forced[0], "snap_forced1"));
  g_assert_true(g_str_equal(refresh_forced[1], "snap_forced2"));

  json_node_unref(node);
}

/* Notices example
{
    "type":"sync",
    "status-code":200,
    "status":"OK",
    "result":[
        {
            "id":"1",
            "user-id":null,
            "type":"change-update",
            "key":"8473",
            "first-occurred":"2024-03-27T11:34:53.34609455Z",
            "last-occurred":"2024-03-27T11:34:54.847953897Z",
            "last-repeated":"2024-03-27T11:34:54.847953897Z",
            "occurrences":3,
            "last-data":{
                "kind":"install-snap"
            },
            "expire-after":"168h0m0s"
        },{
            "id":"2",
            "user-id":null,
            "type":"change-update",
            "key":"8474",
            "first-occurred":"2024-03-27T16:15:09.871332485Z",
            "last-occurred":"2024-03-27T16:15:43.702284133Z",
            "last-repeated":"2024-03-27T16:15:43.702284133Z",
            "occurrences":3,
            "last-data":{
                "kind":"refresh-snap"
            },
            "expire-after":"168h0m0s"
        }
    ]
} */

void test_notices_events_cb(GObject *object, GAsyncResult *result,
                            gpointer user_data) {
  AsyncData *data = user_data;
  g_autoptr(GError) error = NULL;

  g_autoptr(GPtrArray) notices =
      snapd_client_get_notices_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(notices);
  g_assert_cmpint(notices->len, ==, 2);

  g_autoptr(SnapdNotice) notice1 = g_object_ref(notices->pdata[0]);
  g_autoptr(SnapdNotice) notice2 = g_object_ref(notices->pdata[1]);

  g_assert_cmpstr(snapd_notice_get_id(notice1), ==, "1");
  g_assert_null(snapd_notice_get_user_id(notice1));

  g_assert_cmpint(snapd_notice_get_expire_after(notice1), ==,
                  382 * G_TIME_SPAN_DAY + 4 * G_TIME_SPAN_HOUR +
                      5 * G_TIME_SPAN_MINUTE + 6 * G_TIME_SPAN_SECOND +
                      7 * G_TIME_SPAN_MILLISECOND + 8);

  g_assert_cmpint(snapd_notice_get_repeat_after(notice1), ==,
                  -(382 * G_TIME_SPAN_DAY + 4 * G_TIME_SPAN_HOUR +
                    5 * G_TIME_SPAN_MINUTE + 6 * G_TIME_SPAN_SECOND +
                    7 * G_TIME_SPAN_MILLISECOND + 8));

  g_autoptr(GTimeZone) timezone = g_time_zone_new_utc();

  g_autoptr(GDateTime) date1 =
      g_date_time_new(timezone, 2024, 3, 1, 20, 29, 58);
  g_autoptr(GDateTime) date2 = g_date_time_new(timezone, 2025, 4, 2, 23, 28, 8);
  g_autoptr(GDateTime) date3 = g_date_time_new(timezone, 2026, 5, 3, 22, 20, 7);

  g_assert_true(
      g_date_time_equal(snapd_notice_get_first_occurred2(notice1), date1));
  g_assert_true(
      g_date_time_equal(snapd_notice_get_last_occurred2(notice1), date2));
  g_assert_true(
      g_date_time_equal(snapd_notice_get_last_repeated2(notice1), date3));

  g_assert_true(snapd_notice_get_notice_type(notice1) ==
                SNAPD_NOTICE_TYPE_UNKNOWN);

  g_assert_cmpint(snapd_notice_get_occurrences(notice1), ==, 5);

  GHashTable *notice_data1 = snapd_notice_get_last_data2(notice1);
  g_assert_nonnull(notice_data1);
  g_assert_cmpint(g_hash_table_size(notice_data1), ==, 0);

  g_assert_cmpstr(snapd_notice_get_id(notice2), ==, "2");
  g_assert_cmpstr(snapd_notice_get_user_id(notice2), ==, "67");

#if GLIB_CHECK_VERSION(2, 68, 0)
  g_autoptr(GTimeZone) timezone2 = g_time_zone_new_identifier("01:32");
#else
  g_autoptr(GTimeZone) timezone2 = g_time_zone_new("01:32");
#endif
  g_autoptr(GDateTime) date4 =
      g_date_time_new(timezone2, 2023, 2, 5, 21, 23, 3);
  g_autoptr(GDateTime) date5 =
      g_date_time_new(timezone2, 2023, 2, 5, 21, 23, 3.000123);

  g_assert_true(
      g_date_time_equal(snapd_notice_get_first_occurred2(notice2), date4));
  g_assert_true(
      g_date_time_equal(snapd_notice_get_last_occurred2(notice2), date5));
  g_assert_true(
      g_date_time_equal(snapd_notice_get_last_repeated2(notice2), date4));

  g_assert_cmpint(snapd_notice_get_occurrences(notice2), ==, 1);

  g_assert_true(snapd_notice_get_notice_type(notice2) ==
                SNAPD_NOTICE_TYPE_REFRESH_INHIBIT);

  GHashTable *notice_data2 = snapd_notice_get_last_data2(notice2);
  g_assert_nonnull(notice_data2);
  g_assert_cmpint(g_hash_table_size(notice_data2), ==, 1);
  g_assert_true(g_hash_table_contains(notice_data2, "kind"));
  g_assert_cmpstr(g_hash_table_lookup(notice_data2, "kind"), ==, "change-kind");

  // Test it twice, to ensure that multiple calls do work
  if (data->counter == 0) {
    // this was done with parameters

#if GLIB_CHECK_VERSION(2, 66, 0)
    g_autoptr(GHashTable) parameters =
        g_uri_parse_params(mock_snapd_get_notices_parameters(data->snapd), -1,
                           "&", G_URI_PARAMS_NONE, NULL);

    g_assert_nonnull(parameters);
    g_assert_cmpint(g_hash_table_size(parameters), ==, 6);

    g_assert_true(g_hash_table_contains(parameters, "user-id"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "user-id"), ==,
                    "an_user_id");
    g_assert_true(g_hash_table_contains(parameters, "users"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "users"), ==,
                    "id1, id2, an_utf8_íd");
    g_assert_true(g_hash_table_contains(parameters, "types"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "types"), ==,
                    "type1,type2");
    g_assert_true(g_hash_table_contains(parameters, "keys"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "keys"), ==, "key1,key2");
    g_assert_true(g_hash_table_contains(parameters, "after"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "after"), ==,
                    "2029-03-01T20:29:58.123456+00:00");
    g_assert_true(g_hash_table_contains(parameters, "timeout"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "timeout"), ==, "20000us");
#endif
    data->counter++;
    snapd_client_get_notices_async(SNAPD_CLIENT(object), NULL, 0, NULL,
                                   test_notices_events_cb, data);
  } else {
    // and this one without parameters
    gchar *parameters = mock_snapd_get_notices_parameters(data->snapd);
    g_assert_null(parameters);
    g_main_loop_quit(data->loop);
  }
}

static void test_notices_events(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  AsyncData *data = async_data_new(loop, snapd);
  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  MockNotice *notice =
      mock_snapd_add_notice(snapd, "1", "8473", "refresh-snap");

  mock_notice_set_expire_after(notice, "1y2w3d4h5m6s7ms8us9ns");
  mock_notice_set_repeat_after(notice, "-1y2w3d4h5m6s7ms8µs9ns");

  g_autoptr(GTimeZone) timezone = g_time_zone_new_utc();

  g_autoptr(GDateTime) date1 =
      g_date_time_new(timezone, 2024, 3, 1, 20, 29, 58);
  g_autoptr(GDateTime) date2 = g_date_time_new(timezone, 2025, 4, 2, 23, 28, 8);
  g_autoptr(GDateTime) date3 = g_date_time_new(timezone, 2026, 5, 3, 22, 20, 7);
  mock_notice_set_dates(notice, date1, date2, date3, 5);

  notice = mock_snapd_add_notice(snapd, "2", "8474", "refresh-inhibit");
  mock_notice_set_nanoseconds(notice, 123456);

  mock_notice_set_user_id(notice, "67");

#if GLIB_CHECK_VERSION(2, 68, 0)
  g_autoptr(GTimeZone) timezone2 = g_time_zone_new_identifier("01:32");
#else
  g_autoptr(GTimeZone) timezone2 = g_time_zone_new("01:32");
#endif
  g_autoptr(GDateTime) date4 =
      g_date_time_new(timezone2, 2023, 2, 5, 21, 23, 3);
  mock_notice_set_dates(notice, date4, date4, date4, 1);
  mock_notice_add_data_pair(notice, "kind", "change-kind");

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(GDateTime) date5 =
      g_date_time_new(timezone, 2029, 3, 1, 20, 29, 58.123456789);
  snapd_client_get_notices_with_filters_async(
      client, "an_user_id", "id1, id2, an_utf8_íd", "type1,type2", "key1,key2",
      date5, 20000, NULL, test_notices_events_cb, data);
  g_main_loop_run(loop);
}

void test_notices_minimal_data_events_cb(GObject *object, GAsyncResult *result,
                                         gpointer user_data) {
  AsyncData *data = user_data;
  g_autoptr(GError) error = NULL;

  g_autoptr(GPtrArray) notices =
      snapd_client_get_notices_finish(SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(notices);
  g_assert_cmpint(notices->len, ==, 1);

  g_autoptr(SnapdNotice) notice1 = g_object_ref(notices->pdata[0]);

  g_assert_cmpstr(snapd_notice_get_id(notice1), ==, "1");
  g_assert_null(snapd_notice_get_user_id(notice1));

  g_assert_cmpint(snapd_notice_get_expire_after(notice1), ==, 0);

  g_assert_cmpint(snapd_notice_get_repeat_after(notice1), ==, 0);

  g_assert_null(snapd_notice_get_first_occurred2(notice1));
  g_assert_null(snapd_notice_get_last_occurred2(notice1));
  g_assert_null(snapd_notice_get_last_repeated2(notice1));

  g_assert_true(snapd_notice_get_notice_type(notice1) ==
                SNAPD_NOTICE_TYPE_UNKNOWN);

  g_assert_cmpint(snapd_notice_get_occurrences(notice1), ==, -1);

  GHashTable *notice_data = snapd_notice_get_last_data2(notice1);
  g_assert_nonnull(notice_data);
  g_assert_cmpint(g_hash_table_size(notice_data), ==, 0);

  // Test it twice, to ensure that multiple calls do work
  if (data->counter == 0) {
    data->counter++;
    g_autoptr(GTimeZone) timezone = g_time_zone_new_utc();
    g_autoptr(GDateTime) date5 =
        g_date_time_new(timezone, 2029, 3, 1, 20, 29, 58.123456789);
    g_autoptr(SnapdNotice) noticeTest =
        g_object_new(SNAPD_TYPE_NOTICE, "id", "an-id",
                     "last-occurred-nanoseconds", 12345678, NULL);
    snapd_client_notices_set_after_notice(SNAPD_CLIENT(object), noticeTest);
    snapd_client_get_notices_async(SNAPD_CLIENT(object), date5, 0, NULL,
                                   test_notices_minimal_data_events_cb, data);
  } else {
#if GLIB_CHECK_VERSION(2, 66, 0)
    g_autoptr(GHashTable) parameters =
        g_uri_parse_params(mock_snapd_get_notices_parameters(data->snapd), -1,
                           "&", G_URI_PARAMS_NONE, NULL);

    g_assert_nonnull(parameters);
    g_assert_cmpint(g_hash_table_size(parameters), ==, 1);

    g_assert_true(g_hash_table_contains(parameters, "after"));
    g_assert_cmpstr(g_hash_table_lookup(parameters, "after"), ==,
                    "2029-03-01T20:29:58.012345678+00:00");
#endif
    g_main_loop_quit(data->loop);
  }
}

static void test_notices_events_with_minimal_data(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  AsyncData *data = async_data_new(loop, snapd);
  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  mock_snapd_add_notice(snapd, "1", "8473", "refresh-snap");
  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  snapd_client_get_notices_async(client, NULL, 0, NULL,
                                 test_notices_minimal_data_events_cb, data);
  g_main_loop_run(loop);
}

static void test_notice_comparison(void) {
  g_autoptr(GTimeZone) timezone = g_time_zone_new_utc();

  g_autoptr(GDateTime) date0 = g_date_time_new(timezone, 2023, 5, 3, 22, 20, 7);
  g_autoptr(GDateTime) date1 =
      g_date_time_new(timezone, 2024, 3, 1, 20, 29, 58.45);
  g_autoptr(GDateTime) date2 = g_date_time_new(timezone, 2025, 4, 2, 23, 28, 8);

  g_autoptr(SnapdNotice) notice0 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id1", "last-occurred", date1,
                   "last-occurred-nanoseconds", 123456788, NULL);

  g_autoptr(SnapdNotice) notice1 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id1", "last-occurred", date1,
                   "last-occurred-nanoseconds", 123456789, NULL);
  g_autoptr(SnapdNotice) notice2 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id2", "last-occurred", date1,
                   "last-occurred-nanoseconds", 123456789, NULL);
  g_autoptr(SnapdNotice) notice3 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id3", "last-occurred", date1,
                   "last-occurred-nanoseconds", 123456790, NULL);
  g_autoptr(SnapdNotice) notice4 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id4", "last-occurred", date0,
                   "last-occurred-nanoseconds", 123456789, NULL);
  g_autoptr(SnapdNotice) notice5 =
      g_object_new(SNAPD_TYPE_NOTICE, "id", "id5", "last-occurred", date2,
                   "last-occurred-nanoseconds", 123456789, NULL);

  g_assert_true(snapd_notice_compare_last_occurred(notice1, notice0) == 1);
  g_assert_true(snapd_notice_compare_last_occurred(notice1, notice2) == 0);
  g_assert_true(snapd_notice_compare_last_occurred(notice1, notice3) == -1);
  g_assert_true(snapd_notice_compare_last_occurred(notice1, notice4) == 1);
  g_assert_true(snapd_notice_compare_last_occurred(notice1, notice5) == -1);
}

static void test_error_get_change(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  MockChange *change = mock_snapd_add_change(snapd);
  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  error = NULL;
  g_autoptr(SnapdChange) change1 = snapd_client_get_change_sync(
      client, mock_change_get_id(change), NULL, &error);
  g_assert_nonnull(change1);
  g_assert_null(error);

  g_autoptr(SnapdChange) change2 =
      snapd_client_get_change_sync(client, "aninexistentID", NULL, &error);
  g_assert_null(change2);
  g_assert_nonnull(error);
}

static void test_task_data_field(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  MockChange *change = mock_snapd_add_change(snapd);
  MockTask *task1 = mock_change_add_task(change, "task1");
  mock_task_add_affected_snap(task1, "telegram-desktop");
  mock_task_add_affected_snap(task1, "cups");
  MockTask *task2 = mock_change_add_task(change, "task2");
  mock_task_add_affected_snap(task2, "cups");
  mock_change_add_task(change, "task3");

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autoptr(SnapdChange) change1 = snapd_client_get_change_sync(
      client, mock_change_get_id(change), NULL, &error);
  g_assert_nonnull(change1);
  g_assert_null(error);

  GPtrArray *tasks = snapd_change_get_tasks(change1);
  g_assert_nonnull(tasks);
  g_assert_cmpint(tasks->len, ==, 3);

  SnapdTask *task_1 = tasks->pdata[0];
  g_assert_nonnull(task_1);
  SnapdTask *task_2 = tasks->pdata[1];
  g_assert_nonnull(task_2);
  SnapdTask *task_3 = tasks->pdata[2];
  g_assert_nonnull(task_3);

  SnapdTaskData *data1 = snapd_task_get_data(task_1);
  g_assert_nonnull(data1);
  GStrv affected_snaps1 = snapd_task_data_get_affected_snaps(data1);
  g_assert_nonnull(affected_snaps1);
  g_assert_cmpint(g_strv_length(affected_snaps1), ==, 2);
  g_assert_true(g_str_equal(affected_snaps1[0], "telegram-desktop"));
  g_assert_true(g_str_equal(affected_snaps1[1], "cups"));

  SnapdTaskData *data2 = snapd_task_get_data(task_2);
  g_assert_nonnull(data2);
  GStrv affected_snaps2 = snapd_task_data_get_affected_snaps(data2);
  g_assert_nonnull(affected_snaps2);
  g_assert_cmpint(g_strv_length(affected_snaps2), ==, 1);
  g_assert_true(g_str_equal(affected_snaps2[0], "cups"));

  SnapdTaskData *data3 = snapd_task_get_data(task_3);
  g_assert_null(data3);
}

static void test_get_model_assertion_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autofree gchar *model_assertion =
      snapd_client_get_model_assertion_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(model_assertion);
  g_assert_cmpstr(model_assertion, ==, "type: model\n\nSIGNATURE");
}

static void get_model_assertion_cb(GObject *object, GAsyncResult *result,
                                   gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autofree gchar *model_assertion = snapd_client_get_model_assertion_finish(
      SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(model_assertion);
  g_assert_cmpstr(model_assertion, ==, "type: model\n\nSIGNATURE");

  g_main_loop_quit(data->loop);
}

static void test_get_model_assertion_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  AsyncData *data = async_data_new(loop, snapd);
  snapd_client_get_model_assertion_async(client, NULL, get_model_assertion_cb,
                                         data);
  g_main_loop_run(loop);
}

static void test_get_serial_assertion_sync(void) {
  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  g_autofree gchar *serial_assertion =
      snapd_client_get_serial_assertion_sync(client, NULL, &error);
  g_assert_no_error(error);
  g_assert_nonnull(serial_assertion);
  g_assert_cmpstr(serial_assertion, ==, "type: serial\n\nSIGNATURE");
}

static void get_serial_assertion_cb(GObject *object, GAsyncResult *result,
                                    gpointer user_data) {
  g_autoptr(AsyncData) data = user_data;

  g_autoptr(GError) error = NULL;
  g_autofree gchar *serial_assertion = snapd_client_get_serial_assertion_finish(
      SNAPD_CLIENT(object), result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(serial_assertion);
  g_assert_cmpstr(serial_assertion, ==, "type: serial\n\nSIGNATURE");

  g_main_loop_quit(data->loop);
}

static void test_get_serial_assertion_async(void) {
  g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

  g_autoptr(MockSnapd) snapd = mock_snapd_new();

  g_autoptr(GError) error = NULL;
  g_assert_true(mock_snapd_start(snapd, &error));

  g_autoptr(SnapdClient) client = snapd_client_new();
  snapd_client_set_socket_path(client, mock_snapd_get_socket_path(snapd));

  AsyncData *data = async_data_new(loop, snapd);
  snapd_client_get_serial_assertion_async(client, NULL, get_serial_assertion_cb,
                                          data);
  g_main_loop_run(loop);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/notices/test_task_data_field", test_task_data_field);
  g_test_add_func("/errors/test_error_get_change", test_error_get_change);
  g_test_add_func("/notices/test_notices", test_notices_events);
  g_test_add_func("/notices/test_minimal_data",
                  test_notices_events_with_minimal_data);
  g_test_add_func("/notices/test_notice_comparison", test_notice_comparison);

  g_test_add_func("/socket-closed/before-request",
                  test_socket_closed_before_request);
  g_test_add_func("/socket-closed/after-request",
                  test_socket_closed_after_request);
  g_test_add_func("/socket-closed/reconnect", test_socket_closed_reconnect);
  g_test_add_func("/socket-closed/reconnect-after-failure",
                  test_socket_closed_reconnect_after_failure);
  g_test_add_func("/client/set-socket-path", test_client_set_socket_path);
  g_test_add_func("/user-agent/default", test_user_agent_default);
  g_test_add_func("/user-agent/custom", test_user_agent_custom);
  g_test_add_func("/user-agent/null", test_user_agent_null);
  g_test_add_func("/accept-language/basic", test_accept_language);
  g_test_add_func("/accept-language/empty", test_accept_language_empty);
  g_test_add_func("/allow-interaction/basic", test_allow_interaction);
  g_test_add_func("/maintenance/none", test_maintenance_none);
  g_test_add_func("/maintenance/daemon-restart",
                  test_maintenance_daemon_restart);
  g_test_add_func("/maintenance/system-restart",
                  test_maintenance_system_restart);
  g_test_add_func("/maintenance/unknown", test_maintenance_unknown);
  g_test_add_func("/get-system-information/sync",
                  test_get_system_information_sync);
  g_test_add_func("/get-system-information/async",
                  test_get_system_information_async);
  g_test_add_func("/get-system-information/store",
                  test_get_system_information_store);
  g_test_add_func("/get-system-information/refresh",
                  test_get_system_information_refresh);
  g_test_add_func("/get-system-information/refresh_schedule",
                  test_get_system_information_refresh_schedule);
  g_test_add_func("/get-system-information/confinement_strict",
                  test_get_system_information_confinement_strict);
  g_test_add_func("/get-system-information/confinement_none",
                  test_get_system_information_confinement_none);
  g_test_add_func("/get-system-information/confinement_unknown",
                  test_get_system_information_confinement_unknown);
  g_test_add_func("/login/sync", test_login_sync);
  g_test_add_func("/login/async", test_login_async);
  g_test_add_func("/login/invalid-email", test_login_invalid_email);
  g_test_add_func("/login/invalid-password", test_login_invalid_password);
  g_test_add_func("/login/otp-missing", test_login_otp_missing);
  g_test_add_func("/login/otp-invalid", test_login_otp_invalid);
  g_test_add_func("/login/legacy", test_login_legacy);
  g_test_add_func("/logout/sync", test_logout_sync);
  g_test_add_func("/logout/async", test_logout_async);
  g_test_add_func("/logout/no-auth", test_logout_no_auth);
  g_test_add_func("/get-changes/sync", test_get_changes_sync);
  g_test_add_func("/get-changes/async", test_get_changes_async);
  g_test_add_func("/get-changes/filter-in-progress",
                  test_get_changes_filter_in_progress);
  g_test_add_func("/get-changes/filter-ready", test_get_changes_filter_ready);
  g_test_add_func("/get-changes/filter-snap", test_get_changes_filter_snap);
  g_test_add_func("/get-changes/filter-ready-snap",
                  test_get_changes_filter_ready_snap);
  g_test_add_func("/get-changes/data", test_get_changes_data);
  g_test_add_func("/get-change/sync", test_get_change_sync);
  g_test_add_func("/get-change/async", test_get_change_async);
  g_test_add_func("/abort-change/sync", test_abort_change_sync);
  g_test_add_func("/abort-change/async", test_abort_change_async);
  g_test_add_func("/list/sync", test_list_sync);
  g_test_add_func("/list/async", test_list_async);
  g_test_add_func("/get-snaps/sync", test_get_snaps_sync);
  g_test_add_func("/get_snaps/inhibited", test_get_snaps_inhibited);
  g_test_add_func("/get-snaps/async", test_get_snaps_async);
  g_test_add_func("/get-snaps/filter", test_get_snaps_filter);
  g_test_add_func("/list-one/sync", test_list_one_sync);
  g_test_add_func("/list-one/async", test_list_one_async);
  g_test_add_func("/get-snap/sync", test_get_snap_sync);
  g_test_add_func("/get-snap/async", test_get_snap_async);
  g_test_add_func("/get-snap/types", test_get_snap_types);
  g_test_add_func("/get-snap/optional-fields", test_get_snap_optional_fields);
  g_test_add_func("/get-snap/deprecated-fields",
                  test_get_snap_deprecated_fields);
  g_test_add_func("/get-snap/common-ids", test_get_snap_common_ids);
  g_test_add_func("/get-snap/not-installed", test_get_snap_not_installed);
  g_test_add_func("/get-snap/classic-confinement",
                  test_get_snap_classic_confinement);
  g_test_add_func("/get-snap/devmode-confinement",
                  test_get_snap_devmode_confinement);
  g_test_add_func("/get-snap/daemons", test_get_snap_daemons);
  g_test_add_func("/get-snap/publisher-starred",
                  test_get_snap_publisher_starred);
  g_test_add_func("/get-snap/publisher-verified",
                  test_get_snap_publisher_verified);
  g_test_add_func("/get-snap/publisher-unproven",
                  test_get_snap_publisher_unproven);
  g_test_add_func("/get-snap/publisher-unknown-validation",
                  test_get_snap_publisher_unknown_validation);
  g_test_add_func("/get-snap-conf/sync", test_get_snap_conf_sync);
  g_test_add_func("/get-snap-conf/async", test_get_snap_conf_async);
  g_test_add_func("/get-snap-conf/key-filter", test_get_snap_conf_key_filter);
  g_test_add_func("/get-snap-conf/invalid-key", test_get_snap_conf_invalid_key);
  g_test_add_func("/set-snap-conf/sync", test_set_snap_conf_sync);
  g_test_add_func("/set-snap-conf/async", test_set_snap_conf_async);
  g_test_add_func("/set-snap-conf/invalid", test_set_snap_conf_invalid);
  g_test_add_func("/get-apps/sync", test_get_apps_sync);
  g_test_add_func("/get-apps/async", test_get_apps_async);
  g_test_add_func("/get-apps/services", test_get_apps_services);
  g_test_add_func("/get-apps/filter", test_get_apps_filter);
  g_test_add_func("/icon/sync", test_icon_sync);
  g_test_add_func("/icon/async", test_icon_async);
  g_test_add_func("/icon/not-installed", test_icon_not_installed);
  g_test_add_func("/icon/large", test_icon_large);
  g_test_add_func("/get-assertions/sync", test_get_assertions_sync);
  // g_test_add_func ("/get-assertions/async", test_get_assertions_async);
  g_test_add_func("/get-assertions/body", test_get_assertions_body);
  g_test_add_func("/get-assertions/multiple", test_get_assertions_multiple);
  g_test_add_func("/get-assertions/invalid", test_get_assertions_invalid);
  g_test_add_func("/add-assertions/sync", test_add_assertions_sync);
  // g_test_add_func ("/add-assertions/async", test_add_assertions_async);
  g_test_add_func("/assertions/sync", test_assertions_sync);
  // g_test_add_func ("/assertions/async", test_assertions_async);
  g_test_add_func("/assertions/body", test_assertions_body);
  g_test_add_func("/get-connections/sync", test_get_connections_sync);
  g_test_add_func("/get-connections/async", test_get_connections_async);
  g_test_add_func("/get-connections/empty", test_get_connections_empty);
  g_test_add_func("/get-connections/filter-all",
                  test_get_connections_filter_all);
  g_test_add_func("/get-connections/filter-snap",
                  test_get_connections_filter_snap);
  g_test_add_func("/get-connections/filter-interface",
                  test_get_connections_filter_interface);
  g_test_add_func("/get-connections/attributes",
                  test_get_connections_attributes);
  g_test_add_func("/get-interfaces/sync", test_get_interfaces_sync);
  g_test_add_func("/get-interfaces/async", test_get_interfaces_async);
  g_test_add_func("/get-interfaces/no-snaps", test_get_interfaces_no_snaps);
  g_test_add_func("/get-interfaces/attributes", test_get_interfaces_attributes);
  g_test_add_func("/get-interfaces/legacy", test_get_interfaces_legacy);
  g_test_add_func("/get-interfaces2/sync", test_get_interfaces2_sync);
  g_test_add_func("/get-interfaces2/async", test_get_interfaces2_async);
  g_test_add_func("/get-interfaces2/only-connected",
                  test_get_interfaces2_only_connected);
  g_test_add_func("/get-interfaces2/slots", test_get_interfaces2_slots);
  g_test_add_func("/get-interfaces2/plugs", test_get_interfaces2_plugs);
  g_test_add_func("/get-interfaces2/filter", test_get_interfaces2_filter);
  g_test_add_func("/get-interfaces2/make-label",
                  test_get_interfaces2_make_label);
  g_test_add_func("/connect-interface/sync", test_connect_interface_sync);
  g_test_add_func("/connect-interface/async", test_connect_interface_async);
  g_test_add_func("/connect-interface/progress",
                  test_connect_interface_progress);
  g_test_add_func("/connect-interface/invalid", test_connect_interface_invalid);
  g_test_add_func("/disconnect-interface/sync", test_disconnect_interface_sync);
  g_test_add_func("/disconnect-interface/async",
                  test_disconnect_interface_async);
  g_test_add_func("/disconnect-interface/progress",
                  test_disconnect_interface_progress);
  g_test_add_func("/disconnect-interface/invalid",
                  test_disconnect_interface_invalid);
  g_test_add_func("/find/query", test_find_query);
  g_test_add_func("/find/query-private", test_find_query_private);
  g_test_add_func("/find/query-private/not-logged-in",
                  test_find_query_private_not_logged_in);
  g_test_add_func("/find/bad-query", test_find_bad_query);
  g_test_add_func("/find/network-timeout", test_find_network_timeout);
  g_test_add_func("/find/dns-failure", test_find_dns_failure);
  g_test_add_func("/find/name", test_find_name);
  g_test_add_func("/find/name-private", test_find_name_private);
  g_test_add_func("/find/name-private/not-logged-in",
                  test_find_name_private_not_logged_in);
  g_test_add_func("/find/channels", test_find_channels);
  g_test_add_func("/find/channels-match", test_find_channels_match);
  g_test_add_func("/find/cancel", test_find_cancel);
  g_test_add_func("/find/section", test_find_section);
  g_test_add_func("/find/section-query", test_find_section_query);
  g_test_add_func("/find/section-name", test_find_section_name);
  g_test_add_func("/find/category", test_find_category);
  g_test_add_func("/find/category-query", test_find_category_query);
  g_test_add_func("/find/category-name", test_find_category_name);
  g_test_add_func("/find/scope-narrow", test_find_scope_narrow);
  g_test_add_func("/find/scope-wide", test_find_scope_wide);
  g_test_add_func("/find/common-id", test_find_common_id);
  g_test_add_func("/find/categories", test_find_categories);
  g_test_add_func("/find-refreshable/sync", test_find_refreshable_sync);
  g_test_add_func("/find-refreshable/async", test_find_refreshable_async);
  g_test_add_func("/find-refreshable/no-updates",
                  test_find_refreshable_no_updates);
  g_test_add_func("/install/sync", test_install_sync);
  g_test_add_func("/install/sync-multiple", test_install_sync_multiple);
  g_test_add_func("/install/async", test_install_async);
  g_test_add_func("/install/async-multiple", test_install_async_multiple);
  g_test_add_func("/install/async-failure", test_install_async_failure);
  g_test_add_func("/install/async-cancel", test_install_async_cancel);
  g_test_add_func("/install/async-multiple-cancel-first",
                  test_install_async_multiple_cancel_first);
  g_test_add_func("/install/async-multiple-cancel-last",
                  test_install_async_multiple_cancel_last);
  g_test_add_func("/install/progress", test_install_progress);
  g_test_add_func("/install/needs-classic", test_install_needs_classic);
  g_test_add_func("/install/classic", test_install_classic);
  g_test_add_func("/install/not-classic", test_install_not_classic);
  g_test_add_func("/install/needs-classic-system",
                  test_install_needs_classic_system);
  g_test_add_func("/install/needs-devmode", test_install_needs_devmode);
  g_test_add_func("/install/devmode", test_install_devmode);
  g_test_add_func("/install/dangerous", test_install_dangerous);
  g_test_add_func("/install/jailmode", test_install_jailmode);
  g_test_add_func("/install/channel", test_install_channel);
  g_test_add_func("/install/revision", test_install_revision);
  g_test_add_func("/install/not-available", test_install_not_available);
  g_test_add_func("/install/channel-not-available",
                  test_install_channel_not_available);
  g_test_add_func("/install/revision-not-available",
                  test_install_revision_not_available);
  g_test_add_func("/install/snapd-restart", test_install_snapd_restart);
  g_test_add_func("/install/async-snapd-restart",
                  test_install_async_snapd_restart);
  g_test_add_func("/install/auth-cancelled", test_install_auth_cancelled);
  g_test_add_func("/install-stream/sync", test_install_stream_sync);
  g_test_add_func("/install-stream/async", test_install_stream_async);
  g_test_add_func("/install-stream/progress", test_install_stream_progress);
  g_test_add_func("/install-stream/classic", test_install_stream_classic);
  g_test_add_func("/install-stream/dangerous", test_install_stream_dangerous);
  g_test_add_func("/install-stream/devmode", test_install_stream_devmode);
  g_test_add_func("/install-stream/jailmode", test_install_stream_jailmode);
  g_test_add_func("/try/sync", test_try_sync);
  g_test_add_func("/try/async", test_try_async);
  g_test_add_func("/try/progress", test_try_progress);
  g_test_add_func("/try/not-a-snap", test_try_not_a_snap);
  g_test_add_func("/refresh/sync", test_refresh_sync);
  g_test_add_func("/refresh/async", test_refresh_async);
  g_test_add_func("/refresh/progress", test_refresh_progress);
  g_test_add_func("/refresh/channel", test_refresh_channel);
  g_test_add_func("/refresh/no-updates", test_refresh_no_updates);
  g_test_add_func("/refresh/not-installed", test_refresh_not_installed);
  g_test_add_func("/refresh/not-in-store", test_refresh_not_in_store);
  g_test_add_func("/refresh-all/sync", test_refresh_all_sync);
  g_test_add_func("/refresh-all/async", test_refresh_all_async);
  g_test_add_func("/refresh-all/progress", test_refresh_all_progress);
  g_test_add_func("/refresh-all/no-updates", test_refresh_all_no_updates);
  g_test_add_func("/remove/sync", test_remove_sync);
  g_test_add_func("/remove/async", test_remove_async);
  g_test_add_func("/remove/async-failure", test_remove_async_failure);
  g_test_add_func("/remove/async-cancel", test_remove_async_cancel);
  g_test_add_func("/remove/progress", test_remove_progress);
  g_test_add_func("/remove/not-installed", test_remove_not_installed);
  g_test_add_func("/remove/purge", test_remove_purge);
  g_test_add_func("/enable/sync", test_enable_sync);
  g_test_add_func("/enable/async", test_enable_async);
  g_test_add_func("/enable/progress", test_enable_progress);
  g_test_add_func("/enable/already-enabled", test_enable_already_enabled);
  g_test_add_func("/enable/not-installed", test_enable_not_installed);
  g_test_add_func("/disable/sync", test_disable_sync);
  g_test_add_func("/disable/async", test_disable_async);
  g_test_add_func("/disable/progress", test_disable_progress);
  g_test_add_func("/disable/already-disabled", test_disable_already_disabled);
  g_test_add_func("/disable/not-installed", test_disable_not_installed);
  g_test_add_func("/switch/sync", test_switch_sync);
  g_test_add_func("/switch/async", test_switch_async);
  g_test_add_func("/switch/progress", test_switch_progress);
  g_test_add_func("/switch/not-installed", test_switch_not_installed);
  g_test_add_func("/check-buy/sync", test_check_buy_sync);
  g_test_add_func("/check-buy/async", test_check_buy_async);
  g_test_add_func("/check-buy/no-terms-not-accepted",
                  test_check_buy_terms_not_accepted);
  g_test_add_func("/check-buy/no-payment-methods",
                  test_check_buy_no_payment_methods);
  g_test_add_func("/check-buy/not-logged-in", test_check_buy_not_logged_in);
  g_test_add_func("/buy/sync", test_buy_sync);
  g_test_add_func("/buy/async", test_buy_async);
  g_test_add_func("/buy/not-logged-in", test_buy_not_logged_in);
  g_test_add_func("/buy/not-available", test_buy_not_available);
  g_test_add_func("/buy/terms-not-accepted", test_buy_terms_not_accepted);
  g_test_add_func("/buy/no-payment-methods", test_buy_no_payment_methods);
  g_test_add_func("/buy/invalid-price", test_buy_invalid_price);
  g_test_add_func("/create-user/sync", test_create_user_sync);
  // g_test_add_func ("/create-user/async", test_create_user_async);
  g_test_add_func("/create-user/sudo", test_create_user_sudo);
  g_test_add_func("/create-user/known", test_create_user_known);
  g_test_add_func("/create-users/sync", test_create_users_sync);
  // g_test_add_func ("/create-users/async", test_create_users_async);
  g_test_add_func("/get-users/sync", test_get_users_sync);
  g_test_add_func("/get-users/async", test_get_users_async);
  g_test_add_func("/get-sections/sync", test_get_sections_sync);
  g_test_add_func("/get-sections/async", test_get_sections_async);
  g_test_add_func("/get-categories/sync", test_get_categories_sync);
  g_test_add_func("/get-categories/async", test_get_categories_async);
  g_test_add_func("/aliases/get-sync", test_aliases_get_sync);
  g_test_add_func("/aliases/get-async", test_aliases_get_async);
  g_test_add_func("/aliases/get-empty", test_aliases_get_empty);
  g_test_add_func("/aliases/alias-sync", test_aliases_alias_sync);
  g_test_add_func("/aliases/alias-async", test_aliases_alias_async);
  g_test_add_func("/aliases/unalias-sync", test_aliases_unalias_sync);
  g_test_add_func("/aliases/unalias-async", test_aliases_unalias_async);
  g_test_add_func("/aliases/unalias-no-snap-sync",
                  test_aliases_unalias_no_snap_sync);
  g_test_add_func("/aliases/prefer-sync", test_aliases_prefer_sync);
  g_test_add_func("/aliases/prefer-async", test_aliases_prefer_async);
  g_test_add_func("/run-snapctl/sync", test_run_snapctl_sync);
  g_test_add_func("/run-snapctl/async", test_run_snapctl_async);
  g_test_add_func("/run-snapctl/unsuccessful", test_run_snapctl_unsuccessful);
  g_test_add_func("/run-snapctl/legacy", test_run_snapctl_legacy);
  g_test_add_func("/download/sync", test_download_sync);
  g_test_add_func("/download/async", test_download_async);
  g_test_add_func("/download/channel-revision", test_download_channel_revision);
  g_test_add_func("/themes/check/sync", test_themes_check_sync);
  g_test_add_func("/themes/check/async", test_themes_check_async);
  g_test_add_func("/themes/install/sync", test_themes_install_sync);
  g_test_add_func("/themes/install/async", test_themes_install_async);
  g_test_add_func("/themes/install/no-snaps", test_themes_install_no_snaps);
  g_test_add_func("/themes/install/progress", test_themes_install_progress);
  g_test_add_func("/get-logs/sync", test_get_logs_sync);
  g_test_add_func("/get-logs/async", test_get_logs_async);
  g_test_add_func("/get-logs/names", test_get_logs_names);
  g_test_add_func("/get-logs/limit", test_get_logs_limit);
  g_test_add_func("/follow-logs/sync", test_follow_logs_sync);
  g_test_add_func("/follow-logs/async", test_follow_logs_async);
  g_test_add_func("/get-model-assertion/sync", test_get_model_assertion_sync);
  g_test_add_func("/get-model-assertion/async", test_get_model_assertion_async);
  g_test_add_func("/get-serial-assertion/sync", test_get_serial_assertion_sync);
  g_test_add_func("/get-serial-assertion/async",
                  test_get_serial_assertion_async);
  g_test_add_func("/stress/basic", test_stress);

  return g_test_run();
}
