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
    mock_snapd_stop (snapd);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::WriteFailed);
}

static void
test_socket_closed_after_request (void)
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_close_on_request (snapd, TRUE);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::ReadFailed);
}

static void
test_user_agent_default ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    client.setUserAgent (NULL);
    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_user_agent (snapd), ==, NULL);
}

static void
test_accept_language (void)
{
    g_setenv ("LANG", "en_US.UTF-8", TRUE);
    g_setenv ("LANGUAGE", "en_US:fr", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en-us, en;q=0.9, fr;q=0.8");
}

static void
test_accept_language_empty (void)
{
    g_setenv ("LANG", "", TRUE);
    g_setenv ("LANGUAGE", "", TRUE);
    g_setenv ("LC_ALL", "", TRUE);
    g_setenv ("LC_MESSAGES", "", TRUE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_accept_language (snapd), ==, "en");
}

static void
test_allow_interaction ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    /* By default, interaction is allowed */
    g_assert (client.allowInteraction());

    /* ... which sends the X-Allow-Interaction header with requests */
    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, "true");

    /* If interaction is not allowed, the header is not sent */
    client.setAllowInteraction (false);
    g_assert (!client.allowInteraction ());
    infoRequest.reset (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (infoRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_get_last_allow_interaction (snapd), ==, NULL);
}

static void
test_get_system_information ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    g_assert (systemInformation->managed ());
    g_assert (systemInformation->onClassic ());
    g_assert (systemInformation->mountDirectory () == "/snap");
    g_assert (systemInformation->binariesDirectory () == "/snap/bin");
    g_assert (systemInformation->store () == NULL);
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
    g_assert (systemInformation->managed ());
    g_assert (systemInformation->onClassic ());
    g_assert (systemInformation->mountDirectory () == "/snap");
    g_assert (systemInformation->binariesDirectory () == "/snap/bin");
    g_assert (systemInformation->store () == NULL);

    g_main_loop_quit (loop);
}

static void
test_get_system_information_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_managed (snapd, TRUE);
    mock_snapd_set_on_classic (snapd, TRUE);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementStrict);
}

static void
test_get_system_information_confinement_none ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "partial");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementPartial);
}

static void
test_get_system_information_confinement_unknown ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_confinement (snapd, "NOT_DEFINED");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSystemInformationRequest> infoRequest (client.getSystemInformation ());
    infoRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSystemInformation> systemInformation (infoRequest->systemInformation ());
    g_assert_cmpint (systemInformation->confinement (), ==, QSnapdEnums::SystemConfinementUnknown);
}

static void
test_login ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdAuthData> authData (loginRequest->authData ());
    g_assert (authData->macaroon () == a->macaroon);
    g_assert_cmpint (authData->discharges ().count (), ==, g_strv_length (a->discharges));
    for (int i = 0; a->discharges[i]; i++)
        g_assert (authData->discharges ()[i] == a->discharges[i]);
}

static void
test_login_invalid_email ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("not-an-email", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::AuthDataInvalid);
}

static void
test_login_invalid_password ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "invalid"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_login_otp_missing ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", "1234");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::TwoFactorRequired);
}

static void
test_login_otp_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_account (snapd, "test@example.com", "secret", "1234");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdLoginRequest> loginRequest (client.login ("test@example.com", "secret", "0000"));
    loginRequest->runSync ();
    g_assert_cmpint (loginRequest->error (), ==, QSnapdRequest::TwoFactorInvalid);
}

static void
test_list ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap1");
    mock_snapd_add_snap (snapd, "snap2");
    mock_snapd_add_snap (snapd, "snap3");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    ListHandler listHandler (loop, client.list ());
    QObject::connect (listHandler.request, &QSnapdListRequest::complete, &listHandler, &ListHandler::onComplete);
    listHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_list_one ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert (snap->channel () == NULL);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert (snap->contact () == NULL);
    g_assert (snap->description () == NULL);
    g_assert (snap->developer () == "DEVELOPER");
    g_assert (snap->devmode () == FALSE);
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert (snap->jailmode () == FALSE);
    g_assert (snap->license () == NULL);
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert (snap->isPrivate () == FALSE);
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert (snap->summary () == NULL);
    g_assert (snap->trackingChannel () == NULL);
    g_assert (snap->trymode () == FALSE);
    g_assert (snap->version () == "VERSION");
}

