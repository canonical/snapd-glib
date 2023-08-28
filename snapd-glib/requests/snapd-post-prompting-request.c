/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-prompting-request.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostPromptingRequest
{
    SnapdRequest parent_instance;
    gchar *id;
    gchar *outcome;
    gchar *lifespan;
    gint64 duration;
    gchar *path_pattern;
    GStrv  permissions;
};

G_DEFINE_TYPE (SnapdPostPromptingRequest, snapd_post_prompting_request, snapd_request_get_type ())

SnapdPostPromptingRequest *
_snapd_post_prompting_request_new (const gchar *id, const gchar *outcome, const gchar *lifespan, gint64 duration, const gchar *path_pattern, GStrv permissions, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostPromptingRequest *self = SNAPD_POST_PROMPTING_REQUEST (g_object_new (snapd_post_prompting_request_get_type (),
                                                                                  "cancellable", cancellable,
                                                                                  "ready-callback", callback,
                                                                                  "ready-callback-data", user_data,
                                                                                  NULL));
    self->id = g_strdup (id);
    self->outcome = g_strdup (outcome);
    self->lifespan = g_strdup (lifespan);
    self->duration = duration;
    self->path_pattern = g_strdup (path_pattern);
    self->permissions = g_strdupv (permissions);

    return self;
}

static SoupMessage *
generate_post_prompting_request_request (SnapdRequest *request, GBytes **body)
{
    SnapdPostPromptingRequest *self = SNAPD_POST_PROMPTING_REQUEST (request);

    g_autofree gchar *path = g_strdup_printf ("http://snapd/v2/prompting/requests/%s", self->id);
    SoupMessage *message = soup_message_new ("POST", path);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "outcome");
    json_builder_add_string_value (builder, self->outcome);
    json_builder_set_member_name (builder, "lifespan");
    json_builder_add_string_value (builder, self->lifespan);
    json_builder_set_member_name (builder, "duration");
    json_builder_add_int_value (builder, self->duration);
    json_builder_set_member_name (builder, "path-pattern");
    json_builder_add_string_value (builder, self->path_pattern);
    json_builder_set_member_name (builder, "permissions");
    json_builder_begin_array (builder);
    for (gchar **permission = self->permissions; *permission != NULL; permission++) {
        json_builder_add_string_value (builder, *permission);
    }
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder, body);

    return message;
}

static gboolean
parse_post_prompting_request_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_post_prompting_request_finalize (GObject *object)
{
    SnapdPostPromptingRequest *self = SNAPD_POST_PROMPTING_REQUEST (object);

    g_clear_pointer (&self->id, g_free);
    g_clear_pointer (&self->outcome, g_free);
    g_clear_pointer (&self->lifespan, g_free);
    g_clear_pointer (&self->path_pattern, g_free);
    g_clear_pointer (&self->permissions, g_strfreev);

    G_OBJECT_CLASS (snapd_post_prompting_request_parent_class)->finalize (object);
}

static void
snapd_post_prompting_request_class_init (SnapdPostPromptingRequestClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_prompting_request_request;
   request_class->parse_response = parse_post_prompting_request_response;
   gobject_class->finalize = snapd_post_prompting_request_finalize;
}

static void
snapd_post_prompting_request_init (SnapdPostPromptingRequest *self)
{
}
