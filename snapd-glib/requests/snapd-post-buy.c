/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-buy.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostBuy {
  SnapdRequest parent_instance;
  gchar *id;
  gdouble amount;
  gchar *currency;
};

G_DEFINE_TYPE(SnapdPostBuy, snapd_post_buy, snapd_request_get_type())

SnapdPostBuy *_snapd_post_buy_new(const gchar *id, gdouble amount,
                                  const gchar *currency,
                                  GCancellable *cancellable,
                                  GAsyncReadyCallback callback,
                                  gpointer user_data) {
  SnapdPostBuy *self = SNAPD_POST_BUY(g_object_new(
      snapd_post_buy_get_type(), "cancellable", cancellable, "ready-callback",
      callback, "ready-callback-data", user_data, NULL));
  self->id = g_strdup(id);
  self->amount = amount;
  self->currency = g_strdup(currency);

  return self;
}

static SoupMessage *generate_post_buy_request(SnapdRequest *request,
                                              GBytes **body) {
  SnapdPostBuy *self = SNAPD_POST_BUY(request);

  SoupMessage *message = soup_message_new("POST", "http://snapd/v2/buy");

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "snap-id");
  json_builder_add_string_value(builder, self->id);
  json_builder_set_member_name(builder, "price");
  json_builder_add_double_value(builder, self->amount);
  json_builder_set_member_name(builder, "currency");
  json_builder_add_string_value(builder, self->currency);
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static gboolean parse_post_buy_response(SnapdRequest *request,
                                        guint status_code,
                                        const gchar *content_type, GBytes *body,
                                        SnapdMaintenance **maintenance,
                                        GError **error) {
  g_autoptr(JsonObject) response =
      _snapd_json_parse_response(content_type, body, maintenance, NULL, error);
  if (response == NULL)
    return FALSE;

  return TRUE;
}

static void snapd_post_buy_finalize(GObject *object) {
  SnapdPostBuy *self = SNAPD_POST_BUY(object);

  g_clear_pointer(&self->id, g_free);
  g_clear_pointer(&self->currency, g_free);

  G_OBJECT_CLASS(snapd_post_buy_parent_class)->finalize(object);
}

static void snapd_post_buy_class_init(SnapdPostBuyClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_buy_request;
  request_class->parse_response = parse_post_buy_response;
  gobject_class->finalize = snapd_post_buy_finalize;
}

static void snapd_post_buy_init(SnapdPostBuy *self) {}
