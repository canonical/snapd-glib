#ifndef __SNAPD_SNAP_H__
#define __SNAPD_SNAP_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-price.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SNAP  (snapd_snap_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSnap, snapd_snap, SNAPD, SNAP, GObject)

struct _SnapdSnapClass
{
    /*< private >*/
    GObjectClass parent_class;
};

typedef enum
{
    SNAPD_CONFINEMENT_UNKNOWN,
    SNAPD_CONFINEMENT_STRICT,
    SNAPD_CONFINEMENT_DEVMODE
} SnapdConfinement;

typedef enum
{
    SNAPD_SNAP_TYPE_UNKNOWN,
    SNAPD_SNAP_TYPE_APP,
    SNAPD_SNAP_TYPE_KERNEL,
    SNAPD_SNAP_TYPE_GADGET,
    SNAPD_SNAP_TYPE_OS
} SnapdSnapType;

typedef enum
{
    SNAPD_SNAP_STATUS_UNKNOWN,
    SNAPD_SNAP_STATUS_AVAILABLE,
    SNAPD_SNAP_STATUS_PRICED,
    SNAPD_SNAP_STATUS_INSTALLED,
    SNAPD_SNAP_STATUS_ACTIVE
} SnapdSnapStatus;

GPtrArray        *snapd_snap_get_apps           (SnapdSnap *snap);

const gchar      *snapd_snap_get_channel        (SnapdSnap *snap);

SnapdConfinement  snapd_snap_get_confinement    (SnapdSnap *snap);

const gchar      *snapd_snap_get_description    (SnapdSnap *snap);

const gchar      *snapd_snap_get_developer      (SnapdSnap *snap);

gboolean          snapd_snap_get_devmode        (SnapdSnap *snap);

gint64            snapd_snap_get_download_size  (SnapdSnap *snap);

const gchar      *snapd_snap_get_icon           (SnapdSnap *snap);

const gchar      *snapd_snap_get_id             (SnapdSnap *snap);

const gchar      *snapd_snap_get_install_date   (SnapdSnap *snap); // FIXME: GDateTime?

gint64            snapd_snap_get_installed_size (SnapdSnap *snap);

const gchar      *snapd_snap_get_name           (SnapdSnap *snap);

GPtrArray        *snapd_snap_get_prices         (SnapdSnap *snap);

gboolean          snapd_snap_get_private        (SnapdSnap *snap);

const gchar      *snapd_snap_get_revision       (SnapdSnap *snap);

SnapdSnapType     snapd_snap_get_snap_type      (SnapdSnap *snap);

SnapdSnapStatus   snapd_snap_get_status         (SnapdSnap *snap);

const gchar      *snapd_snap_get_summary        (SnapdSnap *snap);

gboolean          snapd_snap_get_trymode        (SnapdSnap *snap);

const gchar      *snapd_snap_get_version        (SnapdSnap *snap);

G_END_DECLS

#endif /* __SNAPD_SNAP_H__ */
