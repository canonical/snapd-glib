/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_PLUG_H__
#define __SNAPD_PLUG_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PLUG  (snapd_plug_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPlug, snapd_plug, SNAPD, PLUG, GObject)

const gchar *snapd_plug_get_name            (SnapdPlug   *plug);

const gchar *snapd_plug_get_snap            (SnapdPlug   *plug);

const gchar *snapd_plug_get_interface       (SnapdPlug   *plug);

GStrv        snapd_plug_get_attribute_names (SnapdPlug   *plug,
                                             guint       *length);

gboolean     snapd_plug_has_attribute       (SnapdPlug   *plug,
                                             const gchar *name);

GVariant    *snapd_plug_get_attribute       (SnapdPlug   *plug,
                                             const gchar *name);

const gchar *snapd_plug_get_label           (SnapdPlug   *plug);

GPtrArray   *snapd_plug_get_connections     (SnapdPlug   *plug) G_DEPRECATED_FOR(snapd_plug_get_connected_slots);

GPtrArray   *snapd_plug_get_connected_slots (SnapdPlug   *plug);

G_END_DECLS

#endif /* __SNAPD_PLUG_H__ */
