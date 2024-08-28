/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-snap.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-snap
 * @short_description: Snap metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdSnap contains the metadata for a given snap. Snap metadata can be
 * retrieved using snapd_client_list_sync(), snapd_client_list_one_sync() or
 * snapd_client_find_sync().
 */

/**
 * SnapdSnap:
 *
 * #SnapdSnap contains Snap metadata.
 *
 * Since: 1.0
 */

struct _SnapdSnap
{
    GObject parent_instance;

    GPtrArray *apps;
    gchar *base;
    gchar *broken;
    GPtrArray *categories;
    gchar *channel;
    GPtrArray *channels;
    GStrv common_ids;
    SnapdConfinement confinement;
    gchar *contact;
    gchar *description;
    gboolean devmode;
    gint64 download_size;
    GPtrArray *donation;
    GDateTime *hold;
    gchar *icon;
    gchar *id;
    GDateTime *install_date;
    gint64 installed_size;
    GPtrArray *issues;
    gboolean jailmode;
    gchar *license;
    GPtrArray *media;
    gchar *mounted_from;
    gchar *name;
    GPtrArray *prices;
    gboolean private;
    gchar *publisher_display_name;
    gchar *publisher_id;
    gchar *publisher_username;
    SnapdPublisherValidation publisher_validation;
    gchar *revision;
    GPtrArray *screenshots;
    GPtrArray *source_code;
    SnapdSnapStatus status;
    gchar *store_url;
    gchar *summary;
    gchar *title;
    gchar *tracking_channel;
    GStrv tracks;
    gboolean trymode;
    SnapdSnapType snap_type;
    gchar *version;
    GPtrArray *website;
    GDateTime *proceed_time;
};

enum
{
    PROP_APPS = 1,
    PROP_CATEGORIES,
    PROP_CHANNEL,
    PROP_CONFINEMENT,
    PROP_CONTACT,
    PROP_DESCRIPTION,
    PROP_DEVELOPER,
    PROP_DEVMODE,
    PROP_DONATION,
    PROP_DOWNLOAD_SIZE,
    PROP_ICON,
    PROP_ID,
    PROP_INSTALL_DATE,
    PROP_INSTALLED_SIZE,
    PROP_ISSUES,
    PROP_JAILMODE,
    PROP_NAME,
    PROP_PRICES,
    PROP_PRIVATE,
    PROP_REVISION,
    PROP_SCREENSHOTS,
    PROP_STATUS,
    PROP_STORE_URL,
    PROP_SUMMARY,
    PROP_TRYMODE,
    PROP_SNAP_TYPE,
    PROP_VERSION,
    PROP_TRACKING_CHANNEL,
    PROP_TITLE,
    PROP_LICENSE,
    PROP_CHANNELS,
    PROP_TRACKS,
    PROP_BROKEN,
    PROP_COMMON_IDS,
    PROP_PUBLISHER_DISPLAY_NAME,
    PROP_PUBLISHER_ID,
    PROP_PUBLISHER_USERNAME,
    PROP_PUBLISHER_VALIDATION,
    PROP_BASE,
    PROP_MOUNTED_FROM,
    PROP_MEDIA,
    PROP_SOURCE_CODE,
    PROP_WEBSITE,
    PROP_HOLD,
    PROP_PROCEED_TIME,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdSnap, snapd_snap, G_TYPE_OBJECT)

/**
 * snapd_snap_get_apps:
 * @snap: a #SnapdSnap.
 *
 * Get the apps this snap provides.
 *
 * Returns: (transfer none) (element-type SnapdApp): an array of #SnapdApp.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_snap_get_apps (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->apps;
}

/**
 * snapd_snap_get_base:
 * @snap: a #SnapdSnap.
 *
 * Get the base snap this snap uses.
 *
 * Returns: (allow-none): a snap name or %NULL if not set.
 *
 * Since: 1.45
 */
const gchar *
snapd_snap_get_base (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->base;
}

/**
 * snapd_snap_get_broken:
 * @snap: a #SnapdSnap.
 *
 * Get the reason this snap is broken.
 *
 * Returns: (allow-none): an error string or %NULL if not broken.
 *
 * Since: 1.25
 */
const gchar *
snapd_snap_get_broken (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->broken;
}

/**
 * snapd_snap_get_categories:
 * @snap: a #SnapdSnap.
 *
 * Gets the categories this snap belongs to.
 *
 * Returns: (transfer none) (element-type SnapdCategory): an array of #SnapdCategory.
 *
 * Since: 1.64
 */
GPtrArray *
snapd_snap_get_categories (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->categories;
}

/**
 * snapd_snap_get_channel:
 * @snap: a #SnapdSnap.
 *
 * Get the channel this snap is from, e.g. "stable".
 *
 * Returns: a channel name.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_channel (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->channel;
}

/**
 * snapd_snap_get_channels:
 * @snap: a #SnapdSnap.
 *
 * Gets the available channels for this snap.
 *
 * Returns: (transfer none) (element-type SnapdChannel): an array of #SnapdChannel.
 *
 * Since: 1.22
 */
