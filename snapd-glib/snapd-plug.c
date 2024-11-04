/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-connection.h"
#include "snapd-plug.h"
#include "snapd-slot-ref.h"

/**
 * SECTION: snapd-plug
 * @short_description: Snap plugs
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPlug represents a part of a snap that can be connected to a
 * #SnapdSlot on another snap. Available plugs can be queried using
 * snapd_client_get_interfaces_sync(). Plugs can be connected / disconnected
 * using snapd_client_connect_interface_sync() and
 * snapd_client_disconnect_interface_sync().
 */

/**
 * SnapdPlug:
 *
 * #SnapdPlug contains information about a Snap plug.
 *
 * Since: 1.0
 */

struct _SnapdPlug {
  GObject parent_instance;

  gchar *name;
  gchar *snap;
  gchar *interface;
  GHashTable *attributes;
  gchar *label;
  GPtrArray *connections;
  GPtrArray *legacy_connections;
};

enum {
  PROP_NAME = 1,
  PROP_SNAP,
  PROP_INTERFACE,
  PROP_LABEL,
  PROP_CONNECTIONS,
  PROP_ATTRIBUTES,
  PROP_LAST
};

G_DEFINE_TYPE(SnapdPlug, snapd_plug, G_TYPE_OBJECT)

/**
 * snapd_plug_get_name:
 * @plug: a #SnapdPlug.
 *
 * Get the name of this plug.
 *
 * Returns: a name.
 *
 * Since: 1.0
 */
const gchar *snapd_plug_get_name(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return self->name;
}

/**
 * snapd_plug_get_snap:
 * @plug: a #SnapdPlug.
 *
 * Get the snap this plug is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.0
 */
const gchar *snapd_plug_get_snap(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return self->snap;
}

/**
 * snapd_plug_get_interface:
 * @plug: a #SnapdPlug.
 *
 * Get the name of the interface this plug provides.
 *
 * Returns: an interface name.
 *
 * Since: 1.0
 */
const gchar *snapd_plug_get_interface(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return self->interface;
}

/**
 * snapd_plug_get_attribute_names:
 * @plug: a #SnapdPlug.
 * @length: (out) (allow-none): location to write number of attributes or %NULL
 * if not required.
 *
 * Get the names of the attributes this plug has.
 *
 * Returns: (transfer full) (array zero-terminated=1): a string array of
 * attribute names. Free with g_strfreev().
 *
 * Since: 1.3
 */
GStrv snapd_plug_get_attribute_names(SnapdPlug *self, guint *length) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);

  guint size = g_hash_table_size(self->attributes);
  GStrv names = g_malloc(sizeof(gchar *) * (size + 1));

  GHashTableIter iter;
  g_hash_table_iter_init(&iter, self->attributes);
  guint i;
  gpointer name;
  for (i = 0; g_hash_table_iter_next(&iter, &name, NULL); i++)
    names[i] = g_strdup(name);
  names[i] = NULL;

  if (length != NULL)
    *length = size;
  return names;
}

/**
 * snapd_plug_has_attribute:
 * @plug: a #SnapdPlug.
 * @name: an attribute name.
 *
 * Check if this plug has an attribute.
 *
 * Returns: %TRUE if this attribute exists.
 *
 * Since: 1.3
 */
gboolean snapd_plug_has_attribute(SnapdPlug *self, const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), FALSE);
  return g_hash_table_contains(self->attributes, name);
}

/**
 * snapd_plug_get_attribute:
 * @plug: a #SnapdPlug.
 * @name: an attribute name.
 *
 * Get an attribute for this interface.
 *
 * Returns: (transfer none) (allow-none): an attribute value or %NULL if not
 * set.
 *
 * Since: 1.3
 */
GVariant *snapd_plug_get_attribute(SnapdPlug *self, const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return g_hash_table_lookup(self->attributes, name);
}

/**
 * snapd_plug_get_label:
 * @plug: a #SnapdPlug.
 *
 * Get a human readable label for this plug.
 *
 * Returns: a label.
 *
 * Since: 1.0
 */
const gchar *snapd_plug_get_label(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return self->label;
}

/**
 * snapd_plug_get_connections:
 * @plug: a #SnapdPlug.
 *
 * Get the connections being made with this plug.
 *
 * Returns: (transfer none) (element-type SnapdConnection): an array of
 * #SnapdConnection.
 *
 * Since: 1.0
 * Deprecated: 1.48: Use snapd_plug_get_connected_slots()
 */
