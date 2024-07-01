/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-notice.h"

/**
 * SECTION: snapd-notice
 * @short_description: Notices element
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdNotice contains information about a notice that is notified.
 */

/**
 * SnapdNotice:
 *
 * #SnapdNotice contains information on a notification element.
 *
 * Since: 1.65
 */

enum
{
    PROP_ID = 1,
    PROP_USER_ID,
    PROP_TYPE,
    PROP_KEY,
    PROP_FIRST_OCCURRED,
    PROP_LAST_OCCURRED,
    PROP_LAST_OCCURRED_NANO,
    PROP_LAST_REPEATED,
    PROP_OCCURRENCES,
    PROP_LAST_DATA,
    PROP_REPEAT_AFTER,
    PROP_EXPIRE_AFTER,
    PROP_LAST
};

struct _SnapdNotice
{
    GObject parent_instance;

    gchar *id;
    gchar *userid;
    SnapdNoticeType type;
    gchar *key;
    GDateTime *first_occurred;
    GDateTime *last_occurred;
    // last_ocurred_internal contains the same date/time, but without
    // microseconds. Useful to speedup comparisons.
    GDateTime *last_occurred_internal;
    gint32 last_occurred_nanosecond;
    GDateTime *last_repeated;
    GHashTable *data;
    gint64 occurrences;
    GTimeSpan repeat_after;
    GTimeSpan expire_after;
};


G_DEFINE_TYPE (SnapdNotice, snapd_notice, G_TYPE_OBJECT)

/**
 * snapd_notice_get_id:
 * @notice: a #SnapdNotice.
 *
 * Get the unique ID for this notice.
 *
 * Returns: an ID.
 *
 * Since: 1.65
 */
const gchar *
snapd_notice_get_id (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    return self->id;
}

/**
 * snapd_notice_get_user_id:
 * @notice: a #SnapdNotice.
 *
 * Get the user ID for this notice, or NULL if no user is defined
 *
 * Returns: an user ID.
 *
 * Since: 1.65
 */
const gchar *
snapd_notice_get_user_id (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    return self->userid;
}

/**
 * snapd_notice_get_notice_type:
 * @notice: a #SnapdNotice.
 *
 * Gets the type of notice this is.
 *
 * Returns: the type of notice.
 *
 * Since: 1.65
 */
SnapdNoticeType
snapd_notice_get_notice_type (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), SNAPD_NOTICE_TYPE_UNKNOWN);
    return self->type;
}

/**
 * snapd_notice_get_key:
 * @notice: a #SnapdNotice.
 *
 * Get the notice-id or the instance-name, depending on the type.
 *
 * Returns: a string with the key.
 *
 * Since: 1.65
 */
const gchar *
snapd_notice_get_key (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    return self->key;
}

/**
 * snapd_notice_get_last_data:
 * @notice: a #SnapdNotice.
 *
 * Get the data of the notice.
 *
 * Returns: (transfer container): a HashTable with the data elements.
 *
 * Since: 1.65
 */
GHashTable *
snapd_notice_get_last_data (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    return g_hash_table_ref (self->data);
}

/**
 * snapd_notice_get_first_occurred:
 * @notice: a #SnapdNotice.
 *
 * Get the time this notification first occurred.
 *
 * Returns: (transfer full): a #GDateTime.
 *
 * Since: 1.65
 */
GDateTime *
snapd_notice_get_first_occurred (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    if (self->first_occurred == NULL)
        return NULL;
    return g_date_time_ref (self->first_occurred);
}

/**
 * snapd_notice_get_last_occurred:
 * @notice: a #SnapdNotice.
 *
 * Get the time this notification last occurred.
 *
 * Returns: (transfer full): a #GDateTime.
 *
 * Since: 1.65
 */
GDateTime *
snapd_notice_get_last_occurred (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    if (self->last_occurred == NULL)
        return NULL;
    return g_date_time_ref (self->last_occurred);
}

/**
 * snapd_notice_get_last_occurred_nanoseconds:
 * @notice: a #SnapdNotice.
 *
 * Get the nanoseconds value of *last-occurred*, exactly as sent by
 * snapd. Useful when combined with #snapd_client_notices_set_since_nanoseconds,
 * and used internally by snapd_client_notices_set_after_notice, to ensure the maximum
 * possible precission when dealing with timestamps.
 *
 * Returns: a gint32 with the nanosecond value between 0 and 999999999, or -1 if no
 * nanosecond value was set.
 *
 * Since: 1.66
 */
gint32
snapd_notice_get_last_occurred_nanoseconds (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), 0);
    return self->last_occurred_nanosecond;
}

/**
 * snapd_notice_get_last_repeated:
 * @notice: a #SnapdNotice.
 *
 * Get the time this notification last repeated.
 *
 * Returns: (transfer full): a #GDateTime.
 *
 * Since: 1.65
 */
