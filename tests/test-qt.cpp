/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "mock-snapd.h"

#include <QBuffer>
#include <QVariant>
#include <Snapd/Client>
#include <Snapd/Assertion>

#include "test-qt.h"

static void
test_socket_closed_before_request ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    mock_snapd_stop (snapd);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::ConnectionFailed);
}

static void
test_socket_closed_after_request ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_close_on_request (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::ReadFailed);
}

static void
test_client_set_socket_path (void)
{
    QSnapdClient client;
    QString default_path = client.socketPath ();

    client.setSocketPath ("first.sock");
    g_assert_true (client.socketPath () == "first.sock");

    client.setSocketPath ("second.sock");
    g_assert_true (client.socketPath () == "second.sock");

    client.setSocketPath (NULL);
    g_assert_true (client.socketPath () == default_path);
}

static void
test_user_agent_default ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_true (client.userAgent () == "snapd-glib/" VERSION);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, "snapd-glib/" VERSION);
}

static void
test_user_agent_custom ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    client.setUserAgent ("Foo/1.0");
    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, "Foo/1.0");
}

static void
test_user_agent_null ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    client.setUserAgent (NULL);
    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, NULL);
}

static void
test_accept_language ()
{
    g_setenv ("LANG", "en_US.UTF-8", TRUE);
    g_setenv ("LANGUAGE", "en_US:fr", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en-us, en;q=0.9, fr;q=0.8");
}

static void
test_accept_language_empty ()
{
    g_setenv ("LANG", "", TRUE);
    g_setenv ("LANGUAGE", "", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en");
}

static void
test_allow_interaction ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    /* By default, interaction is allowed */
    g_assert_true (client.allowInteraction());

    /* ... which sends the X-Allow-Interaction header with requests */
    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, "true");

    /* If interaction is not allowed, the header is not sent */
    client.setAllowInteraction (false);
    g_assert_false (client.allowInteraction ());
    infoRequest.reset (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, NULL);
}

static void
test_maintenance_none ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdMaintenance> maintenance (client.maintenance ());
    g_assert_null (maintenance);
}

static void
test_maintenance_daemon_restart ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_maintenance (snapd, "daemon-restart", "daemon is restarting");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdMaintenance> maintenance (client.maintenance ());
    g_assert_nonnull (maintenance);
    g_assert_true (maintenance->kind () == QSnapdEnums::MaintenanceKindDaemonRestart);
    g_assert_true (maintenance->message () == "daemon is restarting");
}

static void
test_maintenance_system_restart ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_maintenance (snapd, "system-restart", "system is restarting");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdMaintenance> maintenance (client.maintenance ());
    g_assert_nonnull (maintenance);
    g_assert_true (maintenance->kind () == QSnapdEnums::MaintenanceKindSystemRestart);
    g_assert_true (maintenance->message () == "system is restarting");
}

static void
test_maintenance_unknown ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_maintenance (snapd, "no-such-kind", "MESSAGE");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdMaintenance> maintenance (client.maintenance ());
    g_assert_nonnull (maintenance);
    g_assert_true (maintenance->kind () == QSnapdEnums::MaintenanceKindUnknown);
    g_assert_true (maintenance->message () == "MESSAGE");
}

static void
test_get_system_information_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);
    mock_snapd_set_build_id (snapd, "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
    mock_snapd_add_sandbox_feature (snapd, "backend", "feature1");
    mock_snapd_add_sandbox_feature (snapd, "backend", "feature2");
    mock_snapd_set_refresh_timer (snapd, "00:00~24:00/4");
    mock_snapd_set_refresh_next (snapd, "2018-01-19T13:14:15Z");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_true (systemInformation->buildId () == "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
    g_assert_true (systemInformation->confinement () == QSnapdEnums::SystemConfinementUnknown);
    g_assert_true (systemInformation->kernelVersion () == "KERNEL-VERSION");
    g_assert_true (systemInformation->osId () == "OS-ID");
    g_assert_true (systemInformation->osVersion () == "OS-VERSION");
    g_assert_true (systemInformation->series () == "SERIES");
    g_assert_true (systemInformation->version () == "VERSION");
    g_assert_true (systemInformation->managed ());
    g_assert_true (systemInformation->onClassic ());
    g_assert_true (systemInformation->refreshSchedule ().isNull ());
    g_assert_true (systemInformation->refreshTimer () == "00:00~24:00/4");
    g_assert_true (systemInformation->refreshHold ().isNull ());
    g_assert_true (systemInformation->refreshLast ().isNull ());
    g_assert_true (systemInformation->refreshNext () == QDateTime (QDate (2018, 1, 19), QTime (13, 14, 15), Qt::UTC));
    g_assert_true (systemInformation->mountDirectory () == "/snap");
    g_assert_true (systemInformation->binariesDirectory () == "/snap/bin");
    g_assert_null (systemInformation->store ());
    QHash<QString, QStringList> sandbox_features = systemInformation->sandboxFeatures ();
    g_assert_cmpint (sandbox_features["backend"].count (), ==, 2);
    g_assert_true (sandbox_features["backend"][0] == "feature1");
    g_assert_true (sandbox_features["backend"][1] == "feature2");
}

void
GetSystemInformationHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (request->systemInformation ());
    g_assert_true (systemInformation->confinement () == QSnapdEnums::SystemConfinementUnknown);
    g_assert_true (systemInformation->kernelVersion () == "KERNEL-VERSION");
    g_assert_true (systemInformation->osId () == "OS-ID");
    g_assert_true (systemInformation->osVersion () == "OS-VERSION");
    g_assert_true (systemInformation->series () == "SERIES");
    g_assert_true (systemInformation->version () == "VERSION");
    g_assert_true (systemInformation->managed ());
    g_assert_true (systemInformation->onClassic ());
    g_assert_true (systemInformation->mountDirectory () == "/snap");
    g_assert_true (systemInformation->binariesDirectory () == "/snap/bin");
    g_assert_null (systemInformation->store ());

    g_main_loop_quit (loop);
}

static void
test_get_system_information_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetSystemInformationHandler infoHandler (loop, client.getSystemInformation ());
    QObject::connect (infoHandler.request, &QSnapdGetSystemInformationRequest::complete, &infoHandler, &GetSystemInformationHandler::onComplete);
    infoHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_system_information_store ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_store (snapd, "store");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_true (systemInformation->store () == "store");
}

static void
test_get_system_information_refresh ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_refresh_timer (snapd, "00:00~24:00/4");
    mock_snapd_set_refresh_hold (snapd, "2018-01-20T01:02:03Z");
    mock_snapd_set_refresh_last (snapd, "2018-01-19T01:02:03Z");
    mock_snapd_set_refresh_next (snapd, "2018-01-19T13:14:15Z");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
   client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_true (systemInformation->refreshSchedule ().isNull ());
    g_assert_true (systemInformation->refreshTimer () == "00:00~24:00/4");
    g_assert_true (systemInformation->refreshHold () == QDateTime (QDate (2018, 1, 20), QTime (1, 2, 3), Qt::UTC));
    g_assert_true (systemInformation->refreshLast () == QDateTime (QDate (2018, 1, 19), QTime (1, 2, 3), Qt::UTC));
    g_assert_true (systemInformation->refreshNext () == QDateTime (QDate (2018, 1, 19), QTime (13, 14, 15), Qt::UTC));
}

static void
test_get_system_information_refresh_schedule ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_refresh_schedule (snapd, "00:00-04:59/5:00-10:59/11:00-16:59/17:00-23:59");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_true (systemInformation->refreshSchedule () == "00:00-04:59/5:00-10:59/11:00-16:59/17:00-23:59");
    g_assert_true (systemInformation->refreshTimer ().isNull ());
}

static void
test_get_system_information_confinement_strict ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "strict");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementStrict);
}

static void
test_get_system_information_confinement_none ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "partial");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementPartial);
}

static void
test_get_system_information_confinement_unknown ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "NOT_DEFINED");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementUnknown);
}

static void
test_login_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    g_auto(GStrv) keys = g_strsplit ("KEY1;KEY2", ";", -1);
    mock_account_set_ssh_keys (a, keys);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdUserInformation> userInformation (loginRequest->userInformation ());
    g_assert_cmpint (userInformation->id (), ==, 1);
    g_assert_true (userInformation->email () == "test@example.com");
    g_assert_true (userInformation->username () == "test");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 0);
    g_assert_true (userInformation->authData ()->macaroon () == mock_account_get_macaroon (a));
    g_assert_cmpint (userInformation->authData ()->discharges ().count (), ==, g_strv_length (mock_account_get_discharges (a)));
    for (int i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert_true (userInformation->authData ()->discharges ()[i] == mock_account_get_discharges (a)[i]);
}

void
LoginHandler::onComplete ()
{
    MockAccount *a = mock_snapd_find_account_by_username (snapd, "test");

    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdUserInformation> userInformation (request->userInformation ());
    g_assert_cmpint (userInformation->id (), ==, 1);
    g_assert_true (userInformation->email () == "test@example.com");
    g_assert_true (userInformation->username () == "test");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 0);
    g_assert_true (userInformation->authData ()->macaroon () == mock_account_get_macaroon (a));
    g_assert_cmpint (userInformation->authData ()->discharges ().count (), ==, g_strv_length (mock_account_get_discharges (a)));
    for (int i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert_true (userInformation->authData ()->discharges ()[i] == mock_account_get_discharges (a)[i]);

    g_main_loop_quit (loop);
}

static void
test_login_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    g_auto(GStrv) keys = g_strsplit ("KEY1;KEY2", ";", -1);
    mock_account_set_ssh_keys (a, keys);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    LoginHandler loginHandler (loop, snapd, client.login ("test@example.com", "secret"));
    QObject::connect (loginHandler.request, &QSnapdLoginRequest::complete, &loginHandler, &LoginHandler::onComplete);
    loginHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_login_invalid_email ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("not-an-email", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::AuthDataInvalid);
}

static void
test_login_invalid_password ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "invalid"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_login_otp_missing ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_otp (a, "1234");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::TwoFactorRequired);
}

static void
test_login_otp_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_otp (a, "1234");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret", "0000"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::TwoFactorInvalid);
}

static void
test_get_changes_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:00:00Z");
    MockTask *t = mock_change_add_task (c, "download");
    mock_task_set_progress (t, 65535, 65535);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:00Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:10Z");
    t = mock_change_add_task (c, "install");
    mock_task_set_progress (t, 1, 1);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:10Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:30Z");
    mock_change_set_ready_time (c, "2017-01-02T11:00:30Z");

    c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:15:00Z");
    t = mock_change_add_task (c, "remove");
    mock_task_set_progress (t, 0, 1);
    mock_task_set_spawn_time (t, "2017-01-02T11:15:00Z");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges ());
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 2);

    QScopedPointer<QSnapdChange> change0 (changesRequest->change (0));
    g_assert_true (change0->id () == "1");
    g_assert_true (change0->kind () == "KIND");
    g_assert_true (change0->summary () == "SUMMARY");
    g_assert_true (change0->status () == "Done");
    g_assert_true (change0->ready ());
    g_assert_true (change0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (change0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change0->error ());
    g_assert_cmpint (change0->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change0->task (0));
    g_assert_true (task0->id () == "100");
    g_assert_true (task0->kind () == "download");
    g_assert_true (task0->summary () == "SUMMARY");
    g_assert_true (task0->status () == "Done");
    g_assert_true (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert_true (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change0->task (1));
    g_assert_true (task1->id () == "101");
    g_assert_true (task1->kind () == "install");
    g_assert_true (task1->summary () == "SUMMARY");
    g_assert_true (task1->status () == "Done");
    g_assert_true (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert_true (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert_true (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    QScopedPointer<QSnapdChange> change1 (changesRequest->change (1));
    g_assert_true (change1->id () == "2");
    g_assert_true (change1->kind () == "KIND");
    g_assert_true (change1->summary () == "SUMMARY");
    g_assert_true (change1->status () == "Do");
    g_assert_false (change1->ready ());
    g_assert_true (change1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (change1->readyTime ().isValid ());
    g_assert_null (change1->error ());
    g_assert_cmpint (change1->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task (change1->task (0));
    g_assert_true (task->id () == "200");
    g_assert_true (task->kind () == "remove");
    g_assert_true (task->summary () == "SUMMARY");
    g_assert_true (task->status () == "Do");
    g_assert_true (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 0);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert_true (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (task->readyTime ().isValid ());
}

void
GetChangesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->changeCount (), ==, 2);

    QScopedPointer<QSnapdChange> change0 (request->change (0));
    g_assert_true (change0->id () == "1");
    g_assert_true (change0->kind () == "KIND");
    g_assert_true (change0->summary () == "SUMMARY");
    g_assert_true (change0->status () == "Done");
    g_assert_true (change0->ready ());
    g_assert_true (change0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (change0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change0->error ());
    g_assert_cmpint (change0->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change0->task (0));
    g_assert_true (task0->id () == "100");
    g_assert_true (task0->kind () == "download");
    g_assert_true (task0->summary () == "SUMMARY");
    g_assert_true (task0->status () == "Done");
    g_assert_true (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert_true (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change0->task (1));
    g_assert_true (task1->id () == "101");
    g_assert_true (task1->kind () == "install");
    g_assert_true (task1->summary () == "SUMMARY");
    g_assert_true (task1->status () == "Done");
    g_assert_true (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert_true (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert_true (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    QScopedPointer<QSnapdChange> change1 (request->change (1));
    g_assert_true (change1->id () == "2");
    g_assert_true (change1->kind () == "KIND");
    g_assert_true (change1->summary () == "SUMMARY");
    g_assert_true (change1->status () == "Do");
    g_assert_false (change1->ready ());
    g_assert_true (change1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (change1->readyTime ().isValid ());
    g_assert_null (change1->error ());
    g_assert_cmpint (change1->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task (change1->task (0));
    g_assert_true (task->id () == "200");
    g_assert_true (task->kind () == "remove");
    g_assert_true (task->summary () == "SUMMARY");
    g_assert_true (task->status () == "Do");
    g_assert_true (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 0);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert_true (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (task->readyTime ().isValid ());

    g_main_loop_quit (loop);
}

static void
test_get_changes_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:00:00Z");
    MockTask *t = mock_change_add_task (c, "download");
    mock_task_set_progress (t, 65535, 65535);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:00Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:10Z");
    t = mock_change_add_task (c, "install");
    mock_task_set_progress (t, 1, 1);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:10Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:30Z");
    mock_change_set_ready_time (c, "2017-01-02T11:00:30Z");

    c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:15:00Z");
    t = mock_change_add_task (c, "remove");
    mock_task_set_progress (t, 0, 1);
    mock_task_set_spawn_time (t, "2017-01-02T11:15:00Z");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetChangesHandler changesHandler (loop, snapd, client.getChanges ());
    QObject::connect (changesHandler.request, &QSnapdGetChangesRequest::complete, &changesHandler, &GetChangesHandler::onComplete);
    changesHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_changes_filter_in_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    MockTask *t = mock_change_add_task (c, "foo");
    mock_task_set_status (t, "Done");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "foo");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "foo");
    mock_task_set_status (t, "Done");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges (QSnapdClient::FilterInProgress));
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 1);
    g_assert_true (changesRequest->change (0)->id () == "2");
}

static void
test_get_changes_filter_ready ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    MockTask *t = mock_change_add_task (c, "foo");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "foo");
    mock_task_set_status (t, "Done");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "foo");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges (QSnapdClient::FilterReady));
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 1);
    g_assert_true (changesRequest->change (0)->id () == "2");
}

static void
test_get_changes_filter_snap ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    MockTask *t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap1");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap2");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap3");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges ("snap2"));
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 1);
    g_assert_true (changesRequest->change (0)->id () == "2");
}

static void
test_get_changes_filter_ready_snap ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    MockTask *t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap1");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap2");
    mock_task_set_status (t, "Done");

    c = mock_snapd_add_change (snapd);
    t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap2");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges (QSnapdClient::FilterReady, "snap2"));
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 1);
    g_assert_true (changesRequest->change (0)->id () == "2");
}

static void
test_get_changes_data ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-names");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "snap1");
    json_builder_add_string_value (builder, "snap2");
    json_builder_add_string_value (builder, "snap3");
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "refresh-forced");
    json_builder_begin_array (builder);
    json_builder_add_string_value (builder, "snap_forced1");
    json_builder_add_string_value (builder, "snap_forced2");
    json_builder_end_array (builder);
    json_builder_end_object (builder);

    JsonNode *node = json_builder_get_root (builder);

    mock_change_add_data (c, node);
    mock_change_set_kind (c, "auto-refresh");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));


    QScopedPointer<QSnapdGetChangesRequest> changesRequest (client.getChanges ());
    changesRequest->runSync ();
    g_assert_cmpint (changesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (changesRequest->changeCount (), ==, 1);

    QScopedPointer<QSnapdChange> change (changesRequest->change (0));
    QSnapdChangeData *base_data = change->data ();
    g_assert (base_data != NULL);
    QScopedPointer<QSnapdAutorefreshChangeData> data (dynamic_cast<QSnapdAutorefreshChangeData *>(base_data));
    g_assert (data != NULL);
    QStringList snap_names = data->snapNames ();
    g_assert_cmpint (snap_names.length (), ==, 3);
    g_assert_true (snap_names[0] == "snap1");
    g_assert_true (snap_names[1] == "snap2");
    g_assert_true (snap_names[2] == "snap3");

    QStringList refresh_forced = data->refreshForced ();
    g_assert_cmpint (refresh_forced.length (), ==, 2);
    g_assert_true (refresh_forced[0] == "snap_forced1");
    g_assert_true (refresh_forced[1] == "snap_forced2");

    json_node_unref (node);
}


