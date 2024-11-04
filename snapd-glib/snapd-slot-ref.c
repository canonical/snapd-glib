/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-slot-ref.h"

/**
 * SECTION: snapd-slot-ref
 * @short_description: Reference to a slot
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdSlotRef contains a reference to a slot.
 */

/**
 * SnapdSlotRef:
 *
 * #SnapdSlotRef contains the state of Snap a interface slot_ref.
 *
 * Since: 1.0
 */

struct _SnapdSlotRef {
  GObject parent_instance;

  gchar *slot;
  gchar *snap;
};

enum { PROP_SLOT = 1, PROP_SNAP, PROP_LAST };

G_DEFINE_TYPE(SnapdSlotRef, snapd_slot_ref, G_TYPE_OBJECT)

/**
 * snapd_slot_ref_get_slot:
 * @slot_ref: a #SnapdSlotRef.
 *
 * Get the name of the slot.
 *
 * Returns: a name.
 *
 * Since: 1.48
 */
const gchar *snapd_slot_ref_get_slot(SnapdSlotRef *self) {
  g_return_val_if_fail(SNAPD_IS_SLOT_REF(self), NULL);
  return self->slot;
}

/**
 * snapd_slot_ref_get_snap:
 * @slot_ref: a #SnapdSlotRef.
 *
 * Get the snap this slot is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.48
 */
const gchar *snapd_slot_ref_get_snap(SnapdSlotRef *self) {
  g_return_val_if_fail(SNAPD_IS_SLOT_REF(self), NULL);
  return self->snap;
}

static void snapd_slot_ref_set_property(GObject *object, guint prop_id,
                                        const GValue *value,
                                        GParamSpec *pspec) {
  SnapdSlotRef *self = SNAPD_SLOT_REF(object);

  switch (prop_id) {
  case PROP_SLOT:
    g_free(self->slot);
    self->slot = g_strdup(g_value_get_string(value));
    break;
  case PROP_SNAP:
    g_free(self->snap);
    self->snap = g_strdup(g_value_get_string(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_slot_ref_get_property(GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec) {
  SnapdSlotRef *self = SNAPD_SLOT_REF(object);

  switch (prop_id) {
  case PROP_SLOT:
    g_value_set_string(value, self->slot);
    break;
  case PROP_SNAP:
    g_value_set_string(value, self->snap);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_slot_ref_finalize(GObject *object) {
  SnapdSlotRef *self = SNAPD_SLOT_REF(object);

  g_clear_pointer(&self->slot, g_free);
  g_clear_pointer(&self->snap, g_free);

  G_OBJECT_CLASS(snapd_slot_ref_parent_class)->finalize(object);
}

static void snapd_slot_ref_class_init(SnapdSlotRefClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_slot_ref_set_property;
  gobject_class->get_property = snapd_slot_ref_get_property;
  gobject_class->finalize = snapd_slot_ref_finalize;

  g_object_class_install_property(
      gobject_class, PROP_SLOT,
      g_param_spec_string("slot", "slot", "Name of slot", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SNAP,
      g_param_spec_string("snap", "snap", "Snap this slot is on", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
}

static void snapd_slot_ref_init(SnapdSlotRef *self) {}