GPtrArray *
snapd_snap_get_channels (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->channels;
}

static int
parse_risk (const gchar *risk)
{
    if (g_strcmp0 (risk, "stable") == 0)
        return 0;
    else if (g_strcmp0 (risk, "candidate") == 0)
        return 1;
    else if (g_strcmp0 (risk, "beta") == 0)
        return 2;
    else if (g_strcmp0 (risk, "edge") == 0)
        return 3;
    else
        return -1;
}

/**
 * snapd_snap_match_channel:
 * @snap: a #SnapdSnap.
 * @name: a channel name.
 *
 * Finds the available channel that best matches the given name.
 * If none matches %NULL is returned.
 *
 * Returns: (transfer none) (allow-none): an #SnapdChannel or %NULL.
 *
 * Since: 1.22
 */
SnapdChannel *
snapd_snap_match_channel (SnapdSnap *self, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    g_autoptr(SnapdChannel) c = g_object_new (SNAPD_TYPE_CHANNEL,
                                              "name", name,
                                              NULL);
    SnapdChannel *matched_channel = NULL;
    int matched_risk = -1;
    for (guint i = 0; i < self->channels->len; i++) {
        SnapdChannel *channel = self->channels->pdata[i];

        /* Must be same track and branch */
        if (g_strcmp0 (snapd_channel_get_track (channel), snapd_channel_get_track (c)) != 0 ||
            g_strcmp0 (snapd_channel_get_branch (channel), snapd_channel_get_branch (c)) != 0)
            continue;

        /* Must be no riskier than requested */
        int r = parse_risk (snapd_channel_get_risk (channel));
        if (r > parse_risk (snapd_channel_get_risk (c)))
            continue;

        /* Use this if unmatched or a better risk match */
        if (matched_channel == NULL || r > matched_risk) {
            matched_channel = channel;
            matched_risk = r;
        }
    }

    return matched_channel;
}

/**
 * snapd_snap_get_common_ids:
 * @snap: a #SnapdSnap.
 *
 * Get common IDs associated with this snap.
 *
 * Returns: (transfer none) (array zero-terminated=1): an array of common ids.
 *
 * Since: 1.41
 */
GStrv
snapd_snap_get_common_ids (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->common_ids;
}

/**
 * snapd_snap_get_confinement:
 * @snap: a #SnapdSnap.
 *
 * Get the confinement this snap is using, e.g. %SNAPD_CONFINEMENT_STRICT.
 *
 * Returns: a #SnapdConfinement.
 *
 * Since: 1.0
 */
SnapdConfinement
snapd_snap_get_confinement (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), SNAPD_CONFINEMENT_UNKNOWN);
    return self->confinement;
}

/**
 * snapd_snap_get_contact:
 * @snap: a #SnapdSnap.
 *
 * Get the means of contacting the snap developer, e.g. "mailto:developer@example.com".
 *
 * Returns: a contact URL.
 *
 * Since: 1.13
 */
const gchar *
snapd_snap_get_contact (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->contact;
}

/**
 * snapd_snap_get_description:
 * @snap: a #SnapdSnap.
 *
 * Get a multi-line description of this snap. The description is formatted using
 * a subset of Markdown. To parse this use a #SnapdMarkdownParser.
 *
 * Returns: description text.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_description (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->description;
}

/**
 * snapd_snap_get_developer:
 * @snap: a #SnapdSnap.
 *
 * Get the developer who created this snap.
 *
 * Returns: a developer name.
 *
 * Since: 1.0
 * Deprecated: 1.42: Use snapd_snap_get_publisher_username()
 */
const gchar *
snapd_snap_get_developer (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->publisher_username;
}

/**
 * snapd_snap_get_devmode:
 * @snap: a #SnapdSnap.
 *
 * Get if this snap is running in developer mode.
 *
 * Returns: %TRUE if this snap is running in devmode.
 *
 * Since: 1.0
 */
gboolean
snapd_snap_get_devmode (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), FALSE);
    return self->devmode;
}

/**
 * snapd_snap_get_donation:
 * @snap: a #SnapdSnap.
 *
 * Get the donation URLs of the snap, e.g. ["http://example.com/paypal", "http://liberapay.com/test"].
 *
 * Returns: (element-type GPtrArray*) (transfer none): an array of URL for Donation.
 *
 * Since: 1.66
 */
GPtrArray *
snapd_snap_get_donation (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->donation;
}

/**
 * snapd_snap_get_download_size:
 * @snap: a #SnapdSnap.
 *
 * Get the download size of this snap or 0 if unknown.
 *
 * Returns: a byte count.
 *
 * Since: 1.0
 */
gint64
snapd_snap_get_download_size (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), 0);
    return self->download_size;
}

