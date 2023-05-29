/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-category-details.h"

/**
 * SECTION:snapd-category-details
 * @short_description: Snap category details metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdCategoryDetails contains the metadata for a given snap category as returned
 * using snapd_client_get_categories_sync().
 */

/**
 * SnapdCategoryDetails:
 *
 * #SnapdCategoryDetails is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.64
 */

struct _SnapdCategoryDetails
{
    GObject parent_instance;

    gchar *name;
};

enum
{
    PROP_NAME = 1,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdCategoryDetails, snapd_category_details, G_TYPE_OBJECT)

/**
 * snapd_category_details_get_name:
 * @category_details: a #SnapdCategoryDetails.
 *
 * Get the name of this category, e.g. "social".
 *
 * Returns: a name.
 *
 * Since: 1.64
 */
const gchar *
snapd_category_details_get_name (SnapdCategoryDetails *self)
{
    g_return_val_if_fail (SNAPD_IS_CATEGORY_DETAILS (self), NULL);
    return self->name;
}

static void
snapd_category_details_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdCategoryDetails *self = SNAPD_CATEGORY_DETAILS (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (self->name);
        self->name = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_category_details_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdCategoryDetails *self = SNAPD_CATEGORY_DETAILS (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_category_details_finalize (GObject *object)
{
    SnapdCategoryDetails *self = SNAPD_CATEGORY_DETAILS (object);

    g_clear_pointer (&self->name, g_free);

    G_OBJECT_CLASS (snapd_category_details_parent_class)->finalize (object);
}

static void
snapd_category_details_class_init (SnapdCategoryDetailsClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_category_details_set_property;
    gobject_class->get_property = snapd_category_details_get_property;
    gobject_class->finalize = snapd_category_details_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "The category name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_category_details_init (SnapdCategoryDetails *self)
{
}
