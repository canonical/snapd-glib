/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include "snapd-notice.h"

G_BEGIN_DECLS

#define SNAPD_TYPE_NOTICES  (snapd_notices_get_type ())

G_DECLARE_FINAL_TYPE (SnapdNotices, snapd_notices, SNAPD, NOTICES, GObject)

SnapdNotices      *snapd_notices_new           (GPtrArray *data);

guint64            snapd_notices_get_n_notices (SnapdNotices *notices);

SnapdNotice       *snapd_notices_get_notice    (SnapdNotices *notices,
                                                guint64       notice);

G_END_DECLS