static void
test_get_change_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:00:00Z");
    MockTask *t = mock_change_add_task (c, "download");
    mock_task_set_progress (t, 65535, 65535);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:00Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:10Z");
    t = mock_change_add_task (c, "install");
    mock_task_set_progress (t, 1, 1);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:10Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:30Z");
    mock_change_set_ready_time (c, "2017-01-02T11:00:30Z");

    c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:15:00Z");
    t = mock_change_add_task (c, "remove");
    mock_task_set_progress (t, 0, 1);
    mock_task_set_spawn_time (t, "2017-01-02T11:15:00Z");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetChangeRequest> changeRequest (client.getChange ("1"));
    changeRequest->runSync ();
    g_assert_cmpint (changeRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (changeRequest->change ());
    g_assert_true (change->id () == "1");
    g_assert_true (change->kind () == "KIND");
    g_assert_true (change->summary () == "SUMMARY");
    g_assert_true (change->status () == "Done");
    g_assert_true (change->ready ());
    g_assert_true (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (change->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change->error ());
    g_assert_cmpint (change->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert_true (task0->id () == "100");
    g_assert_true (task0->kind () == "download");
    g_assert_true (task0->summary () == "SUMMARY");
    g_assert_true (task0->status () == "Done");
    g_assert_true (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert_true (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change->task (1));
    g_assert_true (task1->id () == "101");
    g_assert_true (task1->kind () == "install");
    g_assert_true (task1->summary () == "SUMMARY");
    g_assert_true (task1->status () == "Done");
    g_assert_true (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert_true (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert_true (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
}

void
GetChangeHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (request->change ());
    g_assert_true (change->id () == "1");
    g_assert_true (change->kind () == "KIND");
    g_assert_true (change->summary () == "SUMMARY");
    g_assert_true (change->status () == "Done");
    g_assert_true (change->ready ());
    g_assert_true (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (change->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change->error ());
    g_assert_cmpint (change->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert_true (task0->id () == "100");
    g_assert_true (task0->kind () == "download");
    g_assert_true (task0->summary () == "SUMMARY");
    g_assert_true (task0->status () == "Done");
    g_assert_true (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert_true (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert_true (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change->task (1));
    g_assert_true (task1->id () == "101");
    g_assert_true (task1->kind () == "install");
    g_assert_true (task1->summary () == "SUMMARY");
    g_assert_true (task1->status () == "Done");
    g_assert_true (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert_true (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert_true (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    g_main_loop_quit (loop);
}

static void
test_logout_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test1@example.com", "test1", "secret");
    MockAccount *a = mock_snapd_add_account (snapd, "test2@example.com", "test2", "secret");
    mock_snapd_add_account (snapd, "test3@example.com", "test3", "secret");
    gint64 id = mock_account_get_id (a);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GStrv d = mock_account_get_discharges (a);
    QStringList discharges;
    for (int i = 0; d[i] != NULL; i++)
        discharges << d[i];
    qDebug() << discharges;
    QSnapdAuthData authData (mock_account_get_macaroon (a), discharges);
    client.setAuthData (&authData);

    g_assert_nonnull (mock_snapd_find_account_by_id (snapd, id));
    QScopedPointer<QSnapdLogoutRequest> logoutRequest (client.logout (id));
    logoutRequest->runSync ();
    g_assert_cmpint (logoutRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_account_by_id (snapd, id));
}

void
LogoutHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_account_by_id (snapd, id));

    g_main_loop_quit (loop);
}

static void
test_logout_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test1@example.com", "test1", "secret");
    MockAccount *a = mock_snapd_add_account (snapd, "test2@example.com", "test2", "secret");
    mock_snapd_add_account (snapd, "test3@example.com", "test3", "secret");
    gint64 id = mock_account_get_id (a);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GStrv d = mock_account_get_discharges (a);
    QStringList discharges;
    for (int i = 0; d[i] != NULL; i++)
        discharges << d[i];
    QSnapdAuthData authData (mock_account_get_macaroon (a), discharges);
    client.setAuthData (&authData);

    g_assert_nonnull (mock_snapd_find_account_by_id (snapd, id));
    LogoutHandler logoutHandler (loop, snapd, id, client.logout (id));
    QObject::connect (logoutHandler.request, &QSnapdLogoutRequest::complete, &logoutHandler, &LogoutHandler::onComplete);
    logoutHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_logout_no_auth ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test1@example.com", "test1", "secret");
    MockAccount *a = mock_snapd_add_account (snapd, "test2@example.com", "test2", "secret");
    mock_snapd_add_account (snapd, "test3@example.com", "test3", "secret");
    gint64 id = mock_account_get_id (a);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_account_by_id (snapd, id));
    QScopedPointer<QSnapdLogoutRequest> logoutRequest (client.logout (id));
    logoutRequest->runSync ();
    g_assert_cmpint (logoutRequest->error (), ==, QSnapdRequest::BadRequest);
    g_assert_nonnull (mock_snapd_find_account_by_id (snapd, id));
}

static void
test_get_change_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:00:00Z");
    MockTask *t = mock_change_add_task (c, "download");
    mock_task_set_progress (t, 65535, 65535);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:00Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:10Z");
    t = mock_change_add_task (c, "install");
    mock_task_set_progress (t, 1, 1);
    mock_task_set_status (t, "Done");
    mock_task_set_spawn_time (t, "2017-01-02T11:00:10Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:30Z");
    mock_change_set_ready_time (c, "2017-01-02T11:00:30Z");

    c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:15:00Z");
    t = mock_change_add_task (c, "remove");
    mock_task_set_progress (t, 0, 1);
    mock_task_set_spawn_time (t, "2017-01-02T11:15:00Z");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetChangeHandler changeHandler (loop, snapd, client.getChange ("1"));
    QObject::connect (changeHandler.request, &QSnapdGetChangeRequest::complete, &changeHandler, &GetChangeHandler::onComplete);
    changeHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_abort_change_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_add_task (c, "foo");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdAbortChangeRequest> abortRequest (client.abortChange ("1"));
    abortRequest->runSync ();
    g_assert_cmpint (abortRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (abortRequest->change ());
    g_assert_true (change->ready ());
    g_assert_true (change->status () == "Error");
    g_assert_true (change->error () == "cancelled");
    g_assert_cmpint (change->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert_true (task0->status () == "Error");
}

void
AbortChangeHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (request->change ());
    g_assert_true (change->ready ());
    g_assert_true (change->status () == "Error");
    g_assert_true (change->error () == "cancelled");
    g_assert_cmpint (change->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert_true (task0->status () == "Error");

    g_main_loop_quit (loop);
}

static void
test_abort_change_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_add_task (c, "foo");

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    AbortChangeHandler abortHandler (loop, snapd, client.abortChange ("1"));
    QObject::connect (abortHandler.request, &QSnapdAbortChangeRequest::complete, &abortHandler, &AbortChangeHandler::onComplete);
    abortHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_list_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdListRequest> listRequest (client.list ());
QT_WARNING_POP
    listRequest->runSync ();
    g_assert_cmpint (listRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (listRequest->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (listRequest->snap (0));
    g_assert_true (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (listRequest->snap (1));
    g_assert_true (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (listRequest->snap (2));
    g_assert_true (snap2->name () == "snap3");
}

void
ListHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert_true (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert_true (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (request->snap (2));
    g_assert_true (snap2->name () == "snap3");

    g_main_loop_quit (loop);
}

static void
test_list_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_status (s, "installed");
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    ListHandler listHandler (loop, client.list ());
QT_WARNING_POP
    QObject::connect (listHandler.request, &QSnapdListRequest::complete, &listHandler, &ListHandler::onComplete);
    listHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_snaps_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_status (s, "installed");
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapsRequest> getSnapsRequest (client.getSnaps ());
    getSnapsRequest->runSync ();
    g_assert_cmpint (getSnapsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSnapsRequest->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (getSnapsRequest->snap (0));
    g_assert_true (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (getSnapsRequest->snap (1));
    g_assert_true (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (getSnapsRequest->snap (2));
    g_assert_true (snap2->name () == "snap3");
}

void
GetSnapsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert_true (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert_true (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (request->snap (2));
    g_assert_true (snap2->name () == "snap3");

    g_main_loop_quit (loop);
}

static void
test_get_snaps_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_status (s, "installed");
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetSnapsHandler getSnapsHandler (loop, client.getSnaps ());
    QObject::connect (getSnapsHandler.request, &QSnapdGetSnapsRequest::complete, &getSnapsHandler, &GetSnapsHandler::onComplete);
    getSnapsHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_snaps_filter ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_status (s, "installed");
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapsRequest> getSnapsRequest (client.getSnaps (QSnapdClient::IncludeInactive, QStringList ("snap1")));
    getSnapsRequest->runSync ();
    g_assert_cmpint (getSnapsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSnapsRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (getSnapsRequest->snap (0));
    g_assert_true (snap0->name () == "snap1");
    g_assert_true (snap0->status () == QSnapdEnums::SnapStatusInstalled);
    QScopedPointer<QSnapdSnap> snap1 (getSnapsRequest->snap (1));
    g_assert_true (snap1->status () == QSnapdEnums::SnapStatusActive);
}

static void
test_list_one_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
QT_WARNING_POP
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_cmpint (snap->categoryCount (), ==, 0);
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert_true (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert_true (snap->hold ().isNull ());
    g_assert_true (snap->icon () == "ICON");
    g_assert_true (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_null (snap->license ());
    g_assert_cmpint (snap->mediaCount (), ==, 0);
    g_assert_null (snap->mountedFrom ());
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert_true (snap->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->storeUrl ());
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert_true (snap->version () == "VERSION");
    g_assert_null (snap->website ());
}

void
ListOneHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (request->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_null (snap->base ());
    g_assert_null (snap->broken ());
    g_assert_cmpint (snap->categoryCount (), ==, 0);
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert_true (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert_true (snap->hold ().isNull ());
    g_assert_true (snap->icon () == "ICON");
    g_assert_true (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_cmpint (snap->mediaCount (), ==, 0);
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert_true (snap->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->storeUrl ());
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert_true (snap->version () == "VERSION");
    g_assert_null (snap->website ());

    g_main_loop_quit (loop);
}

static void
test_list_one_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    ListOneHandler listOneHandler (loop, client.listOne ("snap"));
QT_WARNING_POP
    QObject::connect (listOneHandler.request, &QSnapdListOneRequest::complete, &listOneHandler, &ListOneHandler::onComplete);
    listOneHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_snap_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_cmpint (snap->categoryCount (), ==, 0);
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert_true (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert_true (snap->hold ().isNull ());
    g_assert_true (snap->icon () == "ICON");
    g_assert_true (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_null (snap->license ());
    g_assert_cmpint (snap->mediaCount (), ==, 0);
    g_assert_null (snap->mountedFrom ());
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert_true (snap->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->storeUrl ());
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert_true (snap->version () == "VERSION");
    g_assert_null (snap->website ());
}

void
GetSnapHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (request->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_null (snap->base ());
    g_assert_null (snap->broken ());
    g_assert_cmpint (snap->categoryCount (), ==, 0);
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert_true (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert_true (snap->hold ().isNull ());
    g_assert_true (snap->icon () == "ICON");
    g_assert_true (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_cmpint (snap->mediaCount (), ==, 0);
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert_true (snap->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->storeUrl ());
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert_true (snap->version () == "VERSION");
    g_assert_null (snap->website ());

    g_main_loop_quit (loop);
}

static void
test_get_snap_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetSnapHandler getSnapHandler (loop, client.getSnap ("snap"));
    QObject::connect (getSnapHandler.request, &QSnapdGetSnapRequest::complete, &getSnapHandler, &GetSnapHandler::onComplete);
    getSnapHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_snap_types ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "kernel");
    mock_snap_set_type (s, "kernel");
    s = mock_snapd_add_snap (snapd, "gadget");
    mock_snap_set_type (s, "gadget");
    s = mock_snapd_add_snap (snapd, "os");
    mock_snap_set_type (s, "os");
    s = mock_snapd_add_snap (snapd, "core");
    mock_snap_set_type (s, "core");
    s = mock_snapd_add_snap (snapd, "base");
    mock_snap_set_type (s, "base");
    s = mock_snapd_add_snap (snapd, "snapd");
    mock_snap_set_type (s, "snapd");
    s = mock_snapd_add_snap (snapd, "app");
    mock_snap_set_type (s, "app");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getKernelSnapRequest (client.getSnap ("kernel"));
    getKernelSnapRequest->runSync ();
    g_assert_cmpint (getKernelSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getKernelSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeKernel);
    QScopedPointer<QSnapdGetSnapRequest> getGadgetSnapRequest (client.getSnap ("gadget"));
    getGadgetSnapRequest->runSync ();
    g_assert_cmpint (getGadgetSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getGadgetSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeGadget);
    QScopedPointer<QSnapdGetSnapRequest> getOsSnapRequest (client.getSnap ("os"));
    getOsSnapRequest->runSync ();
    g_assert_cmpint (getOsSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getOsSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeOperatingSystem);
    QScopedPointer<QSnapdGetSnapRequest> getCoreSnapRequest (client.getSnap ("core"));
    getCoreSnapRequest->runSync ();
    g_assert_cmpint (getCoreSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getCoreSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeCore);
    QScopedPointer<QSnapdGetSnapRequest> getBaseSnapRequest (client.getSnap ("base"));
    getBaseSnapRequest->runSync ();
    g_assert_cmpint (getBaseSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getBaseSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeBase);
    QScopedPointer<QSnapdGetSnapRequest> getSnapdSnapRequest (client.getSnap ("snapd"));
    getSnapdSnapRequest->runSync ();
    g_assert_cmpint (getSnapdSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSnapdSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeSnapd);
    QScopedPointer<QSnapdGetSnapRequest> getAppSnapRequest (client.getSnap ("app"));
    getAppSnapRequest->runSync ();
    g_assert_cmpint (getAppSnapRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAppSnapRequest->snap ()->snapType (), ==, QSnapdEnums::SnapTypeApp);
}

static void
test_get_snap_optional_fields ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    mock_app_add_auto_alias (a, "app2");
    mock_app_add_auto_alias (a, "app3");
    mock_app_set_desktop_file (a, "/var/lib/snapd/desktop/applications/app.desktop");
    mock_snap_set_base (s, "BASE");
    mock_snap_set_broken (s, "BROKEN");
    mock_snap_set_confinement (s, "classic");
    mock_snap_set_devmode (s, TRUE);
    mock_snap_set_hold (s, "2315-06-19T13:00:37Z");
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    mock_snap_set_installed_size (s, 1024);
    mock_snap_set_jailmode (s, TRUE);
    mock_snap_set_trymode (s, TRUE);
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_website (s, "WEBSITE");
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_license (s, "LICENSE");
    mock_snap_set_mounted_from (s, "MOUNTED-FROM");
    mock_snap_set_store_url (s, "https://snapcraft.io/snap");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_tracking_channel (s, "CHANNEL");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 1);
    QScopedPointer<QSnapdApp> app (snap->app (0));
    g_assert_true (app->name () == "app");
    g_assert_true (app->snap () == "snap");
    g_assert_true (app->commonId ().isNull ());
    g_assert_cmpint (app->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (app->enabled ());
    g_assert_false (app->active ());
    g_assert_true (app->desktopFile () == "/var/lib/snapd/desktop/applications/app.desktop");
    g_assert_true (snap->base () == "BASE");
    g_assert_true (snap->broken () == "BROKEN");
    g_assert_true (snap->channel () == "CHANNEL");
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
    g_assert_true (snap->contact () == "CONTACT");
    g_assert_true (snap->website () == "WEBSITE");
    g_assert_true (snap->description () == "DESCRIPTION");
    g_assert_true (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_true (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    QDateTime date = QDateTime (QDate (2315, 6, 19), QTime (13, 00, 37), Qt::UTC);
    g_assert_true (snap->hold () == date);
    g_assert_true (snap->icon () == "ICON");
    g_assert_true (snap->id () == "ID");
    date = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    g_assert_true (snap->installDate () == date);
    g_assert_cmpint (snap->installedSize (), ==, 1024);
    g_assert_true (snap->jailmode ());
    g_assert_true (snap->license () == "LICENSE");
    g_assert_cmpint (snap->mediaCount (), ==, 0);
    g_assert_true (snap->mountedFrom () == "MOUNTED-FROM");
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert_true (snap->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_true (snap->storeUrl () == "https://snapcraft.io/snap");
    g_assert_true (snap->summary () == "SUMMARY");
    g_assert_true (snap->trackingChannel () == "CHANNEL");
    g_assert_true (snap->trymode ());
    g_assert_true (snap->version () == "VERSION");
}

static void
test_get_snap_deprecated_fields ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_true (snap->developer () == "PUBLISHER-USERNAME");
QT_WARNING_POP
}

static void
test_get_snap_common_ids ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app1");
    mock_app_set_common_id (a, "ID1");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_common_id (a, "ID2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->commonIds ().count (), ==, 2);
    g_assert_true (snap->commonIds ()[0] == "ID1");
    g_assert_true (snap->commonIds ()[1] == "ID2");
    g_assert_cmpint (snap->appCount (), ==, 2);
    QScopedPointer<QSnapdApp> app1 (snap->app (0));
    g_assert_true (app1->name () == "app1");
    g_assert_true (app1->commonId () == "ID1");
    QScopedPointer<QSnapdApp> app2 (snap->app (1));
    g_assert_true (app2->name () == "app2");
    g_assert_true (app2->commonId () == "ID2");
}

static void
test_get_snap_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NotFound);
}

static void
test_get_snap_classic_confinement ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
}

static void
test_get_snap_devmode_confinement ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementDevmode);
}

static void
test_get_snap_daemons ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app1");
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
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 6);
    QScopedPointer<QSnapdApp> app1 (snap->app (0));
    g_assert_cmpint (app1->daemonType (), ==, QSnapdEnums::DaemonTypeSimple);
    QScopedPointer<QSnapdApp> app2 (snap->app (1));
    g_assert_cmpint (app2->daemonType (), ==, QSnapdEnums::DaemonTypeForking);
    QScopedPointer<QSnapdApp> app3 (snap->app (2));
    g_assert_cmpint (app3->daemonType (), ==, QSnapdEnums::DaemonTypeOneshot);
    QScopedPointer<QSnapdApp> app4 (snap->app (3));
    g_assert_cmpint (app4->daemonType (), ==, QSnapdEnums::DaemonTypeNotify);
    QScopedPointer<QSnapdApp> app5 (snap->app (4));
    g_assert_cmpint (app5->daemonType (), ==, QSnapdEnums::DaemonTypeDbus);
    QScopedPointer<QSnapdApp> app6 (snap->app (5));
    g_assert_cmpint (app6->daemonType (), ==, QSnapdEnums::DaemonTypeUnknown);
}

static void
test_get_snap_publisher_starred ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_publisher_validation (s, "starred");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationStarred);
}

static void
test_get_snap_publisher_verified ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_publisher_validation (s, "verified");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationVerified);
}

static void
test_get_snap_publisher_unproven ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_publisher_validation (s, "unproven");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnproven);
}

static void
test_get_snap_publisher_unknown_validation ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_publisher_validation (s, "NOT-A-VALIDATION");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapRequest> getSnapRequest (client.getSnap ("snap"));
    getSnapRequest->runSync ();
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (getSnapRequest->snap ());
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
}

static void
setup_get_snap_conf (MockSnapd *snapd)
{
    MockSnap *s = mock_snapd_add_snap (snapd, "core");
    mock_snap_set_conf (s, "string-key", "\"value\"");
    mock_snap_set_conf (s, "int-key", "42");
    mock_snap_set_conf (s, "bool-key", "true");
    mock_snap_set_conf (s, "number-key", "1.25");
    mock_snap_set_conf (s, "array-key", "[ 1, \"two\", 3.25 ]");
    mock_snap_set_conf (s, "object-key", "{\"name\": \"foo\", \"value\": 42}");
}

static void
check_get_snap_conf_result (QSnapdGetSnapConfRequest &request)
{
    QScopedPointer<QHash<QString, QVariant>> configuration (request.configuration ());
    g_assert_cmpint (configuration->size (), ==, 6);
    g_assert_true (configuration->value ("string-key") == "value");
    g_assert_true (configuration->value ("int-key") == 42);
    g_assert_true (configuration->value ("bool-key") == true);
    g_assert_true (configuration->value ("number-key") == 1.25);
}

static void
test_get_snap_conf_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_snap_conf (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapConfRequest> getSnapConfRequest (client.getSnapConf ("system"));
    getSnapConfRequest->runSync ();
    g_assert_cmpint (getSnapConfRequest->error (), ==, QSnapdRequest::NoError);

    check_get_snap_conf_result (*getSnapConfRequest);
}

void
GetSnapConfHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    check_get_snap_conf_result (*request);

    g_main_loop_quit (loop);
}

static void
test_get_snap_conf_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_snap_conf (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetSnapConfHandler getSnapConfHandler (loop, client.getSnapConf ("system"));
    QObject::connect (getSnapConfHandler.request, &QSnapdGetSnapConfRequest::complete, &getSnapConfHandler, &GetSnapConfHandler::onComplete);
    getSnapConfHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_snap_conf_key_filter ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "core");
    mock_snap_set_conf (s, "string-key", "\"value\"");
    mock_snap_set_conf (s, "int-key", "42");
    mock_snap_set_conf (s, "bool-key", "true");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapConfRequest> getSnapConfRequest (client.getSnapConf ("system", QStringList ("int-key")));
    getSnapConfRequest->runSync ();
    g_assert_cmpint (getSnapConfRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QHash<QString, QVariant>> configuration (getSnapConfRequest->configuration ());
    g_assert_cmpint (configuration->size (), ==, 1);
    g_assert_true (configuration->value ("int-key") == 42);
}

static void
test_get_snap_conf_invalid_key ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "core");
    mock_snap_set_conf (s, "string-key", "\"value\"");
    mock_snap_set_conf (s, "int-key", "42");
    mock_snap_set_conf (s, "bool-key", "true");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSnapConfRequest> getSnapConfRequest (client.getSnapConf ("system", QStringList ("invalid-key")));
    getSnapConfRequest->runSync ();
    g_assert_cmpint (getSnapConfRequest->error (), ==, QSnapdRequest::OptionNotFound);
}

static QHash<QString, QVariant>
setup_set_snap_conf (MockSnapd *snapd)
{
    mock_snapd_add_snap (snapd, "core");

    QHash<QString, QVariant> configuration;
    configuration["string-key"] = "value";
    configuration["int-key"] = (qlonglong) 42;
    configuration["bool-key"] = true;
    configuration["number-key"] = 1.25;
    configuration["array-key"] = QVariantList () << 1 << "two" << 3.25;
    QHash<QString, QVariant> object;
    object["name"] = "foo";
    object["value"] = 42;
    configuration["object-key"] = object;

    return configuration;
}

static void
check_set_snap_conf_result (MockSnapd *snapd)
{
    MockSnap *snap = mock_snapd_find_snap (snapd, "core");
    g_assert_cmpint (mock_snap_get_conf_count (snap), ==, 6);
    g_assert_cmpstr (mock_snap_get_conf (snap, "string-key"), ==, "\"value\"");
    g_assert_cmpstr (mock_snap_get_conf (snap, "int-key"), ==, "42");
    g_assert_cmpstr (mock_snap_get_conf (snap, "bool-key"), ==, "true");
    g_assert_cmpstr (mock_snap_get_conf (snap, "number-key"), ==, "1.25");
    g_assert_cmpstr (mock_snap_get_conf (snap, "array-key"), ==, "[1,\"two\",3.25]");
    g_assert_cmpstr (mock_snap_get_conf (snap, "object-key"), ==, "{\"name\":\"foo\",\"value\":42}");
}

static void
test_set_snap_conf_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    QHash<QString, QVariant> configuration = setup_set_snap_conf (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdSetSnapConfRequest> setSnapConfRequest (client.setSnapConf ("system", configuration));
    setSnapConfRequest->runSync ();
    g_assert_cmpint (setSnapConfRequest->error (), ==, QSnapdRequest::NoError);

    check_set_snap_conf_result (snapd);
}

void
SetSnapConfHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    check_set_snap_conf_result (snapd);

    g_main_loop_quit (loop);
}

static void
test_set_snap_conf_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    QHash<QString, QVariant> configuration = setup_set_snap_conf (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    SetSnapConfHandler setSnapConfHandler (loop, snapd, client.setSnapConf ("system", configuration));
    QObject::connect (setSnapConfHandler.request, &QSnapdSetSnapConfRequest::complete, &setSnapConfHandler, &SetSnapConfHandler::onComplete);
    setSnapConfHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_set_snap_conf_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "core");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QHash<QString, QVariant> configuration;
    configuration["string-value"] = "value";
    QScopedPointer<QSnapdSetSnapConfRequest> setSnapConfRequest (client.setSnapConf ("invalid", configuration));
    setSnapConfRequest->runSync ();
    g_assert_cmpint (setSnapConfRequest->error (), ==, QSnapdRequest::NotFound);
}

static void
test_get_apps_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app1");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_desktop_file (a, "foo.desktop");
    a = mock_snap_add_app (s, "app3");
    mock_app_set_daemon (a, "simple");
    mock_app_set_active (a, TRUE);
    mock_app_set_enabled (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAppsRequest> appsRequest (client.getApps ());
    appsRequest->runSync ();
    g_assert_cmpint (appsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (appsRequest->appCount (), ==, 3);
    g_assert_true (appsRequest->app (0)->name () == "app1");
    g_assert_true (appsRequest->app (0)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (0)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (appsRequest->app (0)->active ());
    g_assert_false (appsRequest->app (0)->enabled ());
    g_assert_true (appsRequest->app (1)->name () == "app2");
    g_assert_true (appsRequest->app (1)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (1)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (appsRequest->app (1)->active ());
    g_assert_false (appsRequest->app (1)->enabled ());
    g_assert_true (appsRequest->app (2)->name () == "app3");
    g_assert_true (appsRequest->app (2)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (2)->daemonType (), ==, QSnapdEnums::DaemonTypeSimple);
    g_assert_true (appsRequest->app (2)->active ());
    g_assert_true (appsRequest->app (2)->enabled ());
}

void
GetAppsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->appCount (), ==, 3);
    g_assert_true (request->app (0)->name () == "app1");
    g_assert_true (request->app (0)->snap () == "snap");
    g_assert_cmpint (request->app (0)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (request->app (0)->active ());
    g_assert_false (request->app (0)->enabled ());
    g_assert_true (request->app (1)->name () == "app2");
    g_assert_true (request->app (1)->snap () == "snap");
    g_assert_cmpint (request->app (1)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (request->app (1)->active ());
    g_assert_false (request->app (1)->enabled ());
    g_assert_true (request->app (2)->name () == "app3");
    g_assert_true (request->app (2)->snap () == "snap");
    g_assert_cmpint (request->app (2)->daemonType (), ==, QSnapdEnums::DaemonTypeSimple);
    g_assert_true (request->app (2)->active ());
    g_assert_true (request->app (2)->enabled ());

    g_main_loop_quit (loop);
}

static void
test_get_apps_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app1");
    a = mock_snap_add_app (s, "app2");
    mock_app_set_desktop_file (a, "foo.desktop");
    a = mock_snap_add_app (s, "app3");
    mock_app_set_daemon (a, "simple");
    mock_app_set_active (a, TRUE);
    mock_app_set_enabled (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetAppsHandler getAppsHandler (loop, client.getApps ());
    QObject::connect (getAppsHandler.request, &QSnapdGetAppsRequest::complete, &getAppsHandler, &GetAppsHandler::onComplete);
    getAppsHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_apps_services ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_add_app (s, "app1");
    MockApp *a = mock_snap_add_app (s, "app2");
    mock_app_set_daemon (a, "simple");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAppsRequest> appsRequest (client.getApps (QSnapdClient::SelectServices));
    appsRequest->runSync ();
    g_assert_cmpint (appsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (appsRequest->appCount (), ==, 1);
    g_assert_true (appsRequest->app (0)->name () == "app2");
}

static void
test_get_apps_filter ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_app (s, "app1");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_add_app (s, "app2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAppsRequest> appsRequest (client.getApps ("snap1"));
    appsRequest->runSync ();
    g_assert_cmpint (appsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (appsRequest->appCount (), ==, 1);
    g_assert_true (appsRequest->app (0)->snap () == "snap1");
    g_assert_true (appsRequest->app (0)->name () == "app1");
}

static void
test_icon_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    g_autoptr(GBytes) icon_data = g_bytes_new ("ICON-DATA", 9);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetIconRequest> getIconRequest (client.getIcon ("snap"));
    getIconRequest->runSync ();
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdIcon> icon (getIconRequest->icon ());
    g_assert_true (icon->mimeType () == "image/png");
    QByteArray data = icon->data ();
    g_assert_cmpmem (data.data (), data.size (), "ICON-DATA", 9);
}

void
GetIconHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdIcon> icon (request->icon ());
    g_assert_true (icon->mimeType () == "image/png");
    QByteArray data = icon->data ();
    g_assert_cmpmem (data.data (), data.size (), "ICON-DATA", 9);

    g_main_loop_quit (loop);
}

static void
test_icon_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    g_autoptr(GBytes) icon_data = g_bytes_new ("ICON-DATA", 9);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetIconHandler getIconHandler (loop, client.getIcon ("snap"));
    QObject::connect (getIconHandler.request, &QSnapdGetIconRequest::complete, &getIconHandler, &GetIconHandler::onComplete);
    getIconHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_icon_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetIconRequest> getIconRequest (client.getIcon ("snap"));
    getIconRequest->runSync ();
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::NotFound);
}

static void
test_icon_large ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    gsize icon_buffer_length = 1048576;
    g_autofree gchar *icon_buffer = (gchar *) g_malloc (icon_buffer_length);
    for (gsize i = 0; i < icon_buffer_length; i++)
        icon_buffer[i] = i % 255;
    g_autoptr(GBytes) icon_data = g_bytes_new (icon_buffer, icon_buffer_length);
    mock_snap_set_icon_data (s, "image/png", icon_data);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetIconRequest> getIconRequest (client.getIcon ("snap"));
    getIconRequest->runSync ();
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdIcon> icon (getIconRequest->icon ());
    g_assert_true (icon->mimeType () == "image/png");
    QByteArray data = icon->data ();
    g_assert_cmpmem (data.data (), data.size (), icon_buffer, icon_buffer_length);
}

static void
test_get_assertions_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_assertion (snapd,
                              "type: account\n"
                              "list-header:\n"
                              "  - list-value\n"
                              "map-header:\n"
                              "  map-value: foo\n"
                              "\n"
                              "SIGNATURE");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAssertionsRequest> getAssertionsRequest (client.getAssertions ("account"));
    getAssertionsRequest->runSync ();
    g_assert_cmpint (getAssertionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAssertionsRequest->assertions ().count (), ==, 1);
    g_assert_true (getAssertionsRequest->assertions ()[0] == "type: account\n"
                                                        "list-header:\n"
                                                        "  - list-value\n"
                                                        "map-header:\n"
                                                        "  map-value: foo\n"
                                                        "\n"
                                                        "SIGNATURE");
}