void
ListOneHandler::onComplete ()
{
    g_assert_cmpint (request->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (request->snap ());
    g_assert_cmpint (snap->appCount (), ==, 0);
    g_assert (snap->channel () == NULL);
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert (snap->contact () == NULL);
    g_assert (snap->description () == NULL);
    g_assert (snap->developer () == "DEVELOPER");
    g_assert (snap->devmode () == FALSE);
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert (snap->installDate ().isNull ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert (snap->jailmode () == FALSE);
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert (snap->isPrivate () == FALSE);
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert (snap->summary () == NULL);
    g_assert (snap->trackingChannel () == NULL);
    g_assert (snap->trymode () == FALSE);
    g_assert (snap->version () == "VERSION");

    g_main_loop_quit (loop);
}

static void
test_list_one_async ()
{
    g_autoptr(GMainLoop) loop = g_main_loop_new (NULL, FALSE);

    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    mock_app_add_alias (a, "app2");
    mock_app_add_alias (a, "app3");
    mock_app_set_desktop_file (a, "/var/lib/snapd/desktop/applications/app.desktop");
    mock_snap_set_confinement (s, "classic");
    s->devmode = TRUE;
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    s->installed_size = 1024;
    s->jailmode = TRUE;
    s->trymode = TRUE;
    mock_snap_set_contact (s, "CONTACT");
    mock_snap_set_channel (s, "CHANNEL");
    mock_snap_set_description (s, "DESCRIPTION");
    mock_snap_set_license (s, "LICENSE");
    mock_snap_set_summary (s, "SUMMARY");
    mock_snap_set_tracking_channel (s, "CHANNEL");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdSnap> snap (listOneRequest->snap ());
    g_assert_cmpint (snap->appCount (), ==, 1);
    QScopedPointer<QSnapdApp> app (snap->app (0));
    g_assert (app->name () == "app");
    g_assert_cmpint (app->daemonType (), ==, QSnapdEnums::DaemonTypeNone);
    g_assert_cmpint (app->aliases ().count (), ==, 2);
    g_assert (app->aliases ()[0] == "app2");
    g_assert (app->aliases ()[1] == "app3");
    g_assert (app->desktopFile () == "/var/lib/snapd/desktop/applications/app.desktop");
    g_assert (snap->channel () == "CHANNEL");
    g_assert_cmpint (snap->confinement (), ==, QSnapdEnums::SnapConfinementClassic);
    g_assert (snap->contact () == "CONTACT");
    g_assert (snap->description () == "DESCRIPTION");
    g_assert (snap->developer () == "DEVELOPER");
    g_assert (snap->devmode () == TRUE);
    g_assert_cmpint (snap->downloadSize (), ==, 0);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    QDateTime date = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    g_assert (snap->installDate () == date);
    g_assert_cmpint (snap->installedSize (), ==, 1024);
    g_assert (snap->jailmode () == TRUE);
    g_assert (snap->license () == "LICENSE");
    g_assert (snap->name () == "snap");
    g_assert_cmpint (snap->priceCount (), ==, 0);
    g_assert (snap->isPrivate () == FALSE);
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 0);
    g_assert_cmpint (snap->snapType (), ==, QSnapdEnums::SnapTypeApp);
    g_assert_cmpint (snap->status (), ==, QSnapdEnums::SnapStatusActive);
    g_assert (snap->summary () == "SUMMARY");
    g_assert (snap->trackingChannel () == "CHANNEL");
    g_assert (snap->trymode () == TRUE);
    g_assert (snap->version () == "VERSION");
}

static void
test_list_one_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
test_icon ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    g_autoptr(GBytes) icon_data = g_bytes_new ("ICON-DATA", 9);
    mock_snap_set_icon_data (s, "image/png", icon_data);

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    GetIconHandler getIconHandler (loop, client.getIcon ("snap"));
    QObject::connect (getIconHandler.request, &QSnapdGetIconRequest::complete, &getIconHandler, &GetIconHandler::onComplete);
    getIconHandler.request->runAsync ();

    g_main_loop_run (loop);
}

