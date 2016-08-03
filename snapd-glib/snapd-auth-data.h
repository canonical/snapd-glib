#ifndef __SNAPD_AUTH_DATA_H__
#define __SNAPD_AUTH_DATA_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_AUTH_DATA  (snapd_auth_data_get_type ())

G_DECLARE_FINAL_TYPE (SnapdAuthData, snapd_auth_data, SNAPD, AUTH_DATA, GObject)

struct _SnapdAuthDataClass
{
    /*< private >*/
    GObjectClass parent_class;
};

SnapdAuthData *snapd_auth_data_new                (void);

void          snapd_auth_data_set_macaroon        (SnapdAuthData *auth_data,
                                                   const gchar   *macaroon);

const gchar  *snapd_auth_data_get_macaroon        (SnapdAuthData *auth_data);

void          snapd_auth_data_add_discharge       (SnapdAuthData *auth_data,
                                                   const gchar   *discharge);

gsize         snapd_auth_data_get_discharge_count (SnapdAuthData *auth_data);

const gchar  *snapd_auth_data_get_discharge       (SnapdAuthData *auth_data,
                                                   int            index);

G_END_DECLS

#endif /* __SNAPD_AUTH_DATA_H__ */
