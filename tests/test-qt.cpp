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
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
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
    g_autoptr(GPtrArray) changes = NULL;
    g_autoptr(GPtrArray) tasks = NULL;

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_spawn_time (c, "2017-01-02T11:00:00Z");
    MockTask *t = mock_change_add_task (c, "download");
    mock_task_set_progress (t, 65535, 65535);
    mock_task_set_spawn_time (t, "2017-01-02T11:00:00Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:10Z");
    t = mock_change_add_task (c, "install");
    mock_task_set_progress (t, 1, 1);
    mock_task_set_spawn_time (t, "2017-01-02T11:00:10Z");
    mock_task_set_ready_time (t, "2017-01-02T11:00:30Z");
    mock_change_set_ready_time (c, "2017-01-02T11:00:30Z");
    mock_change_set_ready (c, TRUE);
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

    QSnapdChange *change = changesRequest->change (0);
    g_assert (change->id () == "1");
    g_assert (change->kind () == "KIND");
    g_assert (change->summary () == "SUMMARY");
    g_assert (change->status () == "STATUS");
    g_assert_true (change->ready ());
    g_assert (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (change->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));
    g_assert_cmpint (change->taskCount (), ==, 2);

    QSnapdTask *task = change->task (0);
    g_assert (task->id () == "100");
    g_assert (task->kind () == "download");
    g_assert (task->summary () == "SUMMARY");
    g_assert (task->status () == "STATUS");
    g_assert (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 65535);
    g_assert_cmpint (task->progressTotal (), ==, 65535);
    g_assert (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 0), Qt::UTC));
    g_assert (task->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));

    task = change->task (1);
    g_assert (task->id () == "101");
    g_assert (task->kind () == "install");
    g_assert (task->summary () == "SUMMARY");
    g_assert (task->status () == "STATUS");
    g_assert (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 1);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 10), Qt::UTC));
    g_assert (task->readyTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 0, 30), Qt::UTC));

    change = changesRequest->change (1);
    g_assert (change->id () == "2");
    g_assert (change->kind () == "KIND");
    g_assert (change->summary () == "SUMMARY");
    g_assert (change->status () == "STATUS");
    g_assert_false (change->ready ());
    g_assert (change->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (change->readyTime ().isValid ());
    g_assert_cmpint (change->taskCount (), ==, 1);

    task = change->task (0);
    g_assert (task->id () == "200");
    g_assert (task->kind () == "remove");
    g_assert (task->summary () == "SUMMARY");
    g_assert (task->status () == "STATUS");
    g_assert (task->progressLabel () == "LABEL");
    g_assert_cmpint (task->progressDone (), ==, 0);
    g_assert_cmpint (task->progressTotal (), ==, 1);
    g_assert (task->spawnTime () == QDateTime (QDate (2017, 1, 2), QTime (11, 15, 0), Qt::UTC));
    g_assert_false (task->readyTime ().isValid ());
}

static void
test_get_changes_filter_in_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_ready (c, TRUE);
    mock_snapd_add_change (snapd);
    c = mock_snapd_add_change (snapd);
    mock_change_set_ready (c, TRUE);
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
    mock_snapd_add_change (snapd);
    MockChange *c = mock_snapd_add_change (snapd);
    mock_change_set_ready (c, TRUE);
    mock_snapd_add_change (snapd);
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
    mock_change_set_ready (c, TRUE);
    t = mock_change_add_task (c, "install");
    mock_task_set_snap_name (t, "snap2");
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
test_list_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListRequest> listRequest (client.list ());
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
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    ListHandler listHandler (loop, client.list ());
    QObject::connect (listHandler.request, &QSnapdListRequest::complete, &listHandler, &ListHandler::onComplete);
    listHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_list_one_sync ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->tracks ().count (), ==, 0);
    g_assert_cmpint (snap->channelCount (), ==, 0);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->developer () == "DEVELOPER");
    g_assert_false (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert_true (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert_false (snap->jailmode ());
    g_assert_null (snap->license ());
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
    g_assert_null (snap->broken ());
    g_assert_null (snap->channel ());
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_null (snap->contact ());
    g_assert_null (snap->description ());
    g_assert (snap->developer () == "DEVELOPER");
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

    ListOneHandler listOneHandler (loop, client.listOne ("snap"));
    QObject::connect (listOneHandler.request, &QSnapdListOneRequest::complete, &listOneHandler, &ListOneHandler::onComplete);
    listOneHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_list_one_optional_fields ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
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
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 1);
    QScopedPointer<QSnapdApp> app (snap->app (0));
    g_assert (app->name () == "app");
    g_assert (app->snap () == "snap");
    g_assert_cmpint (app->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_false (app->enabled ());
    g_assert_false (app->active ());
    g_assert (app->desktopFile () == "/var/lib/snapd/desktop/applications/app.desktop");
    g_assert (snap->broken () == "BROKEN");
    g_assert (snap->channel () == "CHANNEL");
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
    g_assert (snap->contact () == "CONTACT");
    g_assert (snap->description () == "DESCRIPTION");
    g_assert (snap->developer () == "DEVELOPER");
    g_assert_true (snap->devmode ());
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    QDateTime date = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    g_assert (snap->installDate () == date);
    g_assert_cmpint (snap->installedSize (), ==, 1024);
    g_assert_true (snap->jailmode ());
    g_assert (snap->license () == "LICENSE");
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
test_list_one_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::Failed);
}

