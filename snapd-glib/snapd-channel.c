/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-channel.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-channel
 * @short_description: Snap channel metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdChannel contains the metadata for a given snap channel as returned
 * using snapd_snap_get_channels().
 */

/**
 * SnapdChannel:
 *
 * #SnapdChannel is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.22
 */

struct _SnapdChannel
{
    GObject parent_instance;

    SnapdConfinement confinement;
    gchar *branch;
    gchar *epoch;
    gchar *name;
    GDateTime *released_at;
    gchar *revision;
    gchar *risk;
    gint64 size;
    gchar *track;
    gchar *version;
};

enum
{
    PROP_CONFINEMENT = 1,
    PROP_EPOCH,
    PROP_NAME,
    PROP_REVISION,
    PROP_SIZE,
    PROP_VERSION,
    PROP_RELEASED_AT,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdChannel, snapd_channel, G_TYPE_OBJECT)

/**
 * snapd_channel_get_branch:
 * @channel: a #SnapdChannel.
 *
 * Get the branch this channel is tracking.
 *
 * Returns: (allow-none): a branch name or %NULL if not a branch.
 *
 * Since: 1.31
 */
const gchar *
snapd_channel_get_branch (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->branch;
}

/**
 * snapd_channel_get_confinement:
 * @channel: a #SnapdChannel.
 *
 * Get the confinement this snap is using, e.g. %SNAPD_CONFINEMENT_STRICT.
 *
 * Returns: a #SnapdConfinement.
 *
 * Since: 1.22
 */
SnapdConfinement
snapd_channel_get_confinement (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), SNAPD_CONFINEMENT_UNKNOWN);
    return self->confinement;
}

/**
 * snapd_channel_get_epoch:
 * @channel: a #SnapdChannel.
 *
 * Get the epoch used on this channel, e.g. "1".
 *
 * Returns: an epoch.
 *
 * Since: 1.22
 */
const gchar *
snapd_channel_get_epoch (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->epoch;
}

/**
 * snapd_channel_get_name:
 * @channel: a #SnapdChannel.
 *
 * Get the name of this channel, e.g. "stable".
 *
 * Channel names are in the form `track/risk/branch`
 *
 * `track` is the name of the feature track. Defaults to `latest` and is implied
 *         if the track is not present.
 * `risk` is the risk of the channel, one of `stable`, `candidate`, `beta` or `edge`.
 * `branch` is an optional branch name.
 *
 * Example names:
 * `beta` (alias to `latest/beta`)
 * `xenial/stable` (stable release on xenial track)
 * `latest/stable/red-button` (red button feature branch)
 *
 * Returns: a name.
 *
 * Since: 1.22
 */
const gchar *
snapd_channel_get_name (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->name;
}

/**
 * snapd_channel_get_released_at:
 * @channel: a #SnapdChannel.
 *
 * Get the date this revision was released into the channel or %NULL if unknown.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime.
 *
 * Since: 1.46
 */
GDateTime *
snapd_channel_get_released_at (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->released_at;
}

/**
 * snapd_channel_get_revision:
 * @channel: a #SnapdChannel.
 *
 * Get the revision for this snap. The format of the string is undefined.
 * See also snapd_channel_get_version().
 *
 * Returns: a revision string.
 *
 * Since: 1.22
 */
const gchar *
snapd_channel_get_revision (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->revision;
}

/**
 * snapd_channel_get_risk:
 * @channel: a #SnapdChannel.
 *
 * Get the risk this channel is on, one of `stable`, `candidate`, `beta` or `edge`.
 *
 * Returns: a risk name.
 *
 * Since: 1.31
 */
const gchar *
snapd_channel_get_risk (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->risk;
}

/**
 * snapd_channel_get_size:
 * @channel: a #SnapdChannel.
 *
 * Get the download size of this snap.
 *
 * Returns: a byte count.
 *
 * Since: 1.22
 */
gint64
snapd_channel_get_size (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), 0);
    return self->size;
}

/**
 * snapd_channel_get_track:
 * @channel: a #SnapdChannel.
 *
 * Get the track this channel is on.
 *
 * Returns: a track name.
 *
 * Since: 1.31
 */
const gchar *
snapd_channel_get_track (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->track;
}

/**
 * snapd_channel_get_version:
 * @channel: a #SnapdChannel.
 *
 * Get the version for this snap. The format of the string is undefined.
 * See also snapd_channel_get_revision().
 *
 * Returns: a version string.
 *
 * Since: 1.22
 */
const gchar *
snapd_channel_get_version (SnapdChannel *self)
{
    g_return_val_if_fail (SNAPD_IS_CHANNEL (self), NULL);
    return self->version;
}

