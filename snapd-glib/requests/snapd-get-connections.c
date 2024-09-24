/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-connections.h"

#include "snapd-connection.h"
#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-plug-ref.h"
#include "snapd-plug.h"
#include "snapd-slot-ref.h"
#include "snapd-slot.h"

struct _SnapdGetConnections {
  SnapdRequest parent_instance;
  gchar *snap;
  gchar *interface;
  gchar *select;
  GPtrArray *established;
  GPtrArray *plugs;
  GPtrArray *slots;
  GPtrArray *undesired;
};

G_DEFINE_TYPE(SnapdGetConnections, snapd_get_connections,
              snapd_request_get_type())

SnapdGetConnections *
_snapd_get_connections_new(const gchar *snap, const gchar *interface,
                           const gchar *select, GCancellable *cancellable,
                           GAsyncReadyCallback callback, gpointer user_data) {
  SnapdGetConnections *self = SNAPD_GET_CONNECTIONS(g_object_new(
      snapd_get_connections_get_type(), "cancellable", cancellable,
      "ready-callback", callback, "ready-callback-data", user_data, NULL));
  self->snap = g_strdup(snap);
  self->interface = g_strdup(interface);
  self->select = g_strdup(select);

  return self;
}

GPtrArray *_snapd_get_connections_get_established(SnapdGetConnections *self) {
  return self->established;
}

GPtrArray *_snapd_get_connections_get_plugs(SnapdGetConnections *self) {
  return self->plugs;
}

GPtrArray *_snapd_get_connections_get_slots(SnapdGetConnections *self) {
  return self->slots;
}

GPtrArray *_snapd_get_connections_get_undesired(SnapdGetConnections *self) {
  return self->undesired;
}

static SoupMessage *generate_get_connections_request(SnapdRequest *request,
                                                     GBytes **body) {
  SnapdGetConnections *self = SNAPD_GET_CONNECTIONS(request);

  g_autoptr(GPtrArray) query_attributes =
      g_ptr_array_new_with_free_func(g_free);
  if (self->snap != NULL)
    g_ptr_array_add(query_attributes, g_strdup_printf("snap=%s", self->snap));
  if (self->interface != NULL)
    g_ptr_array_add(query_attributes,
                    g_strdup_printf("interface=%s", self->interface));
  if (self->select != NULL)
    g_ptr_array_add(query_attributes,
                    g_strdup_printf("select=%s", self->select));

  g_autoptr(GString) path = g_string_new("http://snapd/v2/connections");
  if (query_attributes->len > 0) {
    g_string_append_c(path, '?');
    for (guint i = 0; i < query_attributes->len; i++) {
      if (i != 0)
        g_string_append_c(path, '&');
      g_string_append(path, (gchar *)query_attributes->pdata[i]);
    }
  }

  return soup_message_new("GET", path->str);
}

static gboolean
parse_get_connections_response(SnapdRequest *request, guint status_code,
                               const gchar *content_type, GBytes *body,
                               SnapdMaintenance **maintenance, GError **error) {
  SnapdGetConnections *self = SNAPD_GET_CONNECTIONS(request);

  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, maintenance, NULL, error);
  if (response == NULL)
    return FALSE;
  g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o(response, error);
  if (result == NULL)
    return FALSE;

  g_autoptr(JsonArray) established =
      _snapd_json_get_array(result, "established");
  g_autoptr(GPtrArray) established_array =
      g_ptr_array_new_with_free_func(g_object_unref);
  for (guint i = 0; i < json_array_get_length(established); i++) {
    JsonNode *node = json_array_get_element(established, i);

    g_autoptr(SnapdConnection) connection =
        _snapd_json_parse_connection(node, error);
    if (connection == NULL)
      return FALSE;

    g_ptr_array_add(established_array, g_steal_pointer(&connection));
  }

  g_autoptr(JsonArray) undesired = _snapd_json_get_array(result, "undesired");
  g_autoptr(GPtrArray) undesired_array =
      g_ptr_array_new_with_free_func(g_object_unref);
  for (guint i = 0; i < json_array_get_length(undesired); i++) {
    JsonNode *node = json_array_get_element(undesired, i);

    SnapdConnection *connection = _snapd_json_parse_connection(node, error);
    if (connection == NULL)
      return FALSE;

    g_ptr_array_add(undesired_array, connection);
  }

  g_autoptr(JsonArray) plugs = _snapd_json_get_array(result, "plugs");
  g_autoptr(GPtrArray) plug_array =
      g_ptr_array_new_with_free_func(g_object_unref);
  for (guint i = 0; i < json_array_get_length(plugs); i++) {
    JsonNode *node = json_array_get_element(plugs, i);

    SnapdPlug *plug = _snapd_json_parse_plug(node, error);
    if (plug == NULL)
      return FALSE;

    g_ptr_array_add(plug_array, plug);
  }

  g_autoptr(JsonArray) slots = _snapd_json_get_array(result, "slots");
  g_autoptr(GPtrArray) slot_array =
      g_ptr_array_new_with_free_func(g_object_unref);
  for (guint i = 0; i < json_array_get_length(slots); i++) {
    JsonNode *node = json_array_get_element(slots, i);

    SnapdSlot *slot = _snapd_json_parse_slot(node, error);
    if (slot == NULL)
      return FALSE;

    g_ptr_array_add(slot_array, slot);
  }

  self->established = g_steal_pointer(&established_array);
  self->undesired = g_steal_pointer(&undesired_array);
  self->plugs = g_steal_pointer(&plug_array);
  self->slots = g_steal_pointer(&slot_array);

  return TRUE;
}

static void snapd_get_connections_finalize(GObject *object) {
  SnapdGetConnections *self = SNAPD_GET_CONNECTIONS(object);

  g_clear_pointer(&self->snap, g_free);
  g_clear_pointer(&self->interface, g_free);
  g_clear_pointer(&self->select, g_free);
  g_clear_pointer(&self->established, g_ptr_array_unref);
  g_clear_pointer(&self->plugs, g_ptr_array_unref);
  g_clear_pointer(&self->slots, g_ptr_array_unref);
  g_clear_pointer(&self->undesired, g_ptr_array_unref);

  G_OBJECT_CLASS(snapd_get_connections_parent_class)->finalize(object);
}

static void snapd_get_connections_class_init(SnapdGetConnectionsClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_get_connections_request;
  request_class->parse_response = parse_get_connections_response;
  gobject_class->finalize = snapd_get_connections_finalize;
}

static void snapd_get_connections_init(SnapdGetConnections *self) {}
