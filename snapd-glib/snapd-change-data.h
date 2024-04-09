/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_CHANGE_DATA_H__
#define __SNAPD_CHANGE_DATA_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CHANGE_DATA snapd_change_data_get_type()
G_DECLARE_DERIVABLE_TYPE (SnapdChangeData, snapd_change_data, SNAPD, CHANGE_DATA, GObject)

struct _SnapdChangeDataClass
{
  GObjectClass parent_class;
};

G_END_DECLS

#endif /* __SNAPD_CHANGE_DATA_H__ */