static gboolean
is_risk (const gchar *risk)
{
    return g_strcmp0 (risk, "stable") == 0 || g_strcmp0 (risk, "candidate") == 0 || g_strcmp0 (risk, "beta") == 0 || g_strcmp0 (risk, "edge") == 0;
}

static void
set_name (SnapdChannel *self, const gchar *name)
{
    g_free (self->name);
    self->name = g_strdup (name);

    g_clear_pointer (&self->track, g_free);
    g_clear_pointer (&self->risk, g_free);
    g_clear_pointer (&self->branch, g_free);

    g_auto(GStrv) tokens = g_strsplit (name, "/", -1);
    switch (g_strv_length (tokens)) {
    case 1:
        if (is_risk (tokens[0])) {
            self->track = g_strdup ("latest");
            self->risk = g_strdup (tokens[0]);
        }
        else {
            self->track = g_strdup (tokens[0]);
            self->risk = g_strdup ("stable");
        }
        break;
    case 2:
        if (is_risk (tokens[0])) {
            self->track = g_strdup ("latest");
            self->risk = g_strdup (tokens[0]);
            self->branch = g_strdup (tokens[1]);
        }
        else {
            self->track = g_strdup (tokens[0]);
            self->risk = g_strdup (tokens[1]);
        }
        break;
    case 3:
        self->track = g_strdup (tokens[0]);
        self->risk = g_strdup (tokens[1]);
        self->branch = g_strdup (tokens[2]);
        break;
    default:
        break;
    }
}

static void
snapd_channel_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdChannel *self = SNAPD_CHANNEL (object);

    switch (prop_id) {
    case PROP_CONFINEMENT:
        self->confinement = g_value_get_enum (value);
        break;
    case PROP_EPOCH:
        g_free (self->epoch);
        self->epoch = g_strdup (g_value_get_string (value));
        break;
    case PROP_NAME:
        set_name (self, g_value_get_string (value));
        break;
    case PROP_RELEASED_AT:
        g_clear_pointer (&self->released_at, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            self->released_at = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_REVISION:
        g_free (self->revision);
        self->revision = g_strdup (g_value_get_string (value));
        break;
    case PROP_SIZE:
        self->size = g_value_get_int64 (value);
        break;
    case PROP_VERSION:
        g_free (self->version);
        self->version = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_channel_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdChannel *self = SNAPD_CHANNEL (object);

    switch (prop_id) {
    case PROP_CONFINEMENT:
        g_value_set_enum (value, self->confinement);
        break;
    case PROP_EPOCH:
        g_value_set_string (value, self->epoch);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_RELEASED_AT:
        g_value_set_boxed (value, self->released_at);
        break;
    case PROP_REVISION:
        g_value_set_string (value, self->revision);
        break;
    case PROP_SIZE:
        g_value_set_int64 (value, self->size);
        break;
    case PROP_VERSION:
        g_value_set_string (value, self->version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_channel_finalize (GObject *object)
{
    SnapdChannel *self = SNAPD_CHANNEL (object);

    g_clear_pointer (&self->branch, g_free);
    g_clear_pointer (&self->epoch, g_free);
    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->revision, g_free);
    g_clear_pointer (&self->released_at, g_date_time_unref);
    g_clear_pointer (&self->risk, g_free);
    g_clear_pointer (&self->track, g_free);
    g_clear_pointer (&self->version, g_free);

    G_OBJECT_CLASS (snapd_channel_parent_class)->finalize (object);
}

static void
snapd_channel_class_init (SnapdChannelClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_channel_set_property;
    gobject_class->get_property = snapd_channel_get_property;
    gobject_class->finalize = snapd_channel_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_CONFINEMENT,
                                     g_param_spec_enum ("confinement",
                                                        "confinement",
                                                        "Confinement requested by the snap",
                                                        SNAPD_TYPE_CONFINEMENT, SNAPD_CONFINEMENT_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_EPOCH,
                                     g_param_spec_string ("epoch",
                                                          "epoch",
                                                          "Epoch of this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "The channel name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_REVISION,
                                     g_param_spec_string ("revision",
                                                          "revision",
                                                          "Revision of this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_RELEASED_AT,
                                     g_param_spec_boxed ("released-at",
                                                         "released-at",
                                                         "Date revision was released into channel",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SIZE,
                                     g_param_spec_int64 ("size",
                                                         "size",
                                                         "Download size in bytes",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_VERSION,
                                     g_param_spec_string ("version",
                                                          "version",
                                                          "Snap version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_channel_init (SnapdChannel *self)
{
}
