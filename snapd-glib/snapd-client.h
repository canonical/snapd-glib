/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_CLIENT_H__
#define __SNAPD_CLIENT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include <snapd-glib/snapd-auth-data.h>
#include <snapd-glib/snapd-icon.h>
#include <snapd-glib/snapd-maintenance.h>
#include <snapd-glib/snapd-snap.h>
#include <snapd-glib/snapd-system-information.h>
#include <snapd-glib/snapd-change.h>
#include <snapd-glib/snapd-user-information.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CLIENT (snapd_client_get_type ())

G_DECLARE_DERIVABLE_TYPE (SnapdClient, snapd_client, SNAPD, CLIENT, GObject)

struct _SnapdClientClass
{
    /*< private >*/
    GObjectClass parent_class;

    /*< private >*/
    /* padding, for future expansion */
    void (* _snapd_reserved1) (void);
    void (* _snapd_reserved2) (void);
    void (* _snapd_reserved3) (void);
    void (* _snapd_reserved4) (void);
};

/**
 * SnapdChangeFilter:
 * @SNAPD_CHANGE_FILTER_ALL: Return all changes.
 * @SNAPD_CHANGE_FILTER_READY: Return only changes that are ready.
 * @SNAPD_CHANGE_FILTER_IN_PROGRESS: Return only changes that are in-progress.
 *
 * Filter to apply to changes.
 *
 * Since: 1.29
 */
typedef enum
{
    SNAPD_CHANGE_FILTER_ALL,
    SNAPD_CHANGE_FILTER_IN_PROGRESS,
    SNAPD_CHANGE_FILTER_READY
} SnapdChangeFilter;

/**
 * SnapdGetSnapsFlags:
 * @SNAPD_GET_SNAPS_FLAGS_NONE: No flags, default behaviour.
 * @SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE: Return snaps that are installed but not active.
 *
 * Flag to change which snaps are returned.
 *
 * Since: 1.42
 */
typedef enum
{
    SNAPD_GET_SNAPS_FLAGS_NONE              = 0,
    SNAPD_GET_SNAPS_FLAGS_INCLUDE_INACTIVE = 1 << 0
} SnapdGetSnapsFlags;

/**
 * SnapdGetAppsFlags:
 * @SNAPD_GET_APPS_FLAGS_NONE: No flags, default behaviour.
 * @SNAPD_GET_APPS_FLAGS_SELECT_SERVICES: Select services only.
 *
 * Flag to change which apps are returned.
 *
 * Since: 1.25
 */
typedef enum
{
    SNAPD_GET_APPS_FLAGS_NONE            = 0,
    SNAPD_GET_APPS_FLAGS_SELECT_SERVICES = 1 << 0
} SnapdGetAppsFlags;

/**
 * SnapdFindFlags:
 * @SNAPD_FIND_FLAGS_NONE: No flags, default behaviour.
 * @SNAPD_FIND_FLAGS_MATCH_NAME: Search for snaps whose name matches the given
 *     string. The match is exact unless the string ends in *.
 * @SNAPD_FIND_FLAGS_MATCH_COMMON_ID: Search for snaps whose common ID matches
 *     the given string.
 * @SNAPD_FIND_FLAGS_SELECT_PRIVATE: Search private snaps.
 * @SNAPD_FIND_FLAGS_SELECT_REFRESH: Deprecated, do not use.
 * @SNAPD_FIND_FLAGS_SCOPE_WIDE: Search for snaps from any architecture or branch.
 *
 * Flag to change how a find is performed.
 *
 * Since: 1.0
 */
typedef enum
{
    SNAPD_FIND_FLAGS_NONE            = 0,
    SNAPD_FIND_FLAGS_MATCH_NAME      = 1 << 0,
    SNAPD_FIND_FLAGS_SELECT_PRIVATE  = 1 << 1,
    SNAPD_FIND_FLAGS_SELECT_REFRESH  = 1 << 2,
    SNAPD_FIND_FLAGS_SCOPE_WIDE      = 1 << 3,
    SNAPD_FIND_FLAGS_MATCH_COMMON_ID = 1 << 4
} SnapdFindFlags;

/**
 * SnapdInstallFlags:
 * @SNAPD_INSTALL_FLAGS_NONE: No flags, default behaviour.
 * @SNAPD_INSTALL_FLAGS_CLASSIC: Put snap in classic mode and disable security confinement.
 * @SNAPD_INSTALL_FLAGS_DANGEROUS: Install the given snap file even if there are
 *    no pre-acknowledged signatures for it, meaning it was not verified and
 *    could be dangerous (implied by #SNAPD_INSTALL_FLAGS_DEVMODE).
 * @SNAPD_INSTALL_FLAGS_DEVMODE: Put snap in development mode and disable security confinement.
 * @SNAPD_INSTALL_FLAGS_JAILMODE: Put snap in enforced confinement mode.
 *
 * Flags to control install options.
 *
 * Since: 1.12
 */
