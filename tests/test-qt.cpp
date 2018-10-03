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
#include <Snapd/Client>
#include <Snapd/Assertion>

#include "config.h"
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
test_user_agent_default ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    g_assert (client.userAgent () == "snapd-glib/" VERSION);

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
    g_assert (systemInformation->buildId () == "efdd0b5e69b0742fa5e5bad0771df4d1df2459d1");
    g_assert (systemInformation->confinement () == QSnapdEnums::SystemConfinementUnknown);
    g_assert (systemInformation->kernelVersion () == "KERNEL-VERSION");
    g_assert (systemInformation->osId () == "OS-ID");
    g_assert (systemInformation->osVersion () == "OS-VERSION");
    g_assert (systemInformation->series () == "SERIES");
    g_assert (systemInformation->version () == "VERSION");
    g_assert_true (systemInformation->managed ());
    g_assert_true (systemInformation->onClassic ());
    g_assert (systemInformation->refreshSchedule ().isNull ());
    g_assert (systemInformation->refreshTimer () == "00:00~24:00/4");
    g_assert (systemInformation->refreshHold ().isNull ());
    g_assert (systemInformation->refreshLast ().isNull ());
    g_assert (systemInformation->refreshNext () == QDateTime (QDate (2018, 1, 19), QTime (13, 14, 15), Qt::UTC));
    g_assert (systemInformation->mountDirectory () == "/snap");
    g_assert (systemInformation->binariesDirectory () == "/snap/bin");
    g_assert_null (systemInformation->store ());
    QHash<QString, QStringList> sandbox_features = systemInformation->sandboxFeatures ();
    g_assert_cmpint (sandbox_features["backend"].count (), ==, 2);
    g_assert (sandbox_features["backend"][0] == "feature1");
    g_assert (sandbox_features["backend"][1] == "feature2");
}

void
GetSystemInformationHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (request->systemInformation ());
    g_assert (systemInformation->confinement () == QSnapdEnums::SystemConfinementUnknown);
    g_assert (systemInformation->kernelVersion () == "KERNEL-VERSION");
    g_assert (systemInformation->osId () == "OS-ID");
    g_assert (systemInformation->osVersion () == "OS-VERSION");
    g_assert (systemInformation->series () == "SERIES");
    g_assert (systemInformation->version () == "VERSION");
    g_assert_true (systemInformation->managed ());
    g_assert_true (systemInformation->onClassic ());
    g_assert (systemInformation->mountDirectory () == "/snap");
    g_assert (systemInformation->binariesDirectory () == "/snap/bin");
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
    g_assert (systemInformation->store () == "store");
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
    g_assert (systemInformation->refreshSchedule ().isNull ());
    g_assert (systemInformation->refreshTimer () == "00:00~24:00/4");
    g_assert (systemInformation->refreshHold () == QDateTime (QDate (2018, 1, 20), QTime (1, 2, 3), Qt::UTC));
    g_assert (systemInformation->refreshLast () == QDateTime (QDate (2018, 1, 19), QTime (1, 2, 3), Qt::UTC));
    g_assert (systemInformation->refreshNext () == QDateTime (QDate (2018, 1, 19), QTime (13, 14, 15), Qt::UTC));
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
    g_assert (systemInformation->refreshSchedule () == "00:00-04:59/5:00-10:59/11:00-16:59/17:00-23:59");
    g_assert (systemInformation->refreshTimer ().isNull ());
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
    g_assert (userInformation->email () == "test@example.com");
    g_assert (userInformation->username () == "test");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 0);
    g_assert (userInformation->authData ()->macaroon () == mock_account_get_macaroon (a));
    g_assert_cmpint (userInformation->authData ()->discharges ().count (), ==, g_strv_length (mock_account_get_discharges (a)));
    for (int i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert (userInformation->authData ()->discharges ()[i] == mock_account_get_discharges (a)[i]);
}

