/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_CHANNEL_H__
#define __SNAPD_CHANNEL_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CHANNEL  (snapd_channel_get_type ())

G_DECLARE_FINAL_TYPE (SnapdChannel, snapd_channel, SNAPD, CHANNEL, GObject)

/**
 * SnapdConfinement:
 * @SNAPD_CONFINEMENT_UNKNOWN: the confinement of the snap is unknown.
 * @SNAPD_CONFINEMENT_STRICT: the snap is using confinement.
 * @SNAPD_CONFINEMENT_DEVMODE: the snap is in dev mode (i.e. unconfined).
 * @SNAPD_CONFINEMENT_CLASSIC: the snap is using classic confinement.
 *
 * Confinement used by a snap.
 *
 * Since: 1.0
 */
typedef enum
{
    SNAPD_CONFINEMENT_UNKNOWN,
    SNAPD_CONFINEMENT_STRICT,
    SNAPD_CONFINEMENT_DEVMODE,
    SNAPD_CONFINEMENT_CLASSIC
} SnapdConfinement;

const gchar      *snapd_channel_get_branch      (SnapdChannel *channel);

SnapdConfinement  snapd_channel_get_confinement (SnapdChannel *channel);

const gchar      *snapd_channel_get_epoch       (SnapdChannel *channel);

const gchar      *snapd_channel_get_name        (SnapdChannel *channel);

const gchar      *snapd_channel_get_revision    (SnapdChannel *channel);

const gchar      *snapd_channel_get_risk        (SnapdChannel *channel);

gint64            snapd_channel_get_size        (SnapdChannel *channel);

const gchar      *snapd_channel_get_track       (SnapdChannel *channel);

const gchar      *snapd_channel_get_version     (SnapdChannel *channel);

G_END_DECLS

#endif /* __SNAPD_CHANNEL_H__ */
