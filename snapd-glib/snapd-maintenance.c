/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-enum-types.h"
#include "snapd-maintenance.h"

/**
 * SECTION:snapd-maintenance
 * @short_description: Snap maintenance information
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdMaintenance contains maintenance information for the snapd daemon.
 * The current maintenance information can be retrieved using
 * snapd_client_get_maintenance ().
 */

/**
 * SnapdMaintenance:
 *
 * #SnapdMaintenance contains maintenance information.
 *
 * Since: 1.45
 */

struct _SnapdMaintenance {
  GObject parent_instance;

  SnapdMaintenanceKind kind;
  gchar *message;
};

enum { PROP_KIND = 1, PROP_MESSAGE, PROP_LAST };

G_DEFINE_TYPE(SnapdMaintenance, snapd_maintenance, G_TYPE_OBJECT)

/**
 * snapd_maintenance_get_kind:
 * @maintenance: a #SnapdMaintenance.
 *
 * Get the kind of maintenance kind, e.g.
 * %SNAPD_MAINTENANCE_KIND_DAEMON_RESTART.
 *
 * Returns: a #SnapdMaintenanceKind.
 *
 * Since: 1.45
 */
SnapdMaintenanceKind snapd_maintenance_get_kind(SnapdMaintenance *self) {
  g_return_val_if_fail(SNAPD_IS_MAINTENANCE(self),
                       SNAPD_MAINTENANCE_KIND_UNKNOWN);
  return self->kind;
}

/**
 * snapd_maintenance_get_message:
 * @maintenance: a #SnapdMaintenance.
 *
 * Get the user readable message associate with the maintenance state.
 *
 * Returns: message text.
 *
 * Since: 1.45
 */
const gchar *snapd_maintenance_get_message(SnapdMaintenance *self) {
  g_return_val_if_fail(SNAPD_IS_MAINTENANCE(self), NULL);
  return self->message;
}

static void snapd_maintenance_set_property(GObject *object, guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec) {
  SnapdMaintenance *self = SNAPD_MAINTENANCE(object);

  switch (prop_id) {
  case PROP_KIND:
    self->kind = g_value_get_enum(value);
    break;
  case PROP_MESSAGE:
    g_free(self->message);
    self->message = g_strdup(g_value_get_string(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_maintenance_get_property(GObject *object, guint prop_id,
                                           GValue *value, GParamSpec *pspec) {
  SnapdMaintenance *self = SNAPD_MAINTENANCE(object);

  switch (prop_id) {
  case PROP_KIND:
    g_value_set_enum(value, self->kind);
    break;
  case PROP_MESSAGE:
    g_value_set_boxed(value, self->message);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_maintenance_finalize(GObject *object) {
  SnapdMaintenance *self = SNAPD_MAINTENANCE(object);

  g_clear_pointer(&self->message, g_free);

  G_OBJECT_CLASS(snapd_maintenance_parent_class)->finalize(object);
}

static void snapd_maintenance_class_init(SnapdMaintenanceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_maintenance_set_property;
  gobject_class->get_property = snapd_maintenance_get_property;
  gobject_class->finalize = snapd_maintenance_finalize;

  g_object_class_install_property(
      gobject_class, PROP_KIND,
      g_param_spec_enum("kind", "kind", "Maintenance kind",
                        SNAPD_TYPE_MAINTENANCE_KIND,
                        SNAPD_MAINTENANCE_KIND_UNKNOWN,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_MESSAGE,
      g_param_spec_string("message", "message", "User readable message", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void snapd_maintenance_init(SnapdMaintenance *self) {}
