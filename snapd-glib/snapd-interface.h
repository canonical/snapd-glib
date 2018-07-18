/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_INTERFACE_H__
#define __SNAPD_INTERFACE_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_INTERFACE  (snapd_interface_get_type ())

G_DECLARE_FINAL_TYPE (SnapdInterface, snapd_interface, SNAPD, INTERFACE, GObject)

const gchar *snapd_interface_get_name    (SnapdInterface *interface);

const gchar *snapd_interface_get_summary (SnapdInterface *interface);

const gchar *snapd_interface_get_doc_url (SnapdInterface *interface);

GPtrArray   *snapd_interface_get_plugs   (SnapdInterface *interface);

GPtrArray   *snapd_interface_get_slots   (SnapdInterface *interface);

G_END_DECLS

#endif /* __SNAPD_INTERFACE_H__ */
