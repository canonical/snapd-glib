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

/**
 * SECTION: snapd-connection
 * @short_description: Plug to slot connections
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdConnection contains information about how a #SnapdPlug is connected
 * to a #SnapdSlot. Connections are queried using
 * snapd_client_get_connections_sync().
 */

/**
 * SnapdConnection:
 *
 * #SnapdConnection contains the state of Snap a interface connection.
 *
 * Since: 1.0
 */

struct _SnapdConnection {
  GObject parent_instance;

  SnapdSlotRef *slot;
  SnapdPlugRef *plug;
  gchar *interface;
  gboolean manual;
  gboolean gadget;
  GHashTable *slot_attributes;
  GHashTable *plug_attributes;

  /* legacy */
  gchar *name;
  gchar *snap;
};

enum {
  PROP_NAME = 1,
  PROP_SNAP,
  PROP_SLOT,
  PROP_PLUG,
  PROP_INTERFACE,
  PROP_MANUAL,
  PROP_GADGET,
  PROP_SLOT_ATTRIBUTES,
  PROP_PLUG_ATTRIBUTES,
  PROP_LAST
};

G_DEFINE_TYPE(SnapdConnection, snapd_connection, G_TYPE_OBJECT)

/**
 * snapd_connection_get_slot:
 * @connection: a #SnapdConnection.
 *
 * Get the slot this connection is made with.
 *
 * Returns: (transfer none): a reference to a slot.
 *
 * Since: 1.48
 */
SnapdSlotRef *snapd_connection_get_slot(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return self->slot;
}

/**
 * snapd_connection_get_plug:
 * @connection: a #SnapdConnection.
 *
 * Get the plug this connection is made with.
 *
 * Returns: (transfer none): a reference to a plug.
 *
 * Since: 1.48
 */
SnapdPlugRef *snapd_connection_get_plug(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return self->plug;
}

/**
 * snapd_connection_get_interface:
 * @connection: a #SnapdConnection.
 *
 * Get the interface this connections uses.
 *
 * Returns: an interface name.
 *
 * Since: 1.48
 */
const gchar *snapd_connection_get_interface(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return self->interface;
}

/**
 * snapd_connection_get_manual:
 * @connection: a #SnapdConnection.
 *
 * Get if this connection was made manually.
 *
 * Returns: %TRUE if connection was made manually.
 *
 * Since: 1.48
 */
gboolean snapd_connection_get_manual(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), FALSE);
  return self->manual;
}

/**
 * snapd_connection_get_gadget:
 * @connection: a #SnapdConnection.
 *
 * Get if this connection was made by the gadget snap.
 *
 * Returns: %TRUE if connection was made by the gadget snap.
 *
 * Since: 1.48
 */
gboolean snapd_connection_get_gadget(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), FALSE);
  return self->gadget;
}

/**
 * snapd_connection_get_slot_attribute_names:
 * @connection: a #SnapdConnection.
 * @length: (out) (allow-none): location to write number of attributes or %NULL
 * if not required.
 *
 * Get the names of the attributes the connected slot has.
 *
 * Returns: (transfer full) (array zero-terminated=1): a string array of
 * attribute names. Free with g_strfreev().
 *
 * Since: 1.48
 */
GStrv snapd_connection_get_slot_attribute_names(SnapdConnection *self,
                                                guint *length) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);

  GHashTableIter iter;
  g_hash_table_iter_init(&iter, self->slot_attributes);
  guint size = g_hash_table_size(self->slot_attributes);
  GStrv names = g_malloc(sizeof(gchar *) * (size + 1));
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
 * snapd_connection_has_slot_attribute:
 * @connection: a #SnapdConnection.
 * @name: an attribute name.
 *
 * Check if the connected slot has an attribute.
 *
 * Returns: %TRUE if this attribute exists.
 *
 * Since: 1.48
 */