GPtrArray *snapd_plug_get_connections(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);

  if (self->legacy_connections != NULL)
    return self->legacy_connections;

  self->legacy_connections = g_ptr_array_new_with_free_func(g_object_unref);
  for (int i = 0; i < self->connections->len; i++) {
    SnapdSlotRef *slot_ref = g_ptr_array_index(self->connections, i);

    SnapdConnection *connection = g_object_new(
        SNAPD_TYPE_CONNECTION, "name", snapd_slot_ref_get_slot(slot_ref),
        "snap", snapd_slot_ref_get_snap(slot_ref), NULL);
    g_ptr_array_add(self->legacy_connections, connection);
  }

  return self->legacy_connections;
}

/**
 * snapd_plug_get_connected_slots:
 * @plug: a #SnapdPlug.
 *
 * Get the slots connected to this plug.
 *
 * Returns: (transfer none) (element-type SnapdSlotRef): an array of
 * #SnapdSlotRef.
 *
 * Since: 1.48
 */
GPtrArray *snapd_plug_get_connected_slots(SnapdPlug *self) {
  g_return_val_if_fail(SNAPD_IS_PLUG(self), NULL);
  return self->connections;
}

static void snapd_plug_set_property(GObject *object, guint prop_id,
                                    const GValue *value, GParamSpec *pspec) {
  SnapdPlug *self = SNAPD_PLUG(object);

  switch (prop_id) {
  case PROP_NAME:
    g_free(self->name);
    self->name = g_strdup(g_value_get_string(value));
    break;
  case PROP_SNAP:
    g_free(self->snap);
    self->snap = g_strdup(g_value_get_string(value));
    break;
  case PROP_INTERFACE:
    g_free(self->interface);
    self->interface = g_strdup(g_value_get_string(value));
    break;
  case PROP_LABEL:
    g_free(self->label);
    self->label = g_strdup(g_value_get_string(value));
    break;
  case PROP_CONNECTIONS:
    g_clear_pointer(&self->connections, g_ptr_array_unref);
    if (g_value_get_boxed(value) != NULL)
      self->connections = g_ptr_array_ref(g_value_get_boxed(value));
    break;
  case PROP_ATTRIBUTES:
    g_clear_pointer(&self->attributes, g_hash_table_unref);
    if (g_value_get_boxed(value) != NULL)
      self->attributes = g_hash_table_ref(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_plug_get_property(GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec) {
  SnapdPlug *self = SNAPD_PLUG(object);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string(value, self->name);
    break;
  case PROP_SNAP:
    g_value_set_string(value, self->snap);
    break;
  case PROP_INTERFACE:
    g_value_set_string(value, self->interface);
    break;
  case PROP_LABEL:
    g_value_set_string(value, self->label);
    break;
  case PROP_CONNECTIONS:
    g_value_set_boxed(value, self->connections);
    break;
  case PROP_ATTRIBUTES:
    g_value_set_boxed(value, self->attributes);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_plug_finalize(GObject *object) {
  SnapdPlug *self = SNAPD_PLUG(object);

  g_clear_pointer(&self->name, g_free);
  g_clear_pointer(&self->snap, g_free);
  g_clear_pointer(&self->interface, g_free);
  g_clear_pointer(&self->attributes, g_hash_table_unref);
  g_clear_pointer(&self->label, g_free);
  g_clear_pointer(&self->connections, g_ptr_array_unref);
  g_clear_pointer(&self->legacy_connections, g_ptr_array_unref);

  G_OBJECT_CLASS(snapd_plug_parent_class)->finalize(object);
}

static void snapd_plug_class_init(SnapdPlugClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_plug_set_property;
  gobject_class->get_property = snapd_plug_get_property;
  gobject_class->finalize = snapd_plug_finalize;

  g_object_class_install_property(
      gobject_class, PROP_NAME,
      g_param_spec_string("name", "name", "Plug name", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SNAP,
      g_param_spec_string("snap", "snap", "Snap this plug is on", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_INTERFACE,
      g_param_spec_string(
          "interface", "interface", "Interface this plug provides", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_LABEL,
      g_param_spec_string(
          "label", "label", "Short description of this plug", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_CONNECTIONS,
      g_param_spec_boxed("connections", "connections",
                         "Connections with this plug", G_TYPE_PTR_ARRAY,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                             G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_ATTRIBUTES,
      g_param_spec_boxed("attributes", "attributes", "Attributes for this plug",
                         G_TYPE_HASH_TABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                             G_PARAM_STATIC_BLURB));
}

static void snapd_plug_init(SnapdPlug *self) {
  self->attributes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                           (GDestroyNotify)g_variant_unref);
}
