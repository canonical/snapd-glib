/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-plug-ref.h"

/**
 * SECTION: snapd-plug-ref
 * @short_description: Reference to a plug
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPlugRef contains a reference to a plug.
 */

/**
 * SnapdPlugRef:
 *
 * #SnapdPlugRef contains the state of Snap a interface plug_ref.
 *
 * Since: 1.0
 */

struct _SnapdPlugRef
{
    GObject parent_instance;

    gchar *plug;
    gchar *snap;
};

enum
{
    PROP_PLUG = 1,
    PROP_SNAP,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdPlugRef, snapd_plug_ref, G_TYPE_OBJECT)

/**
 * snapd_plug_ref_get_plug:
 * @self: a #SnapdPlugRef.
 *
 * Get the name of the plug.
 *
 * Returns: a name.
 *
 * Since: 1.48
 */
const gchar *
snapd_plug_ref_get_plug (SnapdPlugRef *self)
{
    g_return_val_if_fail (SNAPD_IS_PLUG_REF (self), NULL);
    return self->plug;
}

/**
 * snapd_plug_ref_get_snap:
 * @self: a #SnapdPlugRef.
 *
 * Get the snap this plug is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.48
 */
const gchar *
snapd_plug_ref_get_snap (SnapdPlugRef *self)
{
    g_return_val_if_fail (SNAPD_IS_PLUG_REF (self), NULL);
    return self->snap;
}

static void
snapd_plug_ref_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPlugRef *self = SNAPD_PLUG_REF (object);

    switch (prop_id) {
    case PROP_PLUG:
        g_free (self->plug);
        self->plug = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (self->snap);
        self->snap = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_ref_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPlugRef *self = SNAPD_PLUG_REF (object);

    switch (prop_id) {
    case PROP_PLUG:
        g_value_set_string (value, self->plug);
        break;
    case PROP_SNAP:
        g_value_set_string (value, self->snap);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_ref_finalize (GObject *object)
{
    SnapdPlugRef *self = SNAPD_PLUG_REF (object);

    g_clear_pointer (&self->plug, g_free);
    g_clear_pointer (&self->snap, g_free);

    G_OBJECT_CLASS (snapd_plug_ref_parent_class)->finalize (object);
}

static void
snapd_plug_ref_class_init (SnapdPlugRefClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_plug_ref_set_property;
    gobject_class->get_property = snapd_plug_ref_get_property;
    gobject_class->finalize = snapd_plug_ref_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_PLUG,
                                     g_param_spec_string ("plug",
                                                          "plug",
                                                          "Name of plug",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this plug is on",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_plug_ref_init (SnapdPlugRef *self)
{
}
