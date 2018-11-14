/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-login.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostLogin
{
    SnapdRequest parent_instance;
    gchar *email;
    gchar *password;
    gchar *otp;
    SnapdUserInformation *user_information;
};

G_DEFINE_TYPE (SnapdPostLogin, snapd_post_login, snapd_request_get_type ())

SnapdPostLogin *
_snapd_post_login_new (const gchar *email, const gchar *password, const gchar *otp,
                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostLogin *request;

    request = SNAPD_POST_LOGIN (g_object_new (snapd_post_login_get_type (),
                                              "cancellable", cancellable,
                                              "ready-callback", callback,
                                              "ready-callback-data", user_data,
                                              NULL));
    request->email = g_strdup (email);
    request->password = g_strdup (password);
    request->otp = g_strdup (otp);

    return request;
}

SnapdUserInformation *
_snapd_post_login_get_user_information (SnapdPostLogin *request)
{
    return request->user_information;
}

static SoupMessage *
generate_post_login_request (SnapdRequest *request)
{
    SnapdPostLogin *r = SNAPD_POST_LOGIN (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/login");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "email");
    json_builder_add_string_value (builder, r->email);
    /* Send legacy username field for snapd < 2.16 */
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, r->email);
    json_builder_set_member_name (builder, "password");
    json_builder_add_string_value (builder, r->password);
    if (r->otp != NULL) {
        json_builder_set_member_name (builder, "otp");
        json_builder_add_string_value (builder, r->otp);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
parse_post_login_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostLogin *r = SNAPD_POST_LOGIN (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    r->user_information = _snapd_json_parse_user_information (result, error);
    if (r->user_information == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_post_login_finalize (GObject *object)
{
    SnapdPostLogin *request = SNAPD_POST_LOGIN (object);

    g_clear_pointer (&request->email, g_free);
    g_clear_pointer (&request->password, g_free);
    g_clear_pointer (&request->otp, g_free);
    g_clear_object (&request->user_information);

    G_OBJECT_CLASS (snapd_post_login_parent_class)->finalize (object);
}

static void
snapd_post_login_class_init (SnapdPostLoginClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_login_request;
   request_class->parse_response = parse_post_login_response;
   gobject_class->finalize = snapd_post_login_finalize;
}

static void
snapd_post_login_init (SnapdPostLogin *request)
{
}