gboolean snapd_connection_has_slot_attribute(SnapdConnection *self,
                                             const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), FALSE);
  return g_hash_table_contains(self->slot_attributes, name);
}

/**
 * snapd_connection_get_slot_attribute:
 * @connection: a #SnapdConnection.
 * @name: an attribute name.
 *
 * Get an attribute for connected slot.
 *
 * Returns: (transfer none) (allow-none): an attribute value or %NULL if not
 * set.
 *
 * Since: 1.48
 */
GVariant *snapd_connection_get_slot_attribute(SnapdConnection *self,
                                              const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return g_hash_table_lookup(self->slot_attributes, name);
}

/**
 * snapd_connection_get_plug_attribute_names:
 * @connection: a #SnapdConnection.
 * @length: (out) (allow-none): location to write number of attributes or %NULL
 * if not required.
 *
 * Get the names of the attributes the connected plug has.
 *
 * Returns: (transfer full) (array zero-terminated=1): a string array of
 * attribute names. Free with g_strfreev().
 *
 * Since: 1.48
 */
GStrv snapd_connection_get_plug_attribute_names(SnapdConnection *self,
                                                guint *length) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);

  GHashTableIter iter;
  g_hash_table_iter_init(&iter, self->plug_attributes);
  guint size = g_hash_table_size(self->plug_attributes);
  GStrv names = g_malloc(sizeof(gchar *) * (size + 1));
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
 * snapd_connection_has_plug_attribute:
 * @connection: a #SnapdConnection.
 * @name: an attribute name.
 *
 * Check if the connected plug has an attribute.
 *
 * Returns: %TRUE if this attribute exists.
 *
 * Since: 1.48
 */
gboolean snapd_connection_has_plug_attribute(SnapdConnection *self,
                                             const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), FALSE);
  return g_hash_table_contains(self->plug_attributes, name);
}

/**
 * snapd_connection_get_plug_attribute:
 * @connection: a #SnapdConnection.
 * @name: an attribute name.
 *
 * Get an attribute for connected plug.
 *
 * Returns: (transfer none) (allow-none): an attribute value or %NULL if not
 * set.
 *
 * Since: 1.48
 */
GVariant *snapd_connection_get_plug_attribute(SnapdConnection *self,
                                              const gchar *name) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return g_hash_table_lookup(self->plug_attributes, name);
}

/**
 * snapd_connection_get_name:
 * @connection: a #SnapdConnection.
 *
 * Get the name of this connection (i.e. a slot or plug name).
 *
 * Returns: a name.
 *
 * Since: 1.0
 * Deprecated: 1.48: Use snapd_plug_ref_get_plug() or snapd_slot_ref_get_slot()
 */
const gchar *snapd_connection_get_name(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return self->name;
}

/**
 * snapd_connection_get_snap:
 * @connection: a #SnapdConnection.
 *
 * Get the snap this connection is on.
 *
 * Returns: a snap name.
 *
 * Since: 1.0
 * Deprecated: 1.48: Use snapd_plug_ref_get_snap() or snapd_slot_ref_get_snap()
 */
const gchar *snapd_connection_get_snap(SnapdConnection *self) {
  g_return_val_if_fail(SNAPD_IS_CONNECTION(self), NULL);
  return self->snap;
}