static void
test_get_assertions_body ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_assertion (snapd,
                              "type: account\n"
                              "body-length: 4\n"
                              "\n"
                              "BODY\n"
                              "\n"
                              "SIGNATURE");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAssertionsRequest> getAssertionsRequest (client.getAssertions ("account"));
    getAssertionsRequest->runSync ();
    g_assert_cmpint (getAssertionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAssertionsRequest->assertions ().count (), ==, 1);
    g_assert_true (getAssertionsRequest->assertions()[0] == "type: account\n"
                                                        "body-length: 4\n"
                                                        "\n"
                                                        "BODY\n"
                                                        "\n"
                                                        "SIGNATURE");
}

static void
test_get_assertions_multiple ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
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
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAssertionsRequest> getAssertionsRequest (client.getAssertions ("account"));
    getAssertionsRequest->runSync ();
    g_assert_cmpint (getAssertionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAssertionsRequest->assertions ().count (), ==, 3);
    g_assert_true (getAssertionsRequest->assertions ()[0] == "type: account\n"
                                                        "\n"
                                                        "SIGNATURE1");
    g_assert_true (getAssertionsRequest->assertions ()[1] == "type: account\n"
                                                        "body-length: 4\n"
                                                        "\n"
                                                        "BODY\n"
                                                        "\n"
                                                        "SIGNATURE2");
    g_assert_true (getAssertionsRequest->assertions ()[2] == "type: account\n"
                                                        "\n"
                                                        "SIGNATURE3");
}

static void
test_get_assertions_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAssertionsRequest> getAssertionsRequest (client.getAssertions ("account"));
    getAssertionsRequest->runSync ();
    g_assert_cmpint (getAssertionsRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_add_assertions_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_get_assertions (snapd));
    QScopedPointer<QSnapdAddAssertionsRequest> addAssertionsRequest (client.addAssertions (QStringList () << "type: account\n\nSIGNATURE"));
    addAssertionsRequest->runSync ();
    g_assert_cmpint (addAssertionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (g_list_length (mock_snapd_get_assertions (snapd)), ==, 1);
    g_assert_cmpstr ((gchar*) mock_snapd_get_assertions (snapd)->data, ==, "type: account\n\nSIGNATURE");
}

static void
test_assertions_sync ()
{
    QSnapdAssertion assertion ("type: account\n"
                               "authority-id: canonical\n"
                               "\n"
                               "SIGNATURE");
    g_assert_cmpint (assertion.headers ().count (), ==, 2);
    g_assert_true (assertion.headers ()[0] == "type");
    g_assert_true (assertion.header ("type") == "account");
    g_assert_true (assertion.headers ()[1] == "authority-id");
    g_assert_true (assertion.header ("authority-id") == "canonical");
    g_assert_null (assertion.header ("invalid"));
    g_assert_null (assertion.body ());
    g_assert_true (assertion.signature () == "SIGNATURE");
}

static void
test_assertions_body ()
{
    QSnapdAssertion assertion ("type: account\n"
                               "body-length: 4\n"
                               "\n"
                               "BODY\n"
                               "\n"
                               "SIGNATURE");
    g_assert_cmpint (assertion.headers ().count (), ==, 2);
    g_assert_true (assertion.headers ()[0] == "type");
    g_assert_true (assertion.header ("type") == "account");
    g_assert_true (assertion.headers ()[1] == "body-length");
    g_assert_true (assertion.header ("body-length") == "4");
    g_assert_null (assertion.header ("invalid"));
    g_assert_true (assertion.body () == "BODY");
    g_assert_true (assertion.signature () == "SIGNATURE");
}

static void
setup_get_connections (MockSnapd *snapd)
{
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    mock_snap_add_slot (s, i, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "auto-plug");
    mock_snapd_connect (snapd, p, sl, FALSE, FALSE);
    p = mock_snap_add_plug (s, i, "manual-plug");
    mock_snapd_connect (snapd, p, sl, TRUE, FALSE);
    p = mock_snap_add_plug (s, i, "gadget-plug");
    mock_snapd_connect (snapd, p, sl, FALSE, TRUE);
    p = mock_snap_add_plug (s, i, "undesired-plug");
    mock_snapd_connect (snapd, p, sl, FALSE, FALSE);
    mock_snapd_connect (snapd, p, NULL, TRUE, FALSE);
}

static void
check_plug_no_attributes (QSnapdPlug &plug)
{
    QStringList attribute_names = plug.attributeNames ();
    g_assert_cmpint (attribute_names.length (), ==, 0);
}

static void
check_slot_no_attributes (QSnapdSlot &slot)
{
    QStringList attribute_names = slot.attributeNames ();
    g_assert_cmpint (attribute_names.length (), ==, 0);
}

static void
check_connection_no_plug_attributes (QSnapdConnection &connection)
{
    g_assert_cmpint (connection.plugAttributeNames().length (), ==, 0);
}

static void
check_connection_no_slot_attributes (QSnapdConnection &connection)
{
    g_assert_cmpint (connection.slotAttributeNames().length (), ==, 0);
}

static void
check_get_connections_result (QSnapdGetConnectionsRequest &request, bool selectAll)
{
    g_assert_cmpint (request.establishedCount (), ==, 3);

    QScopedPointer<QSnapdConnection> connection0 (request.established (0));
    g_assert_true (connection0->interface () == "interface");
    QScopedPointer<QSnapdSlotRef> slotRef0 (connection0->slot ());
    g_assert_true (slotRef0->snap () == "snap1");
    g_assert_true (slotRef0->slot () == "slot1");
    QScopedPointer<QSnapdPlugRef> plugRef0 (connection0->plug ());
    g_assert_true (plugRef0->snap () == "snap2");
    g_assert_true (plugRef0->plug () == "auto-plug");
    check_connection_no_slot_attributes (*connection0);
    check_connection_no_plug_attributes (*connection0);
    g_assert_false (connection0->manual ());
    g_assert_false (connection0->gadget ());

    QScopedPointer<QSnapdConnection> connection1 (request.established (1));
    g_assert_true (connection1->interface () == "interface");
    QScopedPointer<QSnapdSlotRef> slotRef1 (connection1->slot ());
    g_assert_true (slotRef1->snap () == "snap1");
    g_assert_true (slotRef1->slot () == "slot1");
    QScopedPointer<QSnapdPlugRef> plugRef1 (connection1->plug ());
    g_assert_true (plugRef1->snap () == "snap2");
    g_assert_true (plugRef1->plug () == "manual-plug");
    check_connection_no_slot_attributes (*connection1);
    check_connection_no_plug_attributes (*connection1);
    g_assert_true (connection1->manual ());
    g_assert_false (connection1->gadget ());

    QScopedPointer<QSnapdConnection> connection2 (request.established (2));
    g_assert_true (connection2->interface () == "interface");
    QScopedPointer<QSnapdSlotRef> slotRef2 (connection2->slot ());
    g_assert_true (slotRef2->snap () == "snap1");
    g_assert_true (slotRef2->slot () == "slot1");
    QScopedPointer<QSnapdPlugRef> plugRef2 (connection2->plug ());
    g_assert_true (plugRef2->snap () == "snap2");
    g_assert_true (plugRef2->plug () == "gadget-plug");
    check_connection_no_slot_attributes (*connection2);
    check_connection_no_plug_attributes (*connection2);
    g_assert_false (connection2->manual ());
    g_assert_true (connection2->gadget ());

    if (selectAll) {
        g_assert_cmpint (request.undesiredCount (), ==, 1);

        QScopedPointer<QSnapdConnection> connection4 (request.undesired (0));
        g_assert_true (connection4->interface () == "interface");
        QScopedPointer<QSnapdSlotRef> slotRef4 (connection4->slot ());
        g_assert_true (slotRef4->snap () == "snap1");
        g_assert_true (slotRef4->slot () == "slot1");
        QScopedPointer<QSnapdPlugRef> plugRef4 (connection4->plug ());
        g_assert_true (plugRef4->snap () == "snap2");
        g_assert_true (plugRef4->plug () == "undesired-plug");
        check_connection_no_slot_attributes (*connection4);
        check_connection_no_plug_attributes (*connection4);
        g_assert_true (connection4->manual ());
        g_assert_false (connection4->gadget ());
    }
    else
        g_assert_cmpint (request.undesiredCount (), ==, 0);

    if (selectAll)
        g_assert_cmpint (request.plugCount (), ==, 4);
    else
        g_assert_cmpint (request.plugCount (), ==, 3);

    QScopedPointer<QSnapdPlug> plug0 (request.plug (0));
    g_assert_true (plug0->name () == "auto-plug");
    g_assert_true (plug0->snap () == "snap2");
    g_assert_true (plug0->interface () == "interface");
    check_plug_no_attributes (*plug0);
    g_assert_true (plug0->label () == "LABEL");
    g_assert_cmpint (plug0->connectedSlotCount (), ==, 1);
    QScopedPointer<QSnapdSlotRef> slotRefp0 (plug0->connectedSlot (0));
    g_assert_true (slotRefp0->snap () == "snap1");
    g_assert_true (slotRefp0->slot () == "slot1");

    QScopedPointer<QSnapdPlug> plug1 (request.plug (1));
    g_assert_true (plug1->name () == "manual-plug");
    g_assert_true (plug1->snap () == "snap2");
    g_assert_true (plug1->interface () == "interface");
    check_plug_no_attributes (*plug1);
    g_assert_true (plug1->label () == "LABEL");
    g_assert_cmpint (plug1->connectedSlotCount (), ==, 1);
    QScopedPointer<QSnapdSlotRef> slotRefp1 (plug1->connectedSlot (0));
    g_assert_true (slotRefp1->snap () == "snap1");
    g_assert_true (slotRefp1->slot () == "slot1");

    QScopedPointer<QSnapdPlug> plug2 (request.plug (2));
    g_assert_true (plug2->name () == "gadget-plug");
    g_assert_true (plug2->snap () == "snap2");
    g_assert_true (plug2->interface () == "interface");
    check_plug_no_attributes (*plug2);
    g_assert_true (plug2->label () == "LABEL");
    g_assert_cmpint (plug2->connectedSlotCount (), ==, 1);
    QScopedPointer<QSnapdSlotRef> slotRefp2 (plug2->connectedSlot (0));
    g_assert_true (slotRefp2->snap () == "snap1");
    g_assert_true (slotRefp2->slot () == "slot1");

    if (selectAll) {
        QScopedPointer<QSnapdPlug> plug3 (request.plug (3));
        g_assert_true (plug3->name () == "undesired-plug");
        g_assert_true (plug3->snap () == "snap2");
        g_assert_true (plug3->interface () == "interface");
        check_plug_no_attributes (*plug3);
        g_assert_true (plug3->label () == "LABEL");
        g_assert_cmpint (plug3->connectedSlotCount (), ==, 0);
    }

    if (selectAll)
        g_assert_cmpint (request.slotCount (), ==, 2);
    else
        g_assert_cmpint (request.slotCount (), ==, 1);

    QScopedPointer<QSnapdSlot> slot0 (request.slot (0));
    g_assert_true (slot0->name () == "slot1");
    g_assert_true (slot0->snap () == "snap1");
    g_assert_true (slot0->interface () == "interface");
    check_slot_no_attributes (*slot0);
    g_assert_true (slot0->label () == "LABEL");
    g_assert_cmpint (slot0->connectedPlugCount (), ==, 3);
    QScopedPointer<QSnapdPlugRef> plugRefs0 (slot0->connectedPlug (0));
    g_assert_true (plugRefs0->snap () == "snap2");
    g_assert_true (plugRefs0->plug () == "auto-plug");
    QScopedPointer<QSnapdPlugRef> plugRefs1 (slot0->connectedPlug (1));
    g_assert_true (plugRefs1->snap () == "snap2");
    g_assert_true (plugRefs1->plug () == "manual-plug");
    QScopedPointer<QSnapdPlugRef> plugRefs2 (slot0->connectedPlug (2));
    g_assert_true (plugRefs2->snap () == "snap2");
    g_assert_true (plugRefs2->plug () == "gadget-plug");

    if (selectAll) {
        QScopedPointer<QSnapdSlot> slot1 (request.slot (1));
        g_assert_true (slot1->name () == "slot2");
        g_assert_true (slot1->snap () == "snap1");
        check_slot_no_attributes (*slot1);
        g_assert_cmpint (slot1->connectedPlugCount (), ==, 0);
    }
}

static void
test_get_connections_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_connections (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections ());
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    check_get_connections_result (*getConnectionsRequest, false);
}

void
GetConnectionsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    check_get_connections_result (*request, false);

    g_main_loop_quit (loop);
}

static void
test_get_connections_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_connections (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetConnectionsHandler getConnectionsHandler (loop, client.getConnections ());
    QObject::connect (getConnectionsHandler.request, &QSnapdGetConnectionsRequest::complete, &getConnectionsHandler, &GetConnectionsHandler::onComplete);
    getConnectionsHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_connections_empty ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections ());
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getConnectionsRequest->establishedCount (), ==, 0);
    g_assert_cmpint (getConnectionsRequest->undesiredCount (), ==, 0);
    g_assert_cmpint (getConnectionsRequest->plugCount (), ==, 0);
    g_assert_cmpint (getConnectionsRequest->slotCount (), ==, 0);
}

static void
test_get_connections_filter_all ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_connections (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections (QSnapdClient::SelectAll));
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    check_get_connections_result (*getConnectionsRequest, true);
}

