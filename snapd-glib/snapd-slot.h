/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_SLOT_H__
#define __SNAPD_SLOT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SLOT  (snapd_slot_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSlot, snapd_slot, SNAPD, SLOT, GObject)

const gchar *snapd_slot_get_name            (SnapdSlot   *slot);

const gchar *snapd_slot_get_snap            (SnapdSlot   *slot);

const gchar *snapd_slot_get_interface       (SnapdSlot   *slot);

GStrv        snapd_slot_get_attribute_names (SnapdSlot   *slot,
                                             guint       *length);

gboolean     snapd_slot_has_attribute       (SnapdSlot   *slot,
                                             const gchar *name);

GVariant    *snapd_slot_get_attribute       (SnapdSlot   *slot,
                                             const gchar *name);

const gchar *snapd_slot_get_label           (SnapdSlot   *slot);

GPtrArray   *snapd_slot_get_connections     (SnapdSlot   *slot) G_DEPRECATED_FOR(snapd_slot_get_connected_plugs);

GPtrArray   *snapd_slot_get_connected_plugs (SnapdSlot   *slot);

G_END_DECLS

#endif /* __SNAPD_SLOT_H__ */
