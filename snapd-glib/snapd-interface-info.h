/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_INTERFACE_INFO_H__
#define __SNAPD_INTERFACE_INFO_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-plug.h>
#include <snapd-glib/snapd-slot.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_INTERFACE_INFO  (snapd_interface_info_get_type ())

G_DECLARE_FINAL_TYPE (SnapdInterfaceInfo, snapd_interface_info, SNAPD, INTERFACE_INFO, GObject)

const gchar *snapd_interface_info_get_name    (SnapdInterfaceInfo *interface_info);

const gchar *snapd_interface_info_get_summary (SnapdInterfaceInfo *interface_info);

const gchar *snapd_interface_info_get_doc_url (SnapdInterfaceInfo *interface_info);

GPtrArray   *snapd_interface_info_get_plugs   (SnapdInterfaceInfo *interface_info);

GPtrArray   *snapd_interface_info_get_slots   (SnapdInterfaceInfo *interface_info);

G_END_DECLS

#endif /* __SNAPD_INTERFACE_INFO_H__ */
