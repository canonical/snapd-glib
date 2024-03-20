/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-change-data.h"

struct _SnapdChangeData {
    GObject parent_instance;
    GStrv snap_names;
    GStrv refresh_forced;
};

G_DEFINE_TYPE (SnapdChangeData, snapd_change_data, G_TYPE_OBJECT)

enum
{
    PROP_SNAP_NAMES = 1,
    PROP_REFRESH_FORCED,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/**
 * snapd_change_data_get_snap_names
 * @change_data: a #SnapdChangeData
 *
 * return: (transfer none): a GStrv with the snap names, or NULL if the property wasn't defined
 */
GStrv     snapd_change_data_get_snap_names         (SnapdChangeData *self) {
    return self->snap_names;
}

/**
 * snapd_change_data_get_refresh_forced
 * @change_data: a #SnapdChangeData
 *
 * return: (transfer none): a GStrv with the snap names, or NULL if the property wasn't defined
 */
GStrv     snapd_change_data_get_refresh_forced     (SnapdChangeData *self) {
    return self->refresh_forced;
}

static void
snapd_change_data_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdChangeData *self = SNAPD_CHANGE_DATA (object);

    switch (prop_id) {
    case PROP_SNAP_NAMES:
        g_strfreev (self->snap_names);
        self->snap_names = g_strdupv (g_value_get_boxed (value));
        break;
    case PROP_REFRESH_FORCED:
        g_strfreev (self->refresh_forced);
        self->refresh_forced = g_strdupv (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_change_data_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdChangeData *self = SNAPD_CHANGE_DATA (object);

    switch (prop_id) {
    case PROP_SNAP_NAMES:
        g_value_set_boxed (value, self->snap_names);
        break;
    case PROP_REFRESH_FORCED:
        g_value_set_boxed (value, self->refresh_forced);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_change_data_finalize (GObject *object)
{
    SnapdChangeData *self = SNAPD_CHANGE_DATA (object);

    g_clear_pointer (&self->snap_names, g_strfreev);
    g_clear_pointer (&self->refresh_forced, g_strfreev);

    G_OBJECT_CLASS (snapd_change_data_parent_class)->finalize (object);
}

static void
snapd_change_data_class_init (SnapdChangeDataClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_change_data_set_property;
    gobject_class->get_property = snapd_change_data_get_property;
    gobject_class->finalize = snapd_change_data_finalize;

    obj_properties[PROP_SNAP_NAMES] =
    g_param_spec_boxed ("snap-names",
                        "Snap Names",
                        "Names of the snaps that have been autorefreshed.",
                        G_TYPE_STRV,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    obj_properties[PROP_REFRESH_FORCED] =
    g_param_spec_boxed ("refresh-forced",
                        "Refresh forced",
                        "Names of the snaps that have been forced to autorefresh.",
                        G_TYPE_STRV,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

static void
snapd_change_data_init (SnapdChangeData *self)
{
}