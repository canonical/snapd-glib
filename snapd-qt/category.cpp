/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/category.h"

QSnapdCategory::QSnapdCategory (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdCategory::name () const
{
    return snapd_category_get_name (SNAPD_CATEGORY (wrapped_object));
}

bool QSnapdCategory::featured () const
{
    return snapd_category_get_featured (SNAPD_CATEGORY (wrapped_object));
}
