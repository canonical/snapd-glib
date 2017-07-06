/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

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
};

/**
 * SnapdSystemConfinement:
 * @SNAPD_SYSTEM_CONFINEMENT_UNKNOWN: the confinement of the system is unknown.
 * @SNAPD_SYSTEM_CONFINEMENT_STRICT: the system supports strict confinement.
 * @SNAPD_SYSTEM_CONFINEMENT_PARTIAL: the system supports partial confinement.
 *
 * Confinment used by a snap.
 *
 * Since: 1.15
 */
typedef enum
{
    SNAPD_SYSTEM_CONFINEMENT_UNKNOWN,
    SNAPD_SYSTEM_CONFINEMENT_STRICT,
    SNAPD_SYSTEM_CONFINEMENT_PARTIAL
} SnapdSystemConfinement;

const gchar *snapd_system_information_get_binaries_directory (SnapdSystemInformation *system_information);

SnapdSystemConfinement snapd_system_information_get_confinement (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_kernel_version     (SnapdSystemInformation *system_information);

gboolean     snapd_system_information_get_managed            (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_mount_directory    (SnapdSystemInformation *system_information);

gboolean     snapd_system_information_get_on_classic         (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_os_id              (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_os_version         (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_series             (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_store              (SnapdSystemInformation *system_information);

const gchar *snapd_system_information_get_version            (SnapdSystemInformation *system_information);

G_END_DECLS

#endif /* __SNAPD_SYSTEM_INFORMATION_H__ */
