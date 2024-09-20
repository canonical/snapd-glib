/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-change-data.h"

/**
 * SECTION: snapd-change-data
 * @short_description: Base class for custom data from a change
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdChangeData is the base class for the custom data in
 * a #SnapdChange.
 */

/**
 * SnapdChangeData:
 *
 * A #SnapdChangeData is the base class for the custom data in
 * a #SnapdChange.
 *
 * Since: 1.65
 */

G_DEFINE_TYPE (SnapdChangeData, snapd_change_data, G_TYPE_OBJECT)

static void snapd_change_data_class_init(SnapdChangeDataClass *klass) {}

static void snapd_change_data_init(SnapdChangeData *self) {}
