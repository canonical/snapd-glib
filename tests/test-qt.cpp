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

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/get-system-information/basic", test_get_system_information);

    return g_test_run ();
}
