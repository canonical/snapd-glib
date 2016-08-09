#ifndef __SNAPD_SLOT_H__
#define __SNAPD_SLOT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-connection.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SLOT  (snapd_slot_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSlot, snapd_slot, SNAPD, SLOT, GObject)

struct _SnapdSlotClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_slot_get_name        (SnapdSlot *slot);

const gchar *snapd_slot_get_snap        (SnapdSlot *slot);

const gchar *snapd_slot_get_interface   (SnapdSlot *slot);

const gchar *snapd_slot_get_label       (SnapdSlot *slot);

GPtrArray   *snapd_slot_get_connections (SnapdSlot *slot);

G_END_DECLS

#endif /* __SNAPD_SLOT_H__ */