static void snapd_connection_set_property(GObject *object, guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec) {
  SnapdConnection *self = SNAPD_CONNECTION(object);

  switch (prop_id) {
  case PROP_NAME:
    g_free(self->name);
    self->name = g_strdup(g_value_get_string(value));
    break;
  case PROP_SNAP:
    g_free(self->snap);
    self->snap = g_strdup(g_value_get_string(value));
    break;
  case PROP_PLUG:
    g_set_object(&self->plug, g_value_get_object(value));
    break;
  case PROP_SLOT:
    g_set_object(&self->slot, g_value_get_object(value));
    break;
  case PROP_INTERFACE:
    g_free(self->interface);
    self->interface = g_strdup(g_value_get_string(value));
    break;
  case PROP_MANUAL:
    self->manual = g_value_get_boolean(value);
    break;
  case PROP_GADGET:
    self->gadget = g_value_get_boolean(value);
    break;
  case PROP_SLOT_ATTRIBUTES:
    g_clear_pointer(&self->slot_attributes, g_hash_table_unref);
    if (g_value_get_boxed(value) != NULL)
      self->slot_attributes = g_hash_table_ref(g_value_get_boxed(value));
    break;
  case PROP_PLUG_ATTRIBUTES:
    g_clear_pointer(&self->plug_attributes, g_hash_table_unref);
    if (g_value_get_boxed(value) != NULL)
      self->plug_attributes = g_hash_table_ref(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_connection_get_property(GObject *object, guint prop_id,
                                          GValue *value, GParamSpec *pspec) {
  SnapdConnection *self = SNAPD_CONNECTION(object);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string(value, self->name);
    break;
  case PROP_SNAP:
    g_value_set_string(value, self->snap);
    break;
  case PROP_PLUG:
    g_value_set_object(value, self->plug);
    break;
  case PROP_SLOT:
    g_value_set_object(value, self->slot);
    break;
  case PROP_INTERFACE:
    g_value_set_string(value, self->snap);
    break;
  case PROP_MANUAL:
    g_value_set_boolean(value, self->manual);
    break;
  case PROP_GADGET:
    g_value_set_boolean(value, self->gadget);
    break;
  case PROP_SLOT_ATTRIBUTES:
    g_value_set_boxed(value, self->slot_attributes);
    break;
  case PROP_PLUG_ATTRIBUTES:
    g_value_set_boxed(value, self->plug_attributes);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_connection_finalize(GObject *object) {
  SnapdConnection *self = SNAPD_CONNECTION(object);

  g_clear_object(&self->slot);
  g_clear_object(&self->plug);
  g_clear_pointer(&self->interface, g_free);
  g_clear_pointer(&self->slot_attributes, g_hash_table_unref);
  g_clear_pointer(&self->plug_attributes, g_hash_table_unref);
  g_clear_pointer(&self->name, g_free);
  g_clear_pointer(&self->snap, g_free);

  G_OBJECT_CLASS(snapd_connection_parent_class)->finalize(object);
}

static void snapd_connection_class_init(SnapdConnectionClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_connection_set_property;
  gobject_class->get_property = snapd_connection_get_property;
  gobject_class->finalize = snapd_connection_finalize;

  g_object_class_install_property(
      gobject_class, PROP_NAME,
      g_param_spec_string("name", "name", "Name of connection/plug on snap",
                          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_SNAP,
      g_param_spec_string("snap", "snap", "Snap this connection is made to",
                          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_SLOT,
      g_param_spec_object("slot", "slot", "Slot this connection is made with",
                          SNAPD_TYPE_SLOT_REF,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_PLUG,
      g_param_spec_object("plug", "plug", "Plug this connection is made with",
                          SNAPD_TYPE_PLUG_REF,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_INTERFACE,
      g_param_spec_string("interface", "interface",
                          "Interface this connection uses", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_MANUAL,
      g_param_spec_boolean("manual", "manual",
                           "TRUE if connection was made manually", FALSE,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_GADGET,
      g_param_spec_boolean("gadget", "gadget",
                           "TRUE if connection was made by the gadget snap",
                           FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_SLOT_ATTRIBUTES,
      g_param_spec_boxed("slot-attrs", "slot-attrs",
                         "Attributes for connected slot", G_TYPE_HASH_TABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property(
      gobject_class, PROP_PLUG_ATTRIBUTES,
      g_param_spec_boxed("plug-attrs", "plug-attrs",
                         "Attributes for connected plug", G_TYPE_HASH_TABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void snapd_connection_init(SnapdConnection *self) {}
