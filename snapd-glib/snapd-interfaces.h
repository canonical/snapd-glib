#ifndef __SNAPD_INTERFACES_H__
#define __SNAPD_INTERFACES_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include <snapd-glib/snapd-plug.h>
#include <snapd-glib/snapd-slot.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_INTERFACES  (snapd_interfaces_get_type ())

G_DECLARE_FINAL_TYPE (SnapdInterfaces, snapd_interfaces, SNAPD, INTERFACES, GObject)

struct _SnapdInterfacesClass
{
    /*< private >*/
    GObjectClass parent_class;
};

// FIXME: Make private
void       _snapd_interfaces_add_plug (SnapdInterfaces *interfaces,
                                       SnapdPlug       *plug);
void       _snapd_interfaces_add_slot (SnapdInterfaces *interfaces,
                                       SnapdSlot       *slot);

GPtrArray *snapd_interfaces_get_plugs (SnapdInterfaces *interfaces);

GPtrArray *snapd_interfaces_get_slots (SnapdInterfaces *interfaces);

G_END_DECLS

#endif /* __SNAPD_INTERFACES_H__ */