static void
test_list_one_classic_confinement ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
}

static void
test_list_one_devmode_confinement ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_confinement (s, "devmode");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementDevmode);
}

static void
test_list_one_daemons ()
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

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
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
    mock_snapd_add_store_snap (snapd, "carrot1");
    MockSnap *s = mock_snapd_add_store_snap (snapd, "carrot2");
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

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::None, "carrot"));
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
    g_assert (snap1->developer () == "DEVELOPER");
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
    MockChannel *c = mock_track_add_channel (t, "beta", NULL);
    mock_channel_set_revision (c, "BETA-REVISION");
    mock_channel_set_version (c, "BETA-VERSION");
    mock_channel_set_epoch (c, "1");
    mock_channel_set_confinement (c, "classic");
    mock_channel_set_size (c, 10000);
    mock_snap_add_track (s, "TRACK");
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
    g_assert (snap->tracks ()[1] == "TRACK");
    g_assert_cmpint (snap->channelCount (), ==, 2);
    QScopedPointer<QSnapdChannel> channel1 (snap->matchChannel ("stable"));
    g_assert_nonnull (channel1);
    g_assert (channel1->name () == "stable");
    g_assert (channel1->revision () == "REVISION");
    g_assert (channel1->version () == "VERSION");
    g_assert (channel1->epoch () == "0");
    g_assert_cmpint (channel1->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert_cmpint (channel1->size (), ==, 65535);
    QScopedPointer<QSnapdChannel> channel2 (snap->matchChannel ("beta"));
    g_assert_nonnull (channel2);
    g_assert (channel2->name () == "beta");
    g_assert (channel2->revision () == "BETA-REVISION");
    g_assert (channel2->version () == "BETA-VERSION");
    g_assert (channel2->epoch () == "1");
    g_assert_cmpint (channel2->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
    g_assert_cmpint (channel2->size (), ==, 10000);
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
    FindHandler findHandler (loop, client.find (QSnapdClient::None, "do-not-respond"));
    QObject::connect (findHandler.request, &QSnapdFindRequest::complete, &findHandler, &FindHandler::onComplete);
    findHandler.request->runAsync ();
    g_idle_add ((GSourceFunc) find_cancel_cb, findHandler.request);

    g_main_loop_run (loop);
}

static void
test_find_section ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection (QSnapdClient::None, "section", NULL));
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
    mock_snapd_set_suggested_currency (snapd, "NZD");
    MockSnap *s = mock_snapd_add_store_snap (snapd, "apple");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "banana");
    s = mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snap_add_store_section (s, "section");
    mock_snapd_add_store_snap (snapd, "carrot2");
    g_assert_true (mock_snapd_start (snapd, NULL));

    QSnapdClient client;
    client.setSocketPath (mock_snapd_get_socket_path (snapd));

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection (QSnapdClient::None, "section", "carrot"));
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
    mock_snapd_set_suggested_currency (snapd, "NZD");
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
    g_assert (change->status () == "STATUS");
    if (progressDone == total)
        g_assert_true (change->ready ());
    else
        g_assert_false (change->ready ());
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
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::BadRequest);
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
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::BadRequest);
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
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::BadRequest);
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
    g_assert (mock_snap_get_disabled (mock_snapd_find_snap (snapd, "snap")));
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
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::BadRequest);
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
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_error (error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED);
    g_assert_cmpint (switchRequest->error (), ==, QSnapdRequest::BadRequest);
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
    g_test_add_func ("/get-system-information/confinement_strict", test_get_system_information_confinement_strict);
    g_test_add_func ("/get-system-information/confinement_none", test_get_system_information_confinement_none);
    g_test_add_func ("/get-system-information/confinement_unknown", test_get_system_information_confinement_unknown);
    g_test_add_func ("/login/sync", test_login_sync);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/get-changes/sync", test_get_changes_sync);
    g_test_add_func ("/get-changes/filter-in-progress", test_get_changes_filter_in_progress);
    g_test_add_func ("/get-changes/filter-ready", test_get_changes_filter_ready);
    g_test_add_func ("/get-changes/filter-snap", test_get_changes_filter_snap);
    g_test_add_func ("/get-changes/filter-ready-snap", test_get_changes_filter_ready_snap);
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
