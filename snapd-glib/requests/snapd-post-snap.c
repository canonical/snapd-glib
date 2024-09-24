/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-snap.h"

#include "snapd-json.h"

struct _SnapdPostSnap {
  SnapdRequestAsync parent_instance;
  gchar *name;
  gchar *action;
  gchar *channel;
  gchar *revision;
  gboolean classic;
  gboolean dangerous;
  gboolean devmode;
  gboolean jailmode;
  gboolean purge;
};

G_DEFINE_TYPE(SnapdPostSnap, snapd_post_snap, snapd_request_async_get_type())

SnapdPostSnap *_snapd_post_snap_new(const gchar *name, const gchar *action,
                                    SnapdProgressCallback progress_callback,
                                    gpointer progress_callback_data,
                                    GCancellable *cancellable,
                                    GAsyncReadyCallback callback,
                                    gpointer user_data) {
  SnapdPostSnap *self = SNAPD_POST_SNAP(
      g_object_new(snapd_post_snap_get_type(), "cancellable", cancellable,
                   "ready-callback", callback, "ready-callback-data", user_data,
                   "progress-callback", progress_callback,
                   "progress-callback-data", progress_callback_data, NULL));
  self->name = g_strdup(name);
  self->action = g_strdup(action);

  return self;
}

void _snapd_post_snap_set_channel(SnapdPostSnap *self, const gchar *channel) {
  g_free(self->channel);
  self->channel = g_strdup(channel);
}

void _snapd_post_snap_set_revision(SnapdPostSnap *self, const gchar *revision) {
  g_free(self->revision);
  self->revision = g_strdup(revision);
}

void _snapd_post_snap_set_classic(SnapdPostSnap *self, gboolean classic) {
  self->classic = classic;
}

void _snapd_post_snap_set_dangerous(SnapdPostSnap *self, gboolean dangerous) {
  self->dangerous = dangerous;
}

void _snapd_post_snap_set_devmode(SnapdPostSnap *self, gboolean devmode) {
  self->devmode = devmode;
}

void _snapd_post_snap_set_jailmode(SnapdPostSnap *self, gboolean jailmode) {
  self->jailmode = jailmode;
}

void _snapd_post_snap_set_purge(SnapdPostSnap *self, gboolean purge) {
  self->purge = purge;
}

static SoupMessage *generate_post_snap_request(SnapdRequest *request,
                                               GBytes **body) {
  SnapdPostSnap *self = SNAPD_POST_SNAP(request);

  g_autoptr(GString) path = g_string_new("http://snapd/v2/snaps/");
  g_string_append_uri_escaped(path, self->name, NULL, TRUE);
  SoupMessage *message = soup_message_new("POST", path->str);

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  json_builder_set_member_name(builder, "action");
  json_builder_add_string_value(builder, self->action);
  if (self->channel != NULL) {
    json_builder_set_member_name(builder, "channel");
    json_builder_add_string_value(builder, self->channel);
  }
  if (self->revision != NULL) {
    json_builder_set_member_name(builder, "revision");
    json_builder_add_string_value(builder, self->revision);
  }
  if (self->classic) {
    json_builder_set_member_name(builder, "classic");
    json_builder_add_boolean_value(builder, TRUE);
  }
  if (self->dangerous) {
    json_builder_set_member_name(builder, "dangerous");
    json_builder_add_boolean_value(builder, TRUE);
  }
  if (self->devmode) {
    json_builder_set_member_name(builder, "devmode");
    json_builder_add_boolean_value(builder, TRUE);
  }
  if (self->jailmode) {
    json_builder_set_member_name(builder, "jailmode");
    json_builder_add_boolean_value(builder, TRUE);
  }
  if (self->purge) {
    json_builder_set_member_name(builder, "purge");
    json_builder_add_boolean_value(builder, TRUE);
  }
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static void snapd_post_snap_finalize(GObject *object) {
  SnapdPostSnap *self = SNAPD_POST_SNAP(object);

  g_clear_pointer(&self->name, g_free);
  g_clear_pointer(&self->action, g_free);
  g_clear_pointer(&self->channel, g_free);
  g_clear_pointer(&self->revision, g_free);

  G_OBJECT_CLASS(snapd_post_snap_parent_class)->finalize(object);
}

static void snapd_post_snap_class_init(SnapdPostSnapClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_snap_request;
  gobject_class->finalize = snapd_post_snap_finalize;
}

static void snapd_post_snap_init(SnapdPostSnap *self) {}
