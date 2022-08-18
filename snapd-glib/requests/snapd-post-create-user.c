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
    gboolean sudoer;
    gboolean known;
    SnapdUserInformation *user_information;
};

G_DEFINE_TYPE (SnapdPostCreateUser, snapd_post_create_user, snapd_request_get_type ())

SnapdPostCreateUser *
_snapd_post_create_user_new (const gchar *email,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostCreateUser *self = SNAPD_POST_CREATE_USER (g_object_new (snapd_post_create_user_get_type (),
                                                                      "cancellable", cancellable,
                                                                      "ready-callback", callback,
                                                                      "ready-callback-data", user_data,
                                                                      NULL));
    self->email = g_strdup (email);

    return self;
}

void
_snapd_post_create_user_set_sudoer (SnapdPostCreateUser *self, gboolean sudoer)
{
    self->sudoer = sudoer;
}

void
_snapd_post_create_user_set_known (SnapdPostCreateUser *self, gboolean known)
{
    self->known = known;
}

SnapdUserInformation *
_snapd_post_create_user_get_user_information (SnapdPostCreateUser *self)
{
    return self->user_information;
}

static SoupMessage *
generate_post_create_user_request (SnapdRequest *request, GBytes **body)
{
    SnapdPostCreateUser *self = SNAPD_POST_CREATE_USER (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/create-user");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "email");
    json_builder_add_string_value (builder, self->email);
    if (self->sudoer) {
        json_builder_set_member_name (builder, "sudoer");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (self->known) {
        json_builder_set_member_name (builder, "known");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder, body);

    return message;
}

static gboolean
parse_post_create_user_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdPostCreateUser *self = SNAPD_POST_CREATE_USER (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(SnapdUserInformation) user_information = _snapd_json_parse_user_information (result, error);
    json_node_unref (result);
    if (user_information == NULL)
        return FALSE;

    self->user_information = g_steal_pointer (&user_information);

    return TRUE;
}

static void
snapd_post_create_user_finalize (GObject *object)
{
    SnapdPostCreateUser *self = SNAPD_POST_CREATE_USER (object);

    g_clear_pointer (&self->email, g_free);
    g_clear_object (&self->user_information);

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
snapd_post_create_user_init (SnapdPostCreateUser *self)
{
}
