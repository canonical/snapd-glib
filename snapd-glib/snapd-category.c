/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-category.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-category
 * @short_description: Snap category metadata
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdCategory contains the metadata for a category membership as returned
 * using snapd_snap_get_categories().
 */

/**
 * SnapdCategory:
 *
 * #SnapdCategory is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.64
 */

struct _SnapdCategory
{
    GObject parent_instance;

    gboolean featured;
    gchar *name;
};

enum
{
    PROP_FEATURED = 1,
    PROP_NAME,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdCategory, snapd_category, G_TYPE_OBJECT)

/**
 * snapd_category_get_featured:
 * @self: a #SnapdCategory.
 *
 * Get if this snap is featured in this category.
 *
 * Returns: %TRUE if this snap is featured in this category.
 *
 * Since: 1.64
 */
gboolean
snapd_category_get_featured (SnapdCategory *self)
{
    g_return_val_if_fail (SNAPD_IS_CATEGORY (self), FALSE);
    return self->featured;
}

/**
 * snapd_category_get_name:
 * @self: a #SnapdCategory.
 *
 * Get the name of this category, e.g. "social".
 *
 * Returns: a name.
 *
 * Since: 1.64
 */
const gchar *
snapd_category_get_name (SnapdCategory *self)
{
    g_return_val_if_fail (SNAPD_IS_CATEGORY (self), NULL);
    return self->name;
}

static void
snapd_category_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdCategory *self = SNAPD_CATEGORY (object);

    switch (prop_id) {
    case PROP_FEATURED:
        self->featured = g_value_get_boolean (value);
        break;
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
snapd_category_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdCategory *self = SNAPD_CATEGORY (object);

    switch (prop_id) {
    case PROP_FEATURED:
        g_value_set_boolean (value, self->featured);
        break;
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_category_finalize (GObject *object)
{
    SnapdCategory *self = SNAPD_CATEGORY (object);

    g_clear_pointer (&self->name, g_free);

    G_OBJECT_CLASS (snapd_category_parent_class)->finalize (object);
}

static void
snapd_category_class_init (SnapdCategoryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_category_set_property;
    gobject_class->get_property = snapd_category_get_property;
    gobject_class->finalize = snapd_category_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_FEATURED,
                                     g_param_spec_boolean ("featured",
                                                           "featured",
                                                           "TRUE if this category is featured",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "The category name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_category_init (SnapdCategory *self)
{
}
