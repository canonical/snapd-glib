/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-aliases.h"

#include "snapd-json.h"

struct _SnapdPostAliases
{
    SnapdRequestAsync parent_instance;
    gchar *action;
    gchar *snap;
    gchar *app;
    gchar *alias;
};

G_DEFINE_TYPE (SnapdPostAliases, snapd_post_aliases, snapd_request_async_get_type ())

SnapdPostAliases *
_snapd_post_aliases_new (const gchar *action,
                         const gchar *snap, const gchar *app, const gchar *alias,
                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostAliases *self;

    self = SNAPD_POST_ALIASES (g_object_new (snapd_post_aliases_get_type (),
                                                "cancellable", cancellable,
                                                "ready-callback", callback,
                                                "ready-callback-data", user_data,
                                                "progress-callback", progress_callback,
                                                "progress-callback-data", progress_callback_data,
                                                NULL));
    self->action = g_strdup (action);
    self->snap = g_strdup (snap);
    self->app = g_strdup (app);
    self->alias = g_strdup (alias);

    return self;
}

static SoupMessage *
generate_post_aliases_request (SnapdRequest *self)
{
    SnapdPostAliases *r = SNAPD_POST_ALIASES (self);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/aliases");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    if (r->snap != NULL) {
        json_builder_set_member_name (builder, "snap");
        json_builder_add_string_value (builder, r->snap);
    }
    if (r->app != NULL) {
        json_builder_set_member_name (builder, "app");
        json_builder_add_string_value (builder, r->app);
    }
    if (r->alias != NULL) {
        json_builder_set_member_name (builder, "alias");
        json_builder_add_string_value (builder, r->alias);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
snapd_post_aliases_finalize (GObject *object)
{
    SnapdPostAliases *self = SNAPD_POST_ALIASES (object);

    g_clear_pointer (&self->action, g_free);
    g_clear_pointer (&self->snap, g_free);
    g_clear_pointer (&self->app, g_free);
    g_clear_pointer (&self->alias, g_free);

    G_OBJECT_CLASS (snapd_post_aliases_parent_class)->finalize (object);
}

static void
snapd_post_aliases_class_init (SnapdPostAliasesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_aliases_request;
   gobject_class->finalize = snapd_post_aliases_finalize;
}

static void
snapd_post_aliases_init (SnapdPostAliases *self)
{
}
