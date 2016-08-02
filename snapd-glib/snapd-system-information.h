#ifndef __SNAPD_SYSTEM_INFORMATION_H__
#define __SNAPD_SYSTEM_INFORMATION_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SYSTEM_INFORMATION  (snapd_system_information_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSystemInformation, snapd_system_information, SNAPD, SYSTEM_INFORMATION, GObject)

struct _SnapdSystemInformationClass
{
    /*< private >*/
    GObjectClass parent_class;

    /* padding, for future expansion */
    void (* _snapd_reserved1) (void);
    void (* _snapd_reserved2) (void);
    void (* _snapd_reserved3) (void);
    void (* _snapd_reserved4) (void);
};

gboolean     snapd_system_information_get_on_classic (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_os_id      (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_os_version (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_series     (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_version    (SnapdSystemInformation *system_information);

G_END_DECLS

#endif /* __SNAPD_SYSTEM_INFORMATION_H__ */
