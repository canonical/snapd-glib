/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_APP_H__
#define __SNAPD_APP_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-connection.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_APP  (snapd_app_get_type ())

G_DECLARE_FINAL_TYPE (SnapdApp, snapd_app, SNAPD, APP, GObject)

struct _SnapdAppClass
{
    /*< private >*/
    GObjectClass parent_class;
};


/**
 * SnapdDaemonType:
 * @SNAPD_DAEMON_TYPE_NONE: Not a daemon
 * @SNAPD_DAEMON_TYPE_UNKNOWN: Unknown daemon type
 * @SNAPD_DAEMON_TYPE_SIMPLE: Simple daemon
 * @SNAPD_DAEMON_TYPE_FORKING: Forking daemon
 * @SNAPD_DAEMON_TYPE_ONESHOT: One-shot daemon
 * @SNAPD_DAEMON_TYPE_DBUS: D-Bus daemon
 * @SNAPD_DAEMON_TYPE_NOTIFY: Notify daemon
 *
 * Type of daemon.
 */
typedef enum
{
    SNAPD_DAEMON_TYPE_NONE,
    SNAPD_DAEMON_TYPE_UNKNOWN,
    SNAPD_DAEMON_TYPE_SIMPLE,
    SNAPD_DAEMON_TYPE_FORKING,
    SNAPD_DAEMON_TYPE_ONESHOT,
    SNAPD_DAEMON_TYPE_DBUS,
    SNAPD_DAEMON_TYPE_NOTIFY
} SnapdDaemonType;

const gchar    *snapd_app_get_name        (SnapdApp *app);

gchar         **snapd_app_get_aliases     (SnapdApp *app);

SnapdDaemonType snapd_app_get_daemon_type (SnapdApp *app);

G_END_DECLS

#endif /* __SNAPD_APP_H__ */
