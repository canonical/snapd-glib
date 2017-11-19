/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_ALIAS_H__
#define __SNAPD_ALIAS_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_ALIAS  (snapd_alias_get_type ())

G_DECLARE_FINAL_TYPE (SnapdAlias, snapd_alias, SNAPD, ALIAS, GObject)

/**
 * SnapdAliasStatus:
 * @SNAPD_ALIAS_STATUS_UNKNOWN: the alias status is unknown.
 * @SNAPD_ALIAS_STATUS_DEFAULT: deprecated, do not use.
 * @SNAPD_ALIAS_STATUS_ENABLED: deprecated, do not use.
 * @SNAPD_ALIAS_STATUS_DISABLED: the alias is disabled.
 * @SNAPD_ALIAS_STATUS_AUTO: the alias is automatically enabled.
 * @SNAPD_ALIAS_STATUS_MANUAL: the alias is manually enabled.
 *
 * Status of an alias.
 *
 * Since: 1.8
 */
typedef enum
{
    SNAPD_ALIAS_STATUS_UNKNOWN,
    SNAPD_ALIAS_STATUS_DEFAULT,
    SNAPD_ALIAS_STATUS_ENABLED,
    SNAPD_ALIAS_STATUS_DISABLED,
    SNAPD_ALIAS_STATUS_AUTO,
    SNAPD_ALIAS_STATUS_MANUAL
} SnapdAliasStatus;

const gchar      *snapd_alias_get_app        (SnapdAlias *alias) G_DEPRECATED;

const gchar      *snapd_alias_get_app_auto   (SnapdAlias *alias);

const gchar      *snapd_alias_get_command    (SnapdAlias *alias);

const gchar      *snapd_alias_get_app_manual (SnapdAlias *alias);

const gchar      *snapd_alias_get_name       (SnapdAlias *alias);

const gchar      *snapd_alias_get_snap       (SnapdAlias *alias);

SnapdAliasStatus  snapd_alias_get_status     (SnapdAlias *alias);

G_END_DECLS

#endif /* __SNAPD_ALIAS_H__ */
