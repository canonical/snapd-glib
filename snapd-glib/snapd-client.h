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
#include <snapd-glib/snapd-payment-method.h>
#include <snapd-glib/snapd-snap.h>
#include <snapd-glib/snapd-system-information.h>
#include <snapd-glib/snapd-task.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CLIENT (snapd_client_get_type ())

G_DECLARE_DERIVABLE_TYPE (SnapdClient, snapd_client, SNAPD, CLIENT, GObject)

struct _SnapdClientClass
{
    /*< private >*/
    GObjectClass parent_class;

    /* padding, for future expansion */
    void (* _snapd_reserved1) (void);
    void (* _snapd_reserved2) (void);
    void (* _snapd_reserved3) (void);
    void (* _snapd_reserved4) (void);
};

typedef enum
{  
    SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
    SNAPD_CLIENT_ERROR_WRITE_ERROR,
    SNAPD_CLIENT_ERROR_READ_ERROR,
    SNAPD_CLIENT_ERROR_PARSE_ERROR,
    SNAPD_CLIENT_ERROR_GENERAL_ERROR,
    SNAPD_CLIENT_ERROR_LOGIN_REQUIRED,
    SNAPD_CLIENT_ERROR_INVALID_AUTH_DATA,  
    SNAPD_CLIENT_ERROR_TWO_FACTOR_REQUIRED,
    SNAPD_CLIENT_ERROR_TWO_FACTOR_FAILED,
    SNAPD_CLIENT_ERROR_BAD_REQUEST,
    SNAPD_CLIENT_ERROR_LAST
} SnapdClientError;

#define SNAPD_CLIENT_ERROR snapd_client_error_quark ()

typedef enum
{
    SNAPD_FIND_FLAGS_NONE            = 0,
    SNAPD_FIND_FLAGS_MATCH_NAME      = 1 << 0,
    SNAPD_FIND_FLAGS_SELECT_PRIVATE  = 1 << 1,
    SNAPD_FIND_FLAGS_SELECT_REFRESH  = 1 << 2
} SnapdFindFlags;

/**
 * SnapdProgressCallback:
 * @client: a #SnapdClient
 * @main_task: a #SnapdTask describing the overall task in progress
 * @tasks: (element-type SnapdTask): tasks to be done / being done.
 * @user_data: user data passed to the callback
 */
typedef void (*SnapdProgressCallback) (SnapdClient *client, SnapdTask *main_task, GPtrArray *tasks, gpointer user_data);


GQuark                  snapd_client_error_quark                   (void) G_GNUC_CONST;

SnapdClient            *snapd_client_new                           (void);

gboolean                snapd_client_connect_sync                  (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);

gboolean                snapd_client_login_sync                    (SnapdClient          *client,
                                                                    const gchar          *username,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_login_async                   (SnapdClient          *client,
                                                                    const gchar          *username,
                                                                    const gchar          *password,
                                                                    const gchar          *otp,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_login_finish                  (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

void                    snapd_client_set_auth_data                 (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data);
SnapdAuthData          *snapd_client_get_auth_data                 (SnapdClient          *client);

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
                                                                    GError              **error);
void                    snapd_client_list_async                    (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_list_finish                   (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdSnap              *snapd_client_list_one_sync                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_list_one_async                (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdSnap              *snapd_client_list_one_finish               (SnapdClient          *client,
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
                                                                    GError              **error);

gboolean                snapd_client_install_sync                  (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_install_async                 (SnapdClient          *client,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_install_finish                (SnapdClient          *client,
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

GPtrArray              *snapd_client_get_payment_methods_sync      (SnapdClient          *client,
                                                                    gboolean             *allows_automatic_payment,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_payment_methods_async     (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_get_payment_methods_finish    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    gboolean             *allows_automatic_payment,
                                                                    GError              **error);

gboolean                snapd_client_buy_sync                      (SnapdClient          *client,
                                                                    SnapdSnap            *snap,
                                                                    SnapdPrice           *price,
                                                                    SnapdPaymentMethod   *payment_method,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_buy_async                     (SnapdClient          *client,
                                                                    SnapdSnap            *snap,
                                                                    SnapdPrice           *price,                                                                    
                                                                    SnapdPaymentMethod   *payment_method,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_buy_finish                    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

G_END_DECLS

#endif /* __SNAPD_CLIENT_H__ */