static void
test_get_connections_filter_snap ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *core = mock_snapd_add_snap (snapd, "core");
    MockSlot *s = mock_snap_add_slot (core, i, "slot");
    MockSnap *snap1 = mock_snapd_add_snap (snapd, "snap1");
    MockPlug *plug1 = mock_snap_add_plug (snap1, i, "plug1");
    mock_snapd_connect (snapd, plug1, s, FALSE, FALSE);
    MockSnap *snap2 = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug2 = mock_snap_add_plug (snap2, i, "plug2");
    mock_snapd_connect (snapd, plug2, s, FALSE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections ("snap1", NULL));
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getConnectionsRequest->plugCount (), ==, 1);
    QScopedPointer<QSnapdPlug> plug (getConnectionsRequest->plug (0));
    g_assert_true (plug->snap () == "snap1");
    g_assert_cmpint (getConnectionsRequest->slotCount (), ==, 1);
    QScopedPointer<QSnapdSlot> slot (getConnectionsRequest->slot (0));
    g_assert_true (slot->snap () == "core");
}

static void
test_get_connections_filter_interface ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i1 = mock_snapd_add_interface (snapd, "interface1");
    MockInterface *i2 = mock_snapd_add_interface (snapd, "interface2");
    MockSnap *core = mock_snapd_add_snap (snapd, "core");
    MockSlot *slot1 = mock_snap_add_slot (core, i1, "slot1");
    MockSlot *slot2 = mock_snap_add_slot (core, i2, "slot2");
    MockSnap *snap = mock_snapd_add_snap (snapd, "snap");
    MockPlug *plug1 = mock_snap_add_plug (snap, i1, "plug1");
    MockPlug *plug2 = mock_snap_add_plug (snap, i2, "plug2");
    mock_snapd_connect (snapd, plug1, slot1, FALSE, FALSE);
    mock_snapd_connect (snapd, plug2, slot2, FALSE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections (NULL, "interface1"));
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getConnectionsRequest->plugCount (), ==, 1);
    QScopedPointer<QSnapdPlug> plug (getConnectionsRequest->plug (0));
    g_assert_true (plug->snap () == "snap");
    g_assert_true (plug->interface () == "interface1");
    g_assert_cmpint (getConnectionsRequest->slotCount (), ==, 1);
    QScopedPointer<QSnapdSlot> slot (getConnectionsRequest->slot (0));
    g_assert_true (slot->snap () == "core");
    g_assert_true (slot->interface () == "interface1");
}

static void
check_names_match (QStringList names, QStringList &expected_names)
{
    g_assert_cmpint (names.length (), ==, expected_names.length ());

    QStringList sorted_names = names;
    sorted_names.sort ();
    QStringList sorted_expected_names = expected_names;
    sorted_expected_names.sort ();
    for (int i = 0; i < names.length (); i++)
        g_assert_true (sorted_names[i] == sorted_expected_names[i]);
}

static void
test_get_connections_attributes ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    mock_slot_add_attribute (sl, "slot-string-key", "\"value\"");
    mock_slot_add_attribute (sl, "slot-int-key", "42");
    mock_slot_add_attribute (sl, "slot-bool-key", "true");
    mock_slot_add_attribute (sl, "slot-number-key", "1.25");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "plug1");
    mock_plug_add_attribute (p, "plug-string-key", "\"value\"");
    mock_plug_add_attribute (p, "plug-int-key", "42");
    mock_plug_add_attribute (p, "plug-bool-key", "true");
    mock_plug_add_attribute (p, "plug-number-key", "1.25");
    mock_snapd_connect (snapd, p, sl, FALSE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetConnectionsRequest> getConnectionsRequest (client.getConnections ());
    getConnectionsRequest->runSync ();
    g_assert_cmpint (getConnectionsRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getConnectionsRequest->establishedCount (), ==, 1);
    QScopedPointer<QSnapdConnection> connection (getConnectionsRequest->established (0));

    check_names_match (connection->plugAttributeNames (), QStringList () << "plug-string-key" << "plug-int-key" << "plug-bool-key" << "plug-number-key");
    g_assert_true (connection->hasPlugAttribute ("plug-string-key"));
    g_assert_true (connection->plugAttribute ("plug-string-key").type () == QMetaType::QString);
    g_assert_true (connection->plugAttribute ("plug-string-key").toString () == "value");
    g_assert_true (connection->hasPlugAttribute ("plug-int-key"));
    g_assert_true (connection->plugAttribute ("plug-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (connection->plugAttribute ("plug-int-key").toLongLong (), ==, 42);
    g_assert_true (connection->hasPlugAttribute ("plug-bool-key"));
    g_assert_true (connection->plugAttribute ("plug-bool-key").type () == QMetaType::Bool);
    g_assert_true (connection->plugAttribute ("plug-bool-key").toBool ());
    g_assert_true (connection->hasPlugAttribute ("plug-number-key"));
    g_assert_true (connection->plugAttribute ("plug-number-key").type () == QMetaType::Double);
    g_assert_cmpfloat (connection->plugAttribute ("plug-number-key").toDouble (), ==, 1.25);
    g_assert_false (connection->hasPlugAttribute ("plug-invalid-key"));
    g_assert_false (connection->plugAttribute ("plug-invalid-key").isValid ());

    check_names_match (connection->slotAttributeNames (), QStringList () << "slot-string-key" << "slot-int-key" << "slot-bool-key" << "slot-number-key");
    g_assert_true (connection->hasSlotAttribute ("slot-string-key"));
    g_assert_true (connection->slotAttribute ("slot-string-key").type () == QMetaType::QString);
    g_assert_true (connection->slotAttribute ("slot-string-key").toString () == "value");
    g_assert_true (connection->hasSlotAttribute ("slot-int-key"));
    g_assert_true (connection->slotAttribute ("slot-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (connection->slotAttribute ("slot-int-key").toLongLong (), ==, 42);
    g_assert_true (connection->hasSlotAttribute ("slot-bool-key"));
    g_assert_true (connection->slotAttribute ("slot-bool-key").type () == QMetaType::Bool);
    g_assert_true (connection->slotAttribute ("slot-bool-key").toBool ());
    g_assert_true (connection->hasSlotAttribute ("slot-number-key"));
    g_assert_true (connection->slotAttribute ("slot-number-key").type () == QMetaType::Double);
    g_assert_cmpfloat (connection->slotAttribute ("slot-number-key").toDouble (), ==, 1.25);
    g_assert_false (connection->hasSlotAttribute ("slot-invalid-key"));
    g_assert_false (connection->slotAttribute ("slot-invalid-key").isValid ());

    g_assert_cmpint (getConnectionsRequest->plugCount (), ==, 1);
    QScopedPointer<QSnapdPlug> plug (getConnectionsRequest->plug (0));
    check_names_match (plug->attributeNames (), QStringList () << "plug-string-key" << "plug-int-key" << "plug-bool-key" << "plug-number-key");
    g_assert_true (plug->hasAttribute ("plug-string-key"));
    g_assert_true (plug->attribute ("plug-string-key").type () == QMetaType::QString);
    g_assert_true (plug->attribute ("plug-string-key").toString () == "value");
    g_assert_true (plug->hasAttribute ("plug-int-key"));
    g_assert_true (plug->attribute ("plug-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (plug->attribute ("plug-int-key").toLongLong (), ==, 42);
    g_assert_true (plug->hasAttribute ("plug-bool-key"));
    g_assert_true (plug->attribute ("plug-bool-key").type () == QMetaType::Bool);
    g_assert_true (plug->attribute ("plug-bool-key").toBool ());
    g_assert_true (plug->hasAttribute ("plug-number-key"));
    g_assert_true (plug->attribute ("plug-number-key").type () == QMetaType::Double);
    g_assert_cmpfloat (plug->attribute ("plug-number-key").toDouble (), ==, 1.25);
    g_assert_false (plug->hasAttribute ("plug-invalid-key"));
    g_assert_false (plug->attribute ("plug-invalid-key").isValid ());

    g_assert_cmpint (getConnectionsRequest->slotCount (), ==, 1);
    QScopedPointer<QSnapdSlot> slot (getConnectionsRequest->slot (0));
    check_names_match (slot->attributeNames (), QStringList () << "slot-string-key" << "slot-int-key" << "slot-bool-key" << "slot-number-key");
    g_assert_true (slot->hasAttribute ("slot-string-key"));
    g_assert_true (slot->attribute ("slot-string-key").type () == QMetaType::QString);
    g_assert_true (slot->attribute ("slot-string-key").toString () == "value");
    g_assert_true (slot->hasAttribute ("slot-int-key"));
    g_assert_true (slot->attribute ("slot-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (slot->attribute ("slot-int-key").toLongLong (), ==, 42);
    g_assert_true (slot->hasAttribute ("slot-bool-key"));
    g_assert_true (slot->attribute ("slot-bool-key").type () == QMetaType::Bool);
    g_assert_true (slot->attribute ("slot-bool-key").toBool ());
    g_assert_true (slot->hasAttribute ("slot-number-key"));
    g_assert_true (slot->attribute ("slot-number-key").type () == QMetaType::Double);
    g_assert_cmpfloat (slot->attribute ("slot-number-key").toDouble (), ==, 1.25);
    g_assert_false (slot->hasAttribute ("slot-invalid-key"));
    g_assert_false (slot->attribute ("slot-invalid-key").isValid ());
}

static void
setup_get_interfaces (MockSnapd *snapd)
{
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    mock_snap_add_slot (s, i, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "plug1");
    mock_snapd_connect (snapd, p, sl, TRUE, FALSE);
}

static void
check_get_interfaces_result (QSnapdGetInterfacesRequest &request)
{
    g_assert_cmpint (request.plugCount (), ==, 1);

    QScopedPointer<QSnapdPlug> plug (request.plug (0));
    g_assert_true (plug->name () == "plug1");
    g_assert_true (plug->snap () == "snap2");
    g_assert_true (plug->interface () == "interface");
    check_plug_no_attributes (*plug);
    g_assert_true (plug->label () == "LABEL");
    g_assert_cmpint (plug->connectedSlotCount (), ==, 1);
    QScopedPointer<QSnapdSlotRef> slotRef (plug->connectedSlot (0));
    g_assert_true (slotRef->snap () == "snap1");
    g_assert_true (slotRef->slot () == "slot1");

    g_assert_cmpint (request.slotCount (), ==, 2);

    QScopedPointer<QSnapdSlot> slot0 (request.slot (0));
    g_assert_true (slot0->name () == "slot1");
    g_assert_true (slot0->snap () == "snap1");
    g_assert_true (slot0->interface () == "interface");
    check_slot_no_attributes (*slot0);
    g_assert_true (slot0->label () == "LABEL");
    g_assert_cmpint (slot0->connectedPlugCount (), ==, 1);
    QScopedPointer<QSnapdPlugRef> plugRef (slot0->connectedPlug (0));
    g_assert_true (plugRef->snap () == "snap2");
    g_assert_true (plugRef->plug () == "plug1");

    QScopedPointer<QSnapdSlot> slot1 (request.slot (1));
    g_assert_true (slot1->name () == "slot2");
    g_assert_true (slot1->snap () == "snap1");
    check_slot_no_attributes (*slot1);
    g_assert_cmpint (slot1->connectedPlugCount (), ==, 0);
}

static void
test_get_interfaces_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_interfaces (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    check_get_interfaces_result (*getInterfacesRequest);
}

void
GetInterfacesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    check_get_interfaces_result (*request);

    g_main_loop_quit (loop);
}

static void
test_get_interfaces_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    setup_get_interfaces (snapd);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetInterfacesHandler getInterfacesHandler (loop, client.getInterfaces ());
    QObject::connect (getInterfacesHandler.request, &QSnapdGetInterfacesRequest::complete, &getInterfacesHandler, &GetInterfacesHandler::onComplete);
    getInterfacesHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_interfaces_no_snaps ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getInterfacesRequest->plugCount (), ==, 0);
    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 0);
}

static void
test_get_interfaces_attributes ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    mock_slot_add_attribute (sl, "slot-string-key", "\"value\"");
    mock_slot_add_attribute (sl, "slot-int-key", "42");
    mock_slot_add_attribute (sl, "slot-bool-key", "true");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "plug1");
    mock_plug_add_attribute (p, "plug-string-key", "\"value\"");
    mock_plug_add_attribute (p, "plug-int-key", "42");
    mock_plug_add_attribute (p, "plug-bool-key", "true");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->plugCount (), ==, 1);
    QScopedPointer<QSnapdPlug> plug (getInterfacesRequest->plug (0));
    check_names_match (plug->attributeNames (), QStringList () << "plug-string-key" << "plug-int-key" << "plug-bool-key");
    g_assert_true (plug->hasAttribute ("plug-string-key"));
    g_assert_true (plug->attribute ("plug-string-key").type () == QMetaType::QString);
    g_assert_true (plug->attribute ("plug-string-key").toString () == "value");
    g_assert_true (plug->hasAttribute ("plug-int-key"));
    g_assert_true (plug->attribute ("plug-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (plug->attribute ("plug-int-key").toLongLong (), ==, 42);
    g_assert_true (plug->hasAttribute ("plug-bool-key"));
    g_assert_true (plug->attribute ("plug-bool-key").type () == QMetaType::Bool);
    g_assert_true (plug->attribute ("plug-bool-key").toBool ());
    g_assert_false (plug->hasAttribute ("plug-invalid-key"));
    g_assert_false (plug->attribute ("plug-invalid-key").isValid ());

    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 1);
    QScopedPointer<QSnapdSlot> slot (getInterfacesRequest->slot (0));
    check_names_match (slot->attributeNames (), QStringList () << "slot-string-key" << "slot-int-key" << "slot-bool-key");
    g_assert_true (slot->hasAttribute ("slot-string-key"));
    g_assert_true (slot->attribute ("slot-string-key").type () == QMetaType::QString);
    g_assert_true (slot->attribute ("slot-string-key").toString () == "value");
    g_assert_true (slot->hasAttribute ("slot-int-key"));
    g_assert_true (slot->attribute ("slot-int-key").type () == QMetaType::LongLong);
    g_assert_cmpint (slot->attribute ("slot-int-key").toLongLong (), ==, 42);
    g_assert_true (slot->hasAttribute ("slot-bool-key"));
    g_assert_true (slot->attribute ("slot-bool-key").type () == QMetaType::Bool);
    g_assert_true (slot->attribute ("slot-bool-key").toBool ());
    g_assert_false (slot->hasAttribute ("slot-invalid-key"));
    g_assert_false (slot->attribute ("slot-invalid-key").isValid ());
}

static void
test_get_interfaces_legacy ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    mock_snap_add_slot (s, i, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "plug1");
    mock_snapd_connect (snapd, p, sl, TRUE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->plugCount (), ==, 1);

    QScopedPointer<QSnapdPlug> plug (getInterfacesRequest->plug (0));
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (plug->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> plugConnection (plug->connection (0));
    g_assert_true (plugConnection->snap () == "snap1");
    g_assert_true (plugConnection->name () == "slot1");
QT_WARNING_POP

    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 2);

    QScopedPointer<QSnapdSlot> slot0 (getInterfacesRequest->slot (0));
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (slot0->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> slotConnection (slot0->connection (0));
    g_assert_true (slotConnection->snap () == "snap2");
    g_assert_true (slotConnection->name () == "plug1");
QT_WARNING_POP

    QScopedPointer<QSnapdSlot> slot1 (getInterfacesRequest->slot (1));
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (slot1->connectionCount (), ==, 0);
QT_WARNING_POP
}

static void
test_get_interfaces2_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i1 = mock_snapd_add_interface (snapd, "interface1");
    mock_interface_set_summary (i1, "summary1");
    mock_interface_set_doc_url (i1, "url1");
    MockInterface *i2 = mock_snapd_add_interface (snapd, "interface2");
    mock_interface_set_summary (i2, "summary2");
    mock_interface_set_doc_url (i2, "url2");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_plug (s, i1, "plug1");
    mock_snap_add_slot (s, i2, "slot1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest (client.getInterfaces2 ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->interfaceCount (), ==, 2);

    QScopedPointer<QSnapdInterface> iface0 (getInterfacesRequest->interface (0));
    g_assert_true (iface0->name () == "interface1");
    g_assert_true (iface0->summary () == "summary1");
    g_assert_true (iface0->docUrl () == "url1");
    g_assert_cmpint (iface0->plugCount (), ==, 0);
    g_assert_cmpint (iface0->slotCount (), ==, 0);

    QScopedPointer<QSnapdInterface> iface1 (getInterfacesRequest->interface (1));
    g_assert_true (iface1->name () == "interface2");
    g_assert_true (iface1->summary () == "summary2");
    g_assert_true (iface1->docUrl () == "url2");
    g_assert_cmpint (iface1->plugCount (), ==, 0);
    g_assert_cmpint (iface1->slotCount (), ==, 0);
}

void
GetInterfaces2Handler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (request->interfaceCount (), ==, 2);

    QScopedPointer<QSnapdInterface> iface0 (request->interface (0));
    g_assert_true (iface0->name () == "interface1");
    g_assert_true (iface0->summary () == "summary1");
    g_assert_true (iface0->docUrl () == "url1");
    g_assert_cmpint (iface0->plugCount (), ==, 0);
    g_assert_cmpint (iface0->slotCount (), ==, 0);

    QScopedPointer<QSnapdInterface> iface1 (request->interface (1));
    g_assert_true (iface1->name () == "interface2");
    g_assert_true (iface1->summary () == "summary2");
    g_assert_true (iface1->docUrl () == "url2");
    g_assert_cmpint (iface1->plugCount (), ==, 0);
    g_assert_cmpint (iface1->slotCount (), ==, 0);

    g_main_loop_quit (loop);
}

static void
test_get_interfaces2_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i1 = mock_snapd_add_interface (snapd, "interface1");
    mock_interface_set_summary (i1, "summary1");
    mock_interface_set_doc_url (i1, "url1");
    MockInterface *i2 = mock_snapd_add_interface (snapd, "interface2");
    mock_interface_set_summary (i2, "summary2");
    mock_interface_set_doc_url (i2, "url2");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_plug (s, i1, "plug1");
    mock_snap_add_slot (s, i2, "slot1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetInterfaces2Handler getInterfacesHandler (loop, client.getInterfaces2 ());
    QObject::connect (getInterfacesHandler.request, &QSnapdGetInterfaces2Request::complete, &getInterfacesHandler, &GetInterfaces2Handler::onComplete);
    getInterfacesHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_get_interfaces2_only_connected ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface1");
    mock_snapd_add_interface (snapd, "interface2");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, i, "slot1");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, i, "plug2");
    mock_snapd_connect (snapd, p, sl, TRUE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest (client.getInterfaces2 (QSnapdClient::OnlyConnected));
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->interfaceCount (), ==, 1);

    QScopedPointer<QSnapdInterface> iface (getInterfacesRequest->interface (0));
    g_assert_true (iface->name () == "interface1");
    g_assert_cmpint (iface->plugCount (), ==, 0);
    g_assert_cmpint (iface->slotCount (), ==, 0);
}

static void
test_get_interfaces2_slots ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_slot (s, i, "slot1");
    mock_snap_add_plug (s, i, "plug1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest (client.getInterfaces2 (QSnapdClient::IncludeSlots));
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->interfaceCount (), ==, 1);

    QScopedPointer<QSnapdInterface> iface (getInterfacesRequest->interface (0));
    g_assert_cmpint (iface->plugCount (), ==, 0);
    g_assert_cmpint (iface->slotCount (), ==, 1);
    QScopedPointer<QSnapdSlot> slot (iface->slot (0));
    g_assert_true (slot->name () == "slot1");
    g_assert_true (slot->snap () == "snap1");
}

static void
test_get_interfaces2_plugs ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_slot (s, i, "slot1");
    mock_snap_add_plug (s, i, "plug1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest (client.getInterfaces2 (QSnapdClient::IncludePlugs));
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->interfaceCount (), ==, 1);

    QScopedPointer<QSnapdInterface> iface (getInterfacesRequest->interface (0));
    g_assert_cmpint (iface->plugCount (), ==, 1);
    QScopedPointer<QSnapdPlug> plug (iface->plug (0));
    g_assert_true (plug->name () == "plug1");
    g_assert_true (plug->snap () == "snap1");
    g_assert_cmpint (iface->slotCount (), ==, 0);
}

static void
test_get_interfaces2_filter ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_interface (snapd, "interface1");
    mock_snapd_add_interface (snapd, "interface2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest (client.getInterfaces2 (QStringList ("interface2")));
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->interfaceCount (), ==, 1);

    QScopedPointer<QSnapdInterface> iface (getInterfacesRequest->interface (0));
    g_assert_true (iface->name () == "interface2");
    g_assert_cmpint (iface->plugCount (), ==, 0);
    g_assert_cmpint (iface->slotCount (), ==, 0);
}