/**
 * snapd_snap_get_hold:
 * @snap: a #SnapdSnap.
 *
 * Get the date this snap will re-enable automatic refreshing or %NULL if no hold is present.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime or %NULL.
 *
 * Since: 1.64
 */
GDateTime *
snapd_snap_get_hold (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->hold;
}

/**
 * snapd_snap_get_proceed_time:
 * @snap: a @SnapdSnap
 *
 * Returns the date and time after which a refresh is forced for this running snap
 * in the next auto-refresh. By substracting the current date and time it's possible
 * to know how many time remains before the snap is forced to be refreshed.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime or %NULL.
 *
 * Since: 1.65
 */
GDateTime *
snapd_snap_get_proceed_time (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->proceed_time;
}

/**
 * snapd_snap_get_icon:
 * @snap: a #SnapdSnap.
 *
 * Get the icon for this Snap, either a URL or an absolute path to retrieve it
 * from snapd directly.
 *
 * Returns: a URL or path.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_icon (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->icon;
}

/**
 * snapd_snap_get_id:
 * @snap: a #SnapdSnap.
 *
 * Gets the unique ID for this snap.
 *
 * Returns: an ID.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_id (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->id;
}

/**
 * snapd_snap_get_install_date:
 * @snap: a #SnapdSnap.
 *
 * Get the date this snap was installed or %NULL if unknown.
 *
 * Returns: (transfer none) (allow-none): a #GDateTime or %NULL.
 *
 * Since: 1.0
 */
GDateTime *
snapd_snap_get_install_date (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->install_date;
}

/**
 * snapd_snap_get_installed_size:
 * @snap: a #SnapdSnap.
 *
 * Get the installed size of this snap or 0 if unknown.
 *
 * Returns: a byte count.
 *
 * Since: 1.0
 */
gint64
snapd_snap_get_installed_size (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), 0);
    return self->installed_size;
}

/**
 * snapd_snap_get_issues:
 * @snap: a #SnapdSnap.
 *
 * Get the issues of the snap, e.g. "http://example.com".
 *
 * Returns: (element-type GPtrArray*) (transfer none): a array of Issue  URLs.
 *
 * Since: 1.66
 */
GPtrArray *
snapd_snap_get_issues (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->issues;
}


/**
 * snapd_snap_get_jailmode:
 * @snap: a #SnapdSnap.
 *
 * Get if this snap is running in enforced confinement (jail) mode.
 *
 * Returns: %TRUE if this snap is running in jailmode.
 *
 * Since: 1.8
 */
gboolean
snapd_snap_get_jailmode (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), FALSE);
    return self->jailmode;
}

/**
 * snapd_snap_get_license:
 * @snap: a #SnapdSnap.
 *
 * Gets the SPDX license expression for this snap, e.g. "GPL-3.0+".
 *
 * Returns: (allow-none): an SPDX license expression or %NULL.
 *
 * Since: 1.19
 */
const gchar *
snapd_snap_get_license (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->license;
}

/**
 * snapd_snap_get_media:
 * @snap: a #SnapdSnap.
 *
 * Get media that is associated with this snap.
 *
 * Returns: (transfer none) (element-type SnapdMedia): an array of #SnapdMedia.
 *
 * Since: 1.45
 */
GPtrArray *
snapd_snap_get_media (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->media;
}

/**
 * snapd_snap_get_mounted_from:
 * @snap: a #SnapdSnap.
 *
 * Gets the path this snap is mounted from, which is a .snap file for installed
 * snaps and a directory for snaps in try mode.
 *
 * Returns: (allow-none): a file path or %NULL.
 *
 * Since: 1.45
 */
const gchar *
snapd_snap_get_mounted_from (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->mounted_from;
}

/**
 * snapd_snap_get_title:
 * @snap: a #SnapdSnap.
 *
 * Get the title for this snap. If not available use the snap name instead.
 *
 * Returns: (allow-none): a title or %NULL.
 *
 * Since: 1.14
 */
const gchar *
snapd_snap_get_title (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->title;
}

/**
 * snapd_snap_get_name:
 * @snap: a #SnapdSnap.
 *
 * Get the name of this snap. This is used to reference this snap, e.g. for
 * installing / removing.
 *
 * Returns: a name.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_name (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->name;
}

/**
 * snapd_snap_get_prices:
 * @snap: a #SnapdSnap.
 *
 * Get the prices that this snap can be purchased at.
 *
 * Returns: (transfer none) (element-type SnapdPrice): an array of #SnapdPrice.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_snap_get_prices (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->prices;
}

/**
 * snapd_snap_get_private:
 * @snap: a #SnapdSnap.
 *
 * Get if this snap is only available to the developer.
 *
 * Returns: %TRUE if this is a private snap.
 *
 * Since: 1.0
 */
gboolean
snapd_snap_get_private (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), FALSE);
    return self->private;
}

