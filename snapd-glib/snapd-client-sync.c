/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

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
snapd_client_connect_sync (SnapdClient *self,
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
snapd_client_login_sync (SnapdClient *self,
                         const gchar *email, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (email != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_login_async (self, email, password, otp, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_login_finish (self, data.result, error);
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
snapd_client_login2_sync (SnapdClient *self,
                         const gchar *email, const gchar *password, const gchar *otp,
                         GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (email != NULL, NULL);
    g_return_val_if_fail (password != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_login2_async (self, email, password, otp, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_login2_finish (self, data.result, error);
}

/**
 * snapd_client_logout_sync:
 * @client: a #SnapdClient.
 * @id: login ID to use.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Log out from snapd.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.55
 */
gboolean
snapd_client_logout_sync (SnapdClient *self,
                          gint64 id,
                          GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_logout_async (self, id, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_logout_finish (self, data.result, error);
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
snapd_client_get_changes_sync (SnapdClient *self,
                               SnapdChangeFilter filter, const gchar *snap_name,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_changes_async (self, filter, snap_name, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_changes_finish (self, data.result, error);
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
snapd_client_get_change_sync (SnapdClient *self,
                              const gchar *id,
                              GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_change_async (self, id, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_change_finish (self, data.result, error);
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
snapd_client_abort_change_sync (SnapdClient *self,
                                const gchar *id,
                                GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_abort_change_async (self, id, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_abort_change_finish (self, data.result, error);
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
snapd_client_get_system_information_sync (SnapdClient *self,
                                          GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_system_information_async (self, cancellable, sync_cb, &data);
    end_sync (&data);

    return snapd_client_get_system_information_finish (self, data.result, error);
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
snapd_client_list_one_sync (SnapdClient *self,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    return snapd_client_get_snap_sync (self, name, cancellable, error);
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
snapd_client_get_snap_sync (SnapdClient *self,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_snap_async (self, name, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_snap_finish (self, data.result, error);
}

/**
 * snapd_client_get_snap_conf_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to get configuration from.
 * @keys: (allow-none): keys to returns or %NULL to return all.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get configuration for a snap. System configuration is stored using the name "system".
 *
 * Returns: (transfer container) (element-type utf8 GVariant): a table of configuration values or %NULL on error.
 *
 * Since: 1.48
 */
GHashTable *
snapd_client_get_snap_conf_sync (SnapdClient *self,
                                 const gchar *name,
                                 GStrv keys,
                                 GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_snap_conf_async (self, name, keys, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_snap_conf_finish (self, data.result, error);
}

/**
 * snapd_client_set_snap_conf_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to set configuration for.
 * @key_values: (element-type utf8 GVariant): Keys to set.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Set configuration for a snap. System configuration is stored using the name "system".
 *
 * Returns: %TRUE if configuration successfully applied.
 *
 * Since: 1.48
 */
gboolean
snapd_client_set_snap_conf_sync (SnapdClient *self,
                                 const gchar *name,
                                 GHashTable *key_values,
                                 GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);
    g_return_val_if_fail (key_values != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_set_snap_conf_async (self, name, key_values, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_set_snap_conf_finish (self, data.result, error);
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
 * Deprecated: 1.45: Use snapd_client_get_apps2_sync()
 */
GPtrArray *
snapd_client_get_apps_sync (SnapdClient *self,
                            SnapdGetAppsFlags flags,
                            GCancellable *cancellable, GError **error)
{
    return snapd_client_get_apps2_sync (self, flags, NULL, cancellable, error);
}

/**
 * snapd_client_get_apps2_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetAppsFlags to control what results are returned.
 * @snaps: (allow-none): A list of snap names to return results for. If %NULL or empty then apps for all installed snaps are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information on installed apps.
 *
 * Returns: (transfer container) (element-type SnapdApp): an array of #SnapdApp or %NULL on error.
 *
 * Since: 1.45
 */
GPtrArray *
snapd_client_get_apps2_sync (SnapdClient *self,
                             SnapdGetAppsFlags flags,
                             GStrv snaps,
                             GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_apps2_async (self, flags, snaps, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_apps2_finish (self, data.result, error);
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
snapd_client_get_icon_sync (SnapdClient *self,
                            const gchar *name,
                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_icon_async (self, name, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_icon_finish (self, data.result, error);
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
snapd_client_list_sync (SnapdClient *self,
                        GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_list_async (self, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_list_finish (self, data.result, error);
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
 * Get information on installed snaps (snaps with status %SNAPD_SNAP_STATUS_ACTIVE).
 * If @flags contains %SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE then also return snaps
 * with status %SNAPD_SNAP_STATUS_INSTALLED.
 *
 * If @flags contains %SNAPD_GET_SNAPS_FLAGS_REFRESH_INHIBITED, then it will return
 * only those snaps that are inhibited from being refreshed, for example due to having a
 * running instace.
 *
 * If @names is not %NULL and contains at least one name only snaps that match these names are
 * returned. If a snap is not installed it is not returned (no error is generated).
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.42
 */
GPtrArray *
snapd_client_get_snaps_sync (SnapdClient *self,
                             SnapdGetSnapsFlags flags, GStrv names,
                             GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_snaps_async (self, flags, names, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_snaps_finish (self, data.result, error);
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
GStrv
snapd_client_get_assertions_sync (SnapdClient *self,
                                  const gchar *type,
                                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_assertions_async (self, type, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_assertions_finish (self, data.result, error);
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
snapd_client_add_assertions_sync (SnapdClient *self,
                                  GStrv assertions,
                                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (assertions != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_add_assertions_async (self, assertions, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_add_assertions_finish (self, data.result, error);
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
 * Deprecated: 1.48: Use snapd_client_get_connections_sync()
 */
gboolean
snapd_client_get_interfaces_sync (SnapdClient *self,
                                  GPtrArray **plugs, GPtrArray **slots,
                                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_interfaces_async (self, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_get_interfaces_finish (self, data.result, plugs, slots, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_get_interfaces2_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetInterfacesFlags to control what information is returned about the interfaces.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of interface names or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get information about the available snap interfaces.
 *
 * Returns: (transfer container) (element-type SnapdInterface): the available interfaces.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_client_get_interfaces2_sync (SnapdClient *self,
                                   SnapdGetInterfacesFlags flags,
                                   GStrv names,
                                   GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_interfaces2_async (self, flags, names, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_interfaces2_finish (self, data.result, error);
}

/**
 * snapd_client_get_connections_sync:
 * @client: a #SnapdClient.
 * @established: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of connections or %NULL.
 * @undesired: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of auto-connected connections that have been manually disconnected or %NULL.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the installed snap connections.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.48
 * Deprecated: 1.49: Use snapd_client_get_connections2_sync()
 */
gboolean
snapd_client_get_connections_sync (SnapdClient *self,
                                   GPtrArray **established, GPtrArray **undesired,
                                   GPtrArray **plugs, GPtrArray **slots,
                                   GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_connections_async (self, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_get_connections_finish (self, data.result, established, undesired, plugs, slots, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_get_connections2_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetConnectionsFlags to control what results are returned.
 * @snap: (allow-none): the name of the snap to get connections for or %NULL for all snaps.
 * @interface: (allow-none): the name of the interface to get connections for or %NULL for all interfaces.
 * @established: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of connections or %NULL.
 * @undesired: (out) (allow-none) (transfer container) (element-type SnapdConnection): the location to store the array of auto-connected connections that have been manually disconnected or %NULL.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Get the installed snap connections.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.49
 */
gboolean
snapd_client_get_connections2_sync (SnapdClient *self,
                                    SnapdGetConnectionsFlags flags, const gchar *snap, const gchar *interface,
                                    GPtrArray **established, GPtrArray **undesired,
                                    GPtrArray **plugs, GPtrArray **slots,
                                    GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_connections2_async (self, flags, snap, interface, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_connections2_finish (self, data.result, established, undesired, plugs, slots, error);
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
snapd_client_connect_interface_sync (SnapdClient *self,
                                     const gchar *plug_snap, const gchar *plug_name,
                                     const gchar *slot_snap, const gchar *slot_name,
                                     SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                     GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_connect_interface_async (self, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_connect_interface_finish (self, data.result, error);
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
snapd_client_disconnect_interface_sync (SnapdClient *self,
                                        const gchar *plug_snap, const gchar *plug_name,
                                        const gchar *slot_snap, const gchar *slot_name,
                                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                        GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_disconnect_interface_async (self, plug_snap, plug_name, slot_snap, slot_name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_disconnect_interface_finish (self, data.result, error);
}

/**
 * snapd_client_find_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: (allow-none): query string to send or %NULL to return featured snaps.
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
snapd_client_find_sync (SnapdClient *self,
                        SnapdFindFlags flags, const gchar *query,
                        gchar **suggested_currency,
                        GCancellable *cancellable, GError **error)
{
    return snapd_client_find_category_sync (self, flags, NULL, query, suggested_currency, cancellable, error);
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
 * Deprecated: 1.64: Use snapd_client_find_category_sync()
 */
GPtrArray *
snapd_client_find_section_sync (SnapdClient *self,
                                SnapdFindFlags flags, const gchar *section, const gchar *query,
                                gchar **suggested_currency,
                                GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_find_section_async (self, flags, section, query, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_find_section_finish (self, data.result, suggested_currency, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_find_category_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @category: (allow-none): store category to search in or %NULL to search in all categories.
 * @query: (allow-none): query string to send or %NULL to get all snaps from the given category.
 * @suggested_currency: (out) (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Find snaps in the store.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_find_category_sync (SnapdClient *self,
                                 SnapdFindFlags flags, const gchar *category, const gchar *query,
                                 gchar **suggested_currency,
                                 GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_find_category_async (self, flags, category, query, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_find_category_finish (self, data.result, suggested_currency, error);
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
snapd_client_find_refreshable_sync (SnapdClient *self,
                                    GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_find_refreshable_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_find_refreshable_finish (self, data.result, error);
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
snapd_client_install_sync (SnapdClient *self,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    return snapd_client_install2_sync (self, SNAPD_INSTALL_FLAGS_NONE, name, channel, NULL, progress_callback, progress_callback_data, cancellable, error);
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
snapd_client_install2_sync (SnapdClient *self,
                            SnapdInstallFlags flags,
                            const gchar *name, const gchar *channel, const gchar *revision,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_install2_async (self, flags, name, channel, revision, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_install2_finish (self, data.result, error);
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
 * snapd_client_install_stream_sync (self, stream, progress_cb, NULL, cancellable, &error);
 * \]
 *
 * Or if you have the file in memory you can use:
 *
 * |[
 * g_autoptr(GInputStream) stream = g_memory_input_stream_new_from_data (data, data_length, free_data);
 * snapd_client_install_stream_sync (self, stream, progress_cb, NULL, cancellable, &error);
 * \]
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_install_stream_sync (SnapdClient *self,
                                  SnapdInstallFlags flags,
                                  GInputStream *stream,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_install_stream_async (self, flags, stream, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_install_stream_finish (self, data.result, error);
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
snapd_client_try_sync (SnapdClient *self,
                       const gchar *path,
                       SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                       GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (path != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_try_async (self, path, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_try_finish (self, data.result, error);
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
snapd_client_refresh_sync (SnapdClient *self,
                           const gchar *name, const gchar *channel,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_refresh_async (self, name, channel, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_refresh_finish (self, data.result, error);
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
GStrv
snapd_client_refresh_all_sync (SnapdClient *self,
                               SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_refresh_all_async (self, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_refresh_all_finish (self, data.result, error);
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
 * Deprecated: 1.50: Use snapd_client_remove2_sync()
 */
gboolean
snapd_client_remove_sync (SnapdClient *self,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    return snapd_client_remove2_sync (self, SNAPD_REMOVE_FLAGS_NONE, name, progress_callback, progress_callback_data, cancellable, error);
}

/**
 * snapd_client_remove2_sync:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdRemoveFlags to control remove options.
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
 * Since: 1.50
 */
gboolean
snapd_client_remove2_sync (SnapdClient *self,
                           SnapdRemoveFlags flags,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_remove2_async (self, flags, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_remove2_finish (self, data.result, error);
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
snapd_client_enable_sync (SnapdClient *self,
                          const gchar *name,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_enable_async (self, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_enable_finish (self, data.result, error);
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
snapd_client_disable_sync (SnapdClient *self,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_disable_async (self, name, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_disable_finish (self, data.result, error);
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
snapd_client_switch_sync (SnapdClient *self,
                          const gchar *name, const gchar *channel,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (name != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_switch_async (self, name, channel, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_switch_finish (self, data.result, error);
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
snapd_client_check_buy_sync (SnapdClient *self,
                             GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_check_buy_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_check_buy_finish (self, data.result, error);
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
snapd_client_buy_sync (SnapdClient *self,
                       const gchar *id, gdouble amount, const gchar *currency,
                       GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (id != NULL, FALSE);
    g_return_val_if_fail (currency != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_buy_async (self, id, amount, currency, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_buy_finish (self, data.result, error);
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
snapd_client_create_user_sync (SnapdClient *self,
                               const gchar *email, SnapdCreateUserFlags flags,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (email != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_create_user_async (self, email, flags, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_create_user_finish (self, data.result, error);
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
snapd_client_create_users_sync (SnapdClient *self,
                                GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_create_users_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_create_users_finish (self, data.result, error);
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
snapd_client_get_users_sync (SnapdClient *self,
                             GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_users_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_users_finish (self, data.result, error);
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
 * Deprecated: 1.64: Use snapd_client_get_categories_sync()
 */
GStrv
snapd_client_get_sections_sync (SnapdClient *self,
                                GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_get_sections_async (self, cancellable, sync_cb, &data);
G_GNUC_END_IGNORE_DEPRECATIONS
    end_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    return snapd_client_get_sections_finish (self, data.result, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_get_categories_sync:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get the store categories.
 *
 * Returns: (transfer container) (element-type SnapdCategoryDetails): an array of #SnapdCategoryDetails or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_get_categories_sync (SnapdClient *self,
                                  GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_categories_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_categories_finish (self, data.result, error);
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
snapd_client_get_aliases_sync (SnapdClient *self,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_aliases_async (self, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_aliases_finish (self, data.result, error);
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
snapd_client_alias_sync (SnapdClient *self,
                         const gchar *snap, const gchar *app, const gchar *alias,
                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                         GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (snap != NULL, FALSE);
    g_return_val_if_fail (app != NULL, FALSE);
    g_return_val_if_fail (alias != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_alias_async (self, snap, app, alias, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_alias_finish (self, data.result, error);
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
snapd_client_unalias_sync (SnapdClient *self,
                           const gchar *snap, const gchar *alias,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (alias != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_unalias_async (self, snap, alias, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_unalias_finish (self, data.result, error);
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
snapd_client_prefer_sync (SnapdClient *self,
                          const gchar *snap,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (snap != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_prefer_async (self, snap, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_prefer_finish (self, data.result, error);
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
snapd_client_enable_aliases_sync (SnapdClient *self,
                                  const gchar *snap, GStrv aliases,
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
snapd_client_disable_aliases_sync (SnapdClient *self,
                                   const gchar *snap, GStrv aliases,
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
snapd_client_reset_aliases_sync (SnapdClient *self,
                                 const gchar *snap, GStrv aliases,
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
 * Deprecated: 1.59: Use snapd_client_run_snapctl2_async()
 */
gboolean
snapd_client_run_snapctl_sync (SnapdClient *self,
                               const gchar *context_id, GStrv args,
                               gchar **stdout_output, gchar **stderr_output,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (context_id != NULL, FALSE);
    g_return_val_if_fail (args != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    snapd_client_run_snapctl_async (self, context_id, args, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_run_snapctl_finish (self, data.result, stdout_output, stderr_output, error);
G_GNUC_END_IGNORE_DEPRECATIONS
}

/**
 * snapd_client_run_snapctl2_sync:
 * @client: a #SnapdClient.
 * @context_id: context for this call.
 * @args: the arguments to pass to snapctl.
 * @stdout_output: (out) (allow-none): the location to write the stdout from the command or %NULL.
 * @stderr_output: (out) (allow-none): the location to write the stderr from the command or %NULL.
 * @exit_code: (out) (allow-none): the location to write the exit code of the command or %NULL.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Run a snapctl command.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.59
 */
gboolean
snapd_client_run_snapctl2_sync (SnapdClient *self,
                                const gchar *context_id, GStrv args,
                                gchar **stdout_output, gchar **stderr_output,
                                int *exit_code,
                                GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);
    g_return_val_if_fail (context_id != NULL, FALSE);
    g_return_val_if_fail (args != NULL, FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_run_snapctl2_async (self, context_id, args, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_run_snapctl2_finish (self, data.result, stdout_output, stderr_output, exit_code, error);
}

/**
 * snapd_client_download_sync:
 * @client: a #SnapdClient.
 * @name: name of snap to download.
 * @channel: (allow-none): channel to download from.
 * @revision: (allow-none): revision to download.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Download the given snap.
 *
 * Returns: the snap contents or %NULL on error.
 *
 * Since: 1.54
 */
GBytes *
snapd_client_download_sync (SnapdClient *self,
                            const gchar *name, const gchar *channel, const gchar *revision,
                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_download_async (self, name, channel, revision, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_download_finish (self, data.result, error);
}

/**
 * snapd_client_check_themes_sync:
 * @client: a #SnapdClient.
 * @gtk_theme_names: (allow-none): a list of GTK theme names.
 * @icon_theme_names: (allow-none): a list of icon theme names.
 * @sound_theme_names: (allow-none): a list of sound theme names.
 * @gtk_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of GTK themes.
 * @icon_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of icon themes.
 * @sound_theme_status: (out) (transfer container) (element-type utf8 SnapdThemeStatus): status of sound themes.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Check the status of snap packaged versions of named desktop
 * themes. For each theme, it will determine whether it is already
 * installed, uninstalled but available on the store, or unavailable.
 *
 * Since: 1.60
 */
gboolean
snapd_client_check_themes_sync (SnapdClient *self, GStrv gtk_theme_names, GStrv icon_theme_names, GStrv sound_theme_names, GHashTable **gtk_theme_status, GHashTable **icon_theme_status, GHashTable **sound_theme_status, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_check_themes_async (self, gtk_theme_names, icon_theme_names, sound_theme_names, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_check_themes_finish (self, data.result, gtk_theme_status, icon_theme_status, sound_theme_status, error);
}

/**
 * snapd_client_install_themes_sync:
 * @client: a #SnapdClient.
 * @gtk_theme_names: (allow-none): a list of GTK theme names.
 * @icon_theme_names: (allow-none): a list of icon theme names.
 * @sound_theme_names: (allow-none): a list of sound theme names.
 * @progress_callback: (allow-none) (scope call): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Install snaps that provide the named desktop themes. If all the
 * named themes are in the "installed" or "unavailable" states, then
 * an error will be returned.
 *
 * Since: 1.60
 */
gboolean
snapd_client_install_themes_sync (SnapdClient *self, GStrv gtk_theme_names, GStrv icon_theme_names, GStrv sound_theme_names, SnapdProgressCallback progress_callback, gpointer progress_callback_data, GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_install_themes_async (self, gtk_theme_names, icon_theme_names, sound_theme_names, progress_callback, progress_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_install_themes_finish (self, data.result, error);
}

/**
 * snapd_client_get_logs_sync:
 * @client: a #SnapdClient.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of service names or %NULL.
 * @n: the number of logs to return or 0 for default.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Get logs for snap services.
 *
 * Returns: (transfer container) (element-type SnapdLog): an array of #SnapdLog or %NULL on error.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_client_get_logs_sync (SnapdClient *self,
                            GStrv names,
                            size_t n,
                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_logs_async (self, names, n, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_logs_finish (self, data.result, error);
}

/**
 * snapd_client_follow_logs_sync:
 * @client: a #SnapdClient.
 * @names: (allow-none) (array zero-terminated=1): a null-terminated array of service names or %NULL.
 * @log_callback: (scope async): a #SnapdLogCallback to call when a log is received.
 * @log_callback_data: (closure): the data to pass to @log_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL
 *     to ignore.
 *
 * Follow logs for snap services. This call will only complete if snapd closes the connection and will
 * stop any other request on this client from being sent.
 *
 * Returns: %TRUE on success.
 *
 * Since: 1.64
 */
gboolean
snapd_client_follow_logs_sync (SnapdClient *self,
                               GStrv names,
                               SnapdLogCallback log_callback, gpointer log_callback_data,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), FALSE);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_follow_logs_async (self, names, log_callback, log_callback_data, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_follow_logs_finish (self, data.result, error);
}

/**
 * snapd_client_get_notices_sync:
 * @client: a #SnapdClient.
 * @since_date_time: send only the notices generated after this moment (NULL for all).
 * @timeout: time, in microseconds, to wait for a new notice (zero to return immediately).
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Synchronously get notifications that have occurred / are occurring on the snap daemon.
 *
 * Returns: (transfer full) (element-type GPtrArray): a list of #SnapdNotice or %NULL on error.
 *
 * Since: 1.65
 */
GPtrArray *
snapd_client_get_notices_sync (SnapdClient *self,
                               GDateTime *since_date_time,
                               GTimeSpan timeout,
                               GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_notices_async (self, since_date_time, timeout, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_notices_finish (self, data.result, error);
}

/**
 * snapd_client_get_notices_with_filters_sync:
 * @client: a #SnapdClient.
 * @user_id: filter by this user-id (NULL for no filter).
 * @users: filter by this comma-separated list of users (NULL for no filter).
 * @types: filter by this comma-separated list of types (NULL for no filter).
 * @keys: filter by this comma-separated list of keys (NULL for no filter).
 * @since_date_time: send only the notices generated after this moment (NULL for all).
 * @timeout: time, in microseconds, to wait for a new notice (zero to return immediately).
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Synchronously get notifications that have occurred / are occurring on the snap daemon.
 *
 * Returns: (transfer full) (element-type GPtrArray): a list of #SnapdNotice or %NULL on error.
 *
 * Since: 1.65
 */
GPtrArray *
snapd_client_get_notices_with_filters_sync (SnapdClient *self,
                                            gchar *user_id,
                                            gchar *users,
                                            gchar *types,
                                            gchar *keys,
                                            GDateTime *since_date_time,
                                            GTimeSpan timeout,
                                            GCancellable *cancellable, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (self), NULL);

    g_auto(SyncData) data = { 0 };
    start_sync (&data);
    snapd_client_get_notices_with_filters_async (self, user_id, users, types, keys,
                                                 since_date_time, timeout, cancellable, sync_cb, &data);
    end_sync (&data);
    return snapd_client_get_notices_finish (self, data.result, error);
}