static void
test_get_interfaces2_make_label ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_interface (snapd, "camera");
    MockInterface *i = mock_snapd_add_interface (snapd, "interface-without-translation");
    mock_interface_set_summary (i, "SUMMARY");
    mock_snapd_add_interface (snapd, "interface-without-summary");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest1 (client.getInterfaces2 (QStringList ("camera")));
    getInterfacesRequest1->runSync ();
    g_assert_cmpint (getInterfacesRequest1->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getInterfacesRequest1->interfaceCount (), ==, 1);
    QScopedPointer<QSnapdInterface> iface1 (getInterfacesRequest1->interface (0));
    g_assert_true (iface1->makeLabel () == "Use your camera"); // FIXME: Won't work if translated

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest2 (client.getInterfaces2 (QStringList ("interface-without-translation")));
    getInterfacesRequest2->runSync ();
    g_assert_cmpint (getInterfacesRequest2->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getInterfacesRequest2->interfaceCount (), ==, 1);
    QScopedPointer<QSnapdInterface> iface2 (getInterfacesRequest2->interface (0));
    g_assert_true (iface2->makeLabel () == "interface-without-translation: SUMMARY");

    QScopedPointer<QSnapdGetInterfaces2Request> getInterfacesRequest3 (client.getInterfaces2 (QStringList ("interface-without-summary")));
    getInterfacesRequest3->runSync ();
    g_assert_cmpint (getInterfacesRequest3->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getInterfacesRequest3->interfaceCount (), ==, 1);
    QScopedPointer<QSnapdInterface> iface3 (getInterfacesRequest3->interface (0));
    g_assert_true (iface3->makeLabel () == "interface-without-summary");
}

static void
test_connect_interface_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, i, "plug");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snapd_find_plug_connection (snapd, plug) == slot);
}

void
ConnectInterfaceHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSlot *slot = mock_snap_find_slot (mock_snapd_find_snap (snapd, "snap1"), "slot");
    MockPlug *plug = mock_snap_find_plug (mock_snapd_find_snap (snapd, "snap2"), "plug");
    g_assert_true (mock_snapd_find_plug_connection (snapd, plug) == slot);

    g_main_loop_quit (loop);
}

static void
test_connect_interface_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_add_plug (s, i, "plug");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    ConnectInterfaceHandler connectInterfaceHandler (loop, snapd, client.connectInterface ("snap2", "plug", "snap1", "slot"));
    QObject::connect (connectInterfaceHandler.request, &QSnapdConnectInterfaceRequest::complete, &connectInterfaceHandler, &ConnectInterfaceHandler::onComplete);
    connectInterfaceHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_connect_interface_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, i, "plug");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (connectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snapd_find_plug_connection (snapd, plug) == slot);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_connect_interface_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_disconnect_interface_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, i, "plug");
    mock_snapd_connect (snapd, plug, slot, TRUE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_plug_connection (snapd, plug));
}

void
DisconnectInterfaceHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSnap *s = mock_snapd_find_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_find_plug (s, "plug");
    g_assert_null (mock_snapd_find_plug_connection (snapd, plug));

    g_main_loop_quit (loop);
}

static void
test_disconnect_interface_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, i, "plug");
    mock_snapd_connect (snapd, plug, slot, TRUE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    DisconnectInterfaceHandler disconnectInterfaceHandler (loop, snapd, client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    QObject::connect (disconnectInterfaceHandler.request, &QSnapdDisconnectInterfaceRequest::complete, &disconnectInterfaceHandler, &DisconnectInterfaceHandler::onComplete);
    disconnectInterfaceHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_disconnect_interface_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockInterface *i = mock_snapd_add_interface (snapd, "interface");
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, i, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, i, "plug");
    mock_snapd_connect (snapd, plug, slot, TRUE, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (disconnectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_plug_connection (snapd, plug));
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_disconnect_interface_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_find_query ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    mock_snapd_add_store_snap (snapd, "apple");
    mock_snapd_add_store_snap (snapd, "banana");
    MockSnap *s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_track_add_channel (mock_snap_add_track (s, "latest"), "stable", NULL);
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_track_add_channel (mock_snap_add_track (s, "latest"), "stable", NULL);
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_website (s, "WEBSITE");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_store_url (s, "https://snapcraft.io/snap");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_download_size (s, 1024);
    mock_snap_add_price (s, 1.25, "NZD");
    mock_snap_add_price (s, 0.75, "USD");
    mock_snap_add_media (s, "screenshot", "screenshot0.png", 0, 0);
    mock_snap_add_media (s, "screenshot", "screenshot1.png", 1024, 1024);
    mock_snap_add_media (s, "banner", "banner.png", 0, 0);
    mock_snap_set_trymode (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("carrot"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    g_assert_true (findRequest->suggestedCurrency () == "NZD");
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert_true (snap0->name () == "carrot1");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert_true (snap1->channel () == "CHANNEL");
    g_assert_cmpint (snap1->tracks ().count (), ==, 1);
    g_assert_true (snap1->tracks ()[0] == "latest");
    g_assert_cmpint (snap1->channelCount (), ==, 1);
    QScopedPointer<QSnapdChannel> channel (snap1->channel (0));
    g_assert_true (channel->name () == "stable");
    g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_true (channel->revision () == "REVISION");
    g_assert_true (channel->version () == "VERSION");
    g_assert_true (channel->epoch () == "0");
    g_assert_cmpint (channel->size (), ==, 65535);
    g_assert_cmpint (snap1->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_true (snap1->contact () == "CONTACT");
    g_assert_true (snap1->website () == "WEBSITE");
    g_assert_true (snap1->description () == "DESCRIPTION");
    g_assert_true (snap1->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert_true (snap1->publisherId () == "PUBLISHER-ID");
    g_assert_true (snap1->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap1->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_cmpint (snap1->downloadSize (), ==, 1024);
    g_assert_true (snap1->hold ().isNull ());
    g_assert_true (snap1->icon () == "ICON");
    g_assert_true (snap1->id () == "ID");
    g_assert_true (snap1->installDate ().isNull ());
    g_assert_cmpint (snap1->installedSize (), ==, 0);
    g_assert_cmpint (snap1->mediaCount (), ==, 3);
    QScopedPointer<QSnapdMedia> media0 (snap1->media (0));
    g_assert_true (media0->type () == "screenshot");
    g_assert_true (media0->url () == "screenshot0.png");
    QScopedPointer<QSnapdMedia> media1 (snap1->media (1));
    g_assert_true (media1->type () == "screenshot");
    g_assert_true (media1->url () == "screenshot1.png");
    g_assert_cmpint (media1->width (), ==, 1024);
    g_assert_cmpint (media1->height (), ==, 1024);
    QScopedPointer<QSnapdMedia> media2 (snap1->media (2));
    g_assert_true (media2->type () == "banner");
    g_assert_true (media2->url () == "banner.png");
    g_assert_true (snap1->name () == "carrot2");
    g_assert_cmpint (snap1->priceCount (), ==, 2);
    QScopedPointer<QSnapdPrice> price0 (snap1->price (0));
    g_assert_cmpfloat (price0->amount (), ==, 1.25);
    g_assert_true (price0->currency () == "NZD");
    QScopedPointer<QSnapdPrice> price1 (snap1->price (1));
    g_assert_cmpfloat (price1->amount (), ==, 0.75);
    g_assert_true (price1->currency () == "USD");
    g_assert_false (snap1->isPrivate ());
    g_assert_true (snap1->revision () == "REVISION");
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    g_assert_cmpint (snap1->screenshotCount (), ==, 0);
QT_WARNING_POP
    g_assert_cmpint (snap1->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap1->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_true (snap1->storeUrl () == "https://snapcraft.io/snap");
    g_assert_true (snap1->summary () == "SUMMARY");
    g_assert_true (snap1->trymode ());
    g_assert_true (snap1->version () == "VERSION");
}

static void
test_find_query_private ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_account_add_private_snap (a, "snap2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap2");
    g_assert_true (snap->isPrivate ());
}

static void
test_find_query_private_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_find_bad_query ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    // '?' is not allowed in queries
    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("snap?"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::BadQuery);
}

static void
test_find_network_timeout ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("network-timeout"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NetworkTimeout);
}

static void
test_find_dns_failure ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("dns-failure"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::DNSFailure);
}

static void
test_find_name ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap");
}

static void
test_find_name_private ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_add_private_snap (a, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName | QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap");
    g_assert_true (snap->isPrivate ());
}

static void
test_find_name_private_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName | QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_find_channels ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    MockTrack *t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "stable", NULL);
    MockChannel *c = mock_track_add_channel (t, "beta", NULL);
    mock_channel_set_revision (c, "BETA-REVISION");
    mock_channel_set_version (c, "BETA-VERSION");
    mock_channel_set_epoch (c, "1");
    mock_channel_set_confinement (c, "classic");
    mock_channel_set_size (c, 10000);
    mock_channel_set_released_at (c, "2018-01-19T13:14:15Z");
    c = mock_track_add_channel (t, "stable", "branch");
    t = mock_snap_add_track (s, "insider");
    c = mock_track_add_channel (t, "stable", NULL);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap");
    g_assert_cmpint (snap->tracks ().count (), ==, 2);
    g_assert_true (snap->tracks ()[0] == "latest");
    g_assert_true (snap->tracks ()[1] == "insider");
    g_assert_cmpint (snap->channelCount (), ==, 4);

    gboolean matched_stable = FALSE, matched_beta = FALSE, matched_branch = FALSE, matched_track = FALSE;
    for (int i = 0; i < snap->channelCount (); i++) {
        QSnapdChannel *channel = snap->channel (i);

        if (channel->name () == "stable") {
            g_assert_true (channel->track () == "latest");
            g_assert_true (channel->risk () == "stable");
            g_assert_true (channel->branch ().isEmpty());
            g_assert_true (channel->revision () == "REVISION");
            g_assert_true (channel->version () == "VERSION");
            g_assert_true (channel->epoch () == "0");
            g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
            g_assert_cmpint (channel->size (), ==, 65535);
            g_assert_true (channel->releasedAt ().isNull ());
            matched_stable = TRUE;
        }
        if (channel->name () == "beta") {
            g_assert_true (channel->name () == "beta");
            g_assert_true (channel->track () == "latest");
            g_assert_true (channel->risk () == "beta");
            g_assert_true (channel->branch ().isEmpty());
            g_assert_true (channel->revision () == "BETA-REVISION");
            g_assert_true (channel->version () == "BETA-VERSION");
            g_assert_true (channel->epoch () == "1");
            g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
            g_assert_cmpint (channel->size (), ==, 10000);
            g_assert_true (channel->releasedAt () == QDateTime (QDate (2018, 1, 19), QTime (13, 14, 15), Qt::UTC));
            matched_beta = TRUE;
        }
        if (channel->name () == "stable/branch") {
            g_assert_true (channel->track () == "latest");
            g_assert_true (channel->risk () == "stable");
            g_assert_true (channel->branch () == "branch");
            g_assert_true (channel->releasedAt ().isNull ());
            matched_branch = TRUE;
        }
        if (channel->name () == "insider/stable") {
            g_assert_true (channel->track () == "insider");
            g_assert_true (channel->risk () == "stable");
            g_assert_true (channel->branch ().isEmpty());
            g_assert_true (channel->releasedAt ().isNull ());
            matched_track = TRUE;
        }
    }
    g_assert_true (matched_stable);
    g_assert_true (matched_beta);
    g_assert_true (matched_branch);
    g_assert_true (matched_track);
}

static void
test_find_channels_match ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    MockSnap *s = mock_snapd_add_store_snap (snapd, "stable-snap");
    MockTrack *t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "stable", NULL);

    s = mock_snapd_add_store_snap (snapd, "full-snap");
    t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "stable", NULL);
    mock_track_add_channel (t, "candidate", NULL);
    mock_track_add_channel (t, "beta", NULL);
    mock_track_add_channel (t, "edge", NULL);

    s = mock_snapd_add_store_snap (snapd, "beta-snap");
    t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "beta", NULL);

    s = mock_snapd_add_store_snap (snapd, "branch-snap");
    t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "stable", NULL);
    mock_track_add_channel (t, "stable", "branch");

    s = mock_snapd_add_store_snap (snapd, "track-snap");
    t = mock_snap_add_track (s, "latest");
    mock_track_add_channel (t, "stable", NULL);
    t = mock_snap_add_track (s, "insider");
    mock_track_add_channel (t, "stable", NULL);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    /* All channels match to stable if only stable defined */
    QScopedPointer<QSnapdFindRequest> findRequest1 (client.find (QSnapdClient::MatchName, "stable-snap"));
    findRequest1->runSync ();
    g_assert_cmpint (findRequest1->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest1->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap1 (findRequest1->snap (0));
    g_assert_true (snap1->name () == "stable-snap");
    QScopedPointer<QSnapdChannel> channel1a (snap1->matchChannel ("stable"));
    g_assert_nonnull (channel1a);
    g_assert_true (channel1a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1b (snap1->matchChannel ("candidate"));
    g_assert_nonnull (channel1b);
    g_assert_true (channel1b->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1c (snap1->matchChannel ("beta"));
    g_assert_nonnull (channel1c);
    g_assert_true (channel1c->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1d (snap1->matchChannel ("edge"));
    g_assert_nonnull (channel1d);
    g_assert_true (channel1d->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1e (snap1->matchChannel ("UNDEFINED"));
    g_assert_null (channel1e);

    /* All channels match if all defined */
    QScopedPointer<QSnapdFindRequest> findRequest2 (client.find (QSnapdClient::MatchName, "full-snap"));
    findRequest2->runSync ();
    g_assert_cmpint (findRequest2->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest2->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap2 (findRequest2->snap (0));
    g_assert_true (snap2->name () == "full-snap");
    QScopedPointer<QSnapdChannel> channel2a (snap2->matchChannel ("stable"));
    g_assert_nonnull (channel2a);
    g_assert_true (channel2a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel2b (snap2->matchChannel ("candidate"));
    g_assert_nonnull (channel2b);
    g_assert_true (channel2b->name () == "candidate");
    QScopedPointer<QSnapdChannel> channel2c (snap2->matchChannel ("beta"));
    g_assert_nonnull (channel2c);
    g_assert_true (channel2c->name () == "beta");
    QScopedPointer<QSnapdChannel> channel2d (snap2->matchChannel ("edge"));
    g_assert_nonnull (channel2d);
    g_assert_true (channel2d->name () == "edge");
    QScopedPointer<QSnapdChannel> channel2e (snap2->matchChannel ("UNDEFINED"));
    g_assert_null (channel2e);

    /* Only match with more stable channels */
    QScopedPointer<QSnapdFindRequest> findRequest3 (client.find (QSnapdClient::MatchName, "beta-snap"));
    findRequest3->runSync ();
    g_assert_cmpint (findRequest3->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest3->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap3 (findRequest3->snap (0));
    g_assert_true (snap3->name () == "beta-snap");
    QScopedPointer<QSnapdChannel> channel3a (snap3->matchChannel ("stable"));
    g_assert_null (channel3a);
    QScopedPointer<QSnapdChannel> channel3b (snap3->matchChannel ("candidate"));
    g_assert_null (channel3b);
    QScopedPointer<QSnapdChannel> channel3c (snap3->matchChannel ("beta"));
    g_assert_nonnull (channel3c);
    g_assert_true (channel3c->name () == "beta");
    QScopedPointer<QSnapdChannel> channel3d (snap3->matchChannel ("edge"));
    g_assert_nonnull (channel3d);
    g_assert_true (channel3d->name () == "beta");
    QScopedPointer<QSnapdChannel> channel3e (snap3->matchChannel ("UNDEFINED"));
    g_assert_null (channel3e);

    /* Match branches */
    QScopedPointer<QSnapdFindRequest> findRequest4 (client.find (QSnapdClient::MatchName, "branch-snap"));
    findRequest4->runSync ();
    g_assert_cmpint (findRequest4->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest4->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap4 (findRequest4->snap (0));
    g_assert_true (snap4->name () == "branch-snap");
    QScopedPointer<QSnapdChannel> channel4a (snap4->matchChannel ("stable"));
    g_assert_nonnull (channel4a);
    g_assert_true (channel4a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4b (snap4->matchChannel ("stable/branch"));
    g_assert_nonnull (channel4b);
    g_assert_true (channel4b->name () == "stable/branch");
    QScopedPointer<QSnapdChannel> channel4c (snap4->matchChannel ("candidate"));
    g_assert_nonnull (channel4c);
    g_assert_true (channel4c->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4d (snap4->matchChannel ("beta"));
    g_assert_nonnull (channel4d);
    g_assert_true (channel4d->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4e (snap4->matchChannel ("edge"));
    g_assert_nonnull (channel4e);
    g_assert_true (channel4e->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4f (snap4->matchChannel ("UNDEFINED"));
    g_assert_null (channel4f);

    /* Match correct tracks */
    QScopedPointer<QSnapdFindRequest> findRequest5 (client.find (QSnapdClient::MatchName, "track-snap"));
    findRequest5->runSync ();
    g_assert_cmpint (findRequest5->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest5->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap5 (findRequest5->snap (0));
    g_assert_true (snap5->name () == "track-snap");
    QScopedPointer<QSnapdChannel> channel5a (snap5->matchChannel ("stable"));
    g_assert_nonnull (channel5a);
    g_assert_true (channel5a->name () == "stable");
    g_assert_true (channel5a->track () == "latest");
    g_assert_true (channel5a->risk () == "stable");
    QScopedPointer<QSnapdChannel> channel5b (snap5->matchChannel ("latest/stable"));
    g_assert_nonnull (channel5b);
    g_assert_true (channel5b->name () == "stable");
    g_assert_true (channel5b->track () == "latest");
    g_assert_true (channel5b->risk () == "stable");
    QScopedPointer<QSnapdChannel> channel5c (snap5->matchChannel ("insider/stable"));
    g_assert_nonnull (channel5c);
    g_assert_true (channel5c->name () == "insider/stable");
    g_assert_true (channel5c->track () == "insider");
    g_assert_true (channel5c->risk () == "stable");
}

static gboolean
find_cancel_cb (QSnapdFindRequest *request)
{
    request->cancel ();
    return G_SOURCE_REMOVE;
}

void
FindHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::Cancelled);
    g_main_loop_quit (loop);
}

static void
test_find_cancel ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    /* Use a special query that never responds */
    FindHandler findHandler (loop, client.find ("do-not-respond"));
    QObject::connect (findHandler.request, &QSnapdFindRequest::complete, &findHandler, &FindHandler::onComplete);
    findHandler.request->runAsync ();
    g_idle_add ((GSourceFunc) find_cancel_cb, findHandler.request);

    g_main_loop_run (loop);
}

static void
test_find_section ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "section", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "section", FALSE);
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection ("section", NULL));
QT_WARNING_POP
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert_true (snap0->name () == "apple");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert_true (snap1->name () == "carrot1");
}

static void
test_find_section_query ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "section", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "section", FALSE);
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection ("section", "carrot"));
QT_WARNING_POP
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "carrot1");
}

static void
test_find_section_name ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "section", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "section", FALSE);
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_snap_add_store_category (s, "section", FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection (QSnapdClient::MatchName, "section", "carrot1"));
QT_WARNING_POP
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "carrot1");
}

static void
test_find_category ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "category", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "category", FALSE);
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findCategory ("category", NULL));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert_true (snap0->name () == "apple");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert_true (snap1->name () == "carrot1");
}

static void
test_find_category_query ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "category", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "category", FALSE);
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findCategory ("category", "carrot"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "carrot1");
}

static void
test_find_category_name ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_category (s, "category", FALSE);
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_category (s, "category", FALSE);
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_snap_add_store_category (s, "category", FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findCategory (QSnapdClient::MatchName, "category", "carrot1"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "carrot1");
}

static void
test_find_scope_narrow ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap1");
    s = mock_snapd_add_store_snap (snapd, "snap2");
    mock_snap_set_scope_is_wide (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap1");
}

static void
test_find_scope_wide ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap1");
    s = mock_snapd_add_store_snap (snapd, "snap2");
    mock_snap_set_scope_is_wide (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::ScopeWide, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (0));
    g_assert_true (snap1->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap2 (findRequest->snap (1));
    g_assert_true (snap2->name () == "snap2");
}

static void
test_find_common_id ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "snap1");
    mock_app_set_common_id (a, "com.example.snap1");
    s = mock_snapd_add_store_snap (snapd, "snap2");
    a = mock_snap_add_app (s, "snap2");
    mock_app_set_common_id (a, "com.example.snap2");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchCommonId, "com.example.snap2"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_true (snap->name () == "snap2");
}

static void
test_find_categories (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_category (s, "fruit", TRUE);
    mock_snap_add_category (s, "food", FALSE);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName, "apple"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert_cmpint (snap->categoryCount (), ==, 2);
    g_assert_true (snap->category (0)->name () == "fruit");
    g_assert_true (snap->category (0)->featured ());
    g_assert_true (snap->category (1)->name () == "food");
    g_assert_false (snap->category (1)->featured ());
}

