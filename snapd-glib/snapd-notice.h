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

typedef enum
{
        SNAPD_NOTICE_TYPE_UNKNOWN = 0,
        SNAPD_NOTICE_TYPE_CHANGE_UPDATE,
        SNAPD_NOTICE_TYPE_REFRESH_INHIBIT,
        SNAPD_NOTICE_TYPE_SNAP_RUN_INHIBIT
} SnapdNoticeType;

G_BEGIN_DECLS

#define SNAPD_TYPE_NOTICE  (snapd_notice_get_type ())

G_DECLARE_FINAL_TYPE (SnapdNotice, snapd_notice, SNAPD, NOTICE, GObject)

const gchar           *snapd_notice_get_id             (SnapdNotice *notice);

const gchar           *snapd_notice_get_user_id        (SnapdNotice *notice);

const SnapdNoticeType  snapd_notice_get_notice_type    (SnapdNotice *notice);

const gchar           *snapd_notice_get_key            (SnapdNotice *notice);

const GDateTime       *snapd_notice_get_first_occurred (SnapdNotice *notice);

const GDateTime       *snapd_notice_get_last_occurred  (SnapdNotice *notice);

const GDateTime       *snapd_notice_get_last_repeated  (SnapdNotice *notice);

const gint64           snapd_notice_get_occurrences    (SnapdNotice *notice);

GHashTable            *snapd_notice_get_last_data      (SnapdNotice *notice);

const GTimeSpan        snapd_notice_get_repeat_after   (SnapdNotice *notice);

const GTimeSpan        snapd_notice_get_expire_after   (SnapdNotice *notice);

G_END_DECLS