static void
test_icon_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetIconRequest> getIconRequest (client.getIcon ("snap"));
    getIconRequest->runSync ();
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::NoError);
    QScopedPointer<QSnapdIcon> icon (getIconRequest->icon ());
    g_assert (icon->mimeType () == "image/png");
    QByteArray data = icon->data ();
    g_assert_cmpmem (data.data (), data.size (), icon_buffer, icon_buffer_length);
}

static void
test_get_assertions ()
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetAssertionsRequest> getAssertionsRequest (client.getAssertions ("account"));
    getAssertionsRequest->runSync ();
    g_assert_cmpint (getAssertionsRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_add_assertions ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_get_assertions (snapd) == NULL);
    QScopedPointer<QSnapdAddAssertionsRequest> addAssertionsRequest (client.addAssertions (QStringList () << "type: account\n\nSIGNATURE"));
    addAssertionsRequest->runSync ();
    g_assert_cmpint (addAssertionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (g_list_length (mock_snapd_get_assertions (snapd)), ==, 1);
    g_assert_cmpstr ((gchar*) mock_snapd_get_assertions (snapd)->data, ==, "type: account\n\nSIGNATURE");
}

static void
test_assertions ()
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
    g_assert (assertion.header ("invalid") == NULL);
    g_assert (assertion.body () == NULL);
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
    g_assert (assertion.header ("invalid") == NULL);
    g_assert (assertion.body () == "BODY");
    g_assert (assertion.signature () == "SIGNATURE");
}

static void
test_get_interfaces ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *sl = mock_snap_add_slot (s, "slot1");
    mock_snap_add_slot (s, "slot2");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *p = mock_snap_add_plug (s, "plug1");
    p->connection = sl;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetInterfacesRequest> getInterfacesRequest (client.getInterfaces ());
    getInterfacesRequest->runSync ();
    g_assert_cmpint (getInterfacesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getInterfacesRequest->plugCount (), ==, 0);
    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 0);
}

static void
test_connect_interface ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (plug->connection == slot);
}

static void
test_connect_interface_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (connectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (plug->connection == slot);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_connect_interface_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdConnectInterfaceRequest> connectInterfaceRequest (client.connectInterface ("snap2", "plug", "snap1", "slot"));
    connectInterfaceRequest->runSync ();
    g_assert_cmpint (connectInterfaceRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_disconnect_interface ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    plug->connection = slot;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (plug->connection == NULL);
}

static void
test_disconnect_interface_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockSlot *slot = mock_snap_add_slot (s, "slot");
    s = mock_snapd_add_snap (snapd, "snap2");
    MockPlug *plug = mock_snap_add_plug (s, "plug");
    plug->connection = slot;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisconnectInterfaceRequest> disconnectInterfaceRequest (client.disconnectInterface ("snap2", "plug", "snap1", "slot"));
    ProgressCounter counter;
    QObject::connect (disconnectInterfaceRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    disconnectInterfaceRequest->runSync ();
    g_assert_cmpint (disconnectInterfaceRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (plug->connection == NULL);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_disconnect_interface_invalid ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    s->download_size = 1024;
    mock_snap_add_price (s, 1.20, "NZD");
    mock_snap_add_price (s, 0.87, "USD");
    mock_snap_add_screenshot (s, "screenshot0.png", 0, 0);
    mock_snap_add_screenshot (s, "screenshot1.png", 1024, 1024);
    s->trymode = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::None, "carrot"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 2);
    g_assert (findRequest->suggestedCurrency () == "NZD");
    QScopedPointer<QSnapdSnap> snap0 (findRequest->snap (0));
    g_assert (snap0->name () == "carrot1");
    QScopedPointer<QSnapdSnap> snap1 (findRequest->snap (1));
    g_assert (snap1->channel () == "CHANNEL");
    g_assert_cmpint (snap1->confinement (), ==, QSnapdEnums::SnapConfinementStrict);
    g_assert (snap1->contact () == "CONTACT");
    g_assert (snap1->description () == "DESCRIPTION");
    g_assert (snap1->developer () == "DEVELOPER");
    g_assert_cmpint (snap1->downloadSize (), ==, 1024);
    g_assert (snap1->icon () == "ICON");
    g_assert (snap1->id () == "ID");
    g_assert (!snap1->installDate ().isValid ());
    g_assert_cmpint (snap1->installedSize (), ==, 0);
    g_assert (snap1->name () == "carrot2");
    g_assert_cmpint (snap1->priceCount (), ==, 2);
    QScopedPointer<QSnapdPrice> price0 (snap1->price (0));
    g_assert_cmpfloat (price0->amount (), ==, 1.20);
    g_assert (price0->currency () == "NZD");
    QScopedPointer<QSnapdPrice> price1 (snap1->price (1));
    g_assert_cmpfloat (price1->amount (), ==, 0.87);
    g_assert (price1->currency () == "USD");
    g_assert (snap1->isPrivate () == FALSE);
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
    g_assert (snap1->trymode () == TRUE);
    g_assert (snap1->version () == "VERSION");
}

static void
test_find_query_private ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    mock_snapd_add_store_snap (snapd, "snap1");
    mock_account_add_private_snap (a, "snap2");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    g_assert (snap->isPrivate () == TRUE);
}