static void
test_find_refreshable_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRefreshableRequest> findRefreshableRequest (client.findRefreshable ());
    findRefreshableRequest->runSync ();
    g_assert_cmpint (findRefreshableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRefreshableRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (findRefreshableRequest->snap (0));
    g_assert_true (snap0->name () == "snap1");
    g_assert_true (snap0->revision () == "1");
    QScopedPointer<QSnapdSnap> snap1 (findRefreshableRequest->snap (1));
    g_assert_true (snap1->name () == "snap3");
    g_assert_true (snap1->revision () == "1");
}

void
FindRefreshableHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert_true (snap0->name () == "snap1");
    g_assert_true (snap0->revision () == "1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert_true (snap1->name () == "snap3");
    g_assert_true (snap1->revision () == "1");

    g_main_loop_quit (loop);
}

static void
test_find_refreshable_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    FindRefreshableHandler findRefreshableHandler (loop, client.findRefreshable ());
    QObject::connect (findRefreshableHandler.request, &QSnapdFindRefreshableRequest::complete, &findRefreshableHandler, &FindRefreshableHandler::onComplete);
    findRefreshableHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_find_refreshable_no_updates ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRefreshableRequest> findRefreshableRequest (client.findRefreshable ());
    findRefreshableRequest->runSync ();
    g_assert_cmpint (findRefreshableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRefreshableRequest->snapCount (), ==, 0);
}

static void
test_install_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (snapd, "snap")), ==, "strict");
    g_assert_false (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_sync_multiple ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest1 (client.install ("snap1"));
    installRequest1->runSync ();
    g_assert_cmpint (installRequest1->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdInstallRequest> installRequest2 (client.install ("snap2"));
    installRequest2->runSync ();
    g_assert_cmpint (installRequest2->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdInstallRequest> installRequest3 (client.install ("snap3"));
    installRequest3->runSync ();
    g_assert_cmpint (installRequest3->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap3"));
}

void
InstallHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (snapd, "snap")), ==, "strict");
    g_assert_false (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
    g_assert_false (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));

    g_main_loop_quit (loop);
}

static void
test_install_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    InstallHandler installHandler (loop, snapd, client.install ("snap"));
    QObject::connect (installHandler.request, &QSnapdInstallRequest::complete, &installHandler, &InstallHandler::onComplete);
    installHandler.request->runAsync ();

    g_main_loop_run (loop);
}

void
InstallMultipleHandler::onComplete ()
{
    QSnapdInstallRequest *request;
    foreach (request, requests)
        g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    counter++;
    if (counter == requests.size ()) {
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap2"));
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap3"));

        g_main_loop_quit (loop);
    }
}

static void
test_install_async_multiple ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));

    QList<QSnapdInstallRequest*> requests;
    requests.append (client.install ("snap1"));
    requests.append (client.install ("snap2"));
    requests.append (client.install ("snap3"));
    InstallMultipleHandler installHandler (loop, snapd, requests);
    QSnapdInstallRequest *request;
    foreach (request, requests) {
        QObject::connect (request, &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleHandler::onComplete);
        request->runAsync ();
    }

    g_main_loop_run (loop);
}

void
InstallErrorHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::Failed);
    g_assert_true (request->errorString () == "ERROR");
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));

    g_main_loop_quit (loop);
}

static void
test_install_async_failure ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_error (s, "ERROR");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    InstallErrorHandler installHandler (loop, snapd, client.install ("snap"));
    QObject::connect (installHandler.request, &QSnapdInstallRequest::complete, &installHandler, &InstallErrorHandler::onComplete);
    installHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static gboolean
install_cancel_cb (QSnapdInstallRequest *request)
{
    request->cancel ();
    return G_SOURCE_REMOVE;
}

void
InstallCancelHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::Cancelled);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));

    g_main_loop_quit (loop);
}

static void
test_install_async_cancel ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    InstallCancelHandler installHandler (loop, snapd, client.install ("snap"));
    QObject::connect (installHandler.request, &QSnapdInstallRequest::complete, &installHandler, &InstallCancelHandler::onComplete);
    installHandler.request->runAsync ();
    g_idle_add ((GSourceFunc) install_cancel_cb, installHandler.request);

    g_main_loop_run (loop);
}

void
InstallMultipleCancelFirstHandler::checkComplete ()
{
    counter++;
    if (counter == requests.size ()) {
        g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap2"));
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap3"));

        g_main_loop_quit (loop);
    }
}

void
InstallMultipleCancelFirstHandler::onCompleteSnap1 ()
{
    g_assert_cmpint (requests[0]->error (), ==, QSnapdRequest::Cancelled);
    checkComplete ();
}

void
InstallMultipleCancelFirstHandler::onCompleteSnap2 ()
{
    g_assert_cmpint (requests[1]->error (), ==, QSnapdRequest::NoError);
    checkComplete ();
}

void
InstallMultipleCancelFirstHandler::onCompleteSnap3 ()
{
    g_assert_cmpint (requests[2]->error (), ==, QSnapdRequest::NoError);
    checkComplete ();
}

static void
test_install_async_multiple_cancel_first ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));

    QList<QSnapdInstallRequest*> requests;
    requests.append (client.install ("snap1"));
    requests.append (client.install ("snap2"));
    requests.append (client.install ("snap3"));
    InstallMultipleCancelFirstHandler installHandler (loop, snapd, requests);
    QObject::connect (requests[0], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelFirstHandler::onCompleteSnap1);
    requests[0]->runAsync ();
    QObject::connect (requests[1], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelFirstHandler::onCompleteSnap2);
    requests[1]->runAsync ();
    QObject::connect (requests[2], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelFirstHandler::onCompleteSnap3);
    requests[2]->runAsync ();
    g_idle_add ((GSourceFunc) install_cancel_cb, requests[0]);

    g_main_loop_run (loop);
}

void
InstallMultipleCancelLastHandler::checkComplete ()
{
    counter++;
    if (counter == requests.size ()) {
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap1"));
        g_assert_nonnull (mock_snapd_find_snap (snapd, "snap2"));
        g_assert_null (mock_snapd_find_snap (snapd, "snap3"));

        g_main_loop_quit (loop);
    }
}

void
InstallMultipleCancelLastHandler::onCompleteSnap1 ()
{
    g_assert_cmpint (requests[0]->error (), ==, QSnapdRequest::NoError);
    checkComplete ();
}

void
InstallMultipleCancelLastHandler::onCompleteSnap2 ()
{
    g_assert_cmpint (requests[1]->error (), ==, QSnapdRequest::NoError);
    checkComplete ();
}

void
InstallMultipleCancelLastHandler::onCompleteSnap3 ()
{
    g_assert_cmpint (requests[2]->error (), ==, QSnapdRequest::Cancelled);
    checkComplete ();
}

static void
test_install_async_multiple_cancel_last ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_snapd_add_store_snap (snapd, "snap2");
    mock_snapd_add_store_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap1"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap2"));
    g_assert_null (mock_snapd_find_snap (snapd, "snap3"));

    QList<QSnapdInstallRequest*> requests;
    requests.append (client.install ("snap1"));
    requests.append (client.install ("snap2"));
    requests.append (client.install ("snap3"));
    InstallMultipleCancelLastHandler installHandler (loop, snapd, requests);
    QObject::connect (requests[0], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelLastHandler::onCompleteSnap1);
    requests[0]->runAsync ();
    QObject::connect (requests[1], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelLastHandler::onCompleteSnap2);
    requests[1]->runAsync ();
    QObject::connect (requests[2], &QSnapdInstallRequest::complete, &installHandler, &InstallMultipleCancelLastHandler::onCompleteSnap3);
    requests[2]->runAsync ();
    g_idle_add ((GSourceFunc) install_cancel_cb, requests[2]);

    g_main_loop_run (loop);
}

void InstallProgressCounter::progress ()
{
    progressDone++;

    QScopedPointer<QSnapdChange> change (request->change ());

    // Check we've been notified of all tasks
    int done = 0, total = 0;
    for (int i = 0; i < change->taskCount (); i++) {
        QScopedPointer<QSnapdTask> task (change->task (i));
        done += task->progressDone ();
        total += task->progressTotal ();
    }
    g_assert_cmpint (progressDone, ==, done);

    g_assert_true (change->kind () == "KIND");
    g_assert_true (change->summary () == "SUMMARY");
    if (progressDone == total) {
        g_assert_true (change->status () == "Done");
        g_assert_true (change->ready ());
    }
    else {
        g_assert_true (change->status () == "Do");
        g_assert_false (change->ready ());
    }
    g_assert_true (change->spawnTime () == spawnTime);
    if (change->ready ())
        g_assert_true (readyTime == readyTime);
    else
        g_assert_false (readyTime.isValid ());
}

static void
test_install_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    InstallProgressCounter counter (installRequest.data ());
    counter.spawnTime = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    counter.readyTime = QDateTime (QDate (2017, 1, 3), QTime (0, 0, 0), Qt::UTC);
    mock_snapd_set_spawn_time (snapd, counter.spawnTime.toString (Qt::ISODate).toStdString ().c_str ());
    mock_snapd_set_ready_time (snapd, counter.readyTime.toString (Qt::ISODate).toStdString ().c_str ());
    QObject::connect (installRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_install_needs_classic ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NeedsClassic);
}

static void
test_install_classic ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_on_classic (snapd, TRUE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_confinement (mock_snapd_find_snap (snapd, "snap")), ==, "classic");
}

static void
test_install_not_classic ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_on_classic (snapd, TRUE);
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NotClassic);
}

static void
test_install_needs_classic_system ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NeedsClassicSystem);
}

static void
test_install_needs_devmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NeedsDevmode);
}

static void
test_install_devmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Devmode, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_dangerous ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Dangerous, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_jailmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Jailmode, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_true (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));
}

static void
test_install_channel ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel1");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", "channel2"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_channel (mock_snapd_find_snap (snapd, "snap")), ==, "channel2");
}

static void
test_install_revision ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.2");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", NULL, "1.1"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpstr (mock_snap_get_revision (mock_snapd_find_snap (snapd, "snap")), ==, "1.1");
}

static void
test_install_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NotFound);
}

static void
test_install_channel_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", "channel"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::ChannelNotAvailable);
}

static void
test_install_revision_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", NULL, "1.1"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::RevisionNotAvailable);
}

static void
test_install_snapd_restart ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_restart_required (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
}

static void
test_install_async_snapd_restart ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_restart_required (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    InstallHandler installHandler (loop, snapd, client.install ("snap"));
    QObject::connect (installHandler.request, &QSnapdInstallRequest::complete, &installHandler, &InstallHandler::onComplete);
    installHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_install_auth_cancelled ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    mock_snapd_set_decline_auth (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::AuthCancelled);
}

static void
test_install_stream_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (&buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

void
InstallStreamHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));

    g_main_loop_quit (loop);
}

static void
test_install_stream_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    InstallStreamHandler installHandler (loop, snapd, client.install (&buffer));
    QObject::connect (installHandler.request, &QSnapdInstallRequest::complete, &installHandler, &InstallStreamHandler::onComplete);
    installHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_install_stream_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (&buffer));
    ProgressCounter counter;
    QObject::connect (installRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_install_stream_classic ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "classic");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_dangerous ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Dangerous, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_true (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_devmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Devmode, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_true (mock_snap_get_devmode (snap));
    g_assert_false (mock_snap_get_jailmode (snap));
}

static void
test_install_stream_jailmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_snap (snapd, "sideload"));
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Jailmode, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_data (snap), ==, "SNAP");
    g_assert_cmpstr (mock_snap_get_confinement (snap), ==, "strict");
    g_assert_false (mock_snap_get_dangerous (snap));
    g_assert_false (mock_snap_get_devmode (snap));
    g_assert_true (mock_snap_get_jailmode (snap));
}

static void
test_try_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdTryRequest> tryRequest (client.trySnap ("/path/to/snap"));
    tryRequest->runSync ();
    g_assert_cmpint (tryRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "try");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_path (snap), ==, "/path/to/snap");
}

void
TryHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "try");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_path (snap), ==, "/path/to/snap");

    g_main_loop_quit (loop);
}

static void
test_try_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    TryHandler tryHandler (loop, snapd, client.trySnap ("/path/to/snap"));
    QObject::connect (tryHandler.request, &QSnapdTryRequest::complete, &tryHandler, &TryHandler::onComplete);
    tryHandler.request->runAsync ();
}

static void
test_try_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdTryRequest> tryRequest (client.trySnap ("/path/to/snap"));
    ProgressCounter counter;
    QObject::connect (tryRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    tryRequest->runSync ();
    g_assert_cmpint (tryRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "try");
    g_assert_nonnull (snap);
    g_assert_cmpstr (mock_snap_get_path (snap), ==, "/path/to/snap");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_try_not_a_snap ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdTryRequest> tryRequest (client.trySnap ("*"));
    tryRequest->runSync ();
    g_assert_cmpint (tryRequest->error (), ==, QSnapdRequest::NotASnap);
}

static void
test_refresh_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoError);
}

void
RefreshHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_main_loop_quit (loop);
}

static void
test_refresh_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    RefreshHandler refreshHandler (loop, client.refresh ("snap"));
    QObject::connect (refreshHandler.request, &QSnapdRefreshRequest::complete, &refreshHandler, &RefreshHandler::onComplete);
    refreshHandler.request->runAsync ();
}

static void
test_refresh_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    ProgressCounter counter;
    QObject::connect (refreshRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_refresh_channel ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    mock_snap_set_channel (s, "channel1");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");
    mock_snap_set_channel (s, "channel2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap", "channel2"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snap_get_channel (mock_snapd_find_snap (snapd, "snap")), ==, "channel2");
}

static void
test_refresh_no_updates ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoUpdateAvailable);
}

static void
test_refresh_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_refresh_not_in_store ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NotInStore);
}

static void
test_refresh_all_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshAllRequest> refreshAllRequest (client.refreshAll ());
    refreshAllRequest->runSync ();
    g_assert_cmpint (refreshAllRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (refreshAllRequest->snapNames ().count (), ==, 2);
    g_assert_true (refreshAllRequest->snapNames ()[0] == "snap1");
    g_assert_true (refreshAllRequest->snapNames ()[1] == "snap3");
}

void
RefreshAllHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapNames ().count (), ==, 2);
    g_assert_true (request->snapNames ()[0] == "snap1");
    g_assert_true (request->snapNames ()[1] == "snap3");

    g_main_loop_quit (loop);
}

static void
test_refresh_all_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    RefreshAllHandler refreshAllHandler (loop, client.refreshAll ());
    QObject::connect (refreshAllHandler.request, &QSnapdRefreshAllRequest::complete, &refreshAllHandler, &RefreshAllHandler::onComplete);
    refreshAllHandler.request->runAsync ();
}

static void
test_refresh_all_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_snap (snapd, "snap3");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap1");
    mock_snap_set_revision (s, "1");
    s = mock_snapd_add_store_snap (snapd, "snap3");
    mock_snap_set_revision (s, "1");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshAllRequest> refreshAllRequest (client.refreshAll ());
    ProgressCounter counter;
    QObject::connect (refreshAllRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    refreshAllRequest->runSync ();
    g_assert_cmpint (refreshAllRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (refreshAllRequest->snapNames ().count (), ==, 2);
    g_assert_true (refreshAllRequest->snapNames ()[0] == "snap1");
    g_assert_true (refreshAllRequest->snapNames ()[1] == "snap3");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_refresh_all_no_updates ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRefreshAllRequest> refreshAllRequest (client.refreshAll ());
    refreshAllRequest->runSync ();
    g_assert_cmpint (refreshAllRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (refreshAllRequest->snapNames ().count (), ==, 0);
}

static void
test_remove_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    g_assert_nonnull (mock_snapd_find_snapshot (snapd, "snap"));
}

void
RemoveHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    g_assert_nonnull (mock_snapd_find_snapshot (snapd, "snap"));

    g_main_loop_quit (loop);
}

static void
test_remove_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    RemoveHandler removeHandler (loop, snapd, client.remove ("snap"));
    QObject::connect (removeHandler.request, &QSnapdRemoveRequest::complete, &removeHandler, &RemoveHandler::onComplete);
    removeHandler.request->runAsync ();

    g_main_loop_run (loop);
}

void
RemoveErrorHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::Failed);
    g_assert_true (request->errorString () == "ERROR");
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));

    g_main_loop_quit (loop);
}

static void
test_remove_async_failure ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_error (s, "ERROR");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    RemoveErrorHandler removeHandler (loop, snapd, client.remove ("snap"));
    QObject::connect (removeHandler.request, &QSnapdRemoveRequest::complete, &removeHandler, &RemoveErrorHandler::onComplete);
    removeHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static gboolean
remove_cancel_cb (QSnapdRemoveRequest *request)
{
    request->cancel ();
    return G_SOURCE_REMOVE;
}

void
RemoveCancelHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::Cancelled);
    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));

    g_main_loop_quit (loop);
}

static void
test_remove_async_cancel ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    RemoveCancelHandler removeHandler (loop, snapd, client.remove ("snap"));
    QObject::connect (removeHandler.request, &QSnapdRemoveRequest::complete, &removeHandler, &RemoveCancelHandler::onComplete);
    removeHandler.request->runAsync ();
    g_idle_add ((GSourceFunc) remove_cancel_cb, removeHandler.request);

    g_main_loop_run (loop);
}

static void
test_remove_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    ProgressCounter counter;
    QObject::connect (removeRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_remove_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_remove_purge ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_nonnull (mock_snapd_find_snap (snapd, "snap"));
    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove (QSnapdClient::Purge, "snap"));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));
    g_assert_null (mock_snapd_find_snapshot (snapd, "snap"));
}

static void
test_enable_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_false (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
}

void
EnableHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_false (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));

    g_main_loop_quit (loop);
}

static void
test_enable_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    EnableHandler enableHandler (loop, snapd, client.enable ("snap"));
    QObject::connect (enableHandler.request, &QSnapdEnableRequest::complete, &enableHandler, &EnableHandler::onComplete);
    enableHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_enable_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    ProgressCounter counter;
    QObject::connect (enableRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_false (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_enable_already_enabled ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_enable_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_disable_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
}

void
DisableHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));

    g_main_loop_quit (loop);
}

static void
test_disable_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    DisableHandler disableHandler (loop, snapd, client.disable ("snap"));
    QObject::connect (disableHandler.request, &QSnapdDisableRequest::complete, &disableHandler, &DisableHandler::onComplete);
    disableHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_disable_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    ProgressCounter counter;
    QObject::connect (disableRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_disable_already_disabled ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_disabled (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_disable_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_switch_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_tracking_channel (s, "stable");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdSwitchChannelRequest> switchRequest (client.switchChannel ("snap", "beta"));
    switchRequest->runSync ();
    g_assert_cmpint (switchRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snap_get_tracking_channel (mock_snapd_find_snap (snapd, "snap")), ==, "beta");
}

void
SwitchChannelHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snap_get_tracking_channel (mock_snapd_find_snap (snapd, "snap")), ==, "beta");

    g_main_loop_quit (loop);
}

static void
test_switch_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_tracking_channel (s, "stable");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    SwitchChannelHandler switchHandler (loop, snapd, client.switchChannel ("snap", "beta"));
    QObject::connect (switchHandler.request, &QSnapdSwitchChannelRequest::complete, &switchHandler, &SwitchChannelHandler::onComplete);
    switchHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_switch_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_tracking_channel (s, "stable");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdSwitchChannelRequest> switchRequest (client.switchChannel ("snap", "beta"));
    ProgressCounter counter;
    QObject::connect (switchRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    switchRequest->runSync ();
    g_assert_cmpint (switchRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snap_get_tracking_channel (mock_snapd_find_snap (snapd, "snap")), ==, "beta");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_switch_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdSwitchChannelRequest> switchRequest (client.switchChannel ("snap", "beta"));
    switchRequest->runSync ();
    g_assert_cmpint (switchRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_check_buy_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdCheckBuyRequest> checkBuyRequest (client.checkBuy ());
    checkBuyRequest->runSync ();
    g_assert_cmpint (checkBuyRequest->error (), ==, QSnapdRequest::NoError);
}

void
CheckBuyHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_main_loop_quit (loop);
}

static void
test_check_buy_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    CheckBuyHandler checkBuyHandler (loop, snapd, client.checkBuy ());
    QObject::connect (checkBuyHandler.request, &QSnapdCheckBuyRequest::complete, &checkBuyHandler, &CheckBuyHandler::onComplete);
    checkBuyHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_check_buy_terms_not_accepted ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, FALSE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdCheckBuyRequest> checkBuyRequest (client.checkBuy ());
    checkBuyRequest->runSync ();
    g_assert_cmpint (checkBuyRequest->error (), ==, QSnapdRequest::TermsNotAccepted);
}

