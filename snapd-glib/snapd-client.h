#ifndef __SNAPD_CLIENT_H__
#define __SNAPD_CLIENT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

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
    SNAPD_CLIENT_ERROR_LAST
} SnapdClientError;

#define SNAPD_CLIENT_ERROR snapd_client_error_quark ()

GQuark snapd_client_error_quark    (void) G_GNUC_CONST;

SnapdClient *snapd_client_new      (void);

gboolean snapd_client_connect_sync (SnapdClient          *client,
                                    GCancellable         *cancellable,
                                    GError              **error);

G_END_DECLS

#endif /* __SNAPD_CLIENT_H__ */