static void
test_find_query_private_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    mock_account_add_private_snap (a, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    g_assert (snap->isPrivate () == TRUE);
}

static void
test_find_name_private_not_logged_in ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName | QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::AuthDataRequired);
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdFindRequest> findRequest (client.findSection (QSnapdClient::MatchName, "section", "carrot1"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    QScopedPointer<QSnapdSnap> snap (findRequest->snap (0));
    g_assert (snap->name () == "carrot1");
}

static void
test_find_refreshable ()
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdFindRefreshableRequest> findRefreshableRequest (client.findRefreshable ());
    findRefreshableRequest->runSync ();
    g_assert_cmpint (findRefreshableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRefreshableRequest->snapCount (), ==, 0);
}

static void
test_install ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->confinement, ==, "strict");
    g_assert (!mock_snapd_find_snap (snapd, "snap")->devmode);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->dangerous);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->jailmode);
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
        g_assert (change->ready ());
    else
        g_assert (!change->ready ());
    g_assert (change->spawnTime () == spawnTime);
    if (change->ready ())
        g_assert (readyTime == readyTime);
    else
        g_assert (!readyTime.isValid ());
}

static void
test_install_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->confinement, ==, "classic");
}

static void
test_install_needs_classic_system ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_confinement (s, "classic");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Devmode, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert (mock_snapd_find_snap (snapd, "snap")->devmode);
}

static void
test_install_dangerous ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Dangerous, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert (mock_snapd_find_snap (snapd, "snap")->dangerous);
}

static void
test_install_jailmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Jailmode, "snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert (mock_snapd_find_snap (snapd, "snap")->jailmode);
}

static void
test_install_channel ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel1");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_channel (s, "channel2");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", "channel2"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->channel, ==, "channel2");
}

static void
test_install_revision ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.2");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1.1");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap", NULL, "1.1"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->revision, ==, "1.1");
}

static void
test_install_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdInstallRequest> installRequest (client.install ("snap"));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_install_stream ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (&buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpstr (snap->confinement, ==, "strict");
    g_assert (snap->dangerous == FALSE);
    g_assert (snap->devmode == FALSE);
    g_assert (snap->jailmode == FALSE);
}

static void
test_install_stream_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
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
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_install_stream_classic ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Classic, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpstr (snap->confinement, ==, "classic");
    g_assert (snap->dangerous == FALSE);
    g_assert (snap->devmode == FALSE);
    g_assert (snap->jailmode == FALSE);
}

static void
test_install_stream_dangerous ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Dangerous, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpstr (snap->confinement, ==, "strict");
    g_assert (snap->dangerous == TRUE);
    g_assert (snap->devmode == FALSE);
    g_assert (snap->jailmode == FALSE);
}

static void
test_install_stream_devmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Devmode, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpstr (snap->confinement, ==, "strict");
    g_assert (snap->dangerous == FALSE);
    g_assert (snap->devmode == TRUE);
    g_assert (snap->jailmode == FALSE);
}

