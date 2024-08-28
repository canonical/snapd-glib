/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_NOTICE2_H__
#define __SNAPD_NOTICE2_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

typedef enum
{
        SNAPD_NOTICE2_TYPE_UNKNOWN = 0,
        SNAPD_NOTICE2_TYPE_CHANGE_UPDATE,
        SNAPD_NOTICE2_TYPE_REFRESH_INHIBIT,
        SNAPD_NOTICE2_TYPE_SNAP_RUN_INHIBIT
} SnapdNotice2Type;

G_BEGIN_DECLS

#define SNAPD_TYPE_NOTICE2  (snapd_notice2_get_type ())

G_DECLARE_FINAL_TYPE (SnapdNotice2, snapd_notice2, SNAPD, NOTICE2, GObject)

const gchar           *snapd_notice2_get_id             (SnapdNotice2 *notice);

const gchar           *snapd_notice2_get_user_id        (SnapdNotice2 *notice);

const SnapdNotice2Type snapd_notice2_get_notice_type    (SnapdNotice2 *notice);

const gchar           *snapd_notice2_get_key            (SnapdNotice2 *notice);

const GDateTime       *snapd_notice2_get_first_occurred (SnapdNotice2 *notice);

const GDateTime       *snapd_notice2_get_last_occurred  (SnapdNotice2 *notice);

int                    snapd_notice2_get_last_occurred_nanoseconds (SnapdNotice2 *notice);

const GDateTime       *snapd_notice2_get_last_repeated  (SnapdNotice2 *notice);

gint64                 snapd_notice2_get_occurrences    (SnapdNotice2 *notice);

const GHashTable      *snapd_notice2_get_last_data      (SnapdNotice2 *notice);

GTimeSpan              snapd_notice2_get_repeat_after   (SnapdNotice2 *notice);

GTimeSpan              snapd_notice2_get_expire_after   (SnapdNotice2 *notice);

gint                   snapd_notice2_compare_last_occurred (SnapdNotice2 *notice,
                                                           SnapdNotice2 *notice_to_compare);

G_END_DECLS

#endif /* __SNAPD_NOTICE2_H__ */
