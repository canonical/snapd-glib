/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-change.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetChange
{
    SnapdRequest parent_instance;
    gchar *change_id;
    SnapdChange *change;
    JsonNode *data;
};

G_DEFINE_TYPE (SnapdGetChange, snapd_get_change, snapd_request_get_type ())

SnapdGetChange *
_snapd_get_change_new (const gchar *change_id, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetChange *self;

    self = SNAPD_GET_CHANGE (g_object_new (snapd_get_change_get_type (),
                                              "cancellable", cancellable,
                                              "ready-callback", callback,
                                              "ready-callback-data", user_data,
                                              NULL));
    self->change_id = g_strdup (change_id);

    return self;
}

const gchar *
_snapd_get_change_get_change_id (SnapdGetChange *self)
{
    return self->change_id;
}

SnapdChange *
_snapd_get_change_get_change (SnapdGetChange *self)
{
    return self->change;
}

JsonNode *
_snapd_get_change_get_data (SnapdGetChange *self)
{
    return self->data;
}

static SoupMessage *
generate_get_change_request (SnapdRequest *self)
{
    SnapdGetChange *r = SNAPD_GET_CHANGE (self);
    g_autofree gchar *path = NULL;

    path = g_strdup_printf ("http://snapd/v2/changes/%s", r->change_id);
    return soup_message_new ("GET", path);
}

static gboolean
parse_get_change_response (SnapdRequest *self, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetChange *r = SNAPD_GET_CHANGE (self);
    g_autoptr(JsonObject) response = NULL;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    r->change = _snapd_json_parse_change (result, error);
    json_node_unref (result);
    if (r->change == NULL)
        return FALSE;

    if (g_strcmp0 (r->change_id, snapd_change_get_id (r->change)) != 0) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected change ID returned");
        return FALSE;
    }

    if (json_object_has_member (json_node_get_object (result), "data"))
        r->data = json_node_ref (json_object_get_member (json_node_get_object (result), "data"));

    return TRUE;
}

static void
snapd_get_change_finalize (GObject *object)
{
    SnapdGetChange *self = SNAPD_GET_CHANGE (object);

    g_clear_pointer (&self->change_id, g_free);
    g_clear_object (&self->change);
    g_clear_pointer (&self->data, json_node_unref);

    G_OBJECT_CLASS (snapd_get_change_parent_class)->finalize (object);
}

static void
snapd_get_change_class_init (SnapdGetChangeClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_change_request;
   request_class->parse_response = parse_get_change_response;
   gobject_class->finalize = snapd_get_change_finalize;
}

static void
snapd_get_change_init (SnapdGetChange *self)
{
}
