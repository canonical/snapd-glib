/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-create-users.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostCreateUsers
{
    SnapdRequest parent_instance;
    GPtrArray *users_information;
};

G_DEFINE_TYPE (SnapdPostCreateUsers, snapd_post_create_users, snapd_request_get_type ())

SnapdPostCreateUsers *
_snapd_post_create_users_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_POST_CREATE_USERS (g_object_new (snapd_post_create_users_get_type (),
                                                  "cancellable", cancellable,
                                                  "ready-callback", callback,
                                                  "ready-callback-data", user_data,
                                                  NULL));
}

GPtrArray *
_snapd_post_create_users_get_users_information (SnapdPostCreateUsers *request)
{
    return request->users_information;
}

static SoupMessage *
generate_post_create_users_request (SnapdRequest *request)
{
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/create-user");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "known");
    json_builder_add_boolean_value (builder, TRUE);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
parse_post_create_users_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostCreateUsers *r = SNAPD_POST_CREATE_USERS (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) users_information = NULL;
    guint i;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    users_information = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        SnapdUserInformation *user_information;

        user_information = _snapd_json_parse_user_information (node, error);
        if (user_information == NULL)
            return FALSE;
        g_ptr_array_add (users_information, user_information);
    }

    r->users_information = g_steal_pointer (&users_information);

    return TRUE;
}

static void
snapd_post_create_users_finalize (GObject *object)
{
    SnapdPostCreateUsers *request = SNAPD_POST_CREATE_USERS (object);

    g_clear_pointer (&request->users_information, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_post_create_users_parent_class)->finalize (object);
}

static void
snapd_post_create_users_class_init (SnapdPostCreateUsersClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_create_users_request;
   request_class->parse_response = parse_post_create_users_response;
   gobject_class->finalize = snapd_post_create_users_finalize;
}

static void
snapd_post_create_users_init (SnapdPostCreateUsers *request)
{
}
