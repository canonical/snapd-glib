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
    gchar *username;
    gchar *password;
    gchar *otp;
    SnapdAuthData *auth_data;
};

G_DEFINE_TYPE (SnapdPostLogin, snapd_post_login, snapd_request_get_type ())

SnapdPostLogin *
_snapd_post_login_new (const gchar *username, const gchar *password, const gchar *otp,
                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostLogin *request;

    request = SNAPD_POST_LOGIN (g_object_new (snapd_post_login_get_type (),
                                              "cancellable", cancellable,
                                              "ready-callback", callback,
                                              "ready-callback-data", user_data,
                                              NULL));
    request->username = g_strdup (username);
    request->password = g_strdup (password);
    request->otp = g_strdup (otp);

    return request;
}

SnapdAuthData *
_snapd_post_login_get_auth_data (SnapdPostLogin *request)
{
    return request->auth_data;
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
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, r->username);
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

static void
parse_post_login_response (SnapdRequest *request, SoupMessage *message)
{
    SnapdPostLogin *r = SNAPD_POST_LOGIN (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(JsonArray) discharges = NULL;
    g_autoptr(GPtrArray) discharge_array = NULL;
    guint i;
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

    discharges = _snapd_json_get_array (result, "discharges");
    discharge_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (discharges); i++) {
        JsonNode *node = json_array_get_element (discharges, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            error = g_error_new (SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected discharge type");
            _snapd_request_complete (request, error);
            return;
        }

        g_ptr_array_add (discharge_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (discharge_array, NULL);
    r->auth_data = snapd_auth_data_new (_snapd_json_get_string (result, "macaroon", NULL), (gchar **) discharge_array->pdata);
    _snapd_request_complete (request, NULL);
}

static void
snapd_post_login_finalize (GObject *object)
{
    SnapdPostLogin *request = SNAPD_POST_LOGIN (object);

    g_clear_pointer (&request->username, g_free);
    g_clear_pointer (&request->password, g_free);
    g_clear_pointer (&request->otp, g_free);
    g_clear_object (&request->auth_data);

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
