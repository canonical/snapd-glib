/*
 * Copyright (C) 2026 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-interfaces-requests.h"

#include "snapd-json.h"

struct _SnapdPostInterfacesRequests {
  SnapdRequest parent_instance;
  gchar *interface;
  GPid pid;
  gboolean allowed;
};

G_DEFINE_TYPE(SnapdPostInterfacesRequests, snapd_post_interfaces_requests,
              snapd_request_get_type())

SnapdPostInterfacesRequests *_snapd_post_interfaces_requests_new(
    const gchar *interface, GPid pid, GCancellable *cancellable,
    GAsyncReadyCallback callback, gpointer user_data) {
  SnapdPostInterfacesRequests *self =
      SNAPD_POST_INTERFACES_REQUESTS(g_object_new(
          snapd_post_interfaces_requests_get_type(), "cancellable", cancellable,
          "ready-callback", callback, "ready-callback-data", user_data, NULL));

  self->interface = g_strdup(interface);
  self->pid = pid;

  return self;
}

static SoupMessage *
generate_post_interfaces_requests_request(SnapdRequest *request,
                                          GBytes **body) {
  SnapdPostInterfacesRequests *self = SNAPD_POST_INTERFACES_REQUESTS(request);

  SoupMessage *message =
      soup_message_new("POST", "http://snapd/v2/interfaces/requests");

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "action");
  json_builder_add_string_value(builder, "ask");
  json_builder_set_member_name(builder, "interface");
  json_builder_add_string_value(builder, self->interface);
  json_builder_set_member_name(builder, "pid");
  json_builder_add_int_value(builder, self->pid);
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static gboolean parse_post_interfaces_requests_response(
    SnapdRequest *request, guint status_code, const gchar *content_type,
    GBytes *body, SnapdMaintenance **maintenance, GError **error) {
  SnapdPostInterfacesRequests *self = SNAPD_POST_INTERFACES_REQUESTS(request);

  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, maintenance, NULL, error);
  if (response == NULL)
    return FALSE;

  JsonObject *result = _snapd_json_get_sync_result_o(response, error);
  if (result == NULL)
    return FALSE;

  const gchar *outcome = _snapd_json_get_string(result, "outcome", "deny");
  self->allowed = g_strcmp0(outcome, "allow") == 0;
  json_object_unref(result);

  return TRUE;
}

gboolean
_snapd_post_interfaces_requests_get_allowed(SnapdPostInterfacesRequests *self) {
  g_return_val_if_fail(SNAPD_IS_POST_INTERFACES_REQUESTS(self), FALSE);
  return self->allowed;
}

static void snapd_post_interfaces_requests_finalize(GObject *object) {
  SnapdPostInterfacesRequests *self = SNAPD_POST_INTERFACES_REQUESTS(object);

  g_clear_pointer(&self->interface, g_free);

  G_OBJECT_CLASS(snapd_post_interfaces_requests_parent_class)->finalize(object);
}

static void snapd_post_interfaces_requests_class_init(
    SnapdPostInterfacesRequestsClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_interfaces_requests_request;
  request_class->parse_response = parse_post_interfaces_requests_response;
  gobject_class->finalize = snapd_post_interfaces_requests_finalize;
}

static void
snapd_post_interfaces_requests_init(SnapdPostInterfacesRequests *self) {}
