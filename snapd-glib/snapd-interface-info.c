/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-interface-info.h"

/**
 * SECTION: snapd-interface-info
 * @short_description: Snap interface info
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdInterfaceInfo represents information about a particular
 * interface type and the related plugs and slots provided by snaps on
 * the system.
 *
 * Available interfaces can be queried using
 * snapd_client_get_interfaces2_sync().
 */

/**
 * SnapdInterfaceInfo:
 *
 * #SnapdInterfaceInfo contains information about a Snap interface.
 *
 * Since: 1.42
 */

struct _SnapdInterfaceInfo
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

G_DEFINE_TYPE (SnapdInterfaceInfo, snapd_interface_info, G_TYPE_OBJECT);

/**
 * snapd_interface_info_get_name:
 * @interface_info: a #SnapdInteraceInfo
 *
 * Get the name of this interface.
 *
 * Returns: a name.
 *
 * Since: 1.42
 */
const gchar *
snapd_interface_info_get_name (SnapdInterfaceInfo *interface_info)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE_INFO (interface_info), NULL);
    return interface_info->name;
}

/**
 * snapd_interface_info_get_summary:
 * @interface_info: a #SnapdInteraceInfo
 *
 * Get the summary of this interface.
 *
 * Returns: a summary.
 *
 * Since: 1.42
 */
const gchar *
snapd_interface_info_get_summary (SnapdInterfaceInfo *interface_info)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE_INFO (interface_info), NULL);
    return interface_info->summary;
}

/**
 * snapd_interface_info_get_doc_url:
 * @interface_info: a #SnapdInteraceInfo
 *
 * Get the documentation URL of this interface.
 *
 * Returns: a URL.
 *
 * Since: 1.42
 */

const gchar *
snapd_interface_info_get_doc_url (SnapdInterfaceInfo *interface_info)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE_INFO (interface_info), NULL);
    return interface_info->doc_url;
}

/**
 * snapd_interface_info_get_plugs:
 * @interface_info: a #SnapdInteraceInfo
 *
 * Get the plugs matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdPlug): an array of #SnapdPlug.
 *
 * Since: 1.42
 */
GPtrArray *
snapd_interface_info_get_plugs (SnapdInterfaceInfo *interface_info)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE_INFO (interface_info), NULL);
    return interface_info->plugs;
}

/**
 * snapd_interface_info_get_slots:
 * @interface_info: a #SnapdInteraceInfo
 *
 * Get the slots matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdSlot): an array of #SnapdSlot.
 *
 * Since: 1.42
 */
GPtrArray *
snapd_interface_info_get_slots (SnapdInterfaceInfo *interface_info)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACE_INFO (interface_info), NULL);
    return interface_info->slots;
}

static void
snapd_interface_info_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
    SnapdInterfaceInfo *interface_info = SNAPD_INTERFACE_INFO (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (interface_info->name);
        interface_info->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SUMMARY:
        g_free (interface_info->summary);
        interface_info->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_DOC_URL:
        g_free (interface_info->doc_url);
        interface_info->doc_url = g_strdup (g_value_get_string (value));
        break;
    case PROP_PLUGS:
        g_clear_pointer (&interface_info->plugs, g_ptr_array_unref);
        interface_info->plugs = g_value_dup_boxed (value);
        break;
    case PROP_SLOTS:
        g_clear_pointer (&interface_info->slots, g_ptr_array_unref);
        interface_info->slots = g_value_dup_boxed (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_interface_info_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
    SnapdInterfaceInfo *interface_info = SNAPD_INTERFACE_INFO (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, interface_info->name);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, interface_info->summary);
        break;
    case PROP_DOC_URL:
        g_value_set_string (value, interface_info->doc_url);
        break;
    case PROP_PLUGS:
        g_value_set_boxed (value, interface_info->plugs);
        break;
    case PROP_SLOTS:
        g_value_set_boxed (value, interface_info->slots);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_interface_info_finalize (GObject *object)
{
    SnapdInterfaceInfo *interface_info = SNAPD_INTERFACE_INFO (object);

    g_clear_pointer (&interface_info->name, g_free);
    g_clear_pointer (&interface_info->summary, g_free);
    g_clear_pointer (&interface_info->doc_url, g_free);
    g_clear_pointer (&interface_info->plugs, g_ptr_array_unref);
    g_clear_pointer (&interface_info->slots, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_interface_info_parent_class)->finalize (object);
}

static void
snapd_interface_info_class_init (SnapdInterfaceInfoClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_interface_info_set_property;
    gobject_class->get_property = snapd_interface_info_get_property;
    gobject_class->finalize = snapd_interface_info_finalize;

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
snapd_interface_info_init (SnapdInterfaceInfo *interface_info)
{
}
