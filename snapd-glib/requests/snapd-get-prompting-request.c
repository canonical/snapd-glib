/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-prompting-request.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetPromptingRequest
{
    SnapdRequest parent_instance;
    gchar *id;
    SnapdPromptingRequest *request;
};

G_DEFINE_TYPE (SnapdGetPromptingRequest, snapd_get_prompting_request, snapd_request_get_type ())

SnapdGetPromptingRequest *
_snapd_get_prompting_request_new (const gchar *id, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetPromptingRequest *self = SNAPD_GET_PROMPTING_REQUEST (g_object_new (snapd_get_prompting_request_get_type (),
                                                                                "cancellable", cancellable,
                                                                                "ready-callback", callback,
                                                                                "ready-callback-data", user_data,
                                                                                NULL));
    self->id = g_strdup (id);

    return self;
}

SnapdPromptingRequest *
_snapd_get_prompting_request_get_request (SnapdGetPromptingRequest *self)
{
    return self->request;
}

static SoupMessage *
generate_get_prompting_request_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetPromptingRequest *self = SNAPD_GET_PROMPTING_REQUEST (request);
    g_autofree gchar *path = g_strdup_printf ("http://snapd/v2/prompting/requests/%s", self->id);
    return soup_message_new ("GET", path);
}

static gboolean
parse_get_prompting_request_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetPromptingRequest *self = SNAPD_GET_PROMPTING_REQUEST (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    self->request = _snapd_json_parse_prompting_request (result, error);
    json_node_unref (result);
    if (request == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_get_prompting_request_finalize (GObject *object)
{
    SnapdGetPromptingRequest *self = SNAPD_GET_PROMPTING_REQUEST (object);

    g_clear_pointer (&self->id, g_free);
    g_clear_object (&self->request);

    G_OBJECT_CLASS (snapd_get_prompting_request_parent_class)->finalize (object);
}

static void
snapd_get_prompting_request_class_init (SnapdGetPromptingRequestClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_prompting_request_request;
   request_class->parse_response = parse_get_prompting_request_response;
   gobject_class->finalize = snapd_get_prompting_request_finalize;
}

static void
snapd_get_prompting_request_init (SnapdGetPromptingRequest *self)
{
}