typedef enum
{
    SNAPD_INSTALL_FLAGS_NONE      = 0,
    SNAPD_INSTALL_FLAGS_CLASSIC   = 1 << 0,
    SNAPD_INSTALL_FLAGS_DANGEROUS = 1 << 1,
    SNAPD_INSTALL_FLAGS_DEVMODE   = 1 << 2,
    SNAPD_INSTALL_FLAGS_JAILMODE  = 1 << 3
} SnapdInstallFlags;

/**
 * SnapdCreateUserFlags:
 * @SNAPD_CREATE_USER_FLAGS_NONE: No flags, default behaviour.
 * @SNAPD_CREATE_USER_FLAGS_SUDO: Gives sudo access to created user.
 * @SNAPD_CREATE_USER_FLAGS_KNOWN: Use the local system-user assertions to create the user.
 *
 * Flag to control when a user accounts is created.
 *
 * Since: 1.3
 */
typedef enum
{
    SNAPD_CREATE_USER_FLAGS_NONE  = 0,
    SNAPD_CREATE_USER_FLAGS_SUDO  = 1 << 0,
    SNAPD_CREATE_USER_FLAGS_KNOWN = 1 << 1
} SnapdCreateUserFlags;

/**
 * SnapdProgressCallback:
 * @client: a #SnapdClient
 * @change: a #SnapdChange describing the change in progress
 * @deprecated: A deprecated field that is no longer used.
 * @user_data: user data passed to the callback
 *
 * Signature for callback function used in
 * snapd_client_connect_interface_sync(),
 * snapd_client_disconnect_interface_async(),
 * snapd_client_install2_sync(),
 * snapd_client_refresh_sync(),
 * snapd_client_remove_sync(),
 * snapd_client_enable_sync() and
 * snapd_client_disable_sync().
 *
 * Since: 1.0
 */
typedef void (*SnapdProgressCallback) (SnapdClient *client, SnapdChange *change, gpointer deprecated, gpointer user_data);

SnapdClient            *snapd_client_new                           (void);

SnapdClient            *snapd_client_new_from_socket               (GSocket              *socket);

