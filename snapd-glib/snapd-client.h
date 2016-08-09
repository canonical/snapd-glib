#ifndef __SNAPD_CLIENT_H__
#define __SNAPD_CLIENT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

#include <snapd-glib/snapd-auth-data.h>
#include <snapd-glib/snapd-payment-method-list.h>
#include <snapd-glib/snapd-icon.h>
#include <snapd-glib/snapd-interfaces.h>
#include <snapd-glib/snapd-snap.h>
#include <snapd-glib/snapd-system-information.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CLIENT             (snapd_client_get_type ())

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

typedef enum {  
    SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
    SNAPD_CLIENT_ERROR_WRITE_ERROR,
    SNAPD_CLIENT_ERROR_READ_ERROR,
    SNAPD_CLIENT_ERROR_PARSE_ERROR,
    SNAPD_CLIENT_ERROR_GENERAL_ERROR,
    SNAPD_CLIENT_ERROR_LOGIN_REQUIRED,
    SNAPD_CLIENT_ERROR_INVALID_AUTH_DATA,  
    SNAPD_CLIENT_ERROR_TWO_FACTOR_REQUIRED,
    SNAPD_CLIENT_ERROR_TWO_FACTOR_FAILED,
    SNAPD_CLIENT_ERROR_LAST
} SnapdClientError;

#define SNAPD_CLIENT_ERROR snapd_client_error_quark ()

typedef void (*SnapdProgressCallback) (gpointer user_data); // FIXME


GQuark                  snapd_client_error_quark                   (void) G_GNUC_CONST;

SnapdClient            *snapd_client_new                           (void);

gboolean                snapd_client_connect_sync                  (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
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
                                                                    GError              **error);
void                    snapd_client_list_async                    (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_list_finish                   (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

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

SnapdInterfaces        *snapd_client_get_interfaces_sync           (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_interfaces_async          (SnapdClient          *client,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdInterfaces        *snapd_client_get_interfaces_finish         (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdAuthData          *snapd_client_login_sync                    (SnapdClient          *client,
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
SnapdAuthData          *snapd_client_login_finish                  (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

GPtrArray              *snapd_client_find_sync                     (SnapdClient          *client,
                                                                    const gchar          *query,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_find_async                    (SnapdClient          *client,
                                                                    const gchar          *query,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
GPtrArray              *snapd_client_find_finish                   (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_install_sync                  (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    const gchar          *name,
                                                                    const gchar          *channel,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_install_async                 (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
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

gboolean                snapd_client_remove_sync                   (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_remove_async                  (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    const gchar          *name,
                                                                    SnapdProgressCallback progress_callback,
                                                                    gpointer              progress_callback_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
gboolean                snapd_client_remove_finish                 (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

SnapdPaymentMethodList *snapd_client_get_payment_methods_sync      (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_get_payment_methods_async     (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    GCancellable         *cancellable,
                                                                    GAsyncReadyCallback   callback,
                                                                    gpointer              user_data);
SnapdPaymentMethodList *snapd_client_get_payment_methods_finish    (SnapdClient          *client,
                                                                    GAsyncResult         *result,
                                                                    GError              **error);

gboolean                snapd_client_buy_sync                      (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
                                                                    SnapdSnap            *snap,
                                                                    SnapdPrice           *price,
                                                                    SnapdPaymentMethod   *payment_method,
                                                                    GCancellable         *cancellable,
                                                                    GError              **error);
void                    snapd_client_buy_async                     (SnapdClient          *client,
                                                                    SnapdAuthData        *auth_data,
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
