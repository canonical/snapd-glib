/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_LINK_H__
#define __SNAPD_LINK_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_LINK (snapd_link_get_type())

G_DECLARE_FINAL_TYPE(SnapdLink, snapd_link, SNAPD, LINK, GObject)

GStrv snapd_link_get_urls(SnapdLink *link);

const gchar *snapd_link_get_url_type(SnapdLink *link);

G_END_DECLS

#endif /* __SNAPD_LINK_H__ */
