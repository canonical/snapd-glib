/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-themes.h"

#include "snapd-json.h"

struct _SnapdPostThemes {
  SnapdRequestAsync parent_instance;
  GStrv gtk_theme_names;
  GStrv icon_theme_names;
  GStrv sound_theme_names;
};

G_DEFINE_TYPE(SnapdPostThemes, snapd_post_themes,
              snapd_request_async_get_type())

SnapdPostThemes *_snapd_post_themes_new(
    GStrv gtk_theme_names, GStrv icon_theme_names, GStrv sound_theme_names,
    SnapdProgressCallback progress_callback, gpointer progress_callback_data,
    GCancellable *cancellable, GAsyncReadyCallback callback,
    gpointer user_data) {
  SnapdPostThemes *self = SNAPD_POST_THEMES(
      g_object_new(snapd_post_themes_get_type(), "cancellable", cancellable,
                   "ready-callback", callback, "ready-callback-data", user_data,
                   "progress-callback", progress_callback,
                   "progress-callback-data", progress_callback_data,
                   "change-api-path", "/v2/accessories/changes", NULL));
  self->gtk_theme_names = g_strdupv(gtk_theme_names);
  self->icon_theme_names = g_strdupv(icon_theme_names);
  self->sound_theme_names = g_strdupv(sound_theme_names);

  return self;
}

static void add_themes(JsonBuilder *builder, const char *member_name,
                       GStrv theme_names) {
  char *const *name;

  if (theme_names == NULL)
    return;

  json_builder_set_member_name(builder, member_name);
  json_builder_begin_array(builder);
  for (name = theme_names; *name != NULL; name++) {
    json_builder_add_string_value(builder, *name);
  }
  json_builder_end_array(builder);
}

static SoupMessage *generate_post_themes_request(SnapdRequest *request,
                                                 GBytes **body) {
  SnapdPostThemes *self = SNAPD_POST_THEMES(request);

  SoupMessage *message =
      soup_message_new("POST", "http://snapd/v2/accessories/themes");

  g_autoptr(JsonBuilder) builder = json_builder_new();
  json_builder_begin_object(builder);
  add_themes(builder, "gtk-themes", self->gtk_theme_names);
  add_themes(builder, "icon-themes", self->icon_theme_names);
  add_themes(builder, "sound-themes", self->sound_theme_names);
  json_builder_end_object(builder);
  _snapd_json_set_body(message, builder, body);

  return message;
}

static void snapd_post_themes_finalize(GObject *object) {
  SnapdPostThemes *self = SNAPD_POST_THEMES(object);

  g_clear_pointer(&self->gtk_theme_names, g_strfreev);
  g_clear_pointer(&self->icon_theme_names, g_strfreev);
  g_clear_pointer(&self->sound_theme_names, g_strfreev);

  G_OBJECT_CLASS(snapd_post_themes_parent_class)->finalize(object);
}

static void snapd_post_themes_class_init(SnapdPostThemesClass *klass) {
  SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS(klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  request_class->generate_request = generate_post_themes_request;
  gobject_class->finalize = snapd_post_themes_finalize;
}

static void snapd_post_themes_init(SnapdPostThemes *self) {}
