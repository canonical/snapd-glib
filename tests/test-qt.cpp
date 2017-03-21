/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "mock-snapd.h"

#include <Snapd/Client>
#include <Snapd/Assertion>

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
    g_assert (infoRequest->systemInformation ()->osId () == "OS-ID");
    g_assert (infoRequest->systemInformation ()->osVersion () == "OS-VERSION");
    g_assert (infoRequest->systemInformation ()->series () == "SERIES");
    g_assert (infoRequest->systemInformation ()->version () == "VERSION");
    g_assert (infoRequest->systemInformation ()->managed ());
    g_assert (infoRequest->systemInformation ()->onClassic ());
    g_assert (infoRequest->systemInformation ()->store () == NULL);
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
    g_assert (infoRequest->systemInformation ()->store () == "store");
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
    g_assert (loginRequest->authData ()->macaroon () == a->macaroon);
    g_assert_cmpint (loginRequest->authData ()->discharges ().count (), ==, g_strv_length (a->discharges));
    for (int i = 0; a->discharges[i]; i++)
        g_assert (loginRequest->authData ()->discharges ()[i] == a->discharges[i]);
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
    g_assert (listRequest->snap (0)->name () == "snap1");
    g_assert (listRequest->snap (1)->name () == "snap2");
    g_assert (listRequest->snap (2)->name () == "snap3");
}

static void
test_list_one ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    MockSnap *s = mock_snapd_add_snap (snapd, "snap");
    MockApp *a = mock_snap_add_app (s, "app");
    mock_app_add_alias (a, "app2");
    mock_app_add_alias (a, "app3");
    mock_snap_set_confinement (s, "strict");
    s->devmode = TRUE;
    mock_snap_set_install_date (s, "2017-01-02T11:23:58Z");
    s->installed_size = 1024;
    s->jailmode = TRUE;
    s->trymode = TRUE;
    mock_snap_set_tracking_channel (s, "CHANNEL");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdListOneRequest> listOneRequest (client.listOne ("snap"));
    listOneRequest->runSync ();
    g_assert_cmpint (listOneRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (listOneRequest->snap ()->appCount (), ==, 1);
    QSnapdApp *app = listOneRequest->snap ()->app (0);
    g_assert (app->name () == "app");
    g_assert_cmpint (app->aliases ().count (), ==, 2);
    g_assert (app->aliases ()[0] == "app2");
    g_assert (app->aliases ()[1] == "app3");
    g_assert (listOneRequest->snap ()->channel () == "CHANNEL");
    g_assert_cmpint (listOneRequest->snap ()->confinement (), ==, QSnapdSnap::Strict);
    g_assert (listOneRequest->snap ()->description () == "DESCRIPTION");
    g_assert (listOneRequest->snap ()->developer () == "DEVELOPER");
    g_assert (listOneRequest->snap ()->devmode () == TRUE);
    g_assert_cmpint (listOneRequest->snap ()->downloadSize (), ==, 0);
    g_assert (listOneRequest->snap ()->icon () == "ICON");
    g_assert (listOneRequest->snap ()->id () == "ID");
    QDateTime date = QDateTime (QDate (2017, 1, 2), QTime (11, 23, 58), Qt::UTC);
    g_assert (listOneRequest->snap ()->installDate () == date);
    g_assert_cmpint (listOneRequest->snap ()->installedSize (), ==, 1024);
    g_assert (listOneRequest->snap ()->jailmode () == TRUE);
    g_assert (listOneRequest->snap ()->name () == "snap");
    g_assert_cmpint (listOneRequest->snap ()->priceCount (), ==, 0);
    g_assert (listOneRequest->snap ()->isPrivate () == FALSE);
    g_assert (listOneRequest->snap ()->revision () == "REVISION");
    g_assert_cmpint (listOneRequest->snap ()->screenshotCount (), ==, 0);
    g_assert_cmpint (listOneRequest->snap ()->snapType (), ==, QSnapdSnap::App);
    g_assert_cmpint (listOneRequest->snap ()->status (), ==, QSnapdSnap::Active);
    g_assert (listOneRequest->snap ()->summary () == "SUMMARY");
    g_assert (listOneRequest->snap ()->trackingChannel () == "CHANNEL");
    g_assert (listOneRequest->snap ()->trymode () == TRUE);
    g_assert (listOneRequest->snap ()->version () == "VERSION");
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
    g_assert_cmpint (listOneRequest->snap ()->confinement (), ==, QSnapdSnap::Classic);
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
    g_assert_cmpint (listOneRequest->snap ()->confinement (), ==, QSnapdSnap::Devmode);
}

