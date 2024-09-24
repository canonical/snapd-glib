/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-change.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostChange {
  SnapdRequest parent_instance;
  gchar *change_id;
  gchar *action;
  SnapdChange *change;
  JsonNode *data;
  gchar *api_path;
};

G_DEFINE_TYPE(SnapdPostChange, snapd_post_change, snapd_request_get_type())

SnapdPostChange *_snapd_post_change_new(const gchar *change_id,
                                        const gchar *action,
                                        GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data) {
  SnapdPostChange *self = SNAPD_POST_CHANGE(g_object_new(
      snapd_post_change_get_type(), "cancellable", cancellable,
      "ready-callback", callback, "ready-callback-data", user_data, NULL));
  self->change_id = g_strdup(change_id);
  self->action = g_strdup(action);

  return self;
}

const gchar *_snapd_post_change_get_change_id(SnapdPostChange *self) {
  return self->change_id;
}

SnapdChange *_snapd_post_change_get_change(SnapdPostChange *self) {
  return self->change;
}

JsonNode *_snapd_post_change_get_data(SnapdPostChange *self) {
  return self->data;
}

void _snapd_post_change_set_api_path(SnapdPostChange *self,
                                     const gchar *api_path) {
  g_free(self->api_path);
  self->api_path = g_strdup(api_path);
}

static SoupMessage *generate_post_change_request(SnapdRequest *request,
                                                 GBytes **body) {
  SnapdPostChange *self = SNAPD_POST_CHANGE(request);

  g_autofree gchar *path = g_strdup_printf(
      "http://snapd%s/%s", self->api_path ? self->api_path : "/v2/changes",
      self->change_id);
  SoupMessage *message = soup_message_new("POST", path);

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "action");
  json_builder_add_string_value(builder, self->action);
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static gboolean
parse_post_change_response(SnapdRequest *request, guint status_code,
                           const gchar *content_type, GBytes *body,
                           SnapdMaintenance **maintenance, GError **error) {
  SnapdPostChange *self = SNAPD_POST_CHANGE(request);

  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, maintenance, NULL, error);
  if (response == NULL)
    return FALSE;
  /* FIXME: Needs json-glib to be fixed to use json_node_unref */
  /*g_autoptr(JsonNode) result = NULL;*/
  JsonNode *result = _snapd_json_get_sync_result(response, error);
  if (result == NULL)
    return FALSE;

  self->change = _snapd_json_parse_change(result, error);
  json_node_unref(result);
  if (self->change == NULL)
    return FALSE;

  if (g_strcmp0(self->change_id, snapd_change_get_id(self->change)) != 0) {
    g_set_error(error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED,
                "Unexpected change ID returned");
    return FALSE;
  }

  if (json_object_has_member(json_node_get_object(result), "data"))
    self->data = json_node_ref(
        json_object_get_member(json_node_get_object(result), "data"));

  return TRUE;
}

static void snapd_post_change_finalize(GObject *object) {
  SnapdPostChange *self = SNAPD_POST_CHANGE(object);

  g_clear_pointer(&self->change_id, g_free);
  g_clear_pointer(&self->action, g_free);
  g_clear_object(&self->change);
  g_clear_pointer(&self->data, json_node_unref);
  g_clear_pointer(&self->api_path, g_free);

  G_OBJECT_CLASS(snapd_post_change_parent_class)->finalize(object);
}

static void snapd_post_change_class_init(SnapdPostChangeClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_change_request;
  request_class->parse_response = parse_post_change_response;
  gobject_class->finalize = snapd_post_change_finalize;
}

static void snapd_post_change_init(SnapdPostChange *self) {}