GDateTime *
snapd_notice_get_last_repeated (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), NULL);
    if (self->last_repeated == NULL)
        return NULL;
    return g_date_time_ref (self->last_repeated);
}

/**
 * snapd_notice_get_occurrences:
 * @notice: a #SnapdNotice.
 *
 * Get the number of times that this notification has been triggered.
 *
 * Returns: a count.
 *
 * Since: 1.65
 */
gint64
snapd_notice_get_occurrences (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), 0);
    return self->occurrences;
}

/**
 * snapd_notice_get_repeat_after:
 * @notice: a #SnapdNotice.
 *
 * Get the time interval after this notification can be repeated.
 *
 * Returns: a #GTimeSpan.
 *
 * Since: 1.65
 */
GTimeSpan
snapd_notice_get_repeat_after (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), 0);
    return self->repeat_after;
}

/**
 * snapd_notice_get_expire_after:
 * @notice: a #SnapdNotice.
 *
 * Get the time interval after this notification can expire.
 *
 * Returns: a #GTimeSpan.
 *
 * Since: 1.65
 */
GTimeSpan
snapd_notice_get_expire_after (SnapdNotice *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), 0);
    return self->expire_after;
}

/**
 * snapd_notice_compare_last_occurred:
 * @notice: a #SnapdNotice.
 * @notice_to_compare: another #SnapdNotice.
 *
 * Compare the last_occurred fields (and the last_occurred_nanosecond if available)
 * of both notices, and returns whether the first one is before, same or after the
 * second one.
 *
 * Returns: -1 if the first one is before the second one; 0 if both are the same
 * time instant, and 1 if the first one is after the second one.
 *
 * Since: 1.66
 */
gint
snapd_notice_compare_last_occurred (SnapdNotice *self, SnapdNotice *notice_to_compare)
{
    g_return_val_if_fail (SNAPD_IS_NOTICE (self), 0);
    g_return_val_if_fail (SNAPD_IS_NOTICE (notice_to_compare), 0);

    // first, compare without micro/nanoseconds. This is a must to avoid errors
    // due to rounding, because the g_date_time objects are created using a gdouble.
    gint c = g_date_time_compare (self->last_occurred_internal,
                                  notice_to_compare->last_occurred_internal);
    if (c != 0)
        return c;

    // now take into account the nanoseconds if available
    gint32 self_nano = self->last_occurred_nanosecond;
    if (self_nano == -1)
        self_nano = 1000 * g_date_time_get_microsecond (self->last_occurred);
    gint32 notice_nano = notice_to_compare->last_occurred_nanosecond;
    if (notice_nano == -1)
        notice_nano = 1000 * g_date_time_get_microsecond (notice_to_compare->last_occurred);
    if (self_nano < notice_nano)
        return -1;
    if (self_nano == notice_nano)
        return 0;
    return 1;
}

