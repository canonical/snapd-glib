/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_PLUG_REF_H__
#define __SNAPD_PLUG_REF_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PLUG_REF  (snapd_plug_ref_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPlugRef, snapd_plug_ref, SNAPD, PLUG_REF, GObject)

const gchar *snapd_plug_ref_get_plug (SnapdPlugRef *plug_ref);

const gchar *snapd_plug_ref_get_snap (SnapdPlugRef *plug_ref);

G_END_DECLS

#endif /* __SNAPD_PLUG_REF_H__ */
