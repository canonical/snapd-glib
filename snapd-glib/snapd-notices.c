/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-notices.h"

/**
 * SECTION: snapd-notices
 * @short_description: Notices container
 * @include: snapd-glib/snapd-glib.h
 *
 * #SnapdNotices contains a list of received notices.
 */

/**
 * SnapdNotices:
 *
 * #SnapdNotices contains a list of received notices.
 *
 * Since: 1.65
 */

struct _SnapdNotices
{
    GObject parent_instance;

    GPtrArray *notices;
};


G_DEFINE_TYPE (SnapdNotices, snapd_notices, G_TYPE_OBJECT)

/**
 * snapd_notices_get_n_notices:
 * @notices: a #SnapdNotices.
 *
 * Get the number of notices.
 *
 * Returns: a guint64 with the number of notices.
 *
 * Since: 1.65
 */
guint64
snapd_notices_get_n_notices (SnapdNotices *self)
{
    g_return_val_if_fail (SNAPD_IS_NOTICES (self), 0);
    return self->notices->len;
}

/**
 * snapd_notice_get_notice:
 * @notice: a #SnapdNotices.
 * @notice_number: the #SnapdNotice number to get
 *
 * Get a specific #SnapdNotice from the list.
 *
 * Returns: the requested #SnapdNotice, or NULL if there was an error.
 *
 * Since: 1.65
 */
SnapdNotice *
snapd_notices_get_notice (SnapdNotices *self, guint64 notice_number)
{
    g_return_val_if_fail (SNAPD_IS_NOTICES (self), NULL);
    if ((self->notices == NULL) || (notice >= self->notices->len))
        return NULL;
    return self->notices->pdata[notice];
}


static void
snapd_notices_finalize (GObject *object)
{
    SnapdNotices *self = SNAPD_NOTICES (object);

    g_clear_pointer (&self->notices, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_notices_parent_class)->finalize (object);
}

static void
snapd_notices_class_init (SnapdNoticesClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = snapd_notices_finalize;
}

static void
snapd_notices_init (SnapdNotices *self)
{
}

SnapdNotices *
snapd_notices_new (GPtrArray *data)
{
    g_return_val_if_fail (data != NULL, NULL);

    SnapdNotices *self = SNAPD_NOTICES (g_object_new (snapd_notices_get_type (), NULL));
    self->notices = g_ptr_array_ref (data);
    return self;
}