gboolean                snapd_client_connect_sync                  (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED;

void                    snapd_client_connect_async                 (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED;

gboolean                snapd_client_connect_finish                (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED;

void                    snapd_client_set_socket_path               (SnapdClient          *client,
                                                                    const gchar          *socket_path);

const gchar            *snapd_client_get_socket_path               (SnapdClient          *client);

void                    snapd_client_set_user_agent                (SnapdClient          *client,
                                                                    const gchar          *user_agent);

const gchar            *snapd_client_get_user_agent                (SnapdClient          *client);

void                    snapd_client_set_allow_interaction         (SnapdClient          *client,
                                                                    gboolean              allow_interaction);

gboolean                snapd_client_get_allow_interaction         (SnapdClient          *client);

SnapdMaintenance       *snapd_client_get_maintenance               (SnapdClient          *client);

SnapdAuthData          *snapd_client_login_sync                    (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_login2_sync);
void                    snapd_client_login_async                   (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED_FOR(snapd_client_login2_async);
SnapdAuthData          *snapd_client_login_finish                  (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_login2_finish);

SnapdUserInformation   *snapd_client_login2_sync                   (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_login2_async                  (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdUserInformation   *snapd_client_login2_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

void                    snapd_client_set_auth_data                 (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data);
SnapdAuthData          *snapd_client_get_auth_data                 (SnapdClient          *client);

GPtrArray              *snapd_client_get_changes_sync              (SnapdClient          *client,
                                                                    SnapdChangeFilter     filter,
                                                                    const gchar          *snap_name,
                                                                    GCancellable         *cancellable,
                                                                    GError             **error);

void                    snapd_client_get_changes_async             (SnapdClient          *client,
                                                                    SnapdChangeFilter     filter,
                                                                    const gchar          *snap_name,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);

GPtrArray              *snapd_client_get_changes_finish            (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdChange            *snapd_client_get_change_sync               (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    GCancellable         *cancellable,
                                                                    GError             **error);

void                    snapd_client_get_change_async              (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);

SnapdChange            *snapd_client_get_change_finish             (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdChange            *snapd_client_abort_change_sync             (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    GCancellable         *cancellable,
                                                                    GError             **error);

void                    snapd_client_abort_change_async            (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);

SnapdChange            *snapd_client_abort_change_finish           (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdSystemInformation *snapd_client_get_system_information_sync   (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_system_information_async  (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdSystemInformation *snapd_client_get_system_information_finish (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_list_sync                     (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_snaps_sync);
void                    snapd_client_list_async                    (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED_FOR(snapd_client_get_snaps_async);
GPtrArray              *snapd_client_list_finish                   (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_snaps_finish);

GPtrArray              *snapd_client_get_snaps_sync                (SnapdClient          *client,
                                                                    SnapdGetSnapsFlags    flags,
                                                                    GStrv                 names,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_snaps_async               (SnapdClient          *client,
                                                                    SnapdGetSnapsFlags    flags,
                                                                    GStrv                 names,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_get_snaps_finish              (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdSnap              *snapd_client_list_one_sync                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_snap_sync);
void                    snapd_client_list_one_async                (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED_FOR(snapd_client_get_snap_async);
SnapdSnap              *snapd_client_list_one_finish               (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_snap_finish);

SnapdSnap              *snapd_client_get_snap_sync                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_snap_async                (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdSnap              *snapd_client_get_snap_finish               (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_get_apps_sync                 (SnapdClient          *client,
                                                                    SnapdGetAppsFlags     flags,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_apps2_sync);
void                    snapd_client_get_apps_async                (SnapdClient          *client,
                                                                    SnapdGetAppsFlags     flags,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED_FOR(snapd_client_get_apps2_async);
GPtrArray              *snapd_client_get_apps_finish               (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_get_apps2_finish);

GPtrArray              *snapd_client_get_apps2_sync                (SnapdClient          *client,
                                                                    SnapdGetAppsFlags     flags,
                                                                    GStrv                 snaps,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_apps2_async               (SnapdClient          *client,
                                                                    SnapdGetAppsFlags     flags,
                                                                    GStrv                 snaps,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_get_apps2_finish              (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdIcon              *snapd_client_get_icon_sync                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_icon_async                (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdIcon              *snapd_client_get_icon_finish               (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GStrv                   snapd_client_get_assertions_sync           (SnapdClient          *client,
                                                                    const gchar          *type,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_assertions_async          (SnapdClient          *client,
                                                                    const gchar          *type,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GStrv                   snapd_client_get_assertions_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_add_assertions_sync           (SnapdClient          *client,
                                                                    GStrv                 assertions,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_add_assertions_async          (SnapdClient          *client,
                                                                    GStrv                 assertions,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_add_assertions_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_get_interfaces_sync           (SnapdClient          *client,
                                                                    GPtrArray           **plugs,
                                                                    GPtrArray           **slots,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_interfaces_async          (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_get_interfaces_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GPtrArray           **plugs,
                                                                    GPtrArray           **slots,
                                                                    GError              **error);

gboolean                snapd_client_get_connections_sync          (SnapdClient          *client,
                                                                    GPtrArray           **established,
                                                                    GPtrArray           **undesired,
                                                                    GPtrArray           **plugs,
                                                                    GPtrArray           **slots,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_connections_async         (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_get_connections_finish        (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GPtrArray           **established,
                                                                    GPtrArray           **undesired,
                                                                    GPtrArray           **plugs,
                                                                    GPtrArray           **slots,
                                                                    GError              **error);

gboolean                snapd_client_connect_interface_sync        (SnapdClient          *client,
                                                                    const gchar          *plug_snap,
                                                                    const gchar          *plug_name,
                                                                    const gchar          *slot_snap,
                                                                    const gchar          *slot_name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_connect_interface_async       (SnapdClient          *client,
                                                                    const gchar          *plug_snap,
                                                                    const gchar          *plug_name,
                                                                    const gchar          *slot_snap,
                                                                    const gchar          *slot_name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_connect_interface_finish      (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_disconnect_interface_sync     (SnapdClient          *client,
                                                                    const gchar          *plug_snap,
                                                                    const gchar          *plug_name,
                                                                    const gchar          *slot_snap,
                                                                    const gchar          *slot_name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_disconnect_interface_async    (SnapdClient          *client,
                                                                    const gchar          *plug_snap,
                                                                    const gchar          *plug_name,
                                                                    const gchar          *slot_snap,
                                                                    const gchar          *slot_name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean               snapd_client_disconnect_interface_finish    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_find_sync                     (SnapdClient          *client,
                                                                    SnapdFindFlags        flags,
                                                                    const gchar          *query,
                                                                    gchar               **suggested_currency,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_find_async                    (SnapdClient          *client,
                                                                    SnapdFindFlags        flags,
                                                                    const gchar          *query,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_find_finish                   (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    gchar               **suggested_currency,
                                                                    GError              **error);

GPtrArray              *snapd_client_find_section_sync             (SnapdClient          *client,
                                                                    SnapdFindFlags        flags,
                                                                    const gchar          *section,
                                                                    const gchar          *query,
                                                                    gchar               **suggested_currency,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_find_section_async            (SnapdClient          *client,
                                                                    SnapdFindFlags        flags,
                                                                    const gchar          *section,
                                                                    const gchar          *query,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_find_section_finish           (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    gchar               **suggested_currency,
                                                                    GError              **error);

GPtrArray              *snapd_client_find_refreshable_sync         (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_find_refreshable_async        (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_find_refreshable_finish       (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_install_sync                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_install2_sync);
void                    snapd_client_install_async                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED_FOR(snapd_client_install2_async);
gboolean                snapd_client_install_finish                (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED_FOR(snapd_client_install2_finish);

gboolean                snapd_client_install2_sync                 (SnapdClient          *client,
                                                                    SnapdInstallFlags     flags,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    const gchar          *revision,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_install2_async                (SnapdClient          *client,
                                                                    SnapdInstallFlags     flags,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    const gchar          *revision,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_install2_finish               (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_install_stream_sync           (SnapdClient          *client,
                                                                    SnapdInstallFlags     flags,
                                                                    GInputStream         *stream,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_install_stream_async          (SnapdClient          *client,
                                                                    SnapdInstallFlags     flags,
                                                                    GInputStream         *stream,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_install_stream_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_try_sync                      (SnapdClient          *client,
                                                                    const gchar          *path,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_try_async                     (SnapdClient          *client,
                                                                    const gchar          *path,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_try_finish                    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_refresh_sync                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_refresh_async                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_refresh_finish                (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GStrv                   snapd_client_refresh_all_sync              (SnapdClient          *client,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_refresh_all_async             (SnapdClient          *client,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GStrv                   snapd_client_refresh_all_finish            (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_remove_sync                   (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_remove_async                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_remove_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_enable_sync                   (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_enable_async                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_enable_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_disable_sync                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_disable_async                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_disable_finish                (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_switch_sync                   (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_switch_async                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_switch_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_check_buy_sync                (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_check_buy_async               (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_check_buy_finish              (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_buy_sync                      (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    gdouble               amount,
                                                                    const gchar          *currency,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_buy_async                     (SnapdClient          *client,
                                                                    const gchar          *id,
                                                                    gdouble               amount,
                                                                    const gchar          *currency,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_buy_finish                    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdUserInformation   *snapd_client_create_user_sync              (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    SnapdCreateUserFlags  flags,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_create_user_async             (SnapdClient          *client,
                                                                    const gchar          *email,
                                                                    SnapdCreateUserFlags  flags,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdUserInformation   *snapd_client_create_user_finish            (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_create_users_sync             (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_create_users_async            (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_create_users_finish           (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_get_users_sync                (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_users_async               (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_get_users_finish              (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GStrv                   snapd_client_get_sections_sync             (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_sections_async            (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GStrv                   snapd_client_get_sections_finish           (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_get_aliases_sync              (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_aliases_async             (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_get_aliases_finish            (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_alias_sync                    (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    const gchar          *app,
                                                                    const gchar          *alias,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_alias_async                   (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    const gchar          *app,
                                                                    const gchar          *alias,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_alias_finish                  (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_unalias_sync                  (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    const gchar          *alias,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_unalias_async                 (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    const gchar          *alias,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_unalias_finish                (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_prefer_sync                   (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_prefer_async                  (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_prefer_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_enable_aliases_sync           (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED;
void                    snapd_client_enable_aliases_async          (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED;
gboolean                snapd_client_enable_aliases_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED;

gboolean                snapd_client_disable_aliases_sync          (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED;
void                    snapd_client_disable_aliases_async         (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED;
gboolean                snapd_client_disable_aliases_finish        (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED;

gboolean                snapd_client_reset_aliases_sync            (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error) G_DEPRECATED;
void                    snapd_client_reset_aliases_async           (SnapdClient          *client,
                                                                    const gchar          *snap,
                                                                    GStrv                 aliases,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data) G_DEPRECATED;
gboolean                snapd_client_reset_aliases_finish          (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error) G_DEPRECATED;

gboolean                snapd_client_run_snapctl_sync              (SnapdClient          *client,
                                                                    const gchar          *context_id,
                                                                    GStrv                 args,
                                                                    GStrv                 stdout_output,
                                                                    GStrv                 stderr_output,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_run_snapctl_async             (SnapdClient          *client,
                                                                    const gchar          *context_id,
                                                                    GStrv                 args,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_run_snapctl_finish            (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    gchar               **stdout_output,
                                                                    gchar               **stderr_output,
                                                                    GError              **error);

G_END_DECLS

#endif /* __SNAPD_CLIENT_H__ */
