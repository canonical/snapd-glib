/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-snapctl.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostSnapctl
{
    SnapdRequest parent_instance;
    gchar *context_id;
    GStrv args;
    gchar *stdout_output;
    gchar *stderr_output;
    int exit_code;
};

G_DEFINE_TYPE (SnapdPostSnapctl, snapd_post_snapctl, snapd_request_get_type ())

SnapdPostSnapctl *
_snapd_post_snapctl_new (const gchar *context_id, GStrv args, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapctl *self = SNAPD_POST_SNAPCTL (g_object_new (snapd_post_snapctl_get_type (),
                                                               "cancellable", cancellable,
                                                               "ready-callback", callback,
                                                               "ready-callback-data", user_data,
                                                               NULL));
    self->context_id = g_strdup (context_id);
    self->args = g_strdupv (args);

    return self;
}

const gchar *
_snapd_post_snapctl_get_stdout_output (SnapdPostSnapctl *self)
{
    return self->stdout_output;
}

const gchar *
_snapd_post_snapctl_get_stderr_output (SnapdPostSnapctl *self)
{
    return self->stderr_output;
}

int
_snapd_post_snapctl_get_exit_code (SnapdPostSnapctl *self)
{
    return self->exit_code;
}

static SoupMessage *
generate_post_snapctl_request (SnapdRequest *request)
{
    SnapdPostSnapctl *self = SNAPD_POST_SNAPCTL (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/snapctl");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "context-id");
    json_builder_add_string_value (builder, self->context_id);
    json_builder_set_member_name (builder, "args");
    json_builder_begin_array (builder);
    for (int i = 0; self->args[i] != NULL; i++)
        json_builder_add_string_value (builder, self->args[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
parse_post_snapctl_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostSnapctl *self = SNAPD_POST_SNAPCTL (request);
    g_autoptr(JsonNode) error_value = NULL;

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, &error_value, error);
    if (response == NULL) {
        if (g_error_matches (*error, SNAPD_ERROR, SNAPD_ERROR_UNSUCCESSFUL) &&
            error_value != NULL && json_node_get_value_type (error_value) == JSON_TYPE_OBJECT) {
            JsonObject *object = json_node_get_object (error_value);

            self->stdout_output = g_strdup (_snapd_json_get_string (object, "stdout", NULL));
            self->stderr_output = g_strdup (_snapd_json_get_string (object, "stderr", NULL));
            self->exit_code = _snapd_json_get_int (object, "exit-code", 0);
        }
        return FALSE;
    }
    g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    self->stdout_output = g_strdup (_snapd_json_get_string (result, "stdout", NULL));
    self->stderr_output = g_strdup (_snapd_json_get_string (result, "stderr", NULL));

    return TRUE;
}

static void
snapd_post_snapctl_finalize (GObject *object)
{
    SnapdPostSnapctl *self = SNAPD_POST_SNAPCTL (object);

    g_clear_pointer (&self->context_id, g_free);
    g_clear_pointer (&self->args, g_strfreev);
    g_clear_pointer (&self->stdout_output, g_free);
    g_clear_pointer (&self->stderr_output, g_free);

    G_OBJECT_CLASS (snapd_post_snapctl_parent_class)->finalize (object);
}

static void
snapd_post_snapctl_class_init (SnapdPostSnapctlClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snapctl_request;
   request_class->parse_response = parse_post_snapctl_response;
   gobject_class->finalize = snapd_post_snapctl_finalize;
}

static void
snapd_post_snapctl_init (SnapdPostSnapctl *self)
{
}
