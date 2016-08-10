#ifndef __SNAPD_APP_H__
#define __SNAPD_APP_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-connection.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_APP  (snapd_app_get_type ())

G_DECLARE_FINAL_TYPE (SnapdApp, snapd_app, SNAPD, APP, GObject)

struct _SnapdAppClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_app_get_name (SnapdApp *app);

G_END_DECLS

#endif /* __SNAPD_APP_H__ */
