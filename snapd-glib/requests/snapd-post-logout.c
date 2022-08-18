/*
 * Copyright (C) 2020 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-logout.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostLogout
{
    SnapdRequest parent_instance;
    gint64 id;
};

G_DEFINE_TYPE (SnapdPostLogout, snapd_post_logout, snapd_request_get_type ())

SnapdPostLogout *
_snapd_post_logout_new (gint64 id,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostLogout *self = SNAPD_POST_LOGOUT (g_object_new (snapd_post_logout_get_type (),
                                                             "cancellable", cancellable,
                                                             "ready-callback", callback,
                                                             "ready-callback-data", user_data,
                                                             NULL));
    self->id = id;

    return self;
}

static SoupMessage *
generate_post_logout_request (SnapdRequest *request, GBytes **body)
{
    SnapdPostLogout *self = SNAPD_POST_LOGOUT (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/logout");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_int_value (builder, self->id);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder, body);

    return message;
}

static gboolean
parse_post_logout_response (SnapdRequest *request, SoupMessage *message, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;
    json_node_unref (result);

    return TRUE;
}

static void
snapd_post_logout_class_init (SnapdPostLogoutClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);

   request_class->generate_request = generate_post_logout_request;
   request_class->parse_response = parse_post_logout_response;
}

static void
snapd_post_logout_init (SnapdPostLogout *self)
{
}
