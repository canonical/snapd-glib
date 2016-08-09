#ifndef __SNAPD_PLUG_H__
#define __SNAPD_PLUG_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-connection.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PLUG  (snapd_plug_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPlug, snapd_plug, SNAPD, PLUG, GObject)

struct _SnapdPlugClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_plug_get_name        (SnapdPlug *plug);

const gchar *snapd_plug_get_snap        (SnapdPlug *plug);

const gchar *snapd_plug_get_interface   (SnapdPlug *plug);

const gchar *snapd_plug_get_label       (SnapdPlug *plug);

GPtrArray   *snapd_plug_get_connections (SnapdPlug *plug);

G_END_DECLS

#endif /* __SNAPD_PLUG_H__ */
