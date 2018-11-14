/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-snapctl.h"

#include "snapd-json.h"

struct _SnapdPostSnapctl
{
    SnapdRequest parent_instance;
    gchar *context_id;
    GStrv args;
    gchar *stdout_output;
    gchar *stderr_output;
};

G_DEFINE_TYPE (SnapdPostSnapctl, snapd_post_snapctl, snapd_request_get_type ())

SnapdPostSnapctl *
_snapd_post_snapctl_new (const gchar *context_id, GStrv args, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapctl *request;

    request = SNAPD_POST_SNAPCTL (g_object_new (snapd_post_snapctl_get_type (),
                                                "cancellable", cancellable,
                                                "ready-callback", callback,
                                                "ready-callback-data", user_data,
                                                NULL));
    request->context_id = g_strdup (context_id);
    request->args = g_strdupv (args);

    return request;
}

const gchar *
_snapd_post_snapctl_get_stdout_output (SnapdPostSnapctl *request)
{
    return request->stdout_output;
}

const gchar *
_snapd_post_snapctl_get_stderr_output (SnapdPostSnapctl *request)
{
    return request->stderr_output;
}

static SoupMessage *
generate_post_snapctl_request (SnapdRequest *request)
{
    SnapdPostSnapctl *r = SNAPD_POST_SNAPCTL (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;
    int i;

    message = soup_message_new ("POST", "http://snapd/v2/snapctl");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "context-id");
    json_builder_add_string_value (builder, r->context_id);
    json_builder_set_member_name (builder, "args");
    json_builder_begin_array (builder);
    for (i = 0; r->args[i] != NULL; i++)
        json_builder_add_string_value (builder, r->args[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
parse_post_snapctl_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostSnapctl *r = SNAPD_POST_SNAPCTL (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    r->stdout_output = g_strdup (_snapd_json_get_string (result, "stdout", NULL));
    r->stderr_output = g_strdup (_snapd_json_get_string (result, "stderr", NULL));

    return TRUE;
}

static void
snapd_post_snapctl_finalize (GObject *object)
{
    SnapdPostSnapctl *request = SNAPD_POST_SNAPCTL (object);

    g_clear_pointer (&request->context_id, g_free);
    g_clear_pointer (&request->args, g_strfreev);
    g_clear_pointer (&request->stdout_output, g_free);
    g_clear_pointer (&request->stderr_output, g_free);

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
snapd_post_snapctl_init (SnapdPostSnapctl *request)
{
}