void
LoginHandler::onComplete ()
{
    MockAccount *a = mock_snapd_find_account_by_username (snapd, "test");

    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdUserInformation> userInformation (request->userInformation ());
    g_assert_cmpint (userInformation->id (), ==, 1);
    g_assert (userInformation->email () == "test@example.com");
    g_assert (userInformation->username () == "test");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 0);
    g_assert (userInformation->authData ()->macaroon () == mock_account_get_macaroon (a));
    g_assert_cmpint (userInformation->authData ()->discharges ().count (), ==, g_strv_length (mock_account_get_discharges (a)));
    for (int i = 0; mock_account_get_discharges (a)[i]; i++)
        g_assert (userInformation->authData ()->discharges ()[i] == mock_account_get_discharges (a)[i]);

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
    g_assert (change0->id () == "1");
    g_assert (change0->kind () == "KIND");
    g_assert (change0->summary () == "SUMMARY");
    g_assert (change0->status () == "Done");
    g_assert_true (change0->ready ());
    g_assert (change0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (change0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change0->error ());
    g_assert_cmpint (change0->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change0->task (0));
    g_assert (task0->id () == "100");
    g_assert (task0->kind () == "download");
    g_assert (task0->summary () == "SUMMARY");
    g_assert (task0->status () == "Done");
    g_assert (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change0->task (1));
    g_assert (task1->id () == "101");
    g_assert (task1->kind () == "install");
    g_assert (task1->summary () == "SUMMARY");
    g_assert (task1->status () == "Done");
    g_assert (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    QScopedPointer<QSnapdChange> change1 (changesRequest->change (1));
    g_assert (change1->id () == "2");
    g_assert (change1->kind () == "KIND");
    g_assert (change1->summary () == "SUMMARY");
    g_assert (change1->status () == "Do");
    g_assert_false (change1->ready ());
    g_assert (change1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (change1->readyTime ().isValid ());
    g_assert_null (change1->error ());
    g_assert_cmpint (change1->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task (change1->task (0));
    g_assert (task->id () == "200");
    g_assert (task->kind () == "remove");
    g_assert (task->summary () == "SUMMARY");
    g_assert (task->status () == "Do");
    g_assert (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 0);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (task->readyTime ().isValid ());
}

void
GetChangesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->changeCount (), ==, 2);

    QScopedPointer<QSnapdChange> change0 (request->change (0));
    g_assert (change0->id () == "1");
    g_assert (change0->kind () == "KIND");
    g_assert (change0->summary () == "SUMMARY");
    g_assert (change0->status () == "Done");
    g_assert_true (change0->ready ());
    g_assert (change0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (change0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change0->error ());
    g_assert_cmpint (change0->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change0->task (0));
    g_assert (task0->id () == "100");
    g_assert (task0->kind () == "download");
    g_assert (task0->summary () == "SUMMARY");
    g_assert (task0->status () == "Done");
    g_assert (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change0->task (1));
    g_assert (task1->id () == "101");
    g_assert (task1->kind () == "install");
    g_assert (task1->summary () == "SUMMARY");
    g_assert (task1->status () == "Done");
    g_assert (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    QScopedPointer<QSnapdChange> change1 (request->change (1));
    g_assert (change1->id () == "2");
    g_assert (change1->kind () == "KIND");
    g_assert (change1->summary () == "SUMMARY");
    g_assert (change1->status () == "Do");
    g_assert_false (change1->ready ());
    g_assert (change1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (change1->readyTime ().isValid ());
    g_assert_null (change1->error ());
    g_assert_cmpint (change1->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task (change1->task (0));
    g_assert (task->id () == "200");
    g_assert (task->kind () == "remove");
    g_assert (task->summary () == "SUMMARY");
    g_assert (task->status () == "Do");
    g_assert (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 0);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
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
    g_assert (changesRequest->change (0)->id () == "2");
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
    g_assert (changesRequest->change (0)->id () == "2");
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
    g_assert (changesRequest->change (0)->id () == "2");
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
    g_assert (changesRequest->change (0)->id () == "2");
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
    g_assert (change->id () == "1");
    g_assert (change->kind () == "KIND");
    g_assert (change->summary () == "SUMMARY");
    g_assert (change->status () == "Done");
    g_assert_true (change->ready ());
    g_assert (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (change->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change->error ());
    g_assert_cmpint (change->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert (task0->id () == "100");
    g_assert (task0->kind () == "download");
    g_assert (task0->summary () == "SUMMARY");
    g_assert (task0->status () == "Done");
    g_assert (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change->task (1));
    g_assert (task1->id () == "101");
    g_assert (task1->kind () == "install");
    g_assert (task1->summary () == "SUMMARY");
    g_assert (task1->status () == "Done");
    g_assert (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
}

void
GetChangeHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (request->change ());
    g_assert (change->id () == "1");
    g_assert (change->kind () == "KIND");
    g_assert (change->summary () == "SUMMARY");
    g_assert (change->status () == "Done");
    g_assert_true (change->ready ());
    g_assert (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (change->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_null (change->error ());
    g_assert_cmpint (change->taskCount (), ==, 2);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert (task0->id () == "100");
    g_assert (task0->kind () == "download");
    g_assert (task0->summary () == "SUMMARY");
    g_assert (task0->status () == "Done");
    g_assert (task0->progressLabel () == "LABEL");
    g_assert_cmpint (task0->progressDone (), ==, 65535);
    g_assert_cmpint (task0->progressTotal (), ==, 65535);
    g_assert (task0->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (task0->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    QScopedPointer<QSnapdTask> task1 (change->task (1));
    g_assert (task1->id () == "101");
    g_assert (task1->kind () == "install");
    g_assert (task1->summary () == "SUMMARY");
    g_assert (task1->status () == "Done");
    g_assert (task1->progressLabel () == "LABEL");
    g_assert_cmpint (task1->progressDone (), ==, 1);
    g_assert_cmpint (task1->progressTotal (), ==, 1);
    g_assert (task1->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert (task1->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    g_main_loop_quit (loop);
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
    g_assert (change->status () == "Error");
    g_assert (change->error () == "cancelled");
    g_assert_cmpint (change->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert (task0->status () == "Error");
}

void
AbortChangeHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdChange> change (request->change ());
    g_assert_true (change->ready ());
    g_assert (change->status () == "Error");
    g_assert (change->error () == "cancelled");
    g_assert_cmpint (change->taskCount (), ==, 1);

    QScopedPointer<QSnapdTask> task0 (change->task (0));
    g_assert (task0->status () == "Error");

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
    g_assert (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (listRequest->snap (1));
    g_assert (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (listRequest->snap (2));
    g_assert (snap2->name () == "snap3");
}

void
ListHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (request->snap (2));
    g_assert (snap2->name () == "snap3");

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
    g_assert (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (getSnapsRequest->snap (1));
    g_assert (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (getSnapsRequest->snap (2));
    g_assert (snap2->name () == "snap3");
}

void
GetSnapsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 3);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert (snap0->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert (snap1->name () == "snap2");
    QScopedPointer<QSnapdSnap> snap2 (request->snap (2));
    g_assert (snap2->name () == "snap3");

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
    g_assert (snap0->name () == "snap1");
    g_assert (snap0->status () == QSnapdEnums::SnapStatusInstalled);
    QScopedPointer<QSnapdSnap> snap1 (getSnapsRequest->snap (1));
    g_assert (snap1->status () == QSnapdEnums::SnapStatusActive);
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
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap->publisherId () == "PUBLISHER-ID");
    g_assert (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_null (snap->license ());
    g_assert_null (snap->mountedFrom ());
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert (snap->version () == "VERSION");
}

void
ListOneHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (request->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_null (snap->base ());
    g_assert_null (snap->broken ());
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap->publisherId () == "PUBLISHER-ID");
    g_assert (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert (snap->version () == "VERSION");

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
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap->publisherId () == "PUBLISHER-ID");
    g_assert (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_null (snap->license ());
    g_assert_null (snap->mountedFrom ());
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert (snap->version () == "VERSION");
}

void
GetSnapHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (request->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_null (snap->base ());
    g_assert_null (snap->broken ());
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->commonIds ().count (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap->publisherId () == "PUBLISHER-ID");
    g_assert (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert_null (snap->summary ());
    g_assert_null (snap->trackingChannel ());
    g_assert_false (snap->trymode ());
    g_assert (snap->version () == "VERSION");

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
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    mock_snap_set_installed_size (s, 1024);
    mock_snap_set_jailmode (s, TRUE);
    mock_snap_set_trymode (s, TRUE);
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_license (s, "LICENSE");
    mock_snap_set_mounted_from (s, "MOUNTED-FROM");
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
    g_assert (app->name () == "app");
    g_assert (app->snap () == "snap");
    g_assert (app->commonId ().isNull ());
    g_assert_cmpint (app->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (app->enabled ());
    g_assert_false (app->active ());
    g_assert (app->desktopFile () == "/var/lib/snapd/desktop/applications/app.desktop");
    g_assert (snap->base () == "BASE");
    g_assert (snap->broken () == "BROKEN");
    g_assert (snap->channel () == "CHANNEL");
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
    g_assert (snap->contact () == "CONTACT");
    g_assert (snap->description () == "DESCRIPTION");
    g_assert (snap->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap->publisherId () == "PUBLISHER-ID");
    g_assert (snap->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_true (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    QDateTime date = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    g_assert (snap->installDate () == date);
    g_assert_cmpint (snap->installedSize (), ==, 1024);
    g_assert_true (snap->jailmode ());
    g_assert (snap->license () == "LICENSE");
    g_assert (snap->mountedFrom () == "MOUNTED-FROM");
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert_false (snap->isPrivate ());
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert (snap->summary () == "SUMMARY");
    g_assert (snap->trackingChannel () == "CHANNEL");
    g_assert_true (snap->trymode ());
    g_assert (snap->version () == "VERSION");
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
    g_assert (snap->developer () == "PUBLISHER-USERNAME");
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
    g_assert (snap->commonIds ()[0] == "ID1");
    g_assert (snap->commonIds ()[1] == "ID2");
    g_assert_cmpint (snap->appCount (), ==, 2);
    QScopedPointer<QSnapdApp> app1 (snap->app (0));
    g_assert (app1->name () == "app1");
    g_assert (app1->commonId () == "ID1");
    QScopedPointer<QSnapdApp> app2 (snap->app (1));
    g_assert (app2->name () == "app2");
    g_assert (app2->commonId () == "ID2");
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
    g_assert_cmpint (getSnapRequest->error (), ==, QSnapdRequest::Failed);
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
    g_assert_cmpint (snap->publisherValidation (), ==, QSnapdEnums::PublisherValidationVerified);
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
    g_assert (appsRequest->app (0)->name () == "app1");
    g_assert (appsRequest->app (0)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (0)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (appsRequest->app (0)->active ());
    g_assert_false (appsRequest->app (0)->enabled ());
    g_assert (appsRequest->app (1)->name () == "app2");
    g_assert (appsRequest->app (1)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (1)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (appsRequest->app (1)->active ());
    g_assert_false (appsRequest->app (1)->enabled ());
    g_assert (appsRequest->app (2)->name () == "app3");
    g_assert (appsRequest->app (2)->snap () == "snap");
    g_assert_cmpint (appsRequest->app (2)->daemonType (), ==, QSnapdEnums::DaemonTypeSimple);
    g_assert_true (appsRequest->app (2)->active ());
    g_assert_true (appsRequest->app (2)->enabled ());
}

void
GetAppsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->appCount (), ==, 3);
    g_assert (request->app (0)->name () == "app1");
    g_assert (request->app (0)->snap () == "snap");
    g_assert_cmpint (request->app (0)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (request->app (0)->active ());
    g_assert_false (request->app (0)->enabled ());
    g_assert (request->app (1)->name () == "app2");
    g_assert (request->app (1)->snap () == "snap");
    g_assert_cmpint (request->app (1)->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (request->app (1)->active ());
    g_assert_false (request->app (1)->enabled ());
    g_assert (request->app (2)->name () == "app3");
    g_assert (request->app (2)->snap () == "snap");
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
    g_assert (appsRequest->app (0)->name () == "app2");
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
    g_assert (icon->mimeType () == "image/png");
    QByteArray data = icon->data ();
    g_assert_cmpmem (data.data (), data.size (), "ICON-DATA", 9);
}

void
GetIconHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdIcon> icon (request->icon ());
    g_assert (icon->mimeType () == "image/png");
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
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::Failed);
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
    g_assert (icon->mimeType () == "image/png");
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
    g_assert (getAssertionsRequest->assertions ()[0] == "type: account\n"
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
    g_assert (getAssertionsRequest->assertions()[0] == "type: account\n"
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
    g_assert (getAssertionsRequest->assertions ()[0] == "type: account\n"
                                                        "\n"
                                                        "SIGNATURE1");
    g_assert (getAssertionsRequest->assertions ()[1] == "type: account\n"
                                                        "body-length: 4\n"
                                                        "\n"
                                                        "BODY\n"
                                                        "\n"
                                                        "SIGNATURE2");
    g_assert (getAssertionsRequest->assertions ()[2] == "type: account\n"
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
    g_assert (assertion.headers ()[0] == "type");
    g_assert (assertion.header ("type") == "account");
    g_assert (assertion.headers ()[1] == "authority-id");
    g_assert (assertion.header ("authority-id") == "canonical");
    g_assert_null (assertion.header ("invalid"));
    g_assert_null (assertion.body ());
    g_assert (assertion.signature () == "SIGNATURE");
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
    g_assert (assertion.headers ()[0] == "type");
    g_assert (assertion.header ("type") == "account");
    g_assert (assertion.headers ()[1] == "body-length");
    g_assert (assertion.header ("body-length") == "4");
    g_assert_null (assertion.header ("invalid"));
    g_assert (assertion.body () == "BODY");
    g_assert (assertion.signature () == "SIGNATURE");
}

static void
test_get_interfaces_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, "slot1");
    mock_snap_add_slot (s, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, "plug1");
    mock_plug_set_connection (p, sl);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (getInterfacesRequest->plugCount (), ==, 1);

    QScopedPointer<QSnapdPlug> plug (getInterfacesRequest->plug (0));
    g_assert (plug->name () == "plug1");
    g_assert (plug->snap () == "snap2");
    g_assert (plug->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (plug->label () == "LABEL");
    g_assert_cmpint (plug->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> plugConnection (plug->connection (0));
    g_assert (plugConnection->snap () == "snap1");
    g_assert (plugConnection->name () == "slot1");

    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 2);

    QScopedPointer<QSnapdSlot> slot0 (getInterfacesRequest->slot (0));
    g_assert (slot0->name () == "slot1");
    g_assert (slot0->snap () == "snap1");
    g_assert (slot0->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (slot0->label () == "LABEL");
    g_assert_cmpint (slot0->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> slotConnection (slot0->connection (0));
    g_assert (slotConnection->snap () == "snap2");
    g_assert (slotConnection->name () == "plug1");

    QScopedPointer<QSnapdSlot> slot1 (getInterfacesRequest->slot (1));
    g_assert (slot1->name () == "slot2");
    g_assert (slot1->snap () == "snap1");
    g_assert_cmpint (slot1->connectionCount (), ==, 0);
}

void
GetInterfacesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);

    g_assert_cmpint (request->plugCount (), ==, 1);

    QScopedPointer<QSnapdPlug> plug (request->plug (0));
    g_assert (plug->name () == "plug1");
    g_assert (plug->snap () == "snap2");
    g_assert (plug->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (plug->label () == "LABEL");
    g_assert_cmpint (plug->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> plugConnection (plug->connection (0));
    g_assert (plugConnection->snap () == "snap1");
    g_assert (plugConnection->name () == "slot1");

    g_assert_cmpint (request->slotCount (), ==, 2);

    QScopedPointer<QSnapdSlot> slot0 (request->slot (0));
    g_assert (slot0->name () == "slot1");
    g_assert (slot0->snap () == "snap1");
    g_assert (slot0->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (slot0->label () == "LABEL");
    g_assert_cmpint (slot0->connectionCount (), ==, 1);
    QScopedPointer<QSnapdConnection> slotConnection (slot0->connection (0));
    g_assert (slotConnection->snap () == "snap2");
    g_assert (slotConnection->name () == "plug1");

    QScopedPointer<QSnapdSlot> slot1 (request->slot (1));
    g_assert (slot1->name () == "slot2");
    g_assert (slot1->snap () == "snap1");
    g_assert_cmpint (slot1->connectionCount (), ==, 0);

    g_main_loop_quit (loop);
}

static void
test_get_interfaces_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, "slot1");
    mock_snap_add_slot (s, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, "plug1");
    mock_plug_set_connection (p, sl);
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
test_connect_interface_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_plug_get_connection (plug) == slot);
}

void
ConnectInterfaceHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSlot *slot = mock_snap_find_slot (mock_snapd_find_snap (snapd, "snap1"), "slot");
    MockPlug *plug = mock_snap_find_plug (mock_snapd_find_snap (snapd, "snap2"), "plug");
    g_assert (mock_plug_get_connection (plug) == slot);

    g_main_loop_quit (loop);
}

static void
test_connect_interface_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    mock_snap_add_plug (s, "plug");
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
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (connectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_plug_get_connection (plug) == slot);
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
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    mock_plug_set_connection (plug, slot);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_plug_get_connection (plug));
}

void
DisconnectInterfaceHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    MockSnap *s = mock_snapd_find_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_find_plug (s, "plug");
    g_assert_null (mock_plug_get_connection (plug));

    g_main_loop_quit (loop);
}

static void
test_disconnect_interface_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    mock_plug_set_connection (plug, slot);
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
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    mock_plug_set_connection (plug, slot);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (disconnectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_plug_get_connection (plug));
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
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_download_size (s, 1024);
    mock_snap_add_price (s, 1.20, "NZD");
    mock_snap_add_price (s, 0.87, "USD");
    mock_snap_add_screenshot (s, "screenshot0.png", 0, 0);
    mock_snap_add_screenshot (s, "screenshot1.png", 1024, 1024);
    mock_snap_set_trymode (s, TRUE);
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.find ("carrot"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    g_assert (findRequest->suggestedCurrency () == "NZD");
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert (snap0->name () == "carrot1");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert (snap1->channel () == "CHANNEL");
    g_assert_cmpint (snap1->tracks ().count (), ==, 1);
    g_assert (snap1->tracks ()[0] == "latest");
    g_assert (snap1->channelCount () == 1);
    QScopedPointer<QSnapdChannel> channel (snap1->channel (0));
    g_assert (channel->name () == "stable");
    g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert (channel->revision () == "REVISION");
    g_assert (channel->version () == "VERSION");
    g_assert (channel->epoch () == "0");
    g_assert_cmpint (channel->size (), ==, 65535);
    g_assert_cmpint (snap1->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert (snap1->contact () == "CONTACT");
    g_assert (snap1->description () == "DESCRIPTION");
    g_assert (snap1->publisherDisplayName () == "PUBLISHER-DISPLAY-NAME");
    g_assert (snap1->publisherId () == "PUBLISHER-ID");
    g_assert (snap1->publisherUsername () == "PUBLISHER-USERNAME");
    g_assert_cmpint (snap1->publisherValidation (), ==, QSnapdEnums::PublisherValidationUnknown);
    g_assert_cmpint (snap1->downloadSize (), ==, 1024);
    g_assert (snap1->icon () == "ICON");
    g_assert (snap1->id () == "ID");
    g_assert_false (snap1->installDate ().isValid ());
    g_assert_cmpint (snap1->installedSize (), ==, 0);
    g_assert (snap1->name () == "carrot2");
    g_assert_cmpint (snap1->priceCount (), ==, 2);
    QScopedPointer<QSnapdPrice> price0 (snap1->price (0));
    g_assert_cmpfloat (price0->amount (), ==, 1.20);
    g_assert (price0->currency () == "NZD");
    QScopedPointer<QSnapdPrice> price1 (snap1->price (1));
    g_assert_cmpfloat (price1->amount (), ==, 0.87);
    g_assert (price1->currency () == "USD");
    g_assert_false (snap1->isPrivate ());
    g_assert (snap1->revision () == "REVISION");
    g_assert_cmpint (snap1->screenshotCount (), ==, 2);
    QScopedPointer<QSnapdScreenshot> screenshot0 (snap1->screenshot (0));
    g_assert (screenshot0->url () == "screenshot0.png");
    QScopedPointer<QSnapdScreenshot> screenshot1 (snap1->screenshot (1));
    g_assert (screenshot1->url () == "screenshot1.png");
    g_assert_cmpint (screenshot1->width (), ==, 1024);
    g_assert_cmpint (screenshot1->height (), ==, 1024);
    g_assert_cmpint (snap1->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap1->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert (snap1->summary () == "SUMMARY");
    g_assert_true (snap1->trymode ());
    g_assert (snap1->version () == "VERSION");
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
    g_assert (snap->name () == "snap2");
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
    g_assert (snap->name () == "snap");
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
    g_assert (snap->name () == "snap");
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
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->tracks ().count (), ==, 2);
    g_assert (snap->tracks ()[0] == "latest");
    g_assert (snap->tracks ()[1] == "insider");
    g_assert_cmpint (snap->channelCount (), ==, 4);

    gboolean matched_stable = FALSE, matched_beta = FALSE, matched_branch = FALSE, matched_track = FALSE;
    for (int i = 0; i < snap->channelCount (); i++) {
        QSnapdChannel *channel = snap->channel (i);

        if (channel->name () == "stable") {
            g_assert (channel->track () == "latest");
            g_assert (channel->risk () == "stable");
            g_assert (channel->branch () == NULL);
            g_assert (channel->revision () == "REVISION");
            g_assert (channel->version () == "VERSION");
            g_assert (channel->epoch () == "0");
            g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
            g_assert_cmpint (channel->size (), ==, 65535);
            matched_stable = TRUE;
        }
        if (channel->name () == "beta") {
            g_assert (channel->name () == "beta");
            g_assert (channel->track () == "latest");
            g_assert (channel->risk () == "beta");
            g_assert (channel->branch () == NULL);
            g_assert (channel->revision () == "BETA-REVISION");
            g_assert (channel->version () == "BETA-VERSION");
            g_assert (channel->epoch () == "1");
            g_assert_cmpint (channel->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
            g_assert_cmpint (channel->size (), ==, 10000);
            matched_beta = TRUE;
        }
        if (channel->name () == "stable/branch") {
            g_assert (channel->track () == "latest");
            g_assert (channel->risk () == "stable");
            g_assert (channel->branch () == "branch");
            matched_branch = TRUE;
        }
        if (channel->name () == "insider/stable") {
            g_assert (channel->track () == "insider");
            g_assert (channel->risk () == "stable");
            g_assert (channel->branch () == NULL);
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
    g_assert (snap1->name () == "stable-snap");
    QScopedPointer<QSnapdChannel> channel1a (snap1->matchChannel ("stable"));
    g_assert_nonnull (channel1a);
    g_assert (channel1a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1b (snap1->matchChannel ("candidate"));
    g_assert_nonnull (channel1b);
    g_assert (channel1b->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1c (snap1->matchChannel ("beta"));
    g_assert_nonnull (channel1c);
    g_assert (channel1c->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1d (snap1->matchChannel ("edge"));
    g_assert_nonnull (channel1d);
    g_assert (channel1d->name () == "stable");
    QScopedPointer<QSnapdChannel> channel1e (snap1->matchChannel ("UNDEFINED"));
    g_assert_null (channel1e);

    /* All channels match if all defined */
    QScopedPointer<QSnapdFindRequest> findRequest2 (client.find (QSnapdClient::MatchName, "full-snap"));
    findRequest2->runSync ();
    g_assert_cmpint (findRequest2->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest2->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap2 (findRequest2->snap (0));
    g_assert (snap2->name () == "full-snap");
    QScopedPointer<QSnapdChannel> channel2a (snap2->matchChannel ("stable"));
    g_assert_nonnull (channel2a);
    g_assert (channel2a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel2b (snap2->matchChannel ("candidate"));
    g_assert_nonnull (channel2b);
    g_assert (channel2b->name () == "candidate");
    QScopedPointer<QSnapdChannel> channel2c (snap2->matchChannel ("beta"));
    g_assert_nonnull (channel2c);
    g_assert (channel2c->name () == "beta");
    QScopedPointer<QSnapdChannel> channel2d (snap2->matchChannel ("edge"));
    g_assert_nonnull (channel2d);
    g_assert (channel2d->name () == "edge");
    QScopedPointer<QSnapdChannel> channel2e (snap2->matchChannel ("UNDEFINED"));
    g_assert_null (channel2e);

    /* Only match with more stable channels */
    QScopedPointer<QSnapdFindRequest> findRequest3 (client.find (QSnapdClient::MatchName, "beta-snap"));
    findRequest3->runSync ();
    g_assert_cmpint (findRequest3->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest3->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap3 (findRequest3->snap (0));
    g_assert (snap3->name () == "beta-snap");
    QScopedPointer<QSnapdChannel> channel3a (snap3->matchChannel ("stable"));
    g_assert_null (channel3a);
    QScopedPointer<QSnapdChannel> channel3b (snap3->matchChannel ("candidate"));
    g_assert_null (channel3b);
    QScopedPointer<QSnapdChannel> channel3c (snap3->matchChannel ("beta"));
    g_assert_nonnull (channel3c);
    g_assert (channel3c->name () == "beta");
    QScopedPointer<QSnapdChannel> channel3d (snap3->matchChannel ("edge"));
    g_assert_nonnull (channel3d);
    g_assert (channel3d->name () == "beta");
    QScopedPointer<QSnapdChannel> channel3e (snap3->matchChannel ("UNDEFINED"));
    g_assert_null (channel3e);

    /* Match branches */
    QScopedPointer<QSnapdFindRequest> findRequest4 (client.find (QSnapdClient::MatchName, "branch-snap"));
    findRequest4->runSync ();
    g_assert_cmpint (findRequest4->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest4->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap4 (findRequest4->snap (0));
    g_assert (snap4->name () == "branch-snap");
    QScopedPointer<QSnapdChannel> channel4a (snap4->matchChannel ("stable"));
    g_assert_nonnull (channel4a);
    g_assert (channel4a->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4b (snap4->matchChannel ("stable/branch"));
    g_assert_nonnull (channel4b);
    g_assert (channel4b->name () == "stable/branch");
    QScopedPointer<QSnapdChannel> channel4c (snap4->matchChannel ("candidate"));
    g_assert_nonnull (channel4c);
    g_assert (channel4c->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4d (snap4->matchChannel ("beta"));
    g_assert_nonnull (channel4d);
    g_assert (channel4d->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4e (snap4->matchChannel ("edge"));
    g_assert_nonnull (channel4e);
    g_assert (channel4e->name () == "stable");
    QScopedPointer<QSnapdChannel> channel4f (snap4->matchChannel ("UNDEFINED"));
    g_assert_null (channel4f);

    /* Match correct tracks */
    QScopedPointer<QSnapdFindRequest> findRequest5 (client.find (QSnapdClient::MatchName, "track-snap"));
    findRequest5->runSync ();
    g_assert_cmpint (findRequest5->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest5->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap5 (findRequest5->snap (0));
    g_assert (snap5->name () == "track-snap");
    QScopedPointer<QSnapdChannel> channel5a (snap5->matchChannel ("stable"));
    g_assert_nonnull (channel5a);
    g_assert (channel5a->name () == "stable");
    g_assert (channel5a->track () == "latest");
    g_assert (channel5a->risk () == "stable");
    QScopedPointer<QSnapdChannel> channel5b (snap5->matchChannel ("latest/stable"));
    g_assert_nonnull (channel5b);
    g_assert (channel5b->name () == "stable");
    g_assert (channel5b->track () == "latest");
    g_assert (channel5b->risk () == "stable");
    QScopedPointer<QSnapdChannel> channel5c (snap5->matchChannel ("insider/stable"));
    g_assert_nonnull (channel5c);
    g_assert (channel5c->name () == "insider/stable");
    g_assert (channel5c->track () == "insider");
    g_assert (channel5c->risk () == "stable");
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
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection ("section", NULL));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert (snap0->name () == "apple");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert (snap1->name () == "carrot1");
}

static void
test_find_section_query ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection ("section", "carrot"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert (snap->name () == "carrot1");
}

static void
test_find_section_name ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    s = mock_snapd_add_store_snap (snapd, "carrot2");
    mock_snap_add_store_section (s, "section");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection (QSnapdClient::MatchName, "section", "carrot1"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert (snap->name () == "carrot1");
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
    g_assert (snap->name () == "snap1");
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
    g_assert (snap1->name () == "snap1");
    QScopedPointer<QSnapdSnap> snap2 (findRequest->snap (1));
    g_assert (snap2->name () == "snap2");
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
    g_assert (snap0->name () == "snap1");
    g_assert (snap0->revision () == "1");
    QScopedPointer<QSnapdSnap> snap1 (findRefreshableRequest->snap (1));
    g_assert (snap1->name () == "snap3");
    g_assert (snap1->revision () == "1");
}

void
FindRefreshableHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapCount (), ==, 2);
    QScopedPointer<QSnapdSnap> snap0 (request->snap (0));
    g_assert (snap0->name () == "snap1");
    g_assert (snap0->revision () == "1");
    QScopedPointer<QSnapdSnap> snap1 (request->snap (1));
    g_assert (snap1->name () == "snap3");
    g_assert (snap1->revision () == "1");

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
    g_assert (request->errorString () == "ERROR");
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

    g_assert (change->kind () == "KIND");
    g_assert (change->summary () == "SUMMARY");
    if (progressDone == total) {
        g_assert (change->status () == "Done");
        g_assert_true (change->ready ());
    }
    else {
        g_assert (change->status () == "Do");
        g_assert_false (change->ready ());
    }
    g_assert (change->spawnTime () == spawnTime);
    if (change->ready ())
        g_assert (readyTime == readyTime);
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
    g_assert (mock_snap_get_devmode (mock_snapd_find_snap (snapd, "snap")));
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
    g_assert (mock_snap_get_dangerous (mock_snapd_find_snap (snapd, "snap")));
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
    g_assert (mock_snap_get_jailmode (mock_snapd_find_snap (snapd, "snap")));
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
    g_assert (refreshAllRequest->snapNames ()[0] == "snap1");
    g_assert (refreshAllRequest->snapNames ()[1] == "snap3");
}

void
RefreshAllHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->snapNames ().count (), ==, 2);
    g_assert (request->snapNames ()[0] == "snap1");
    g_assert (request->snapNames ()[1] == "snap3");

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
    g_assert (refreshAllRequest->snapNames ()[0] == "snap1");
    g_assert (refreshAllRequest->snapNames ()[1] == "snap3");
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
}

void
RemoveHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_null (mock_snapd_find_snap (snapd, "snap"));

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
    g_assert (request->errorString () == "ERROR");
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
    g_assert (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    BuyHandler buyHandler (loop, snapd, client.buy ("ABCDEF", 1.20, "NZD"));
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
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

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::Failed);
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
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
    mock_snap_add_price (s, 1.20, "NZD");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    client.setAuthData (authData.data ());

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 0.6, "NZD"));
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
    g_assert (userInformation->username () == "user");
    g_assert_cmpint (userInformation->sshKeys ().count (), ==, 2);
    g_assert (userInformation->sshKeys ()[0] == "KEY1");
    g_assert (userInformation->sshKeys ()[1] == "KEY2");
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
    g_assert (userInformation0->username () == "admin");
    g_assert_cmpint (userInformation0->sshKeys ().count (), ==, 2);
    g_assert (userInformation0->sshKeys ()[0] == "KEY1");
    g_assert (userInformation0->sshKeys ()[1] == "KEY2");
    QScopedPointer<QSnapdUserInformation> userInformation1 (createRequest->userInformation (1));
    g_assert (userInformation1->username () == "alice");
    QScopedPointer<QSnapdUserInformation> userInformation2 (createRequest->userInformation (2));
    g_assert (userInformation2->username () == "bob");
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
    g_assert (getUsersRequest->userInformation (0)->username () == "alice");
    g_assert (getUsersRequest->userInformation (0)->email () == "alice@example.com");
    g_assert_cmpint (getUsersRequest->userInformation (1)->id (), ==, 2);
    g_assert (getUsersRequest->userInformation (1)->username () == "bob");
    g_assert (getUsersRequest->userInformation (1)->email () == "bob@example.com");
}

void
GetUsersHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->userInformationCount (), ==, 2);
    g_assert_cmpint (request->userInformation (0)->id (), ==, 1);
    g_assert (request->userInformation (0)->username () == "alice");
    g_assert (request->userInformation (0)->email () == "alice@example.com");
    g_assert_cmpint (request->userInformation (1)->id (), ==, 2);
    g_assert (request->userInformation (1)->username () == "bob");
    g_assert (request->userInformation (1)->email () == "bob@example.com");

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
    mock_snapd_add_store_section (snapd, "SECTION1");
    mock_snapd_add_store_section (snapd, "SECTION2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSectionsRequest> getSectionsRequest (client.getSections ());
    getSectionsRequest->runSync ();
    g_assert_cmpint (getSectionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSectionsRequest->sections ().count (), ==, 2);
    g_assert (getSectionsRequest->sections ()[0] == "SECTION1");
    g_assert (getSectionsRequest->sections ()[1] == "SECTION2");
}

void
GetSectionsHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->sections ().count (), ==, 2);
    g_assert (request->sections ()[0] == "SECTION1");
    g_assert (request->sections ()[1] == "SECTION2");

    g_main_loop_quit (loop);
}

static void
test_get_sections_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_section (snapd, "SECTION1");
    mock_snapd_add_store_section (snapd, "SECTION2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    GetSectionsHandler getSectionsHandler (loop, client.getSections ());
    QObject::connect (getSectionsHandler.request, &QSnapdGetSectionsRequest::complete, &getSectionsHandler, &GetSectionsHandler::onComplete);
    getSectionsHandler.request->runAsync ();
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
    QScopedPointer<QSnapdAlias> alias1 (getAliasesRequest->alias (0));
    g_assert (alias1->name () == "alias1");
    g_assert (alias1->snap () == "snap");
    g_assert_cmpint (alias1->status (), ==, QSnapdEnums::AliasStatusAuto);
    g_assert (alias1->appAuto () == "app");
    g_assert_null (alias1->appManual ());
    QScopedPointer<QSnapdAlias> alias2 (getAliasesRequest->alias (1));
    g_assert (alias2->name () == "alias2");
    g_assert (alias2->snap () == "snap");
    g_assert_cmpint (alias2->status (), ==, QSnapdEnums::AliasStatusManual);
    g_assert_null (alias2->appAuto ());
    g_assert (alias2->appManual () == "app");
    QScopedPointer<QSnapdAlias> alias3 (getAliasesRequest->alias (2));
    g_assert (alias3->name () == "alias3");
    g_assert (alias3->snap () == "snap");
    g_assert_cmpint (alias3->status (), ==, QSnapdEnums::AliasStatusDisabled);
    g_assert (alias3->appAuto () == "app");
    g_assert_null (alias3->appManual ());
}

void
GetAliasesHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (request->aliasCount (), ==, 3);
    QScopedPointer<QSnapdAlias> alias1 (request->alias (0));
    g_assert (alias1->name () == "alias1");
    g_assert (alias1->snap () == "snap");
    g_assert_cmpint (alias1->status (), ==, QSnapdEnums::AliasStatusAuto);
    g_assert (alias1->appAuto () == "app");
    g_assert_null (alias1->appManual ());
    QScopedPointer<QSnapdAlias> alias2 (request->alias (1));
    g_assert (alias2->name () == "alias2");
    g_assert (alias2->snap () == "snap");
    g_assert_cmpint (alias2->status (), ==, QSnapdEnums::AliasStatusManual);
    g_assert_null (alias2->appAuto ());
    g_assert (alias2->appManual () == "app");
    QScopedPointer<QSnapdAlias> alias3 (request->alias (2));
    g_assert (alias3->name () == "alias3");
    g_assert (alias3->snap () == "snap");
    g_assert_cmpint (alias3->status (), ==, QSnapdEnums::AliasStatusDisabled);
    g_assert (alias3->appAuto () == "app");
    g_assert_null (alias3->appManual ());

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
    g_assert (runSnapCtlRequest->stdout () == "STDOUT:ABC:arg1:arg2");
    g_assert (runSnapCtlRequest->stderr () == "STDERR");
}

void
RunSnapCtlHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    g_assert (request->stdout () == "STDOUT:ABC:arg1:arg2");
    g_assert (request->stderr () == "STDERR");

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
        g_assert (systemInformation->version () == "VERSION");
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
    g_test_add_func ("/get-changes/sync", test_get_changes_sync);
    g_test_add_func ("/get-changes/async", test_get_changes_async);
    g_test_add_func ("/get-changes/filter-in-progress", test_get_changes_filter_in_progress);
    g_test_add_func ("/get-changes/filter-ready", test_get_changes_filter_ready);
    g_test_add_func ("/get-changes/filter-snap", test_get_changes_filter_snap);
    g_test_add_func ("/get-changes/filter-ready-snap", test_get_changes_filter_ready_snap);
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
    g_test_add_func ("/get-snap/optional-fields", test_get_snap_optional_fields);
    g_test_add_func ("/get-snap/deprecated-fields", test_get_snap_deprecated_fields);
    g_test_add_func ("/get-snap/common-ids", test_get_snap_common_ids);
    g_test_add_func ("/get-snap/not-installed", test_get_snap_not_installed);
    g_test_add_func ("/get-snap/classic-confinement", test_get_snap_classic_confinement);
    g_test_add_func ("/get-snap/devmode-confinement", test_get_snap_devmode_confinement);
    g_test_add_func ("/get-snap/daemons", test_get_snap_daemons);
    g_test_add_func ("/get-snap/publisher-verified", test_get_snap_publisher_verified);
    g_test_add_func ("/get-snap/publisher-unproven", test_get_snap_publisher_unproven);
    g_test_add_func ("/get-snap/publisher-unknown-validation", test_get_snap_publisher_unknown_validation);
    g_test_add_func ("/get-apps/sync", test_get_apps_sync);
    g_test_add_func ("/get-apps/async", test_get_apps_async);
    g_test_add_func ("/get-apps/services", test_get_apps_services);
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
    g_test_add_func ("/get-interfaces/sync", test_get_interfaces_sync);
    g_test_add_func ("/get-interfaces/async", test_get_interfaces_async);
    g_test_add_func ("/get-interfaces/no-snaps", test_get_interfaces_no_snaps);
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
    g_test_add_func ("/find/name", test_find_name);
    g_test_add_func ("/find/name-private", test_find_name_private);
    g_test_add_func ("/find/name-private/not-logged-in", test_find_name_private_not_logged_in);
    g_test_add_func ("/find/channels", test_find_channels);
    g_test_add_func ("/find/channels-match", test_find_channels_match);
    g_test_add_func ("/find/cancel", test_find_cancel);
    g_test_add_func ("/find/section", test_find_section);
    g_test_add_func ("/find/section-query", test_find_section_query);
    g_test_add_func ("/find/section-name", test_find_section_name);
    g_test_add_func ("/find/scope-narrow", test_find_scope_narrow);
    g_test_add_func ("/find/scope-wide", test_find_scope_wide);
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
    g_test_add_func ("/stress/basic", test_stress);

    return g_test_run ();
}
