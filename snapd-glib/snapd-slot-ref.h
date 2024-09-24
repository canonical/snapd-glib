/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_SLOT_REF_H__
#define __SNAPD_SLOT_REF_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SLOT_REF (snapd_slot_ref_get_type())

G_DECLARE_FINAL_TYPE(SnapdSlotRef, snapd_slot_ref, SNAPD, SLOT_REF, GObject)

const gchar *snapd_slot_ref_get_slot(SnapdSlotRef *slot_ref);

const gchar *snapd_slot_ref_get_snap(SnapdSlotRef *slot_ref);

G_END_DECLS

#endif /* __SNAPD_SLOT_REF_H__ */
