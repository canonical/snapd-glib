/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdio.h>

#include <glib.h>
#include <snapd-glib/snapd-glib.h>

int main (int argc, char **argv)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_autoptr(GError) error = NULL;
    const char *context;
    g_autofree GStrv args = NULL;
    g_autofree gchar *stdout_output = NULL;
    g_autofree gchar *stderr_output = NULL;
    int exit_code, i;

    /* snapctl commands are sent over a different socket that is made
     * available within the snap sandbox */
    snapd_client_set_socket_path (client, "/run/snapd-snap.socket");

    /* Take context from the environment if available */
    context = g_getenv ("SNAP_COOKIE");
    if (!context)
        context = "";

    /* run_snapctl expects a NULL terminated argument array */
    args = g_new0 (char *, argc);
    for (i = 0; i < argc-1; i++)
        args[i] = argv[i+1];

    if (!snapd_client_run_snapctl2_sync (client, context, args, &stdout_output, &stderr_output, &exit_code, NULL, &error)) {
        fprintf(stderr, "error: %s\n", error->message);
        return 1;
    }
    if (stdout_output) {
        fputs(stdout_output, stdout);
    }
    if (stderr_output) {
        fputs(stderr_output, stderr);
    }
    return exit_code;
}