/**
 * snapd_snap_get_publisher_display_name:
 * @snap: a #SnapdSnap.
 *
 * Get the display name of the publisher who created this snap.
 *
 * Returns: a publisher display name.
 *
 * Since: 1.42
 */
const gchar *
snapd_snap_get_publisher_display_name (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->publisher_display_name;
}

/**
 * snapd_snap_get_publisher_id:
 * @snap: a #SnapdSnap.
 *
 * Get the ID of the publisher who created this snap.
 *
 * Returns: a publisher ID.
 *
 * Since: 1.42
 */
const gchar *
snapd_snap_get_publisher_id (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->publisher_id;
}

/**
 * snapd_snap_get_publisher_username:
 * @snap: a #SnapdSnap.
 *
 * Get the username of the publisher who created this snap.
 *
 * Returns: a publisher username.
 *
 * Since: 1.42
 */
const gchar *
snapd_snap_get_publisher_username (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->publisher_username;
}

/**
 * snapd_snap_get_publisher_validation:
 * @snap: a #SnapdSnap.
 *
 * Get the validation for the snap publisher, e.g. %SNAPD_PUBLISHER_VALIDATION_VERIFIED
 *
 * Returns: a #SnapdPublisherValidation.
 *
 * Since: 1.42
 */
SnapdPublisherValidation
snapd_snap_get_publisher_validation (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), SNAPD_PUBLISHER_VALIDATION_UNKNOWN);
    return self->publisher_validation;
}

/**
 * snapd_snap_get_revision:
 * @snap: a #SnapdSnap.
 *
 * Get the revision for this snap. The format of the string is undefined.
 * See also snapd_snap_get_version().
 *
 * Returns: a revision string.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_revision (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->revision;
}

/**
 * snapd_snap_get_screenshots:
 * @snap: a #SnapdSnap.
 *
 * Get the screenshots that are available for this snap.
 *
 * Returns: (transfer none) (element-type SnapdScreenshot): an array of #SnapdScreenshot.
 *
 * Since: 1.0
 * Deprecated: 1.45: Use snapd_snap_get_media()
 */
GPtrArray *
snapd_snap_get_screenshots (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->screenshots;
}

/**
 * snapd_snap_get_snap_type:
 * @snap: a #SnapdSnap.
 *
 * Get the type of snap, e.g. %SNAPD_SNAP_TYPE_APP
 *
 * Returns: a #SnapdSnapType.
 *
 * Since: 1.0
 */
SnapdSnapType
snapd_snap_get_snap_type (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), SNAPD_SNAP_TYPE_UNKNOWN);
    return self->snap_type;
}

/**
 * snapd_snap_get_source_code:
 * @snap: a #SnapdSnap.
 *
 * Get the source-code URLs of the snap, e.g. ["http://example.com", "https://vcs-browser.com/source-code"].
 *
 * Returns: (element-type GPtrArray*) (transfer none): a array of URL for the source code.
 *
 * Since: 1.66
 */
GPtrArray *
snapd_snap_get_source_code (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->source_code;
}

/**
 * snapd_snap_get_status:
 * @snap: a #SnapdSnap.
 *
 * Get the current status of this snap, e.g. SNAPD_SNAP_STATUS_INSTALLED.
 *
 * Returns: a #SnapdSnapStatus.
 *
 * Since: 1.0
 */
SnapdSnapStatus
snapd_snap_get_status (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), SNAPD_SNAP_STATUS_UNKNOWN);
    return self->status;
}

/**
 * snapd_snap_get_store_url:
 * @snap: a #SnapdSnap.
 *
 * Get a URL to the web snap store, e.g. "https://snapcraft.io/example"
 *
 * Returns: (allow-none): a URL or %NULL.
 *
 * Since: 1.60
 */
const gchar *
snapd_snap_get_store_url (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->store_url;
}

/**
 * snapd_snap_get_summary:
 * @snap: a #SnapdSnap.
 *
 * Get a single line summary for this snap, e.g. "Best app ever!".
 *
 * Returns: a summary string.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_summary (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->summary;
}

/**
 * snapd_snap_get_tracking_channel:
 * @snap: a #SnapdSnap.
 *
 * Get the channel that updates will be installed from, e.g. "stable".
 *
 * Returns: a channel name.
 *
 * Since: 1.7
 */
const gchar *
snapd_snap_get_tracking_channel (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->tracking_channel;
}

/**
 * snapd_snap_get_tracks:
 * @snap: a #SnapdSnap.
 *
 * Get the tracks that are available.
 *
 * Returns: (transfer none) (array zero-terminated=1): an ordered array of track names.
 *
 * Since: 1.22
 */
GStrv
snapd_snap_get_tracks (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->tracks;
}

/**
 * snapd_snap_get_trymode:
 * @snap: a #SnapdSnap.
 *
 * Get if this snap is running in try mode (installed locally and able to be
 * directly modified).
 *
 * Returns: %TRUE if using trymode.
 *
 * Since: 1.0
 */
