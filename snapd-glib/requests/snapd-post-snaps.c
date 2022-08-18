/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-post-snaps.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdPostSnaps
{
    SnapdRequestAsync parent_instance;
    gchar *action;
    GStrv snap_names;
};

G_DEFINE_TYPE (SnapdPostSnaps, snapd_post_snaps, snapd_request_async_get_type ())

SnapdPostSnaps *
_snapd_post_snaps_new (const gchar *action,
                       SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnaps *self = SNAPD_POST_SNAPS (g_object_new (snapd_post_snaps_get_type (),
                                                           "cancellable", cancellable,
                                                           "ready-callback", callback,
                                                           "ready-callback-data", user_data,
                                                           "progress-callback", progress_callback,
                                                           "progress-callback-data", progress_callback_data,
                                                           NULL));
    self->action = g_strdup (action);

    return self;
}

GStrv
_snapd_post_snaps_get_snap_names (SnapdPostSnaps *self)
{
    return self->snap_names;
}

static SoupMessage *
generate_post_snaps_request (SnapdRequest *request, GBytes **body)
{
    SnapdPostSnaps *self = SNAPD_POST_SNAPS (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/snaps");

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, self->action);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder, body);

    return message;
}

static gboolean
parse_post_snaps_result (SnapdRequestAsync *request, JsonNode *result, GError **error)
{
    SnapdPostSnaps *self = SNAPD_POST_SNAPS (request);

    if (result == NULL || json_node_get_value_type (result) != JSON_TYPE_OBJECT) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected result type");
        return FALSE;
    }
    JsonObject *o = json_node_get_object (result);
    if (o == NULL) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        return FALSE;
    }
    g_autoptr(JsonArray) a = _snapd_json_get_array (o, "snap-names");
    g_autoptr(GPtrArray) snap_names = g_ptr_array_new ();
    for (guint i = 0; i < json_array_get_length (a); i++) {
        JsonNode *node = json_array_get_element (a, i);
        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected snap name type");
            return FALSE;
        }

        g_ptr_array_add (snap_names, g_strdup (json_node_get_string (node)));
    }
    g_ptr_array_add (snap_names, NULL);

    self->snap_names = (GStrv) g_steal_pointer (&snap_names->pdata);

    return TRUE;
}

static void
snapd_post_snaps_finalize (GObject *object)
{
    SnapdPostSnaps *self = SNAPD_POST_SNAPS (object);

    g_clear_pointer (&self->action, g_free);
    g_clear_pointer (&self->snap_names, g_strfreev);

    G_OBJECT_CLASS (snapd_post_snaps_parent_class)->finalize (object);
}

static void
snapd_post_snaps_class_init (SnapdPostSnapsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   SnapdRequestAsyncClass *request_async_class = SNAPD_REQUEST_ASYNC_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snaps_request;
   request_async_class->parse_result = parse_post_snaps_result;
   gobject_class->finalize = snapd_post_snaps_finalize;
}

static void
snapd_post_snaps_init (SnapdPostSnaps *self)
{
}