static void
test_icon ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_add_snap (snapd, "snap");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdGetIconRequest> getIconRequest (client.getIcon ("snap"));
    getIconRequest->runSync ();
    g_assert_cmpint (getIconRequest->error (), ==, QSnapdRequest::NoError);
    g_assert (getIconRequest->icon ()->mimeType () == "image/png");
    QByteArray data = getIconRequest->icon ()->data ();
    g_assert_cmpmem (data.data (), data.size (), "ICON", 4);
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

    QSnapdPlug *plug = getInterfacesRequest->plug (0);
    g_assert (plug->name () == "plug1");
    g_assert (plug->snap () == "snap2");
    g_assert (plug->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (plug->label () == "LABEL");
    g_assert_cmpint (plug->connectionCount (), ==, 1);
    g_assert (plug->connection (0)->snap () == "snap1");
    g_assert (plug->connection (0)->name () == "slot1");

    g_assert_cmpint (getInterfacesRequest->slotCount (), ==, 2);

    QSnapdSlot *slot = getInterfacesRequest->slot (0);
    g_assert (slot->name () == "slot1");
    g_assert (slot->snap () == "snap1");
    g_assert (slot->interface () == "INTERFACE");
    // FIXME: Attributes
    g_assert (slot->label () == "LABEL");
    g_assert_cmpint (slot->connectionCount (), ==, 1);
    g_assert (slot->connection (0)->snap () == "snap2");
    g_assert (slot->connection (0)->name () == "plug1");  

    slot = getInterfacesRequest->slot (1);
    g_assert (slot->name () == "slot2");
    g_assert (slot->snap () == "snap1");
    g_assert_cmpint (slot->connectionCount (), ==, 0);
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
    g_assert (findRequest->snap (0)->name () == "carrot1");
    QSnapdSnap *snap = findRequest->snap (1);
    g_assert (snap->channel () == "CHANNEL");
    g_assert_cmpint (snap->confinement (), ==, QSnapdSnap::Strict);
    g_assert (snap->description () == "DESCRIPTION");
    g_assert (snap->developer () == "DEVELOPER");
    g_assert_cmpint (snap->downloadSize (), ==, 1024);
    g_assert (snap->icon () == "ICON");
    g_assert (snap->id () == "ID");
    g_assert (!snap->installDate ().isValid ());
    g_assert_cmpint (snap->installedSize (), ==, 0);
    g_assert (snap->name () == "carrot2");
    g_assert_cmpint (snap->priceCount (), ==, 2);
    g_assert_cmpfloat (snap->price (0)->amount (), ==, 1.20);
    g_assert (snap->price (0)->currency () == "NZD");
    g_assert_cmpfloat (snap->price (1)->amount (), ==, 0.87);
    g_assert (snap->price (1)->currency () == "USD");
    g_assert (snap->isPrivate () == FALSE);
    g_assert (snap->revision () == "REVISION");
    g_assert_cmpint (snap->screenshotCount (), ==, 2);
    g_assert (snap->screenshot (0)->url () == "screenshot0.png");
    g_assert (snap->screenshot (1)->url () == "screenshot1.png");
    g_assert_cmpint (snap->screenshot (1)->width (), ==, 1024);
    g_assert_cmpint (snap->screenshot (1)->height (), ==, 1024);
    g_assert_cmpint (snap->snapType (), ==, QSnapdSnap::App);
    g_assert_cmpint (snap->status (), ==, QSnapdSnap::Active);
    g_assert (snap->summary () == "SUMMARY");
    g_assert (snap->trymode () == TRUE);
    g_assert (snap->version () == "VERSION");
}