gboolean
snapd_snap_get_trymode (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), FALSE);
    return self->trymode;
}

/**
 * snapd_snap_get_version:
 * @snap: a #SnapdSnap.
 *
 * Get the version for this snap. The format of the string is undefined.
 * See also snapd_snap_get_revision().
 *
 * Returns: a version string.
 *
 * Since: 1.0
 */
const gchar *
snapd_snap_get_version (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->version;
}

/**
 * snapd_snap_get_website:
 * @snap: a #SnapdSnap.
 *
 * Get the website of the snap developer, e.g. "http://example.com".
 *
 * Returns: a website URL.
 *
 * Since: 1.50
 * Deprecated: 1.66
 */
const gchar *
snapd_snap_get_website (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return g_ptr_array_index(self->website, 0);
}

/**
 * snapd_snap_get_website_urls:
 * @snap: a #SnapdSnap.
 *
 * Get the websites of the snap developer, e.g. ["http://example.com", "https://test.com"].
 *
 * Returns: (element-type GPtrArray*) (transfer none): an array of URLs for website.
 *
 * Since: 1.66
 */
GPtrArray *
snapd_snap_get_website_urls (SnapdSnap *self)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (self), NULL);
    return self->website;
}

static void
snapd_snap_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSnap *self = SNAPD_SNAP (object);

    switch (prop_id) {
    case PROP_APPS:
        g_clear_pointer (&self->apps, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->apps = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_BASE:
        g_free (self->base);
        self->base = g_strdup (g_value_get_string (value));
        break;
    case PROP_BROKEN:
        g_free (self->broken);
        self->broken = g_strdup (g_value_get_string (value));
        break;
    case PROP_CATEGORIES:
        g_clear_pointer (&self->categories, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->categories = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_CHANNEL:
        g_free (self->channel);
        self->channel = g_strdup (g_value_get_string (value));
        break;
    case PROP_CHANNELS:
        g_clear_pointer (&self->channels, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->channels = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_CONFINEMENT:
        self->confinement = g_value_get_enum (value);
        break;
    case PROP_CONTACT:
        g_free (self->contact);
        self->contact = g_strdup (g_value_get_string (value));
        break;
    case PROP_DESCRIPTION:
        g_free (self->description);
        self->description = g_strdup (g_value_get_string (value));
        break;
    case PROP_DEVMODE:
        self->devmode = g_value_get_boolean (value);
        break;
    case PROP_DONATION:
        g_clear_pointer(&self->donation, g_ptr_array_unref);
        if (g_value_get_boxed(value))
            self->donation = g_ptr_array_ref(g_value_get_boxed(value));
        break;
    case PROP_DOWNLOAD_SIZE:
        self->download_size = g_value_get_int64 (value);
        break;
    case PROP_HOLD:
        g_clear_pointer (&self->hold, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            self->hold = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_PROCEED_TIME:
        g_clear_pointer (&self->proceed_time, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            self->proceed_time = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_ICON:
        g_free (self->icon);
        self->icon = g_strdup (g_value_get_string (value));
        break;
    case PROP_ID:
        g_free (self->id);
        self->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_INSTALL_DATE:
        g_clear_pointer (&self->install_date, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            self->install_date = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_INSTALLED_SIZE:
        self->installed_size = g_value_get_int64 (value);
        break;
    case PROP_ISSUES:
        g_clear_pointer(&self->issues, g_ptr_array_unref);
        if (g_value_get_boxed(value))
            self->issues = g_ptr_array_ref(g_value_get_boxed(value));
        break;
    case PROP_JAILMODE:
        self->jailmode = g_value_get_boolean (value);
        break;
    case PROP_MOUNTED_FROM:
        g_free (self->mounted_from);
        self->mounted_from = g_strdup (g_value_get_string (value));
        break;
    case PROP_MEDIA:
        g_clear_pointer (&self->media, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->media = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_NAME:
        g_free (self->name);
        self->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_PRICES:
        g_clear_pointer (&self->prices, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->prices = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_PRIVATE:
        self->private = g_value_get_boolean (value);
        break;
    case PROP_PUBLISHER_DISPLAY_NAME:
        g_free (self->publisher_display_name);
        self->publisher_display_name = g_strdup (g_value_get_string (value));
        break;
    case PROP_PUBLISHER_ID:
        g_free (self->publisher_id);
        self->publisher_id = g_strdup (g_value_get_string (value));
        break;
    case PROP_PUBLISHER_USERNAME:
    case PROP_DEVELOPER:
        g_free (self->publisher_username);
        self->publisher_username = g_strdup (g_value_get_string (value));
        break;
    case PROP_PUBLISHER_VALIDATION:
        self->publisher_validation = g_value_get_enum (value);
        break;
    case PROP_REVISION:
        g_free (self->revision);
        self->revision = g_strdup (g_value_get_string (value));
        break;
    case PROP_SCREENSHOTS:
        g_clear_pointer (&self->screenshots, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            self->screenshots = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    case PROP_SNAP_TYPE:
        self->snap_type = g_value_get_enum (value);
        break;
    case PROP_SOURCE_CODE:
        g_clear_pointer(&self->source_code, g_ptr_array_unref);
        if (g_value_get_boxed(value))
            self->source_code = g_ptr_array_ref(g_value_get_boxed(value));
        break;
    case PROP_STATUS:
        self->status = g_value_get_enum (value);
        break;
    case PROP_STORE_URL:
        g_free (self->store_url);
        self->store_url = g_strdup (g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        g_free (self->summary);
        self->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_TITLE:
        g_free (self->title);
        self->title = g_strdup (g_value_get_string (value));
        break;
    case PROP_TRACKING_CHANNEL:
        g_free (self->tracking_channel);
        self->tracking_channel = g_strdup (g_value_get_string (value));
        break;
    case PROP_TRACKS:
        g_strfreev (self->tracks);
        self->tracks = g_strdupv (g_value_get_boxed (value));
        break;
    case PROP_TRYMODE:
        self->trymode = g_value_get_boolean (value);
        break;
    case PROP_VERSION:
        g_free (self->version);
        self->version = g_strdup (g_value_get_string (value));
        break;
    case PROP_LICENSE:
        g_free (self->license);
        self->license = g_strdup (g_value_get_string (value));
        break;
    case PROP_COMMON_IDS:
        g_strfreev (self->common_ids);
        self->common_ids = g_strdupv (g_value_get_boxed (value));
        break;
    case PROP_WEBSITE:
        g_clear_pointer(&self->website, g_ptr_array_unref);
        if (g_value_get_boxed(value))
            self->website = g_ptr_array_ref(g_value_get_boxed(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snap_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSnap *self = SNAPD_SNAP (object);

    switch (prop_id) {
    case PROP_APPS:
        g_value_set_boxed (value, self->apps);
        break;
    case PROP_BASE:
        g_value_set_string (value, self->base);
        break;
    case PROP_BROKEN:
        g_value_set_string (value, self->broken);
        break;
    case PROP_CATEGORIES:
        g_value_set_boxed (value, self->categories);
        break;
    case PROP_CHANNEL:
        g_value_set_string (value, self->channel);
        break;
    case PROP_CHANNELS:
        g_value_set_boxed (value, self->channels);
        break;
    case PROP_CONFINEMENT:
        g_value_set_enum (value, self->confinement);
        break;
    case PROP_CONTACT:
        g_value_set_string (value, self->contact);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, self->description);
        break;
    case PROP_DEVMODE:
        g_value_set_boolean (value, self->devmode);
        break;
    case PROP_DONATION:
        g_value_set_boxed (value, self->donation);
        break;
    case PROP_DOWNLOAD_SIZE:
        g_value_set_int64 (value, self->download_size);
        break;
    case PROP_HOLD:
        g_value_set_boxed (value, self->hold);
        break;
    case PROP_PROCEED_TIME:
        g_value_set_boxed (value, self->proceed_time);
        break;
    case PROP_ICON:
        g_value_set_string (value, self->icon);
        break;
    case PROP_ID:
        g_value_set_string (value, self->id);
        break;
    case PROP_INSTALL_DATE:
        g_value_set_boxed (value, self->install_date);
        break;
    case PROP_INSTALLED_SIZE:
        g_value_set_int64 (value, self->installed_size);
        break;
    case PROP_ISSUES:
        g_value_set_boxed (value, self->issues);
        break;
    case PROP_JAILMODE:
        g_value_set_boolean (value, self->jailmode);
        break;
    case PROP_MEDIA:
        g_value_set_boxed (value, self->media);
        break;
    case PROP_MOUNTED_FROM:
        g_value_set_string (value, self->mounted_from);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_PRICES:
        g_value_set_boxed (value, self->prices);
        break;
    case PROP_PRIVATE:
        g_value_set_boolean (value, self->private);
        break;
    case PROP_PUBLISHER_DISPLAY_NAME:
        g_value_set_string (value, self->publisher_display_name);
        break;
    case PROP_PUBLISHER_ID:
        g_value_set_string (value, self->publisher_id);
        break;
    case PROP_PUBLISHER_USERNAME:
    case PROP_DEVELOPER:
        g_value_set_string (value, self->publisher_username);
        break;
    case PROP_PUBLISHER_VALIDATION:
        g_value_set_enum (value, self->publisher_validation);
        break;
    case PROP_REVISION:
        g_value_set_string (value, self->revision);
        break;
    case PROP_SCREENSHOTS:
        g_value_set_boxed (value, self->screenshots);
        break;
    case PROP_SNAP_TYPE:
        g_value_set_enum (value, self->snap_type);
        break;
    case PROP_SOURCE_CODE:
        g_value_set_boxed (value, self->source_code);
        break;
    case PROP_STATUS:
        g_value_set_enum (value, self->status);
        break;
    case PROP_STORE_URL:
        g_value_set_string (value, self->store_url);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, self->summary);
        break;
    case PROP_TITLE:
        g_value_set_string (value, self->title);
        break;
    case PROP_TRACKING_CHANNEL:
        g_value_set_string (value, self->tracking_channel);
        break;
    case PROP_TRACKS:
        g_value_set_boxed (value, self->tracks);
        break;
    case PROP_TRYMODE:
        g_value_set_boolean (value, self->trymode);
        break;
    case PROP_VERSION:
        g_value_set_string (value, self->version);
        break;
    case PROP_LICENSE:
        g_value_set_string (value, self->license);
        break;
    case PROP_COMMON_IDS:
        g_value_set_boxed (value, self->common_ids);
        break;
    case PROP_WEBSITE:
        g_value_set_boxed (value, self->website);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snap_finalize (GObject *object)
{
    SnapdSnap *self = SNAPD_SNAP (object);

    g_clear_pointer (&self->apps, g_ptr_array_unref);
    g_clear_pointer (&self->base, g_free);
    g_clear_pointer (&self->broken, g_free);
    g_clear_pointer (&self->categories, g_ptr_array_unref);
    g_clear_pointer (&self->channel, g_free);
    g_clear_pointer (&self->channels, g_ptr_array_unref);
    g_clear_pointer (&self->common_ids, g_strfreev);
    g_clear_pointer (&self->contact, g_free);
    g_clear_pointer (&self->description, g_free);
    g_clear_pointer (&self->donation, g_ptr_array_unref);
    g_clear_pointer (&self->hold, g_date_time_unref);
    g_clear_pointer (&self->proceed_time, g_date_time_unref);
    g_clear_pointer (&self->icon, g_free);
    g_clear_pointer (&self->id, g_free);
    g_clear_pointer (&self->install_date, g_date_time_unref);
    g_clear_pointer (&self->issues, g_ptr_array_unref);
    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->license, g_free);
    g_clear_pointer (&self->media, g_ptr_array_unref);
    g_clear_pointer (&self->mounted_from, g_free);
    g_clear_pointer (&self->prices, g_ptr_array_unref);
    g_clear_pointer (&self->publisher_display_name, g_free);
    g_clear_pointer (&self->publisher_id, g_free);
    g_clear_pointer (&self->publisher_username, g_free);
    g_clear_pointer (&self->revision, g_free);
    g_clear_pointer (&self->screenshots, g_ptr_array_unref);
    g_clear_pointer (&self->source_code, g_ptr_array_unref);
    g_clear_pointer (&self->store_url, g_free);
    g_clear_pointer (&self->summary, g_free);
    g_clear_pointer (&self->title, g_free);
    g_clear_pointer (&self->tracking_channel, g_free);
    g_clear_pointer (&self->tracks, g_strfreev);
    g_clear_pointer (&self->version, g_free);
    g_clear_pointer (&self->website, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_snap_parent_class)->finalize (object);
}

static void
snapd_snap_class_init (SnapdSnapClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_snap_set_property;
    gobject_class->get_property = snapd_snap_get_property;
    gobject_class->finalize = snapd_snap_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_APPS,
                                     g_param_spec_boxed ("apps",
                                                         "apps",
                                                         "Apps this snap contains",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CATEGORIES,
                                     g_param_spec_boxed ("categories",
                                                         "categories",
                                                         "Categories this snap belongs to",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_BASE,
                                     g_param_spec_string ("base",
                                                          "base",
                                                          "Base snap this snap uses",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_BROKEN,
                                     g_param_spec_string ("broken",
                                                          "broken",
                                                          "Error string if snap is broken",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CHANNEL,
                                     g_param_spec_string ("channel",
                                                          "channel",
                                                          "Channel the snap is from",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CHANNELS,
                                     g_param_spec_boxed ("channels",
                                                         "channels",
                                                         "Channels this snap is available on",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_COMMON_IDS,
                                     g_param_spec_boxed ("common-ids",
                                                         "common-ids",
                                                         "Common IDs",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CONFINEMENT,
                                     g_param_spec_enum ("confinement",
                                                        "confinement",
                                                        "Confinement requested by the snap",
                                                        SNAPD_TYPE_CONFINEMENT, SNAPD_CONFINEMENT_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CONTACT,
                                     g_param_spec_string ("contact",
                                                          "contact",
                                                          "Method of contacting developer",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DESCRIPTION,
                                     g_param_spec_string ("description",
                                                          "description",
                                                          "Description of the snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DEVELOPER,
                                     g_param_spec_string ("developer",
                                                          "developer",
                                                          "Developer who created the snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_DEPRECATED));
    g_object_class_install_property (gobject_class,
                                     PROP_DEVMODE,
                                     g_param_spec_boolean ("devmode",
                                                           "devmode",
                                                           "TRUE if the snap is currently installed in devmode",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DONATION,
                                     g_param_spec_boxed ("donation",
                                                          "donation",
                                                          "Donation links for a Snap",
                                                          G_TYPE_PTR_ARRAY,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DOWNLOAD_SIZE,
                                     g_param_spec_int64 ("download-size",
                                                         "download-size",
                                                         "Download size in bytes",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_HOLD,
                                     g_param_spec_boxed ("hold",
                                                         "hold",
                                                         "Date this snap will re-enable automatic refreshing",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PROCEED_TIME,
                                     g_param_spec_boxed ("proceed-time",
                                                         "proceed-time",
                                                         "Describes time after which a refresh is forced for a running snap in the next auto-refresh.",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ICON,
                                     g_param_spec_string ("icon",
                                                          "icon",
                                                          "URL to the snap icon",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          "id",
                                                          "Unique ID for this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INSTALL_DATE,
                                     g_param_spec_boxed ("install-date",
                                                         "install-date",
                                                         "Date this snap was installed",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INSTALLED_SIZE,
                                     g_param_spec_int64 ("installed-size",
                                                         "installed-size",
                                                         "Installed size in bytes",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ISSUES,
                                     g_param_spec_boxed ("issues",
                                                          "issues",
                                                          "Issues for a Snap",
                                                          G_TYPE_PTR_ARRAY,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_JAILMODE,
                                     g_param_spec_boolean ("jailmode",
                                                           "jailmode",
                                                           "TRUE if the snap is currently installed in jailmode",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LICENSE,
                                     g_param_spec_string ("license",
                                                          "license",
                                                          "The snap license as an SPDX expression",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_MEDIA,
                                     g_param_spec_boxed ("media",
                                                         "media",
                                                         "Media associated with this snap",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_MOUNTED_FROM,
                                     g_param_spec_string ("mounted-from",
                                                          "mounted-from",
                                                          "Path snap is mounted from",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "The snap name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PRICES,
                                     g_param_spec_boxed ("prices",
                                                         "prices",
                                                         "Prices this snap can be purchased for",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PRIVATE,
                                     g_param_spec_boolean ("private",
                                                           "private",
                                                           "TRUE if this snap is only available to its author",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PUBLISHER_DISPLAY_NAME,
                                     g_param_spec_string ("publisher-display-name",
                                                          "publisher-display-name",
                                                          "Display name for snap publisher",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PUBLISHER_ID,
                                     g_param_spec_string ("publisher-id",
                                                          "publisher-id",
                                                          "ID for snap publisher",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PUBLISHER_USERNAME,
                                     g_param_spec_string ("publisher-username",
                                                          "publisher-username",
                                                          "Username for snap publisher",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PUBLISHER_VALIDATION,
                                     g_param_spec_enum ("publisher-validation",
                                                        "publisher-validation",
                                                        "Validation for snap publisher",
                                                        SNAPD_TYPE_PUBLISHER_VALIDATION, SNAPD_PUBLISHER_VALIDATION_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_REVISION,
                                     g_param_spec_string ("revision",
                                                          "revision",
                                                          "Revision of this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SCREENSHOTS,
                                     g_param_spec_boxed ("screenshots",
                                                         "screenshots",
                                                         "Screenshots of this snap",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SOURCE_CODE,
                                     g_param_spec_boxed ("source-code",
                                                          "source-code",
                                                          "Source Code URL for a Snap",
                                                          G_TYPE_PTR_ARRAY,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STATUS,
                                     g_param_spec_enum ("status",
                                                        "status",
                                                        "State of this snap",
                                                        SNAPD_TYPE_SNAP_STATUS, SNAPD_SNAP_STATUS_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary",
                                                          "summary",
                                                          "One line description",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STORE_URL,
                                     g_param_spec_string ("store-url",
                                                          "store-url",
                                                          "Web store URL",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TITLE,
                                     g_param_spec_string ("title",
                                                          "title",
                                                          "The snap title",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TRACKING_CHANNEL,
                                     g_param_spec_string ("tracking-channel",
                                                          "tracking-channel",
                                                          "Channel the snap is currently tracking",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TRACKS,
                                     g_param_spec_boxed ("tracks",
                                                         "tracks",
                                                         "Track names",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TRYMODE,
                                     g_param_spec_boolean ("trymode",
                                                           "trymode",
                                                           "TRUE if this snap is installed in try mode",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP_TYPE,
                                     g_param_spec_enum ("snap-type",
                                                        "snap-type",
                                                        "Snap type",
                                                        SNAPD_TYPE_SNAP_TYPE, SNAPD_SNAP_TYPE_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_VERSION,
                                     g_param_spec_string ("version",
                                                          "version",
                                                          "Snap version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_WEBSITE,
                                     g_param_spec_boxed ("website",
                                                          "website",
                                                          "Websites of the developer",
                                                          G_TYPE_PTR_ARRAY,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_snap_init (SnapdSnap *self)
{
}
