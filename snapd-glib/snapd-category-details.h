/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_CATEGORY_DETAILS_H__
#define __SNAPD_CATEGORY_DETAILS_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CATEGORY_DETAILS  (snapd_category_details_get_type ())

G_DECLARE_FINAL_TYPE (SnapdCategoryDetails, snapd_category_details, SNAPD, CATEGORY_DETAILS, GObject)

const gchar *snapd_category_details_get_name (SnapdCategoryDetails *category_details);

G_END_DECLS

#endif /* __SNAPD_CATEGORY_DETAILS_H__ */