static void
test_install_stream_jailmode ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "sideload") == NULL);
    QBuffer buffer;
    buffer.open (QBuffer::ReadWrite);
    buffer.write ("SNAP");
    buffer.seek (0);
    QScopedPointer<QSnapdInstallRequest> installRequest (client.install (QSnapdClient::Jailmode, &buffer));
    installRequest->runSync ();
    g_assert_cmpint (installRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "sideload");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_data, ==, "SNAP");
    g_assert_cmpstr (snap->confinement, ==, "strict");
    g_assert (snap->dangerous == FALSE);
    g_assert (snap->devmode == FALSE);
    g_assert (snap->jailmode == TRUE);
}

static void
test_try ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdTryRequest> tryRequest (client.trySnap ("/path/to/snap"));
    tryRequest->runSync ();
    g_assert_cmpint (tryRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "try");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_path, ==, "/path/to/snap");
}

static void
test_try_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdTryRequest> tryRequest (client.trySnap ("/path/to/snap"));
    ProgressCounter counter;
    QObject::connect (tryRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    tryRequest->runSync ();
    g_assert_cmpint (tryRequest->error (), ==, QSnapdRequest::NoError);
    MockSnap *snap = mock_snapd_find_snap (snapd, "try");
    g_assert (snap != NULL);
    g_assert_cmpstr (snap->snap_path, ==, "/path/to/snap");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_refresh ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "1");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap", "channel2"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (mock_snapd_find_snap (snapd, "snap")->channel, ==, "channel2");
}

static void
test_refresh_no_updates ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");
    s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_revision (s, "0");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NoUpdateAvailable);
}

static void
test_refresh_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdRefreshRequest> refreshRequest (client.refresh ("snap"));
    refreshRequest->runSync ();
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (refreshRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_refresh_all ()
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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdRefreshAllRequest> refreshAllRequest (client.refreshAll ());
    refreshAllRequest->runSync ();
    g_assert_cmpint (refreshAllRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (refreshAllRequest->snapNames ().count (), ==, 0);
}

static void
test_remove ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
}

static void
test_remove_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    g_assert (mock_snapd_find_snap (snapd, "snap") != NULL);
    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    ProgressCounter counter;
    QObject::connect (removeRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap") == NULL);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_remove_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdRemoveRequest> removeRequest (client.remove ("snap"));
    removeRequest->runSync ();
    g_assert_cmpint (removeRequest->error (), ==, QSnapdRequest::NotInstalled);
}

static void
test_enable ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->disabled);
}

static void
test_enable_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    ProgressCounter counter;
    QObject::connect (enableRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (!mock_snapd_find_snap (snapd, "snap")->disabled);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_enable_already_enabled ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = FALSE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_enable_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableRequest> enableRequest (client.enable ("snap"));
    enableRequest->runSync ();
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (enableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_disable ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = FALSE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap")->disabled);
}

static void
test_disable_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = FALSE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    ProgressCounter counter;
    QObject::connect (disableRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (mock_snapd_find_snap (snapd, "snap")->disabled);
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_disable_already_disabled ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    s->disabled = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_disable_not_installed ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisableRequest> disableRequest (client.disable ("snap"));
    disableRequest->runSync ();
    // FIXME: Should be a not installed error, see https://bugs.launchpad.net/bugs/1659106
    //g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::NotInstalled);
    g_assert_cmpint (disableRequest->error (), ==, QSnapdRequest::BadRequest);
}

static void
test_check_buy ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = FALSE;
    a->has_payment_methods = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = FALSE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdCheckBuyRequest> checkBuyRequest (client.checkBuy ());
    checkBuyRequest->runSync ();
    g_assert_cmpint (checkBuyRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_buy ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdBuyRequest> buyRequest (client.buy ("ABCDEF", 1.20, "NZD"));
    buyRequest->runSync ();
    g_assert_cmpint (buyRequest->error (), ==, QSnapdRequest::AuthDataRequired);
}

static void
test_buy_not_available ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = FALSE;
    a->has_payment_methods = FALSE;
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = FALSE;
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    MockAccount *a = mock_snapd_add_account (snapd, "test@example.com", "secret", NULL);
    a->terms_accepted = TRUE;
    a->has_payment_methods = TRUE;
    MockSnap *s = mock_snapd_add_store_snap (snapd, "snap");
    mock_snap_set_id (s, "ABCDEF");
    mock_snap_add_price (s, 1.20, "NZD");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
test_get_sections ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_store_section (snapd, "SECTION1");
    mock_snapd_add_store_section (snapd, "SECTION2");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetSectionsRequest> getSectionsRequest (client.getSections ());
    getSectionsRequest->runSync ();
    g_assert_cmpint (getSectionsRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getSectionsRequest->sections ().count (), ==, 2);
    g_assert (getSectionsRequest->sections ()[0] == "SECTION1");
    g_assert (getSectionsRequest->sections ()[1] == "SECTION2");
}

