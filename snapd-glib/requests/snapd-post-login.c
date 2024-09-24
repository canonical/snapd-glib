/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-login.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostLogin {
  SnapdRequest parent_instance;
  gchar *email;
  gchar *password;
  gchar *otp;
  SnapdUserInformation *user_information;
};

G_DEFINE_TYPE(SnapdPostLogin, snapd_post_login, snapd_request_get_type())

SnapdPostLogin *_snapd_post_login_new(const gchar *email, const gchar *password,
                                      const gchar *otp,
                                      GCancellable *cancellable,
                                      GAsyncReadyCallback callback,
                                      gpointer user_data) {
  SnapdPostLogin *self = SNAPD_POST_LOGIN(g_object_new(
      snapd_post_login_get_type(), "cancellable", cancellable, "ready-callback",
      callback, "ready-callback-data", user_data, NULL));
  self->email = g_strdup(email);
  self->password = g_strdup(password);
  self->otp = g_strdup(otp);

  return self;
}

SnapdUserInformation *
_snapd_post_login_get_user_information(SnapdPostLogin *self) {
  return self->user_information;
}

static SoupMessage *generate_post_login_request(SnapdRequest *request,
                                                GBytes **body) {
  SnapdPostLogin *self = SNAPD_POST_LOGIN(request);

  SoupMessage *message = soup_message_new("POST", "http://snapd/v2/login");

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "email");
  json_builder_add_string_value(builder, self->email);
  /* Send legacy username field for snapd < 2.16 */
  json_builder_set_member_name(builder, "username");
  json_builder_add_string_value(builder, self->email);
  json_builder_set_member_name(builder, "password");
  json_builder_add_string_value(builder, self->password);
  if (self->otp != NULL) {
    json_builder_set_member_name(builder, "otp");
    json_builder_add_string_value(builder, self->otp);
  }
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static gboolean
parse_post_login_response(SnapdRequest *request, guint status_code,
                          const gchar *content_type, GBytes *body,
                          SnapdMaintenance **maintenance, GError **error) {
  SnapdPostLogin *self = SNAPD_POST_LOGIN(request);

  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, maintenance, NULL, error);
  if (response == NULL)
    return FALSE;
  /* FIXME: Needs json-glib to be fixed to use json_node_unref */
  /*g_autoptr(JsonNode) result = NULL;*/
  JsonNode *result = _snapd_json_get_sync_result(response, error);
  if (result == NULL)
    return FALSE;

  self->user_information = _snapd_json_parse_user_information(result, error);
  json_node_unref(result);
  if (self->user_information == NULL)
    return FALSE;

  return TRUE;
}

static void snapd_post_login_finalize(GObject *object) {
  SnapdPostLogin *self = SNAPD_POST_LOGIN(object);

  g_clear_pointer(&self->email, g_free);
  g_clear_pointer(&self->password, g_free);
  g_clear_pointer(&self->otp, g_free);
  g_clear_object(&self->user_information);

  G_OBJECT_CLASS(snapd_post_login_parent_class)->finalize(object);
}

static void snapd_post_login_class_init(SnapdPostLoginClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_login_request;
  request_class->parse_response = parse_post_login_response;
  gobject_class->finalize = snapd_post_login_finalize;
}

static void snapd_post_login_init(SnapdPostLogin *self) {}
