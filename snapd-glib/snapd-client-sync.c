/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-client.h"
#include "snapd-error.h"

typedef struct {
    GMainContext *context;
    GMainLoop    *loop;
    GAsyncResult *result;
} SyncData;

static void
start_sync (SyncData *data)
{
    data->context = g_main_context_new ();
    data->loop = g_main_loop_new (data->context, FALSE);
    g_main_context_push_thread_default (data->context);
}

static void
end_sync (SyncData *data)
{
    if (data->loop != NULL)
        g_main_loop_run (data->loop);
    g_main_context_pop_thread_default (data->context);
}

static void
sync_data_clear (SyncData *data)
{
    g_clear_pointer (&data->loop, g_main_loop_unref);
    g_clear_pointer (&data->context, g_main_context_unref);
    g_clear_object (&data->result);
}

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (SyncData, sync_data_clear)

static void
sync_cb (GObject *object, GAsyncResult *result, gpointer user_data)
{
    SyncData *data = user_data;
    data->result = g_object_ref (result);
    g_main_loop_quit (data->loop);
    g_clear_pointer (&data->loop, g_main_loop_unref);
}

/**
 * snapd_client_connect_sync:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * This method is no longer required and does nothing, snapd-glib now connects on demand.
 *
 * Returns: %TRUE if successfully connected to snapd.
 *
 * Since: 1.0
 */
gboolean
snapd_client_connect_sync (SnapdClient *client,
                           GCancellable *cancellable, GError **error)
{
    return TRUE;
}

/**
 * snapd_client_login_sync:
 * @client: a #SnapdClient.
 * @email: email address to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Log in to snapd and get authorization to install/remove snaps.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 *
 * Since: 1.0
 * Deprecated: 1.26: Use snapd_client_login2_sync()
 */