static void
test_get_aliases ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *al = mock_app_add_alias (a, "alias1");
    mock_alias_set_status (al, "enabled");
    a = mock_snap_add_app (s, "app2");
    al = mock_app_add_alias (a, "alias2");
    mock_alias_set_status (al, "disabled");
    s = mock_snapd_add_snap (snapd, "snap2");
    a = mock_snap_add_app (s, "app3");
    mock_app_add_alias (a, "alias3");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetAliasesRequest> getAliasesRequest (client.getAliases ());
    getAliasesRequest->runSync ();
    g_assert_cmpint (getAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAliasesRequest->aliasCount (), ==, 3);
    QScopedPointer<QSnapdAlias> alias0 (getAliasesRequest->alias (0));
    g_assert (alias0->snap () == "snap1");
    g_assert (alias0->name () == "alias1");
    g_assert (alias0->app () == "app1");
    g_assert_cmpint (alias0->status (), ==, QSnapdEnums::AliasStatusEnabled);
    QScopedPointer<QSnapdAlias> alias1 (getAliasesRequest->alias (1));
    g_assert (alias1->snap () == "snap1");
    g_assert (alias1->name () == "alias2");
    g_assert (alias1->app () == "app2");
    g_assert_cmpint (alias1->status (), ==, QSnapdEnums::AliasStatusDisabled);
    QScopedPointer<QSnapdAlias> alias2 (getAliasesRequest->alias (2));
    g_assert (alias2->snap () == "snap2");
    g_assert (alias2->name () == "alias3");
    g_assert (alias2->app () == "app3");
    g_assert_cmpint (alias2->status (), ==, QSnapdEnums::AliasStatusDefault);
}

static void
test_get_aliases_empty ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetAliasesRequest> getAliasesRequest (client.getAliases ());
    getAliasesRequest->runSync ();
    g_assert_cmpint (getAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (getAliasesRequest->aliasCount (), ==, 0);
}

static void
test_enable_aliases ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *alias = mock_app_add_alias (a, "alias1");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableAliasesRequest> enableAliasesRequest (client.enableAliases ("snap1", QStringList () << "alias1"));
    enableAliasesRequest->runSync ();
    g_assert_cmpint (enableAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (alias->status, ==, "enabled");
}

static void
test_enable_aliases_multiple ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *alias1 = mock_app_add_alias (a, "alias1");
    a = mock_snap_add_app (s, "app2");
    MockAlias *alias2 = mock_app_add_alias (a, "alias2");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableAliasesRequest> enableAliasesRequest (client.enableAliases ("snap1", QStringList () << "alias1" << "alias2"));
    enableAliasesRequest->runSync ();
    g_assert_cmpint (enableAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (alias1->status, ==, "enabled");
    g_assert_cmpstr (alias2->status, ==, "enabled");
}

static void
test_enable_aliases_progress ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *alias = mock_app_add_alias (a, "alias1");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdEnableAliasesRequest> enableAliasesRequest (client.enableAliases ("snap1", QStringList () << "alias1"));
    ProgressCounter counter;
    QObject::connect (enableAliasesRequest.data (), SIGNAL (progress ()), &counter, SLOT (progress ()));
    enableAliasesRequest->runSync ();
    g_assert_cmpint (enableAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (alias->status, ==, "enabled");
    g_assert_cmpint (counter.progressDone, >, 0);
}

static void
test_disable_aliases ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *alias = mock_app_add_alias (a, "alias1");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdDisableAliasesRequest> disableAliasesRequest (client.disableAliases ("snap1", QStringList () << "alias1"));
    disableAliasesRequest->runSync ();
    g_assert_cmpint (disableAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpstr (alias->status, ==, "disabled");
}