static void
test_check_buy_no_payment_methods ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, FALSE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdCheckBuyRequest> checkBuyRequest (client.checkBuy ());
    checkBuyRequest->runSync ();
    g_assert_cmpint (checkBuyRequest->error (), ==, QSnapdRequest::PaymentNotSetup);
}

static void
test_check_buy_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdCheckBuyRequest> checkBuyRequest (client.checkBuy ());
    checkBuyRequest->runSync ();
    g_assert_cmpint (checkBuyRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_buy_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.25, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::NoError);
}

void
BuyHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_main_loop_quit (loop);
}

static void
test_buy_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    BuyHandler buyHandler (loop, snapd, client.buy ("ABCDEF", 1.25, "NZD"));
    QObject::connect (buyHandler.request, &QSnapdBuyRequest::complete, &buyHandler, &BuyHandler::onComplete);
    buyHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_buy_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.25, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_buy_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.25, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::NotFound);
}

static void
test_buy_terms_not_accepted ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, FALSE);
    mock_account_set_has_payment_methods (a, FALSE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.25, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::TermsNotAccepted);
}

static void
test_buy_no_payment_methods ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, FALSE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.25, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::PaymentNotSetup);
}

static void
test_buy_invalid_price ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "test", "secret");
    mock_account_set_terms_accepted (a, TRUE);
    mock_account_set_has_payment_methods (a, TRUE);
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.25, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 0.75, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::PaymentDeclined);
}

static void
test_create_user_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    QScopedPointer<QSnapdCreateUserRequest> createRequest (client.createUser ("user@example.com"));
    createRequest->runSync ();
    g_assert_cmpint (createRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdUserInformation> userInformation (createRequest->userInformation ());
    g_assert_true (userInformation->username () == "user");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 2);
    g_assert_true (userInformation->sshKeys ()[0] == "KEY1");
    g_assert_true (userInformation->sshKeys ()[1] == "KEY2");
    MockAccount *account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_false (mock_account_get_sudoer (account));
    g_assert_false (mock_account_get_known (account));
}

static void
test_create_user_sudo ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    QScopedPointer<QSnapdCreateUserRequest> createRequest (client.createUser ("user@example.com", QSnapdClient::Sudo));
    createRequest->runSync ();
    g_assert_cmpint (createRequest->error (), ==, QSnapdRequest::NoError);
    MockAccount *account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_true (mock_account_get_sudoer (account));
}

static void
test_create_user_known ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_snapd_find_account_by_username (snapd, "user"));
    QScopedPointer<QSnapdCreateUserRequest> createRequest (client.createUser ("user@example.com", QSnapdClient::Known));
    createRequest->runSync ();
    g_assert_cmpint (createRequest->error (), ==, QSnapdRequest::NoError);
    MockAccount *account = mock_snapd_find_account_by_username (snapd, "user");
    g_assert_nonnull (account);
    g_assert_true (mock_account_get_known (account));
}

static void
test_create_users_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdCreateUsersRequest> createRequest (client.createUsers ());
    createRequest->runSync ();
    g_assert_cmpint (createRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (createRequest->userInformationCount (), ==, 3);
    QScopedPointer<QSnapdUserInformation> userInformation0 (createRequest->userInformation (0));
    g_assert_true (userInformation0->username () == "admin");
    g_assert_cmpint (userInformation0->sshKeys ().count (), ==, 2);
    g_assert_true (userInformation0->sshKeys ()[0] == "KEY1");
    g_assert_true (userInformation0->sshKeys ()[1] == "KEY2");
    QScopedPointer<QSnapdUserInformation> userInformation1 (createRequest->userInformation (1));
    g_assert_true (userInformation1->username () == "alice");
    QScopedPointer<QSnapdUserInformation> userInformation2 (createRequest->userInformation (2));
    g_assert_true (userInformation2->username () == "bob");
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "admin"));
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "alice"));
    g_assert_nonnull (mock_snapd_find_account_by_username (snapd, "bob"));
}

static void
test_get_users_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "alice@example.com", "alice", "secret");
    mock_snapd_add_account (snapd, "bob@example.com", "bob", "secret");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetUsersRequest> getUsersRequest (client.getUsers ());
    getUsersRequest->runSync ();
    g_assert_cmpint (getUsersRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getUsersRequest->userInformationCount (), ==, 2);
    g_assert_cmpint (getUsersRequest->userInformation (0)->id (), ==, 1);
    g_assert_true (getUsersRequest->userInformation (0)->username () == "alice");
    g_assert_true (getUsersRequest->userInformation (0)->email () == "alice@example.com");
    g_assert_cmpint (getUsersRequest->userInformation (1)->id (), ==, 2);
    g_assert_true (getUsersRequest->userInformation (1)->username () == "bob");
    g_assert_true (getUsersRequest->userInformation (1)->email () == "bob@example.com");
}

void
GetUsersHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->userInformationCount (), ==, 2);
    g_assert_cmpint (request->userInformation (0)->id (), ==, 1);
    g_assert_true (request->userInformation (0)->username () == "alice");
    g_assert_true (request->userInformation (0)->email () == "alice@example.com");
    g_assert_cmpint (request->userInformation (1)->id (), ==, 2);
    g_assert_true (request->userInformation (1)->username () == "bob");
    g_assert_true (request->userInformation (1)->email () == "bob@example.com");

    g_main_loop_quit (loop);
}

static void
test_get_users_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "alice@example.com", "alice", "secret");
    mock_snapd_add_account (snapd, "bob@example.com", "bob", "secret");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetUsersHandler getUsersHandler (loop, snapd, client.getUsers ());
    QObject::connect (getUsersHandler.request, &QSnapdGetUsersRequest::complete, &getUsersHandler, &GetUsersHandler::onComplete);
    getUsersHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_get_sections_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_category (snapd, "SECTION1");
    mock_snapd_add_store_category (snapd, "SECTION2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdGetSectionsRequest> getSectionsRequest (client.getSections ());
QT_WARNING_POP
    getSectionsRequest->runSync ();
    g_assert_cmpint (getSectionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSectionsRequest->sections ().count (), ==, 2);
    g_assert_true (getSectionsRequest->sections ()[0] == "SECTION1");
    g_assert_true (getSectionsRequest->sections ()[1] == "SECTION2");
}

void
GetSectionsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->sections ().count (), ==, 2);
    g_assert_true (request->sections ()[0] == "SECTION1");
    g_assert_true (request->sections ()[1] == "SECTION2");

    g_main_loop_quit (loop);
}

static void
test_get_sections_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_category (snapd, "SECTION1");
    mock_snapd_add_store_category (snapd, "SECTION2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    GetSectionsHandler getSectionsHandler (loop, client.getSections ());
QT_WARNING_POP
    QObject::connect (getSectionsHandler.request, &QSnapdGetSectionsRequest::complete, &getSectionsHandler, &GetSectionsHandler::onComplete);
    getSectionsHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_get_categories_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_category (snapd, "CATEGORY1");
    mock_snapd_add_store_category (snapd, "CATEGORY2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QScopedPointer<QSnapdGetCategoriesRequest> getCategoriesRequest (client.getCategories ());
QT_WARNING_POP
    getCategoriesRequest->runSync ();
    g_assert_cmpint (getCategoriesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getCategoriesRequest->categoryCount (), ==, 2);
    g_assert_true (getCategoriesRequest->category (0)->name () == "CATEGORY1");
    g_assert_true (getCategoriesRequest->category (1)->name () == "CATEGORY2");
}

void
GetCategoriesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->categoryCount (), ==, 2);
    g_assert_true (request->category (0)->name () == "CATEGORY1");
    g_assert_true (request->category (1)->name () == "CATEGORY2");

    g_main_loop_quit (loop);
}

static void
test_get_categories_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_category (snapd, "CATEGORY1");
    mock_snapd_add_store_category (snapd, "CATEGORY2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    GetCategoriesHandler getCategoriesHandler (loop, client.getCategories ());
QT_WARNING_POP
    QObject::connect (getCategoriesHandler.request, &QSnapdGetCategoriesRequest::complete, &getCategoriesHandler, &GetCategoriesHandler::onComplete);
    getCategoriesHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_aliases_get_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");

    mock_app_add_auto_alias (a, "alias1");

    mock_app_add_manual_alias (a, "alias2", TRUE);

    mock_app_add_auto_alias (a, "alias3");
    mock_app_add_manual_alias (a, "alias3", FALSE);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAliasesRequest> getAliasesRequest (client.getAliases ());
    getAliasesRequest->runSync ();
    g_assert_cmpint (getAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAliasesRequest->aliasCount (), ==, 3);
    QList<QSnapdAlias*> aliases;
    aliases.append (getAliasesRequest->alias (0));
    aliases.append (getAliasesRequest->alias (1));
    aliases.append (getAliasesRequest->alias (2));
    std::sort (aliases.begin (), aliases.end (), [](QSnapdAlias *a, QSnapdAlias *b) { return a->name () < b->name (); });
    g_assert_true (aliases[0]->name () == "alias1");
    g_assert_true (aliases[0]->snap () == "snap");
    g_assert_cmpint (aliases[0]->status (), ==, QSnapdEnums::AliasStatusAuto);
    g_assert_true (aliases[0]->appAuto () == "app");
    g_assert_null (aliases[0]->appManual ());
    g_assert_true (aliases[1]->name () == "alias2");
    g_assert_true (aliases[1]->snap () == "snap");
    g_assert_cmpint (aliases[1]->status (), ==, QSnapdEnums::AliasStatusManual);
    g_assert_null (aliases[1]->appAuto ());
    g_assert_true (aliases[1]->appManual () == "app");
    g_assert_true (aliases[2]->name () == "alias3");
    g_assert_true (aliases[2]->snap () == "snap");
    g_assert_cmpint (aliases[2]->status (), ==, QSnapdEnums::AliasStatusDisabled);
    g_assert_true (aliases[2]->appAuto () == "app");
    g_assert_null (aliases[2]->appManual ());
}

void
GetAliasesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->aliasCount (), ==, 3);
    QList<QSnapdAlias*> aliases;
    aliases.append (request->alias (0));
    aliases.append (request->alias (1));
    aliases.append (request->alias (2));
    std::sort (aliases.begin (), aliases.end (), [](QSnapdAlias *a, QSnapdAlias *b) { return a->name () < b->name (); });
    g_assert_true (aliases[0]->name () == "alias1");
    g_assert_true (aliases[0]->snap () == "snap");
    g_assert_cmpint (aliases[0]->status (), ==, QSnapdEnums::AliasStatusAuto);
    g_assert_true (aliases[0]->appAuto () == "app");
    g_assert_null (aliases[0]->appManual ());
    g_assert_true (aliases[1]->name () == "alias2");
    g_assert_true (aliases[1]->snap () == "snap");
    g_assert_cmpint (aliases[1]->status (), ==, QSnapdEnums::AliasStatusManual);
    g_assert_null (aliases[1]->appAuto ());
    g_assert_true (aliases[1]->appManual () == "app");
    g_assert_true (aliases[2]->name () == "alias3");
    g_assert_true (aliases[2]->snap () == "snap");
    g_assert_cmpint (aliases[2]->status (), ==, QSnapdEnums::AliasStatusDisabled);
    g_assert_true (aliases[2]->appAuto () == "app");
    g_assert_null (aliases[2]->appManual ());

    g_main_loop_quit (loop);
}

static void
test_aliases_get_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");

    mock_app_add_auto_alias (a, "alias1");

    mock_app_add_manual_alias (a, "alias2", TRUE);

    mock_app_add_auto_alias (a, "alias3");
    mock_app_add_manual_alias (a, "alias3", FALSE);

    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetAliasesHandler getAliasesHandler (loop, client.getAliases ());
    QObject::connect (getAliasesHandler.request, &QSnapdGetAliasesRequest::complete, &getAliasesHandler, &GetAliasesHandler::onComplete);
    getAliasesHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_aliases_get_empty ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetAliasesRequest> getAliasesRequest (client.getAliases ());
    getAliasesRequest->runSync ();
    g_assert_cmpint (getAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAliasesRequest->aliasCount (), ==, 0);
}

static void
test_aliases_alias_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_app_find_alias (a, "foo"));
    QScopedPointer<QSnapdAliasRequest> aliasRequest (client.alias ("snap", "app", "foo"));
    aliasRequest->runSync ();
    g_assert_cmpint (aliasRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_app_find_alias (a, "foo"));
}

void
AliasHandler::onComplete ()
{
    MockSnap *s = mock_snapd_find_snap (snapd, "snap");
    MockApp *a = mock_snap_find_app (s, "app");

    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_nonnull (mock_app_find_alias (a, "foo"));

    g_main_loop_quit (loop);
}

static void
test_aliases_alias_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_null (mock_app_find_alias (a, "foo"));
    AliasHandler aliasHandler (loop, snapd, client.alias ("snap", "app", "foo"));
    QObject::connect (aliasHandler.request, &QSnapdAliasRequest::complete, &aliasHandler, &AliasHandler::onComplete);
    aliasHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_aliases_unalias_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    mock_app_add_manual_alias (a, "foo", TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdUnaliasRequest> unaliasRequest (client.unalias ("snap", "foo"));
    unaliasRequest->runSync ();
    g_assert_cmpint (unaliasRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_app_find_alias (a, "foo"));
}

void
UnaliasHandler::onComplete ()
{
    MockSnap *s = mock_snapd_find_snap (snapd, "snap");
    MockApp *a = mock_snap_find_app (s, "app");

    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_app_find_alias (a, "foo"));

    g_main_loop_quit (loop);
}

static void
test_aliases_unalias_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    mock_app_add_manual_alias (a, "foo", TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    UnaliasHandler unaliasHandler (loop, snapd, client.unalias ("snap", "foo"));
    QObject::connect (unaliasHandler.request, &QSnapdUnaliasRequest::complete, &unaliasHandler, &UnaliasHandler::onComplete);
    unaliasHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_aliases_unalias_no_snap_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    mock_app_add_manual_alias (a, "foo", TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdUnaliasRequest> unaliasRequest (client.unalias ("foo"));
    unaliasRequest->runSync ();
    g_assert_cmpint (unaliasRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_app_find_alias (a, "foo"));
}

static void
test_aliases_prefer_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_false (mock_snap_get_preferred (s));
    QScopedPointer<QSnapdPreferRequest> preferRequest (client.prefer ("snap"));
    preferRequest->runSync ();
    g_assert_cmpint (preferRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snap_get_preferred (s));
}

void
PreferHandler::onComplete ()
{
    MockSnap *s = mock_snapd_find_snap (snapd, "snap");

    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_true (mock_snap_get_preferred (s));

    g_main_loop_quit (loop);
}

static void
test_aliases_prefer_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert_false (mock_snap_get_preferred (s));
    PreferHandler preferHandler (loop, snapd, client.prefer ("snap"));
    QObject::connect (preferHandler.request, &QSnapdPreferRequest::complete, &preferHandler, &PreferHandler::onComplete);
    preferHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_run_snapctl_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRunSnapCtlRequest> runSnapCtlRequest (client.runSnapCtl ("ABC", QStringList () << "arg1" << "arg2"));
    runSnapCtlRequest->runSync ();
    g_assert_cmpint (runSnapCtlRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (runSnapCtlRequest->stdout () == "STDOUT:ABC:arg1:arg2");
    g_assert_true (runSnapCtlRequest->stderr () == "STDERR");
    g_assert_cmpint (runSnapCtlRequest->exitCode (), ==, 0);
}

static void
test_run_snapctl_unsuccessful ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdRunSnapCtlRequest> runSnapCtlRequest (client.runSnapCtl ("return-error", QStringList () << "arg1" << "arg2"));
    runSnapCtlRequest->runSync ();
    g_assert_cmpint (runSnapCtlRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_true (runSnapCtlRequest->stdout () == "STDOUT:return-error:arg1:arg2");
    g_assert_true (runSnapCtlRequest->stderr () == "STDERR");
    g_assert_cmpint (runSnapCtlRequest->exitCode (), ==, 1);
}

void
RunSnapCtlHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_true (request->stdout () == "STDOUT:ABC:arg1:arg2");
    g_assert_true (request->stderr () == "STDERR");
    g_assert_cmpint (request->exitCode (), ==, 0);

    g_main_loop_quit (loop);
}

static void
test_run_snapctl_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    RunSnapCtlHandler runSnapCtlHandler (loop, client.runSnapCtl ("ABC", QStringList () << "arg1" << "arg2"));
    QObject::connect (runSnapCtlHandler.request, &QSnapdRunSnapCtlRequest::complete, &runSnapCtlHandler, &RunSnapCtlHandler::onComplete);
    runSnapCtlHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_download_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDownloadRequest> downloadRequest (client.download ("test"));
    downloadRequest->runSync ();
    g_assert_cmpint (downloadRequest->error (), ==, QSnapdRequest::NoError);
    QByteArray data = downloadRequest->data ();
    g_assert_cmpmem (data.data (), data.size (), "SNAP:name=test", 14);
}

void
DownloadHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    QByteArray data = request->data ();
    g_assert_cmpmem (data.data (), data.size (), "SNAP:name=test", 14);

    g_main_loop_quit (loop);
}

static void
test_download_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    DownloadHandler downloadHandler (loop, client.download ("test"));
    QObject::connect (downloadHandler.request, &QSnapdDownloadRequest::complete, &downloadHandler, &DownloadHandler::onComplete);
    downloadHandler.request->runAsync ();
    g_main_loop_run (loop);
}

static void
test_download_channel_revision ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDownloadRequest> downloadRequest (client.download ("test", "CHANNEL", "REVISION"));
    downloadRequest->runSync ();
    g_assert_cmpint (downloadRequest->error (), ==, QSnapdRequest::NoError);
    QByteArray data = downloadRequest->data ();
    g_assert_cmpmem (data.data (), data.size (), "SNAP:name=test:channel=CHANNEL:revision=REVISION", 48);
}