SnapdAuthData *
snapd_client_login_sync (SnapdClient *client,
                         const gchar *email, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (email != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_login_async (client, email, password, otp, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_login_finish (client, data.result, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_login2_sync:
 * @client: a #SnapdClient.
 * @email: email address to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Log in to snapd and get authorization to install/remove snaps.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.26
 */
SnapdUserInformation *
snapd_client_login2_sync (SnapdClient *client,
                         const gchar *email, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (email != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    start_sync (&data);
    snapd_client_login2_async (client, email, password, otp, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_login2_finish (client, data.result, error);
}

/**
 * snapd_client_get_changes_sync:
 * @client: a #SnapdClient.
 * @filter: changes to filter on.
 * @snap_name: (allow-none): name of snap to filter on or %NULL for changes for any snap.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get changes that have occurred / are occurring on the snap daemon.
 *
 * Returns: (transfer container) (element-type SnapdChange): an array of #SnapdChange or %NULL on error.
 *
 * Since: 1.29
 */
GPtrArray *
snapd_client_get_changes_sync (SnapdClient *client,
                               SnapdChangeFilter filter, const gchar *snap_name,
                               GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_changes_async (client, filter, snap_name, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_changes_finish (client, data.result, error);
}

/**
 * snapd_client_get_change_sync:
 * @client: a #SnapdClient.
 * @id: a change ID to get information on.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on a change.
 *
 * Returns: (transfer full): a #SnapdChange or %NULL on error.
 *
 * Since: 1.29
 */
SnapdChange *
snapd_client_get_change_sync (SnapdClient *client,
                              const gchar *id,
                              GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    start_sync (&data);
    snapd_client_get_change_async (client, id, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_change_finish (client, data.result, error);
}

/**
 * snapd_client_abort_change_sync:
 * @client: a #SnapdClient.
 * @id: a change ID to abort.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on a change.
 *
 * Returns: (transfer full): a #SnapdChange or %NULL on error.
 *
 * Since: 1.30
 */
SnapdChange *
snapd_client_abort_change_sync (SnapdClient *client,
                                const gchar *id,
                                GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    start_sync (&data);
    snapd_client_abort_change_async (client, id, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_abort_change_finish (client, data.result, error);
}

/**
 * snapd_client_get_system_information_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Request system information from snapd.
 * While this blocks, snapd is expected to return the information quickly.
 *
 * Returns: (transfer full): a #SnapdSystemInformation or %NULL on error.
 *
 * Since: 1.0
 */
SnapdSystemInformation *
snapd_client_get_system_information_sync (SnapdClient *client,
                                          GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_system_information_async (client, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_system_information_finish (client, data.result, error);
}

/**
 * snapd_client_list_one_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information of a single installed snap.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 * Deprecated: 1.42: Use snapd_client_get_snap_sync()
 */
SnapdSnap *
snapd_client_list_one_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    return snapd_client_get_snap_sync (client, name, cancellable, error);
}

/**
 * snapd_client_get_snap_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information of a single installed snap. If the snap does not exist an error occurs.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 *
 * Since: 1.42
 */
SnapdSnap *
snapd_client_get_snap_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_snap_async (client, name, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_snap_finish (client, data.result, error);
}

/**
 * snapd_client_get_apps_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetAppsFlags to control what results are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on installed apps.
 *
 * Returns: (transfer container) (element-type SnapdApp): an array of #SnapdApp or %NULL on error.
 *
 * Since: 1.25
 */
GPtrArray *
snapd_client_get_apps_sync (SnapdClient *client,
                            SnapdGetAppsFlags flags,
                            GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_apps_async (client, flags, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_apps_finish (client, data.result, error);
}

/**
 * snapd_client_get_icon_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the icon for an installed snap.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 *
 * Since: 1.0
 */
SnapdIcon *
snapd_client_get_icon_sync (SnapdClient *client,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_icon_async (client, name, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_icon_finish (client, data.result, error);
}

/**
 * snapd_client_list_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on all installed snaps.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 * Deprecated: 1.42: Use snapd_client_get_snaps_sync()
 */
GPtrArray *
snapd_client_list_sync (SnapdClient *client,
                        GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_list_async (client, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_list_finish (client, data.result, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_get_snaps_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetSnapsFlags to control what results are returned.
 * @names: (allow-none): A list of snap names or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on installed snaps. If @flags contains %SNAPD_GET_SNAPS_FLAGS_ALL_REVISIONS
 * then all installed revisions are returned (there may be more than one revision per snap).
 * Otherwise only the active revisions are returned.
 *
 * If @names is not %NULL and contains at least one name only snaps that match these names are
 * returned. If a snap is not installed it is not returned (no error is generated).
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.42
 */
GPtrArray *
snapd_client_get_snaps_sync (SnapdClient *client,
                             SnapdGetSnapsFlags flags, gchar **names,
                             GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_snaps_async (client, flags, names, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_snaps_finish (client, data.result, error);
}

/**
 * snapd_client_get_assertions_sync:
 * @client: a #SnapdClient.
 * @type: assertion type to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get assertions.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of assertions or %NULL on error.
 *
 * Since: 1.8
 */
gchar **
snapd_client_get_assertions_sync (SnapdClient *client,
                                  const gchar *type,
                                  GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_assertions_async (client, type, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_assertions_finish (client, data.result, error);
}

/**
 * snapd_client_add_assertions_sync:
 * @client: a #SnapdClient.
 * @assertions: assertions to add.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Add an assertion.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_add_assertions_sync (SnapdClient *client,
                                  gchar **assertions,
                                  GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (assertions != NULL, FALSE);

    start_sync (&data);
    snapd_client_add_assertions_async (client, assertions, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_add_assertions_finish (client, data.result, error);
}

/**
 * snapd_client_get_interfaces_sync:
 * @client: a #SnapdClient.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the installed snap interfaces.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_get_interfaces_sync (SnapdClient *client,
                                  GPtrArray **plugs, GPtrArray **slots,
                                  GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    start_sync (&data);
    snapd_client_get_interfaces_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_interfaces_finish (client, data.result, plugs, slots, error);
}

/**
 * snapd_client_connect_interface_sync:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Connect two interfaces together.
 * An asynchronous version of this function is snapd_client_connect_interface_async().
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_connect_interface_sync (SnapdClient *client,
                                     const gchar *plug_snap, const gchar *plug_name,
                                     const gchar *slot_snap, const gchar *slot_name,
                                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                     GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    start_sync (&data);
    snapd_client_connect_interface_async (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_connect_interface_finish (client, data.result, error);
}

/**
 * snapd_client_disconnect_interface_sync:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Disconnect two interfaces.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disconnect_interface_sync (SnapdClient *client,
                                        const gchar *plug_snap, const gchar *plug_name,
                                        const gchar *slot_snap, const gchar *slot_name,
                                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                        GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    start_sync (&data);
    snapd_client_disconnect_interface_async (client, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_disconnect_interface_finish (client, data.result, error);
}

/**
 * snapd_client_find_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: query string to send.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in the store.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_client_find_sync (SnapdClient *client,
                        SnapdFindFlags flags, const gchar *query,
                        gchar **suggested_currency,
                        GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (query != NULL, NULL);
    return snapd_client_find_section_sync (client, flags, NULL, query, suggested_currency, cancellable, error);
}

/**
 * snapd_client_find_section_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @section: (allow-none): store section to search in or %NULL to search in all sections.
 * @query: (allow-none): query string to send or %NULL to get all snaps from the given section.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in the store.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.7
 */
GPtrArray *
snapd_client_find_section_sync (SnapdClient *client,
                                SnapdFindFlags flags, const gchar *section, const gchar *query,
                                gchar **suggested_currency,
                                GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_find_section_async (client, flags, section, query, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_find_section_finish (client, data.result, suggested_currency, error);
}

/**
 * snapd_client_find_refreshable_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in store that are newer revisions than locally installed versions.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.8
 */
GPtrArray *
snapd_client_find_refreshable_sync (SnapdClient *client,
                                    GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_find_refreshable_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_find_refreshable_finish (client, data.result, error);
}

/**
 * snapd_client_install_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Install a snap from the store.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use snapd_client_install2_sync()
 */
gboolean
snapd_client_install_sync (SnapdClient *client,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    return snapd_client_install2_sync (client, SNAPD_INSTALL_FLAGS_NONE, name, channel, NULL, progress_callback, progress_callback_data, cancellable, error);
}

/**
 * snapd_client_install2_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @revision: (allow-none): revision to install or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Install a snap from the store.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.12
 */
gboolean
snapd_client_install2_sync (SnapdClient *client,
                            SnapdInstallFlags flags,
                            const gchar *name, const gchar *channel, const gchar *revision,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_install2_async (client, flags, name, channel, revision, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_install2_finish (client, data.result, error);
}

/**
 * snapd_client_install_stream_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @stream: a #GInputStream containing the snap file contents to install.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Install a snap. The snap contents are provided in the form of an input stream.
 * To install from a local file, do the following:
 *
 * |[
 * g_autoptr(GFile) file = g_file_new_for_path (path_to_snap_file);
 * g_autoptr(GInputStream) stream = g_file_read (file, cancellable, &error);
 * snapd_client_install_stream_sync (client, stream, progress_cb, NULL, cancellable, &error);
 * \]
 *
 * Or if you have the file in memory you can use:
 *
 * |[
 * g_autoptr(GInputStream) stream = g_memory_input_stream_new_from_data (data, data_length, free_data);
 * snapd_client_install_stream_sync (client, stream, progress_cb, NULL, cancellable, &error);
 * \]
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_install_stream_sync (SnapdClient *client,
                                  SnapdInstallFlags flags,
                                  GInputStream *stream,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);

    start_sync (&data);
    snapd_client_install_stream_async (client, flags, stream, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_install_stream_finish (client, data.result, error);
}

/**
 * snapd_client_try_sync:
 * @client: a #SnapdClient.
 * @path: path to snap directory to try.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Try a snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_try_sync (SnapdClient *client,
                       const gchar *path,
                       SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                       GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (path != NULL, FALSE);

    start_sync (&data);
    snapd_client_try_async (client, path, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_try_finish (client, data.result, error);
}

/**
 * snapd_client_refresh_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Ensure an installed snap is at the latest version.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_refresh_sync (SnapdClient *client,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_refresh_async (client, name, channel, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_refresh_finish (client, data.result, error);
}

/**
 * snapd_client_refresh_all_sync:
 * @client: a #SnapdClient.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Update all installed snaps to their latest version.
 *
 * Returns: (transfer full): a %NULL-terminated array of the snap names refreshed or %NULL on error.
 *
 * Since: 1.5
 */
gchar **
snapd_client_refresh_all_sync (SnapdClient *client,
                               SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                               GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_refresh_all_async (client, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_refresh_all_finish (client, data.result, error);
}

/**
 * snapd_client_remove_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Uninstall a snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_remove_sync (SnapdClient *client,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_remove_async (client, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_remove_finish (client, data.result, error);
}

/**
 * snapd_client_enable_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Enable an installed snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_enable_sync (SnapdClient *client,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_enable_async (client, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_enable_finish (client, data.result, error);
}

/**
 * snapd_client_disable_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Disable an installed snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disable_sync (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_disable_async (client, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_disable_finish (client, data.result, error);
}

/**
 * snapd_client_switch_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to switch channel.
 * @channel: channel to track.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Set the tracking channel on an installed snap.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.26
 */
gboolean
snapd_client_switch_sync (SnapdClient *client,
                          const gchar *name, const gchar *channel,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    start_sync (&data);
    snapd_client_switch_async (client, name, channel, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_switch_finish (client, data.result, error);
}

/**
 * snapd_client_check_buy_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Check if able to buy snaps.
 *
 * Returns: %TRUE if able to buy snaps or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_check_buy_sync (SnapdClient *client,
                             GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    start_sync (&data);
    snapd_client_check_buy_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_check_buy_finish (client, data.result, error);
}

/**
 * snapd_client_buy_sync:
 * @client: a #SnapdClient.
 * @id: id of snap to buy.
 * @amount: amount of currency to spend, e.g. 0.99.
 * @currency: the currency to buy with as an ISO 4217 currency code, e.g. "NZD".
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Buy a snap from the store. Once purchased, this snap can be installed with
 * snapd_client_install2_sync().
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_buy_sync (SnapdClient *client,
                       const gchar *id, gdouble amount, const gchar *currency,
                       GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (id != NULL, FALSE);
    g_return_val_if_fail (currency != NULL, FALSE);

    start_sync (&data);
    snapd_client_buy_async (client, id, amount, currency, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_buy_finish (client, data.result, error);
}

/**
 * snapd_client_create_user_sync:
 * @client: a #SnapdClient.
 * @email: the email of the user to create.
 * @flags: a set of #SnapdCreateUserFlags to control how the user account is created.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Create a local user account for the given user.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
SnapdUserInformation *
snapd_client_create_user_sync (SnapdClient *client,
                               const gchar *email, SnapdCreateUserFlags flags,
                               GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (email != NULL, NULL);

    start_sync (&data);
    snapd_client_create_user_async (client, email, flags, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_create_user_finish (client, data.result, error);
}

/**
 * snapd_client_create_users_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Create local user accounts using the system-user assertions that are valid for this device.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
GPtrArray *
snapd_client_create_users_sync (SnapdClient *client,
                                GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_create_users_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_create_users_finish (client, data.result, error);
}

/**
 * snapd_client_get_users_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get user accounts that are valid for this device.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.26
 */
GPtrArray *
snapd_client_get_users_sync (SnapdClient *client,
                             GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_users_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_users_finish (client, data.result, error);
}

/**
 * snapd_client_get_sections_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get the store sections.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of section names or %NULL on error.
 *
 * Since: 1.7
 */
gchar **
snapd_client_get_sections_sync (SnapdClient *client,
                                GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_sections_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_sections_finish (client, data.result, error);
}

/**
 * snapd_client_get_aliases_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get the available aliases.
 *
 * Returns: (transfer container) (element-type SnapdAlias): an array of #SnapdAlias or %NULL on error.
 *
 * Since: 1.8
 */
GPtrArray *
snapd_client_get_aliases_sync (SnapdClient *client,
                               GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    start_sync (&data);
    snapd_client_get_aliases_async (client, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_aliases_finish (client, data.result, error);
}

/**
 * snapd_client_alias_sync:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @app: an app in the snap to make the alias to.
 * @alias: the name of the alias (i.e. the command that will run this app).
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Create an alias to an app.
 *
 * Since: 1.25
 */
gboolean
snapd_client_alias_sync (SnapdClient *client,
                         const gchar *snap, const gchar *app, const gchar *alias,
                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                         GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (snap != NULL, FALSE);
    g_return_val_if_fail (app != NULL, FALSE);
    g_return_val_if_fail (alias != NULL, FALSE);

    start_sync (&data);
    snapd_client_alias_async (client, snap, app, alias, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_alias_finish (client, data.result, error);
}

/**
 * snapd_client_unalias_sync:
 * @client: a #SnapdClient.
 * @snap: (allow-none): the name of the snap to modify or %NULL.
 * @alias: (allow-none): the name of the alias to remove or %NULL to remove all aliases for the given snap.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Remove an alias from an app.
 *
 * Since: 1.25
 */
gboolean
snapd_client_unalias_sync (SnapdClient *client,
                           const gchar *snap, const gchar *alias,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (alias != NULL, FALSE);

    start_sync (&data);
    snapd_client_unalias_async (client, snap, alias, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_unalias_finish (client, data.result, error);
}

/**
 * snapd_client_prefer_sync:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * ???
 *
 * Since: 1.25
 */
gboolean
snapd_client_prefer_sync (SnapdClient *client,
                          const gchar *snap,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (snap != NULL, FALSE);

    start_sync (&data);
    snapd_client_prefer_async (client, snap, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_prefer_finish (client, data.result, error);
}

/**
 * snapd_client_enable_aliases_sync:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Change the state of aliases.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_alias_sync()
 */
gboolean
snapd_client_enable_aliases_sync (SnapdClient *client,
                                  const gchar *snap, gchar **aliases,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GError **error)
{
    g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_enable_aliases_async is deprecated");
    return FALSE;
}

/**
 * snapd_client_disable_aliases_sync:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Change the state of aliases.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_unalias_sync()
 */
gboolean
snapd_client_disable_aliases_sync (SnapdClient *client,
                                   const gchar *snap, gchar **aliases,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GError **error)
{
    g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_disable_aliases_async is deprecated");
    return FALSE;
}

/**
 * snapd_client_reset_aliases_sync:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Change the state of aliases.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_disable_aliases_sync()
 */
gboolean
snapd_client_reset_aliases_sync (SnapdClient *client,
                                 const gchar *snap, gchar **aliases,
                                 SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                 GCancellable *cancellable, GError **error)
{
    g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_reset_aliases_async is deprecated");
    return FALSE;
}

/**
 * snapd_client_run_snapctl_sync:
 * @client: a #SnapdClient.
 * @context_id: context for this call.
 * @args: the arguments to pass to snapctl.
 * @stdout_output: (out) (allow-none): the location to write the stdout from the command or %NULL.
 * @stderr_output: (out) (allow-none): the location to write the stderr from the command or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Run a snapctl command.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_run_snapctl_sync (SnapdClient *client,
                               const gchar *context_id, gchar **args,
                               gchar **stdout_output, gchar **stderr_output,
                               GCancellable *cancellable, GError **error)
{
    g_auto(SyncData) data = { 0 };

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (context_id != NULL, FALSE);
    g_return_val_if_fail (args != NULL, FALSE);

    start_sync (&data);
    snapd_client_run_snapctl_async (client, context_id, args, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_run_snapctl_finish (client, data.result, stdout_output, stderr_output, error);
}
