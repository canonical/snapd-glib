#ifndef __SNAPD_SNAP_LIST_H__
#define __SNAPD_SNAP_LIST_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include <snapd-glib/snapd-snap.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SNAP_LIST  (snapd_snap_list_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSnapList, snapd_snap_list, SNAPD, SNAP_LIST, GObject)

struct _SnapdSnapListClass
{
    /*< private >*/
    GObjectClass parent_class;

    /* padding, for future expansion */
    void (* _snapd_reserved1) (void);
    void (* _snapd_reserved2) (void);
    void (* _snapd_reserved3) (void);
    void (* _snapd_reserved4) (void);
};

// FIXME: Make private
void             _snapd_snap_list_add    (SnapdSnapList *snap_list,
                                          SnapdSnap     *snap);

gsize             snapd_snap_list_length (SnapdSnapList *snap_list);

SnapdSnap        *snapd_snap_list_index  (SnapdSnapList *snap_list,
                                          gsize          index);

G_END_DECLS

#endif /* __SNAPD_SNAP_LIST_H__ */
