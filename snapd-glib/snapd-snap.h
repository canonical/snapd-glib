/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_SNAP_H__
#define __SNAPD_SNAP_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>
#include <snapd-glib/snapd-channel.h>
#include <snapd-glib/snapd-price.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SNAP  (snapd_snap_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSnap, snapd_snap, SNAPD, SNAP, GObject)

/**
 * SnapdSnapType:
 * @SNAPD_SNAP_TYPE_UNKNOWN: the type of snap is unknown.
 * @SNAPD_SNAP_TYPE_APP: the snap is an application.
 * @SNAPD_SNAP_TYPE_KERNEL: the snap is a kernel.
 * @SNAPD_SNAP_TYPE_GADGET: the snapd is a gadget.
 * @SNAPD_SNAP_TYPE_OS: the snap is an operating system.
 *
 * Type of snap.
 *
 * Since: 1.0
 */
typedef enum
{
    SNAPD_SNAP_TYPE_UNKNOWN,
    SNAPD_SNAP_TYPE_APP,
    SNAPD_SNAP_TYPE_KERNEL,
    SNAPD_SNAP_TYPE_GADGET,
    SNAPD_SNAP_TYPE_OS
} SnapdSnapType;

/**
 * SnapdSnapStatus:
 * @SNAPD_SNAP_STATUS_UNKNOWN: the snap state is unknown.
 * @SNAPD_SNAP_STATUS_AVAILABLE: the snap is available for installation.
 * @SNAPD_SNAP_STATUS_PRICED: the snap is available for purchase.
 * @SNAPD_SNAP_STATUS_INSTALLED: the snap is installed but not active.
 * @SNAPD_SNAP_STATUS_ACTIVE: the snap is installed and active.
 *
 * The current state of a snap.
 *
 * Since: 1.0
 */
typedef enum
{
    SNAPD_SNAP_STATUS_UNKNOWN,
    SNAPD_SNAP_STATUS_AVAILABLE,
    SNAPD_SNAP_STATUS_PRICED,
    SNAPD_SNAP_STATUS_INSTALLED,
    SNAPD_SNAP_STATUS_ACTIVE
} SnapdSnapStatus;

/**
 * SnapdPublisherValidation:
 * @SNAPD_PUBLISHER_VALIDATION_UNKNOWN: the validation state of the publisher is unknown.
 * @SNAPD_PUBLISHER_VALIDATION_UNPROVEN: the publisher has not proven their identity.
 * @SNAPD_PUBLISHER_VALIDATION_VERIFIED: the publisher has had their identity verified.
 *
 * State of validation for a publisher.
 *
 * Since: 1.42
 */
typedef enum
{
    SNAPD_PUBLISHER_VALIDATION_UNKNOWN,
    SNAPD_PUBLISHER_VALIDATION_UNPROVEN,
    SNAPD_PUBLISHER_VALIDATION_VERIFIED
} SnapdPublisherValidation;

GPtrArray               *snapd_snap_get_apps                   (SnapdSnap   *snap);

const gchar             *snapd_snap_get_broken                 (SnapdSnap   *snap);

const gchar             *snapd_snap_get_channel                (SnapdSnap   *snap);

GPtrArray               *snapd_snap_get_channels               (SnapdSnap   *snap);

SnapdChannel            *snapd_snap_match_channel              (SnapdSnap   *snap,
                                                        const gchar *name);

gchar                  **snapd_snap_get_common_ids             (SnapdSnap   *snap);

SnapdConfinement         snapd_snap_get_confinement            (SnapdSnap   *snap);

const gchar             *snapd_snap_get_contact                (SnapdSnap   *snap);

const gchar             *snapd_snap_get_description            (SnapdSnap   *snap);

const gchar             *snapd_snap_get_developer              (SnapdSnap   *snap) G_DEPRECATED_FOR(snapd_snap_get_publisher_username);

gboolean                 snapd_snap_get_devmode                (SnapdSnap   *snap);

gint64                   snapd_snap_get_download_size          (SnapdSnap   *snap);

const gchar             *snapd_snap_get_icon                   (SnapdSnap   *snap);

const gchar             *snapd_snap_get_id                     (SnapdSnap   *snap);

GDateTime               *snapd_snap_get_install_date           (SnapdSnap   *snap);

gint64                   snapd_snap_get_installed_size         (SnapdSnap   *snap);

gboolean                 snapd_snap_get_jailmode               (SnapdSnap   *snap);

const gchar             *snapd_snap_get_license                (SnapdSnap   *snap);

const gchar             *snapd_snap_get_name                   (SnapdSnap   *snap);

GPtrArray               *snapd_snap_get_prices                 (SnapdSnap   *snap);

gboolean                 snapd_snap_get_private                (SnapdSnap   *snap);

const gchar             *snapd_snap_get_publisher_display_name (SnapdSnap   *snap);

const gchar             *snapd_snap_get_publisher_id           (SnapdSnap   *snap);

const gchar             *snapd_snap_get_publisher_username     (SnapdSnap   *snap);

SnapdPublisherValidation snapd_snap_get_publisher_validation   (SnapdSnap   *snap);

const gchar             *snapd_snap_get_revision               (SnapdSnap   *snap);

GPtrArray               *snapd_snap_get_screenshots            (SnapdSnap   *snap);

SnapdSnapType            snapd_snap_get_snap_type              (SnapdSnap   *snap);

SnapdSnapStatus          snapd_snap_get_status                 (SnapdSnap   *snap);

const gchar             *snapd_snap_get_summary                (SnapdSnap   *snap);

const gchar             *snapd_snap_get_title                  (SnapdSnap   *snap);

const gchar             *snapd_snap_get_tracking_channel       (SnapdSnap   *snap);

gchar                  **snapd_snap_get_tracks                 (SnapdSnap   *snap);

gboolean                 snapd_snap_get_trymode                (SnapdSnap   *snap);

const gchar             *snapd_snap_get_version                (SnapdSnap   *snap);

G_END_DECLS

#endif /* __SNAPD_SNAP_H__ */
