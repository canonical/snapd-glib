/*
 * Copyright (C) 2022 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

static void print_table (GPtrArray *columns)
{
    g_autofree size_t *column_widths = g_malloc0 (sizeof (size_t) * columns->len);
    guint n_rows = 0;
    for (guint c = 0; c < columns->len; c++) {
        GPtrArray *row = g_ptr_array_index (columns, c);
        if (row->len > n_rows) {
            n_rows = row->len;
        }
        for (guint r = 0; r < row->len; r++) {
            const gchar *value = g_ptr_array_index (row, r);
            size_t width = strlen (value);
            if (width > column_widths[c]) {
                column_widths[c] = width;
            }
        }
    }

    for (guint r = 0; r < n_rows; r++) {
       size_t padding_required = 0;
       for (guint c = 0; c < columns->len; c++) {
           GPtrArray *row = g_ptr_array_index (columns, c);
           const gchar *value = r < row->len ? g_ptr_array_index (row, r) : "";
           for (size_t i = 0; i < padding_required; i++) {
                g_printerr (" ");
           }
           if (c != 0) {
                g_printerr ("  ");
           }
           g_printerr ("%s", value);
           padding_required = column_widths[c] - g_utf8_strlen (value, -1);
       }
       g_printerr ("\n");
    }
}

static void add_to_column (GPtrArray *column, const gchar *value)
{
    g_ptr_array_add (column, value != NULL ? (gpointer) value : "–");
}

static int find (int argc, char **argv)
{
    if (argc > 1) {
        g_printerr ("error: too many arguments for command\n");
        return EXIT_FAILURE;
    }
    const gchar *query = argc > 0 ? argv[0] : NULL;

    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_find_category_sync (client, SNAPD_FIND_FLAGS_NONE, NULL, query, NULL, NULL, &error);
    if (snaps == NULL) {
        g_printerr ("error: failed to find: %s\n", error->message);
        return EXIT_FAILURE;
    }

    if (snaps->len == 0) {
        g_printerr ("No matching snaps for \"%s\"\n", query);
        return EXIT_FAILURE;
    }

    g_autoptr(GPtrArray) name_column = g_ptr_array_new ();
    add_to_column (name_column, "Name");
    g_autoptr(GPtrArray) version_column = g_ptr_array_new ();
    add_to_column (version_column, "Version");
    g_autoptr(GPtrArray) publisher_column = g_ptr_array_new ();
    add_to_column (publisher_column, "Publisher");
    g_autoptr(GPtrArray) summary_column = g_ptr_array_new ();
    add_to_column (summary_column, "Summary");
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        add_to_column (name_column, snapd_snap_get_name (snap));
        add_to_column (version_column, snapd_snap_get_version (snap));
        add_to_column (publisher_column, snapd_snap_get_publisher_display_name (snap));
        add_to_column (summary_column, snapd_snap_get_summary (snap));
    }
    g_autoptr(GPtrArray) columns = g_ptr_array_new ();
    g_ptr_array_add (columns, name_column);
    g_ptr_array_add (columns, version_column);
    g_ptr_array_add (columns, publisher_column);
    g_ptr_array_add (columns, summary_column);
    print_table (columns);

    return EXIT_SUCCESS;
}

static int info (int argc, char **argv)
{
    if (argc < 1) {
        g_printerr ("error: missing snap name(s)\n");
        return EXIT_FAILURE;
    }

    g_autoptr(SnapdClient) client = snapd_client_new ();
    for (int i = 0; i < argc; i++) {
        const char *name = argv[i];

        gchar *names[2] = { (gchar*) name, NULL };
        g_autoptr(GError) error = NULL;
        g_autoptr(GPtrArray) local_snaps = snapd_client_get_snaps_sync (client, SNAPD_GET_SNAPS_FLAGS_NONE, names, NULL, &error);
        g_autoptr(GPtrArray) store_snaps = snapd_client_find_category_sync (client, SNAPD_FIND_FLAGS_MATCH_NAME, NULL, name, NULL, NULL, &error);

        SnapdSnap *local_snap = local_snaps != NULL && local_snaps->len > 0 ? g_ptr_array_index (local_snaps, 0) : NULL;
        SnapdSnap *store_snap = store_snaps != NULL && store_snaps->len > 0 ? g_ptr_array_index (store_snaps, 0) : NULL;
        SnapdSnap *snap = store_snap ? store_snap : local_snap;
        if (snap == NULL) {
            g_printerr ("error: no snap found for \"%s\"\n", name);
        }

        const gchar *publisher = snapd_snap_get_publisher_display_name (snap);
        const gchar *license = snapd_snap_get_license (snap);

        if (i != 0) {
            g_printerr ("---\n");
        }
        g_printerr ("name:      %s\n", snapd_snap_get_name (snap));
        g_printerr ("summary:   %s\n", snapd_snap_get_summary (snap));
        g_printerr ("publisher: %s\n", publisher != NULL ? publisher : "–");
        if (store_snap != NULL) {
            g_printerr ("store-url: %s\n", snapd_snap_get_store_url (store_snap));
            g_printerr ("contact:   %s\n", snapd_snap_get_contact (store_snap));
        }
        g_printerr ("license:   %s\n", license != NULL ? license : "unset");
        g_printerr ("description: |\n");
        g_printerr ("  %s\n", snapd_snap_get_description (snap));
        if (store_snap != NULL) {
            g_printerr ("snap-id:   %s\n", snapd_snap_get_id (snap));
            g_printerr ("tracking:  %s\n", snapd_snap_get_tracking_channel (local_snap));
        }
    }

    return EXIT_SUCCESS;
}

static int install (int argc, char **argv)
{
    if (argc < 1) {
        g_printerr ("error: missing snap name(s)\n");
        return EXIT_FAILURE;
    }

    g_autoptr(SnapdClient) client = snapd_client_new ();
    for (int i = 0; i < argc; i++) {
        const char *name = argv[i];

        g_autoptr(GError) error = NULL;
        if (!snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, name, NULL, NULL, NULL, NULL, NULL, &error)) {
            g_printerr ("error: failed to install \"%s\": %s\n", name, error->message);
            continue;
        }

        gchar *names[2] = { (gchar*) name, NULL };
        g_autoptr(GPtrArray) local_snaps = snapd_client_get_snaps_sync (client, SNAPD_GET_SNAPS_FLAGS_NONE, names, NULL, &error);
        if (local_snaps == NULL || local_snaps->len == 0) {
            g_printerr ("error: failed to get information on installed snap %s: %s\n", name, error->message);
            continue;
        }
        SnapdSnap *local_snap = g_ptr_array_index (local_snaps, 0);

        g_printerr ("%s %s from %s (%s) installed\n", name, snapd_snap_get_version (local_snap), snapd_snap_get_publisher_display_name (local_snap), snapd_snap_get_publisher_username (local_snap));
    }

    return EXIT_SUCCESS;
}

static int remove (int argc, char **argv)
{
    if (argc < 1) {
        g_printerr ("error: missing snap name(s)\n");
        return EXIT_FAILURE;
    }

    g_autoptr(SnapdClient) client = snapd_client_new ();
    for (int i = 0; i < argc; i++) {
        const char *name = argv[i];

        g_autoptr(GError) error = NULL;
        if (!snapd_client_remove2_sync (client, SNAPD_REMOVE_FLAGS_NONE, name, NULL, NULL, NULL, &error)) {
            g_printerr ("error: failed to remove \"%s\": %s\n", name, error->message);
            continue;
        }

        g_printerr ("%s removed\n", name);
    }

    return EXIT_SUCCESS;
}

static int list (int argc, char **argv)
{
    if (argc > 1) {
        g_printerr ("error: too many arguments for command\n");
        return EXIT_FAILURE;
    }
    const gchar *name = argc > 0 ? argv[0] : NULL;
    gchar *names[2] = { (gchar*) name, NULL };

    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps = snapd_client_get_snaps_sync (client, SNAPD_GET_SNAPS_FLAGS_NONE, names, NULL, &error);
    if (snaps == NULL) {
        g_printerr ("error: failed to list: %s\n", error->message);
        return EXIT_FAILURE;
    }

    g_autoptr(GPtrArray) name_column = g_ptr_array_new ();
    add_to_column (name_column, "Name");
    g_autoptr(GPtrArray) version_column = g_ptr_array_new ();
    add_to_column (version_column, "Version");
    g_autoptr(GPtrArray) revision_column = g_ptr_array_new ();
    add_to_column (revision_column, "Rev");
    g_autoptr(GPtrArray) tracking_column = g_ptr_array_new ();
    add_to_column (tracking_column, "Tracking");
    g_autoptr(GPtrArray) publisher_column = g_ptr_array_new ();
    add_to_column (publisher_column, "Publisher");
    for (guint i = 0; i < snaps->len; i++) {
        SnapdSnap *snap = g_ptr_array_index (snaps, i);
        add_to_column (name_column, snapd_snap_get_name (snap));
        add_to_column (version_column, snapd_snap_get_version (snap));
        add_to_column (revision_column, snapd_snap_get_revision (snap));
        add_to_column (tracking_column, snapd_snap_get_tracking_channel (snap));
        add_to_column (publisher_column, snapd_snap_get_publisher_display_name (snap));
    }
    g_autoptr(GPtrArray) columns = g_ptr_array_new ();
    g_ptr_array_add (columns, name_column);
    g_ptr_array_add (columns, version_column);
    g_ptr_array_add (columns, revision_column);
    g_ptr_array_add (columns, tracking_column);
    g_ptr_array_add (columns, publisher_column);
    print_table (columns);

    return EXIT_SUCCESS;
}

static void print_log (SnapdLog *log)
{
    g_autofree gchar *timestamp = g_date_time_format (snapd_log_get_timestamp (log), "%Y-%m-%dT%H:%M:%SZ");
    g_printerr ("%s %s[%s]: %s\n", timestamp,
                snapd_log_get_sid (log),
                snapd_log_get_pid (log),
                snapd_log_get_message (log));
}

static void log_cb (SnapdClient *client, SnapdLog *log, gpointer user_data)
{
    print_log (log);
}

static int logs (int argc, char **argv)
{
    if (argc < 1) {
        g_printerr ("error: the required argument `<service> (at least 1 argument)` was not provided\n");
        return EXIT_FAILURE;
    }
    gboolean follow = FALSE;
    g_autoptr(GPtrArray) names_array = g_ptr_array_new ();
    for (int i = 0; i < argc; i++) {
        if (strcmp (argv[i], "-f") == 0) {
            follow = TRUE;
        } else {
            g_ptr_array_add (names_array, g_strdup (argv[i]));
        }
    }
    g_ptr_array_add (names_array, NULL);
    g_auto(GStrv) names = g_steal_pointer ((GStrv *)&names_array->pdata);

    g_autoptr(SnapdClient) client = snapd_client_new ();
    if (follow) {
        g_autoptr(GError) error = NULL;
        if (!snapd_client_follow_logs_sync (client, names, log_cb, NULL, NULL, &error)) {
            g_printerr ("error: failed to get logs: %s\n", error->message);
            return EXIT_FAILURE;
        }
    } else {
        g_autoptr(GError) error = NULL;
        g_autoptr(GPtrArray) logs = snapd_client_get_logs_sync (client, names, 0, NULL, &error);
        if (logs == NULL) {
            g_printerr ("error: failed to get logs: %s\n", error->message);
            return EXIT_FAILURE;
        }

        for (guint i = 0; i < logs->len; i++) {
            SnapdLog *log = g_ptr_array_index (logs, i);
            print_log (log);
        }
    }

    return EXIT_SUCCESS;
}

static int model (int argc, char **argv)
{
    g_autoptr(SnapdClient) client = snapd_client_new ();
    g_autoptr(GError) error = NULL;
    g_autofree gchar *model_assertion_text = snapd_client_get_model_assertion_sync (client, NULL, &error);
    if (model_assertion_text == NULL) {
        g_printerr ("error: failed to get model assertion: %s\n", error->message);
        return EXIT_FAILURE;
    }
    g_autofree gchar *serial_assertion_text = snapd_client_get_serial_assertion_sync (client, NULL, &error);
    if (serial_assertion_text == NULL) {
        g_printerr ("error: failed to get serial assertion: %s\n", error->message);
        return EXIT_FAILURE;
    }

    g_autoptr(SnapdAssertion) model_assertion = snapd_assertion_new (model_assertion_text);
    g_autoptr(SnapdAssertion) serial_assertion = snapd_assertion_new (serial_assertion_text);

    g_printerr ("brand  %s\n", snapd_assertion_get_header (model_assertion, "brand-id"));
    g_printerr ("model  %s\n", snapd_assertion_get_header (model_assertion, "model"));
    g_printerr ("serial %s\n", snapd_assertion_get_header (serial_assertion, "serial"));

    return EXIT_SUCCESS;
}

static int usage()
{
    g_printerr ("Usage snap-glib <command> [<options>...]\n");
    g_printerr ("Commands: find, info, install, remove, list, logs, model, help\n");

    return EXIT_SUCCESS;
}

int main (int argc, char **argv)
{
    if (argc < 2) {
        return usage ();
    }
    const char *command = argv[1];
    int command_argc = argc - 2;
    char **command_argv = argv + 2;

    if (strcmp (command, "find") == 0) {
        return find (command_argc, command_argv);
    }
    else if (strcmp (command, "info") == 0) {
        return info (command_argc, command_argv);
    }
    else if (strcmp (command, "install") == 0) {
        return install (command_argc, command_argv);
    }
    else if (strcmp (command, "remove") == 0) {
        return remove (command_argc, command_argv);
    }
    else if (strcmp (command, "list") == 0) {
        return list (command_argc, command_argv);
    }
    else if (strcmp (command, "logs") == 0) {
        return logs (command_argc, command_argv);
    }
    else if (strcmp (command, "model") == 0) {
        return model (command_argc, command_argv);
    }
    else if (strcmp (command, "help") == 0) {
        return usage ();
    }
    else {
        g_printerr ("error: unknown command \"%s\", see 'snap help'.\n", command);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