static void
test_reset_aliases ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap1");
    MockApp *a = mock_snap_add_app (s, "app1");
    MockAlias *alias = mock_app_add_alias (a, "alias1");
    mock_alias_set_status (alias, "enabled");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdResetAliasesRequest> resetAliasesRequest (client.resetAliases ("snap1", QStringList () << "alias1"));
    resetAliasesRequest->runSync ();
    g_assert_cmpint (resetAliasesRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (alias->status == NULL);
}

static void
test_run_snapctl ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

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
    g_test_add_func ("/get-system-information/basic", test_get_system_information);
    g_test_add_func ("/get-system-information/async", test_get_system_information_async);
    g_test_add_func ("/get-system-information/store", test_get_system_information_store);
    g_test_add_func ("/get-system-information/confinement_strict", test_get_system_information_confinement_strict);
    g_test_add_func ("/get-system-information/confinement_none", test_get_system_information_confinement_none);
    g_test_add_func ("/get-system-information/confinement_unknown", test_get_system_information_confinement_unknown);
    g_test_add_func ("/login/basic", test_login);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/list/basic", test_list);
    g_test_add_func ("/list/async", test_list_async);
    g_test_add_func ("/list-one/basic", test_list_one);
    g_test_add_func ("/list-one/async", test_list_one_async);
    g_test_add_func ("/list-one/optional-fields", test_list_one_optional_fields);
    g_test_add_func ("/list-one/not-installed", test_list_one_not_installed);
    g_test_add_func ("/list-one/classic-confinement", test_list_one_classic_confinement);
    g_test_add_func ("/list-one/devmode-confinement", test_list_one_devmode_confinement);
    g_test_add_func ("/list-one/daemons", test_list_one_daemons);
    g_test_add_func ("/icon/basic", test_icon);
    g_test_add_func ("/icon/async", test_icon_async);
    g_test_add_func ("/icon/not-installed", test_icon_not_installed);
    g_test_add_func ("/icon/large", test_icon_large);
    g_test_add_func ("/get-assertions/basic", test_get_assertions);
    g_test_add_func ("/get-assertions/body", test_get_assertions_body);
    g_test_add_func ("/get-assertions/multiple", test_get_assertions_multiple);
    g_test_add_func ("/get-assertions/invalid", test_get_assertions_invalid);
    g_test_add_func ("/add-assertions/basic", test_add_assertions);
    g_test_add_func ("/assertions/basic", test_assertions);
    g_test_add_func ("/assertions/body", test_assertions_body);
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
    g_test_add_func ("/find/cancel", test_find_cancel);
    g_test_add_func ("/find/section", test_find_section);
    g_test_add_func ("/find/section_query", test_find_section_query);
    g_test_add_func ("/find/section_name", test_find_section_name);
    g_test_add_func ("/find-refreshable/basic", test_find_refreshable);
    g_test_add_func ("/find-refreshable/no-updates", test_find_refreshable_no_updates);
    g_test_add_func ("/install/basic", test_install);
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
    g_test_add_func ("/install-stream/basic", test_install_stream);
    g_test_add_func ("/install-stream/progress", test_install_stream_progress);
    g_test_add_func ("/install-stream/classic", test_install_stream_classic);
    g_test_add_func ("/install-stream/dangerous", test_install_stream_dangerous);
    g_test_add_func ("/install-stream/devmode", test_install_stream_devmode);
    g_test_add_func ("/install-stream/jailmode", test_install_stream_jailmode);
    g_test_add_func ("/try/basic", test_try);
    g_test_add_func ("/try/progress", test_try_progress);
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
    g_test_add_func ("/get-sections/basic", test_get_sections);
    g_test_add_func ("/get-aliases/basic", test_get_aliases);
    g_test_add_func ("/get-aliases/empty", test_get_aliases_empty);
    g_test_add_func ("/enable-aliases/basic", test_enable_aliases);
    g_test_add_func ("/enable-aliases/multiple", test_enable_aliases_multiple);
    g_test_add_func ("/enable-aliases/progress", test_enable_aliases_progress);
    g_test_add_func ("/disable-aliases/basic", test_disable_aliases);
    g_test_add_func ("/reset-aliases/basic", test_reset_aliases);
    g_test_add_func ("/run-snapctl/basic", test_run_snapctl);
    g_test_add_func ("/stress/basic", test_stress);

    return g_test_run ();
}