static void
test_themes_check_sync (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "installed");
    mock_snapd_set_gtk_theme_status (snapd, "gtktheme2", "available");
    mock_snapd_set_gtk_theme_status (snapd, "gtktheme3", "unavailable");
    mock_snapd_set_icon_theme_status (snapd, "icontheme1", "installed");
    mock_snapd_set_icon_theme_status (snapd, "icontheme2", "available");
    mock_snapd_set_icon_theme_status (snapd, "icontheme3", "unavailable");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme1", "installed");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme2", "available");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme3", "unavailable");

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdCheckThemesRequest> checkThemesRequest (client.checkThemes (QStringList () << "gtktheme1" << "gtktheme2" << "gtktheme3", QStringList () << "icontheme1" << "icontheme2" << "icontheme3", QStringList() << "soundtheme1" << "soundtheme2" << "soundtheme3"));
    checkThemesRequest->runSync ();
    g_assert_cmpint (checkThemesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (checkThemesRequest->gtkThemeStatus ("gtktheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (checkThemesRequest->gtkThemeStatus ("gtktheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (checkThemesRequest->gtkThemeStatus ("gtktheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);

    g_assert_cmpint (checkThemesRequest->iconThemeStatus ("icontheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (checkThemesRequest->iconThemeStatus ("icontheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (checkThemesRequest->iconThemeStatus ("icontheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);

    g_assert_cmpint (checkThemesRequest->soundThemeStatus ("soundtheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (checkThemesRequest->soundThemeStatus ("soundtheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (checkThemesRequest->soundThemeStatus ("soundtheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);
}

void
CheckThemesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (request->gtkThemeStatus ("gtktheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (request->gtkThemeStatus ("gtktheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (request->gtkThemeStatus ("gtktheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);

    g_assert_cmpint (request->iconThemeStatus ("icontheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (request->iconThemeStatus ("icontheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (request->iconThemeStatus ("icontheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);

    g_assert_cmpint (request->soundThemeStatus ("soundtheme1"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeInstalled);
    g_assert_cmpint (request->soundThemeStatus ("soundtheme2"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeAvailable);
    g_assert_cmpint (request->soundThemeStatus ("soundtheme3"), ==, QSnapdCheckThemesRequest::ThemeStatus::ThemeUnavailable);

    g_main_loop_quit (loop);
}

static void
test_themes_check_async (void)
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "installed");
    mock_snapd_set_gtk_theme_status (snapd, "gtktheme2", "available");
    mock_snapd_set_gtk_theme_status (snapd, "gtktheme3", "unavailable");
    mock_snapd_set_icon_theme_status (snapd, "icontheme1", "installed");
    mock_snapd_set_icon_theme_status (snapd, "icontheme2", "available");
    mock_snapd_set_icon_theme_status (snapd, "icontheme3", "unavailable");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme1", "installed");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme2", "available");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme3", "unavailable");

    g_autoptr(GError) error = NULL;
    g_assert_true (mock_snapd_start (snapd, &error));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    CheckThemesHandler checkThemesHandler (loop, client.checkThemes (QStringList () << "gtktheme1" << "gtktheme2" << "gtktheme3", QStringList () << "icontheme1" << "icontheme2" << "icontheme3", QStringList() << "soundtheme1" << "soundtheme2" << "soundtheme3"));
    QObject::connect (checkThemesHandler.request, &QSnapdCheckThemesRequest::complete, &checkThemesHandler, &CheckThemesHandler::onComplete);
    checkThemesHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_themes_install_sync (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "available");
    mock_snapd_set_icon_theme_status (snapd, "icontheme1", "available");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme1", "available");

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallThemesRequest> installThemesRequest (client.installThemes (QStringList ("gtktheme1"), QStringList ("icontheme1"), QStringList("soundtheme1")));
    installThemesRequest->runSync ();
    g_assert_cmpint (installThemesRequest->error (), ==, QSnapdRequest::NoError);
}

void
InstallThemesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_main_loop_quit (loop);
}

static void
test_themes_install_async (void)
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "available");
    mock_snapd_set_icon_theme_status (snapd, "icontheme1", "available");
    mock_snapd_set_sound_theme_status (snapd, "soundtheme1", "available");

    g_autoptr(GError) error = NULL;
    g_assert_true (mock_snapd_start (snapd, &error));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    InstallThemesHandler installThemesHandler (loop, client.installThemes (QStringList ("gtktheme1"), QStringList ("icontheme1"), QStringList("soundtheme1")));
    QObject::connect (installThemesHandler.request, &QSnapdInstallThemesRequest::complete, &installThemesHandler, &InstallThemesHandler::onComplete);
    installThemesHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_themes_install_no_snaps (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "installed");
    mock_snapd_set_icon_theme_status (snapd, "icontheme1", "unavailable");

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallThemesRequest> installThemesRequest (client.installThemes (QStringList ("gtktheme1"), QStringList ("icontheme1"), QStringList()));
    installThemesRequest->runSync ();
    g_assert_cmpint (installThemesRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_themes_install_progress (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_spawn_time (snapd, "2017-01-02T11:23:58Z");
    mock_snapd_set_ready_time (snapd, "2017-01-03T00:00:00Z");
    mock_snapd_set_gtk_theme_status (snapd, "gtktheme1", "available");

    g_autoptr(GError) error = NULL;
    g_assert_true (mock_snapd_start (snapd, &error));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdInstallThemesRequest> installThemesRequest (client.installThemes (QStringList ("gtktheme1"), QStringList ("icontheme1"), QStringList()));
    ProgressCounter counter;
    QObject::connect (installThemesRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    installThemesRequest->runSync ();
    g_assert_cmpint (installThemesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_stress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    for (int i = 0; i < 10000; i++) {
        QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
        infoRequest->runSync ();
        g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
        QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
        g_assert_true (systemInformation->version () == "VERSION");
    }
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/socket-closed/before-request", test_socket_closed_before_request);
    g_test_add_func ("/socket-closed/after-request", test_socket_closed_after_request);
    g_test_add_func ("/client/set-socket-path", test_client_set_socket_path);
    g_test_add_func ("/user-agent/default", test_user_agent_default);
    g_test_add_func ("/user-agent/custom", test_user_agent_custom);
    g_test_add_func ("/user-agent/null", test_user_agent_null);
    g_test_add_func ("/accept-language/basic", test_accept_language);
    g_test_add_func ("/accept-language/empty", test_accept_language_empty);
    g_test_add_func ("/allow-interaction/basic", test_allow_interaction);
    g_test_add_func ("/maintenance/none", test_maintenance_none);
    g_test_add_func ("/maintenance/daemon-restart", test_maintenance_daemon_restart);
    g_test_add_func ("/maintenance/system-restart", test_maintenance_system_restart);
    g_test_add_func ("/maintenance/unknown", test_maintenance_unknown);
    g_test_add_func ("/get-system-information/sync", test_get_system_information_sync);
    g_test_add_func ("/get-system-information/async", test_get_system_information_async);
    g_test_add_func ("/get-system-information/store", test_get_system_information_store);
    g_test_add_func ("/get-system-information/refresh", test_get_system_information_refresh);
    g_test_add_func ("/get-system-information/refresh_schedule", test_get_system_information_refresh_schedule);
    g_test_add_func ("/get-system-information/confinement_strict", test_get_system_information_confinement_strict);
    g_test_add_func ("/get-system-information/confinement_none", test_get_system_information_confinement_none);
    g_test_add_func ("/get-system-information/confinement_unknown", test_get_system_information_confinement_unknown);
    g_test_add_func ("/login/sync", test_login_sync);
    g_test_add_func ("/login/async", test_login_async);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/logout/sync", test_logout_sync);
    g_test_add_func ("/logout/async", test_logout_async);
    g_test_add_func ("/logout/no-auth", test_logout_no_auth);
    g_test_add_func ("/get-changes/sync", test_get_changes_sync);
    g_test_add_func ("/get-changes/async", test_get_changes_async);
    g_test_add_func ("/get-changes/filter-in-progress", test_get_changes_filter_in_progress);
    g_test_add_func ("/get-changes/filter-ready", test_get_changes_filter_ready);
    g_test_add_func ("/get-changes/filter-snap", test_get_changes_filter_snap);
    g_test_add_func ("/get-changes/filter-ready-snap", test_get_changes_filter_ready_snap);
    g_test_add_func ("/get-change/data", test_get_changes_data);
    g_test_add_func ("/get-change/sync", test_get_change_sync);
    g_test_add_func ("/get-change/async", test_get_change_async);
    g_test_add_func ("/abort-change/sync", test_abort_change_sync);
    g_test_add_func ("/abort-change/async", test_abort_change_async);
    g_test_add_func ("/list/sync", test_list_sync);
    g_test_add_func ("/list/async", test_list_async);
    g_test_add_func ("/get-snaps/sync", test_get_snaps_sync);
    g_test_add_func ("/get-snaps/async", test_get_snaps_async);
    g_test_add_func ("/get-snaps/filter", test_get_snaps_filter);
    g_test_add_func ("/list-one/sync", test_list_one_sync);
    g_test_add_func ("/list-one/async", test_list_one_async);
    g_test_add_func ("/get-snap/sync", test_get_snap_sync);
    g_test_add_func ("/get-snap/async", test_get_snap_async);
    g_test_add_func ("/get-snap/types", test_get_snap_types);
    g_test_add_func ("/get-snap/optional-fields", test_get_snap_optional_fields);
    g_test_add_func ("/get-snap/deprecated-fields", test_get_snap_deprecated_fields);
    g_test_add_func ("/get-snap/common-ids", test_get_snap_common_ids);
    g_test_add_func ("/get-snap/not-installed", test_get_snap_not_installed);
    g_test_add_func ("/get-snap/classic-confinement", test_get_snap_classic_confinement);
    g_test_add_func ("/get-snap/devmode-confinement", test_get_snap_devmode_confinement);
    g_test_add_func ("/get-snap/daemons", test_get_snap_daemons);
    g_test_add_func ("/get-snap/publisher-starred", test_get_snap_publisher_starred);
    g_test_add_func ("/get-snap/publisher-verified", test_get_snap_publisher_verified);
    g_test_add_func ("/get-snap/publisher-unproven", test_get_snap_publisher_unproven);
    g_test_add_func ("/get-snap/publisher-unknown-validation", test_get_snap_publisher_unknown_validation);
    g_test_add_func ("/get-snap-conf/sync", test_get_snap_conf_sync);
    g_test_add_func ("/get-snap-conf/async", test_get_snap_conf_async);
    g_test_add_func ("/get-snap-conf/key-filter", test_get_snap_conf_key_filter);
    g_test_add_func ("/get-snap-conf/invalid-key", test_get_snap_conf_invalid_key);
    g_test_add_func ("/set-snap-conf/sync", test_set_snap_conf_sync);
    g_test_add_func ("/set-snap-conf/async", test_set_snap_conf_async);
    g_test_add_func ("/set-snap-conf/invalid", test_set_snap_conf_invalid);
    g_test_add_func ("/get-apps/sync", test_get_apps_sync);
    g_test_add_func ("/get-apps/async", test_get_apps_async);
    g_test_add_func ("/get-apps/services", test_get_apps_services);
    g_test_add_func ("/get-apps/filter", test_get_apps_filter);
    g_test_add_func ("/icon/sync", test_icon_sync);
    g_test_add_func ("/icon/async", test_icon_async);
    g_test_add_func ("/icon/not-installed", test_icon_not_installed);
    g_test_add_func ("/icon/large", test_icon_large);
    g_test_add_func ("/get-assertions/sync", test_get_assertions_sync);
    //g_test_add_func ("/get-assertions/async", test_get_assertions_async);
    g_test_add_func ("/get-assertions/body", test_get_assertions_body);
    g_test_add_func ("/get-assertions/multiple", test_get_assertions_multiple);
    g_test_add_func ("/get-assertions/invalid", test_get_assertions_invalid);
    g_test_add_func ("/add-assertions/sync", test_add_assertions_sync);
    //g_test_add_func ("/add-assertions/async", test_add_assertions_async);
    g_test_add_func ("/assertions/sync", test_assertions_sync);
    //g_test_add_func ("/assertions/async", test_assertions_async);
    g_test_add_func ("/assertions/body", test_assertions_body);
    g_test_add_func ("/get-connections/sync", test_get_connections_sync);
    g_test_add_func ("/get-connections/async", test_get_connections_async);
    g_test_add_func ("/get-connections/empty", test_get_connections_empty);
    g_test_add_func ("/get-connections/filter-all", test_get_connections_filter_all);
    g_test_add_func ("/get-connections/filter-snap", test_get_connections_filter_snap);
    g_test_add_func ("/get-connections/filter-interface", test_get_connections_filter_interface);
    g_test_add_func ("/get-connections/attributes", test_get_connections_attributes);
    g_test_add_func ("/get-interfaces/sync", test_get_interfaces_sync);
    g_test_add_func ("/get-interfaces/async", test_get_interfaces_async);
    g_test_add_func ("/get-interfaces/no-snaps", test_get_interfaces_no_snaps);
    g_test_add_func ("/get-interfaces/attributes", test_get_interfaces_attributes);
    g_test_add_func ("/get-interfaces/legacy", test_get_interfaces_legacy);
    g_test_add_func ("/get-interfaces2/sync", test_get_interfaces2_sync);
    g_test_add_func ("/get-interfaces2/async", test_get_interfaces2_async);
    g_test_add_func ("/get-interfaces2/only-connected", test_get_interfaces2_only_connected);
    g_test_add_func ("/get-interfaces2/slots", test_get_interfaces2_slots);
    g_test_add_func ("/get-interfaces2/plugs", test_get_interfaces2_plugs);
    g_test_add_func ("/get-interfaces2/filter", test_get_interfaces2_filter);
    g_test_add_func ("/get-interfaces2/make-label", test_get_interfaces2_make_label);
    g_test_add_func ("/connect-interface/sync", test_connect_interface_sync);
    g_test_add_func ("/connect-interface/async", test_connect_interface_async);
    g_test_add_func ("/connect-interface/progress", test_connect_interface_progress);
    g_test_add_func ("/connect-interface/invalid", test_connect_interface_invalid);
    g_test_add_func ("/disconnect-interface/sync", test_disconnect_interface_sync);
    g_test_add_func ("/disconnect-interface/async", test_disconnect_interface_async);
    g_test_add_func ("/disconnect-interface/progress", test_disconnect_interface_progress);
    g_test_add_func ("/disconnect-interface/invalid", test_disconnect_interface_invalid);
    g_test_add_func ("/find/query", test_find_query);
    g_test_add_func ("/find/query-private", test_find_query_private);
    g_test_add_func ("/find/query-private/not-logged-in", test_find_query_private_not_logged_in);
    g_test_add_func ("/find/bad-query", test_find_bad_query);
    g_test_add_func ("/find/network-timeout", test_find_network_timeout);
    g_test_add_func ("/find/dns-failure", test_find_dns_failure);
    g_test_add_func ("/find/name", test_find_name);
    g_test_add_func ("/find/name-private", test_find_name_private);
    g_test_add_func ("/find/name-private/not-logged-in", test_find_name_private_not_logged_in);
    g_test_add_func ("/find/channels", test_find_channels);
    g_test_add_func ("/find/channels-match", test_find_channels_match);
    g_test_add_func ("/find/cancel", test_find_cancel);
    g_test_add_func ("/find/section", test_find_section);
    g_test_add_func ("/find/section-query", test_find_section_query);
    g_test_add_func ("/find/section-name", test_find_section_name);
    g_test_add_func ("/find/category", test_find_category);
    g_test_add_func ("/find/category-query", test_find_category_query);
    g_test_add_func ("/find/category-name", test_find_category_name);
    g_test_add_func ("/find/scope-narrow", test_find_scope_narrow);
    g_test_add_func ("/find/scope-wide", test_find_scope_wide);
    g_test_add_func ("/find/common-id", test_find_common_id);
    g_test_add_func ("/find/categories", test_find_categories);
    g_test_add_func ("/find-refreshable/sync", test_find_refreshable_sync);
    g_test_add_func ("/find-refreshable/async", test_find_refreshable_async);
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
    g_test_add_func ("/install/not-classic", test_install_not_classic);
    g_test_add_func ("/install/needs-classic-system", test_install_needs_classic_system);
    g_test_add_func ("/install/needs-devmode", test_install_needs_devmode);
    g_test_add_func ("/install/devmode", test_install_devmode);
    g_test_add_func ("/install/dangerous", test_install_dangerous);
    g_test_add_func ("/install/jailmode", test_install_jailmode);
    g_test_add_func ("/install/channel", test_install_channel);
    g_test_add_func ("/install/revision", test_install_revision);
    g_test_add_func ("/install/not-available", test_install_not_available);
    g_test_add_func ("/install/channel-not-available", test_install_channel_not_available);
    g_test_add_func ("/install/revision-not-available", test_install_revision_not_available);
    g_test_add_func ("/install/snapd-restart", test_install_snapd_restart);
    g_test_add_func ("/install/async-snapd-restart", test_install_async_snapd_restart);
    g_test_add_func ("/install/auth-cancelled", test_install_auth_cancelled);
    g_test_add_func ("/install-stream/sync", test_install_stream_sync);
    g_test_add_func ("/install-stream/async", test_install_stream_async);
    g_test_add_func ("/install-stream/progress", test_install_stream_progress);
    g_test_add_func ("/install-stream/classic", test_install_stream_classic);
    g_test_add_func ("/install-stream/dangerous", test_install_stream_dangerous);
    g_test_add_func ("/install-stream/devmode", test_install_stream_devmode);
    g_test_add_func ("/install-stream/jailmode", test_install_stream_jailmode);
    g_test_add_func ("/try/sync", test_try_sync);
    g_test_add_func ("/try/async", test_try_async);
    g_test_add_func ("/try/progress", test_try_progress);
    g_test_add_func ("/try/not-a-snap", test_try_not_a_snap);
    g_test_add_func ("/refresh/sync", test_refresh_sync);
    g_test_add_func ("/refresh/async", test_refresh_async);
    g_test_add_func ("/refresh/progress", test_refresh_progress);
    g_test_add_func ("/refresh/channel", test_refresh_channel);
    g_test_add_func ("/refresh/no-updates", test_refresh_no_updates);
    g_test_add_func ("/refresh/not-installed", test_refresh_not_installed);
    g_test_add_func ("/refresh/not-in-store", test_refresh_not_in_store);
    g_test_add_func ("/refresh-all/sync", test_refresh_all_sync);
    g_test_add_func ("/refresh-all/async", test_refresh_all_async);
    g_test_add_func ("/refresh-all/progress", test_refresh_all_progress);
    g_test_add_func ("/refresh-all/no-updates", test_refresh_all_no_updates);
    g_test_add_func ("/remove/sync", test_remove_sync);
    g_test_add_func ("/remove/async", test_remove_async);
    g_test_add_func ("/remove/async-failure", test_remove_async_failure);
    g_test_add_func ("/remove/async-cancel", test_remove_async_cancel);
    g_test_add_func ("/remove/progress", test_remove_progress);
    g_test_add_func ("/remove/not-installed", test_remove_not_installed);
    g_test_add_func ("/remove/purge", test_remove_purge);
    g_test_add_func ("/enable/sync", test_enable_sync);
    g_test_add_func ("/enable/async", test_enable_async);
    g_test_add_func ("/enable/progress", test_enable_progress);
    g_test_add_func ("/enable/already-enabled", test_enable_already_enabled);
    g_test_add_func ("/enable/not-installed", test_enable_not_installed);
    g_test_add_func ("/disable/sync", test_disable_sync);
    g_test_add_func ("/disable/async", test_disable_async);
    g_test_add_func ("/disable/progress", test_disable_progress);
    g_test_add_func ("/disable/already-disabled", test_disable_already_disabled);
    g_test_add_func ("/disable/not-installed", test_disable_not_installed);
    g_test_add_func ("/switch/sync", test_switch_sync);
    g_test_add_func ("/switch/async", test_switch_async);
    g_test_add_func ("/switch/progress", test_switch_progress);
    g_test_add_func ("/switch/not-installed", test_switch_not_installed);
    g_test_add_func ("/check-buy/sync", test_check_buy_sync);
    g_test_add_func ("/check-buy/async", test_check_buy_async);
    g_test_add_func ("/check-buy/no-terms-not-accepted", test_check_buy_terms_not_accepted);
    g_test_add_func ("/check-buy/no-payment-methods", test_check_buy_no_payment_methods);
    g_test_add_func ("/check-buy/not-logged-in", test_check_buy_not_logged_in);
    g_test_add_func ("/buy/sync", test_buy_sync);
    g_test_add_func ("/buy/async", test_buy_async);
    g_test_add_func ("/buy/not-logged-in", test_buy_not_logged_in);
    g_test_add_func ("/buy/not-available", test_buy_not_available);
    g_test_add_func ("/buy/terms-not-accepted", test_buy_terms_not_accepted);
    g_test_add_func ("/buy/no-payment-methods", test_buy_no_payment_methods);
    g_test_add_func ("/buy/invalid-price", test_buy_invalid_price);
    g_test_add_func ("/create-user/sync", test_create_user_sync);
    //g_test_add_func ("/create-user/async", test_create_user_async);
    g_test_add_func ("/create-user/sudo", test_create_user_sudo);
    g_test_add_func ("/create-user/known", test_create_user_known);
    g_test_add_func ("/create-users/sync", test_create_users_sync);
    //g_test_add_func ("/create-users/async", test_create_users_async);
    g_test_add_func ("/get-users/sync", test_get_users_sync);
    g_test_add_func ("/get-users/async", test_get_users_async);
    g_test_add_func ("/get-sections/sync", test_get_sections_sync);
    g_test_add_func ("/get-sections/async", test_get_sections_async);
    g_test_add_func ("/get-categories/sync", test_get_categories_sync);
    g_test_add_func ("/get-categories/async", test_get_categories_async);
    g_test_add_func ("/aliases/get-sync", test_aliases_get_sync);
    g_test_add_func ("/aliases/get-async", test_aliases_get_async);
    g_test_add_func ("/aliases/get-empty", test_aliases_get_empty);
    g_test_add_func ("/aliases/alias-sync", test_aliases_alias_sync);
    g_test_add_func ("/aliases/alias-async", test_aliases_alias_async);
    g_test_add_func ("/aliases/unalias-sync", test_aliases_unalias_sync);
    g_test_add_func ("/aliases/unalias-async", test_aliases_unalias_async);
    g_test_add_func ("/aliases/unalias-no-snap-sync", test_aliases_unalias_no_snap_sync);
    g_test_add_func ("/aliases/prefer-sync", test_aliases_prefer_sync);
    g_test_add_func ("/aliases/prefer-async", test_aliases_prefer_async);
    g_test_add_func ("/run-snapctl/sync", test_run_snapctl_sync);
    g_test_add_func ("/run-snapctl/async", test_run_snapctl_async);
    g_test_add_func ("/run-snapctl/unsuccessful", test_run_snapctl_unsuccessful);
    g_test_add_func ("/download/sync", test_download_sync);
    g_test_add_func ("/download/async", test_download_async);
    g_test_add_func ("/download/channel-revision", test_download_channel_revision);
    g_test_add_func ("/themes/check/sync", test_themes_check_sync);
    g_test_add_func ("/themes/check/async", test_themes_check_async);
    g_test_add_func ("/themes/install/sync", test_themes_install_sync);
    g_test_add_func ("/themes/install/async", test_themes_install_async);
    g_test_add_func ("/themes/install/no-snaps", test_themes_install_no_snaps);
    g_test_add_func ("/themes/install/progress", test_themes_install_progress);
    g_test_add_func ("/stress/basic", test_stress);

    return g_test_run ();
}
