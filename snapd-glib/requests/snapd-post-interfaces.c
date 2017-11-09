/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-interfaces.h"

#include "snapd-json.h"

struct _SnapdPostInterfaces
{
    SnapdRequestAsync parent_instance;
    gchar *action;
    gchar *plug_snap;
    gchar *plug_name;
    gchar *slot_snap;
    gchar *slot_name;
};

G_DEFINE_TYPE (SnapdPostInterfaces, snapd_post_interfaces, snapd_request_async_get_type ())

SnapdPostInterfaces *
_snapd_post_interfaces_new (const gchar *action,
                            const gchar *plug_snap, const gchar *plug_name,
                            const gchar *slot_snap, const gchar *slot_name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostInterfaces *request;

    request = SNAPD_POST_INTERFACES (g_object_new (snapd_post_interfaces_get_type (),
                                                   "cancellable", cancellable,
                                                   "ready-callback", callback,
                                                   "ready-callback-data", user_data,
                                                   "progress-callback", progress_callback,
                                                   "progress-callback-data", progress_callback_data,
                                                   NULL));
    request->action = g_strdup (action);
    request->plug_snap = g_strdup (plug_snap);
    request->plug_name = g_strdup (plug_name);
    request->slot_snap = g_strdup (slot_snap);
    request->slot_name = g_strdup (slot_name);

    return request;
}

static SoupMessage *
generate_post_interfaces_request (SnapdRequest *request)
{
    SnapdPostInterfaces *r = SNAPD_POST_INTERFACES (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/interfaces");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, r->plug_snap);
    json_builder_set_member_name (builder, "plug");
    json_builder_add_string_value (builder, r->plug_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "slots");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, r->slot_snap);
    json_builder_set_member_name (builder, "slot");
    json_builder_add_string_value (builder, r->slot_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
snapd_post_interfaces_finalize (GObject *object)
{
    SnapdPostInterfaces *request = SNAPD_POST_INTERFACES (object);

    g_clear_pointer (&request->action, g_free);
    g_clear_pointer (&request->plug_snap, g_free);
    g_clear_pointer (&request->plug_name, g_free);
    g_clear_pointer (&request->slot_snap, g_free);
    g_clear_pointer (&request->slot_name, g_free);

    G_OBJECT_CLASS (snapd_post_interfaces_parent_class)->finalize (object);
}

static void
snapd_post_interfaces_class_init (SnapdPostInterfacesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_interfaces_request;
   gobject_class->finalize = snapd_post_interfaces_finalize;
}

static void
snapd_post_interfaces_init (SnapdPostInterfaces *request)
{
}
