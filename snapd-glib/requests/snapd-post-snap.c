/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-snap.h"

#include "snapd-json.h"

struct _SnapdPostSnap
{
    SnapdRequestAsync parent_instance;
    gchar *name;
    gchar *action;
    gchar *channel;
    gchar *revision;
    gboolean classic;
    gboolean dangerous;
    gboolean devmode;
    gboolean jailmode;
};

G_DEFINE_TYPE (SnapdPostSnap, snapd_post_snap, snapd_request_async_get_type ())

SnapdPostSnap *
_snapd_post_snap_new (const gchar *name, const gchar *action,
                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             "progress-callback", progress_callback,
                                             "progress-callback-data", progress_callback_data,
                                             NULL));
    request->name = g_strdup (name);
    request->action = g_strdup (action);

    return request;
}

void
_snapd_post_snap_set_channel (SnapdPostSnap *request, const gchar *channel)
{
    g_free (request->channel);
    request->channel = g_strdup (channel);
}

void
_snapd_post_snap_set_revision (SnapdPostSnap *request, const gchar *revision)
{
    g_free (request->revision);
    request->revision = g_strdup (revision);
}

void
_snapd_post_snap_set_classic (SnapdPostSnap *request, gboolean classic)
{
    request->classic = classic;
}

void
_snapd_post_snap_set_dangerous (SnapdPostSnap *request, gboolean dangerous)
{
    request->dangerous = dangerous;
}

void
_snapd_post_snap_set_devmode (SnapdPostSnap *request, gboolean devmode)
{
    request->devmode = devmode;
}

void
_snapd_post_snap_set_jailmode (SnapdPostSnap *request, gboolean jailmode)
{
    request->jailmode = jailmode;
}

static SoupMessage *
generate_post_snap_request (SnapdRequest *request)
{
    SnapdPostSnap *r = SNAPD_POST_SNAP (request);
    g_autofree gchar *escaped = NULL, *path = NULL;
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    escaped = soup_uri_encode (r->name, NULL);
    path = g_strdup_printf ("http://snapd/v2/snaps/%s", escaped);
    message = soup_message_new ("POST", path);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    if (r->channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, r->channel);
    }
    if (r->revision != NULL) {
        json_builder_set_member_name (builder, "revision");
        json_builder_add_string_value (builder, r->revision);
    }
    if (r->classic) {
        json_builder_set_member_name (builder, "classic");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (r->dangerous) {
        json_builder_set_member_name (builder, "dangerous");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (r->devmode) {
        json_builder_set_member_name (builder, "devmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (r->jailmode) {
        json_builder_set_member_name (builder, "jailmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
snapd_post_snap_finalize (GObject *object)
{
    SnapdPostSnap *request = SNAPD_POST_SNAP (object);

    g_clear_pointer (&request->name, g_free);
    g_clear_pointer (&request->action, g_free);
    g_clear_pointer (&request->channel, g_free);
    g_clear_pointer (&request->revision, g_free);

    G_OBJECT_CLASS (snapd_post_snap_parent_class)->finalize (object);
}

static void
snapd_post_snap_class_init (SnapdPostSnapClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_request;
   gobject_class->finalize = snapd_post_snap_finalize;
}

static void
snapd_post_snap_init (SnapdPostSnap *request)
{
}
