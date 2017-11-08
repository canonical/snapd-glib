/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-create-user.h"

#include "snapd-json.h"
#include "snapd-user-information.h"

struct _SnapdPostCreateUser
{
    SnapdRequest parent_instance;
    gchar *email;
    SnapdCreateUserFlags flags;
    SnapdUserInformation *user_information;
};

G_DEFINE_TYPE (SnapdPostCreateUser, snapd_post_create_user, snapd_request_get_type ())

SnapdPostCreateUser *
_snapd_post_create_user_new (const gchar *email, SnapdCreateUserFlags flags,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostCreateUser *request;

    request = SNAPD_POST_CREATE_USER (g_object_new (snapd_post_create_user_get_type (),
                                                    "cancellable", cancellable,
                                                    "ready-callback", callback,
                                                    "ready-callback-data", user_data,
                                                    NULL));
    request->email = g_strdup (email);
    request->flags = flags;

    return request;
}

SnapdUserInformation *
_snapd_post_create_user_get_user_information (SnapdPostCreateUser *request)
{
    return request->user_information;
}

static SoupMessage *
generate_post_create_user_request (SnapdRequest *request)
{
    SnapdPostCreateUser *r = SNAPD_POST_CREATE_USER (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/create-user");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "email");
    json_builder_add_string_value (builder, r->email);
    if ((r->flags & SNAPD_CREATE_USER_FLAGS_SUDO) != 0) {
        json_builder_set_member_name (builder, "sudoer");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((r->flags & SNAPD_CREATE_USER_FLAGS_KNOWN) != 0) {
        json_builder_set_member_name (builder, "known");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
parse_post_create_user_response (SnapdRequest *request, SoupMessage *message)
{
    SnapdPostCreateUser *r = SNAPD_POST_CREATE_USER (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdUserInformation) user_information = NULL;
    GError *error = NULL;

    response = _snapd_json_parse_response (message, &error);
    if (response == NULL) {
        _snapd_request_complete (request, error);
        return;
    }
    result = _snapd_json_get_sync_result_o (response, &error);
    if (result == NULL) {
        _snapd_request_complete (request, error);
        return;
    }

    user_information = _snapd_json_parse_user_information (result, &error);
    if (user_information == NULL) {
        _snapd_request_complete (request, error);
        return;
    }

    r->user_information = g_steal_pointer (&user_information);
    _snapd_request_complete (request, NULL);
}

static void
snapd_post_create_user_finalize (GObject *object)
{
    SnapdPostCreateUser *request = SNAPD_POST_CREATE_USER (object);

    g_clear_pointer (&request->email, g_free);
    g_clear_object (&request->user_information);

    G_OBJECT_CLASS (snapd_post_create_user_parent_class)->finalize (object);
}

static void
snapd_post_create_user_class_init (SnapdPostCreateUserClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_create_user_request;
   request_class->parse_response = parse_post_create_user_response;
   gobject_class->finalize = snapd_post_create_user_finalize;
}

static void
snapd_post_create_user_init (SnapdPostCreateUser *request)
{
}
