#ifndef __SNAPD_ICON_H__
#define __SNAPD_ICON_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_ICON (snapd_icon_get_type ())

G_DECLARE_FINAL_TYPE (SnapdIcon, snapd_icon, SNAPD, ICON, GObject)

struct _SnapdIconClass
{
    /*< private >*/
    GObjectClass parent_class;
};

SnapdIcon    *snapd_icon_new             (void);

const gchar  *snapd_icon_get_mime_type   (SnapdIcon *icon);

GBytes       *snapd_icon_get_data        (SnapdIcon *icon);

G_END_DECLS

#endif /* __SNAPD_ICON_H__ */