static void
snapd_notice_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdNotice *self = SNAPD_NOTICE (object);
    GDateTime *tmpdate;

    switch (prop_id) {
    case PROP_ID:
        g_free (self->id);
        self->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_USER_ID:
        g_free (self->userid);
        self->userid = g_strdup (g_value_get_string (value));
        break;
    case PROP_TYPE:
        self->type = g_value_get_uint (value);
        break;
    case PROP_KEY:
        g_free (self->key);
        self->key = g_strdup (g_value_get_string (value));
        break;
    case PROP_FIRST_OCCURRED:
        g_clear_pointer (&self->first_occurred, g_date_time_unref);
        tmpdate = g_value_get_boxed (value);
        self->first_occurred = tmpdate == NULL ? NULL : g_date_time_ref (tmpdate);
        break;
    case PROP_LAST_OCCURRED:
        g_clear_pointer (&self->last_occurred, g_date_time_unref);
        g_clear_pointer (&self->last_occurred_internal, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL) {
            self->last_occurred = g_date_time_ref (g_value_get_boxed (value));
            // store the same date/time, but without microseconds
            self->last_occurred_internal = g_date_time_add (self->last_occurred,
                                                            -g_date_time_get_microsecond (self->last_occurred));
        }
        break;
    case PROP_LAST_OCCURRED_NANO:
        self->last_occurred_nanosecond = g_value_get_int (value);
        break;
    case PROP_LAST_REPEATED:
        g_clear_pointer (&self->last_repeated, g_date_time_unref);
        if (g_value_get_boxed (value) != NULL)
            self->last_repeated = g_date_time_ref (g_value_get_boxed (value));
        break;
    case PROP_OCCURRENCES:
        self->occurrences = g_value_get_int64 (value);
        break;
    case PROP_LAST_DATA:
        g_clear_pointer (&self->data, g_hash_table_unref);
        if (g_value_get_boxed (value) != NULL)
            self->data = g_hash_table_ref (g_value_get_boxed (value));
        else
            self->data = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
        break;
    case PROP_EXPIRE_AFTER:
        self->expire_after = g_value_get_int64 (value);
        break;
    case PROP_REPEAT_AFTER:
        self->repeat_after = g_value_get_int64 (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_notice_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdNotice *self = SNAPD_NOTICE (object);

    switch (prop_id) {
    case PROP_ID:
        g_value_set_string (value, self->id);
        break;
    case PROP_USER_ID:
        g_value_set_string (value, self->userid);
        break;
    case PROP_TYPE:
        g_value_set_uint (value, self->type);
        break;
    case PROP_KEY:
        g_value_set_string (value, self->key);
        break;
    case PROP_FIRST_OCCURRED:
        g_value_set_boxed (value, self->first_occurred);
        break;
    case PROP_LAST_OCCURRED:
        g_value_set_boxed (value, self->last_occurred);
        break;
    case PROP_LAST_OCCURRED_NANO:
        g_value_set_int (value, self->last_occurred_nanosecond);
        break;
    case PROP_LAST_REPEATED:
        g_value_set_boxed (value, self->last_repeated);
        break;
    case PROP_OCCURRENCES:
        g_value_set_int (value, self->occurrences);
        break;
    case PROP_LAST_DATA:
        g_value_set_boxed (value, self->data);
        break;
    case PROP_EXPIRE_AFTER:
        g_value_set_int64 (value, self->expire_after);
        break;
    case PROP_REPEAT_AFTER:
        g_value_set_int64 (value, self->repeat_after);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_notice_finalize (GObject *object)
{
    SnapdNotice *self = SNAPD_NOTICE (object);

    g_clear_pointer (&self->id, g_free);
    g_clear_pointer (&self->userid, g_free);
    g_clear_pointer (&self->key, g_free);
    g_clear_pointer (&self->first_occurred, g_date_time_unref);
    g_clear_pointer (&self->last_occurred, g_date_time_unref);
    g_clear_pointer (&self->last_occurred_internal, g_date_time_unref);
    g_clear_pointer (&self->last_repeated, g_date_time_unref);
    g_clear_pointer (&self->data, g_hash_table_unref);

    G_OBJECT_CLASS (snapd_notice_parent_class)->finalize (object);
}

static void
snapd_notice_class_init (SnapdNoticeClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_notice_set_property;
    gobject_class->get_property = snapd_notice_get_property;
    gobject_class->finalize = snapd_notice_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          "id",
                                                          "ID of notice",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_USER_ID,
                                     g_param_spec_string ("user-id",
                                                          "user-id",
                                                          "UserID of the user who may view this notice (NULL means all users)",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TYPE,
                                     g_param_spec_uint ("notice-type",
                                                        "notice-type",
                                                        "Type of notice",
                                                        0, G_MAXUINT, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_KEY,
                                     g_param_spec_string ("key",
                                                          "key",
                                                          "Key of notice",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_FIRST_OCCURRED,
                                     g_param_spec_boxed ("first-occurred",
                                                         "first-occurred",
                                                         "Time this notice first occurred",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LAST_OCCURRED,
                                     g_param_spec_boxed ("last-occurred",
                                                         "last-occurred",
                                                         "Time this notice last occurred",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LAST_OCCURRED_NANO,
                                     g_param_spec_int ("last-occurred-nanoseconds",
                                                       "last-occurred-nanoseconds",
                                                       "Time this notice last ocurred, in string format and with nanosecond accuracy",
                                                       -1, 999999999, -1,
                                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LAST_REPEATED,
                                     g_param_spec_boxed ("last-repeated",
                                                         "last-repeated",
                                                         "Time this notice was last repeated",
                                                         G_TYPE_DATE_TIME,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_OCCURRENCES,
                                     g_param_spec_int64 ("occurrences",
                                                         "occurrences",
                                                         "Number of time one of these notices has occurred",
                                                         G_MININT, G_MAXINT, -1,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LAST_DATA,
                                     g_param_spec_boxed ("last-data",
                                                         "last-data",
                                                         "Data for this notice",
                                                         G_TYPE_HASH_TABLE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_REPEAT_AFTER,
                                     g_param_spec_int64 ("repeat-after",
                                                          "repeat-after",
                                                          "Time (in ms) after one of these was last repeated should we allow it to repeat",
                                                          G_MININT64, G_MAXINT64, 0,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_EXPIRE_AFTER,
                                     g_param_spec_int64 ("expire-after",
                                                         "expire-after",
                                                         "Time (in ms) since one of these last occurred until we should drop the notice",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_notice_init (SnapdNotice *self)
{
    self->data = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}