static void
test_find_empty ()
{
    g_autoptr(MockSnapd) snapd = mock_snapd_new ();
    mock_snapd_set_suggested_currency (snapd, "NZD");
    mock_snapd_add_store_snap (snapd, "apple");
    mock_snapd_add_store_snap (snapd, "banana");
    mock_snapd_add_store_snap (snapd, "carrot1");
    mock_snapd_add_store_snap (snapd, "carrot2");

    QSnapdClient client (g_socket_get_fd (mock_snapd_get_client_socket (snapd)));
    QScopedPointer<QSnapdConnectRequest> connectRequest (client.connect ());
    connectRequest->runSync ();
    g_assert_cmpint (connectRequest->error (), ==, QSnapdRequest::NoError);

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::None));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 4);
    g_assert (findRequest->snap (0)->name () == "apple");
    g_assert (findRequest->snap (1)->name () == "banana");
    g_assert (findRequest->snap (2)->name () == "carrot1");
    g_assert (findRequest->snap (3)->name () == "carrot2");
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
    client.setAuthData (loginRequest->authData ());

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    g_assert (findRequest->snap (0)->name () == "snap2");
    g_assert (findRequest->snap (0)->isPrivate () == TRUE);
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
    g_assert (findRequest->snap (0)->name () == "snap");
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
    client.setAuthData (loginRequest->authData ());

    QScopedPointer<QSnapdFindRequest> findRequest (client.find (QSnapdClient::MatchName | QSnapdClient::SelectPrivate, "snap"));
    findRequest->runSync ();
    g_assert_cmpint (findRequest->error (), ==, QSnapdRequest::NoError);
    g_assert_cmpint (findRequest->snapCount (), ==, 1);
    g_assert (findRequest->snap (0)->name () == "snap");
    g_assert (findRequest->snap (0)->isPrivate () == TRUE);
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
    g_assert (findRequest->snap (0)->name () == "apple");
    g_assert (findRequest->snap (1)->name () == "carrot1");
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
    g_assert (findRequest->snap (0)->name () == "carrot1");
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
    g_assert (findRequest->snap (0)->name () == "carrot1");
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
    g_assert (findRefreshableRequest->snap (0)->name () == "snap1");
    g_assert (findRefreshableRequest->snap (0)->revision () == "1");
    g_assert (findRefreshableRequest->snap (1)->name () == "snap3");
    g_assert (findRefreshableRequest->snap (1)->revision () == "1");
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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    client.setAuthData (loginRequest->authData ());

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
    g_assert (getAliasesRequest->alias (0)->snap () == "snap1");
    g_assert (getAliasesRequest->alias (0)->name () == "alias1");
    g_assert (getAliasesRequest->alias (0)->app () == "app1");
    g_assert_cmpint (getAliasesRequest->alias (0)->status (), ==, QSnapdAlias::Enabled);
    g_assert (getAliasesRequest->alias (1)->snap () == "snap1");
    g_assert (getAliasesRequest->alias (1)->name () == "alias2");
    g_assert (getAliasesRequest->alias (1)->app () == "app2");
    g_assert_cmpint (getAliasesRequest->alias (1)->status (), ==, QSnapdAlias::Disabled);
    g_assert (getAliasesRequest->alias (2)->snap () == "snap2");
    g_assert (getAliasesRequest->alias (2)->name () == "alias3");
    g_assert (getAliasesRequest->alias (2)->app () == "app3");
    g_assert_cmpint (getAliasesRequest->alias (2)->status (), ==, QSnapdAlias::Default);
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

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/get-system-information/basic", test_get_system_information);
    g_test_add_func ("/get-system-information/store", test_get_system_information_store);
    g_test_add_func ("/login/basic", test_login);
    g_test_add_func ("/login/invalid-email", test_login_invalid_email);
    g_test_add_func ("/login/invalid-password", test_login_invalid_password);
    g_test_add_func ("/login/otp-missing", test_login_otp_missing);
    g_test_add_func ("/login/otp-invalid", test_login_otp_invalid);
    g_test_add_func ("/list/basic", test_list);
    g_test_add_func ("/list-one/basic", test_list_one);
    g_test_add_func ("/list-one/not-installed", test_list_one_not_installed);
    g_test_add_func ("/list-one/classic-confinement", test_list_one_classic_confinement);
    g_test_add_func ("/list-one/devmode-confinement", test_list_one_devmode_confinement);
    g_test_add_func ("/icon/basic", test_icon);
    g_test_add_func ("/icon/not-installed", test_icon_not_installed);
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
    //g_test_add_func ("/connect-interface/progress", test_connect_interface_progress);
    g_test_add_func ("/connect-interface/invalid", test_connect_interface_invalid);
    g_test_add_func ("/disconnect-interface/basic", test_disconnect_interface);
    //g_test_add_func ("/disconnect-interface/progress", test_disconnect_interface_progress);
    g_test_add_func ("/disconnect-interface/invalid", test_disconnect_interface_invalid);
    g_test_add_func ("/find/query", test_find_query);
    g_test_add_func ("/find/empty", test_find_empty);
    g_test_add_func ("/find/query-private", test_find_query_private);
    g_test_add_func ("/find/query-private/not-logged-in", test_find_query_private_not_logged_in);
    g_test_add_func ("/find/name", test_find_name);
    g_test_add_func ("/find/name-private", test_find_name_private);
    g_test_add_func ("/find/name-private/not-logged-in", test_find_name_private_not_logged_in);
    g_test_add_func ("/find/section", test_find_section);
    g_test_add_func ("/find/section_query", test_find_section_query);
    g_test_add_func ("/find/section_name", test_find_section_name);
    g_test_add_func ("/find-refreshable/basic", test_find_refreshable);
    g_test_add_func ("/find-refreshable/no-updates", test_find_refreshable_no_updates);
    g_test_add_func ("/install/basic", test_install);
    //g_test_add_func ("/install/progress", test_install_progress);
    g_test_add_func ("/install/channel", test_install_channel);
    g_test_add_func ("/install/not-available", test_install_not_available);
    g_test_add_func ("/refresh/basic", test_refresh);
    //g_test_add_func ("/refresh/progress", test_refresh_progress);
    g_test_add_func ("/refresh/channel", test_refresh_channel);
    g_test_add_func ("/refresh/no-updates", test_refresh_no_updates);
    g_test_add_func ("/refresh/not-installed", test_refresh_not_installed);
    g_test_add_func ("/refresh-all/basic", test_refresh_all);
    //g_test_add_func ("/refresh-all/progress", test_refresh_all_progress);
    g_test_add_func ("/refresh-all/no-updates", test_refresh_all_no_updates);
    g_test_add_func ("/remove/basic", test_remove);
    //g_test_add_func ("/remove/progress", test_remove_progress);
    g_test_add_func ("/remove/not-installed", test_remove_not_installed);
    g_test_add_func ("/enable/basic", test_enable);
    //g_test_add_func ("/enable/progress", test_enable_progress);
    g_test_add_func ("/enable/already-enabled", test_enable_already_enabled);
    g_test_add_func ("/enable/not-installed", test_enable_not_installed);
    g_test_add_func ("/disable/basic", test_disable);
    //g_test_add_func ("/disable/progress", test_disable_progress);
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
    //g_test_add_func ("/enable-aliases/progress", test_enable_aliases_progress);
    g_test_add_func ("/disable-aliases/basic", test_disable_aliases);
    g_test_add_func ("/reset-aliases/basic", test_reset_aliases);
    g_test_add_func ("/run-snapctl/basic", test_run_snapctl);

    return g_test_run ();
}
