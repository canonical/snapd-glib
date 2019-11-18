/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-interface.h"

/**
 * SECTION: snapd-interface
 * @short_description: Snap interface info
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdInterface represents information about a particular
 * interface type and the related plugs and slots provided by snaps on
 * the system.
 *
 * Available interfaces can be queried using
 * snapd_client_get_interfaces2_sync().
 */

/**
 * SnapdInterface:
 *
 * #SnapdInterface contains information about a Snap interface.
 *
 * Since: 1.48
 */

struct _SnapdInterface
{
    GObject parent_instance;

    gchar *name;
    gchar *summary;
    gchar *doc_url;
    GPtrArray *plugs;
    GPtrArray *slots;
};

enum
{
    PROP_NAME = 1,
    PROP_SUMMARY,
    PROP_DOC_URL,
    PROP_PLUGS,
    PROP_SLOTS,
};

G_DEFINE_TYPE (SnapdInterface, snapd_interface, G_TYPE_OBJECT);

/**
 * snapd_interface_get_name:
 * @interface: a #SnapdInterface
 *
 * Get the name of this interface.
 *
 * Returns: a name.
 *
 * Since: 1.48
 */
const gchar *
snapd_interface_get_name (SnapdInterface *self)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE (self), NULL);
    return self->name;
}

/**
 * snapd_interface_get_summary:
 * @interface: a #SnapdInterface
 *
 * Get the summary of this interface.
 *
 * Returns: a summary.
 *
 * Since: 1.48
 */
const gchar *
snapd_interface_get_summary (SnapdInterface *self)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE (self), NULL);
    return self->summary;
}

/**
 * snapd_interface_get_doc_url:
 * @interface: a #SnapdInterface
 *
 * Get the documentation URL of this interface.
 *
 * Returns: a URL.
 *
 * Since: 1.48
 */
const gchar *
snapd_interface_get_doc_url (SnapdInterface *self)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE (self), NULL);
    return self->doc_url;
}

/**
 * snapd_interface_get_plugs:
 * @interface: a #SnapdInterface
 *
 * Get the plugs matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdPlug): an array of #SnapdPlug.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_interface_get_plugs (SnapdInterface *self)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE (self), NULL);
    return self->plugs;
}

/**
 * snapd_interface_get_slots:
 * @interface: a #SnapdInterface
 *
 * Get the slots matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdSlot): an array of #SnapdSlot.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_interface_get_slots (SnapdInterface *self)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE (self), NULL);
    return self->slots;
}

static void
snapd_interface_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
    SnapdInterface *self = SNAPD_INTERFACE (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (self->name);
        self->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        g_free (self->summary);
        self->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_DOC_URL:
        g_free (self->doc_url);
        self->doc_url = g_strdup (g_value_get_string (value));
        break;
    case PROP_PLUGS:
        g_clear_pointer (&self->plugs, g_ptr_array_unref);
        self->plugs = g_value_dup_boxed (value);
        break;
    case PROP_SLOTS:
        g_clear_pointer (&self->slots, g_ptr_array_unref);
        self->slots = g_value_dup_boxed (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_interface_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
    SnapdInterface *self = SNAPD_INTERFACE (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, self->summary);
        break;
    case PROP_DOC_URL:
        g_value_set_string (value, self->doc_url);
        break;
    case PROP_PLUGS:
        g_value_set_boxed (value, self->plugs);
        break;
    case PROP_SLOTS:
        g_value_set_boxed (value, self->slots);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_interface_finalize (GObject *object)
{
    SnapdInterface *self = SNAPD_INTERFACE (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->summary, g_free);
    g_clear_pointer (&self->doc_url, g_free);
    g_clear_pointer (&self->plugs, g_ptr_array_unref);
    g_clear_pointer (&self->slots, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_interface_parent_class)->finalize (object);
}

static void
snapd_interface_class_init (SnapdInterfaceClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_interface_set_property;
    gobject_class->get_property = snapd_interface_get_property;
    gobject_class->finalize = snapd_interface_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Interface name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary",
                                                          "summary",
                                                          "Interface summary",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DOC_URL,
                                     g_param_spec_string ("doc-url",
                                                          "doc-url",
                                                          "Interface documentation URL",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PLUGS,
                                     g_param_spec_boxed ("plugs",
                                                         "plugs",
                                                         "Plugs of this interface type",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SLOTS,
                                     g_param_spec_boxed ("slots",
                                                         "slots",
                                                         "Slots of this interface type",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_interface_init (SnapdInterface *self)
{
}
