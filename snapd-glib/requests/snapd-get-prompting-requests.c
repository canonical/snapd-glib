/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-prompting-requests.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetPromptingRequests
{
    SnapdRequest parent_instance;
    GPtrArray *requests;
};

G_DEFINE_TYPE (SnapdGetPromptingRequests, snapd_get_prompting_requests, snapd_request_get_type ())

SnapdGetPromptingRequests *
_snapd_get_prompting_requests_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_PROMPTING_REQUESTS (g_object_new (snapd_get_prompting_requests_get_type (),
                                                       "cancellable", cancellable,
                                                       "ready-callback", callback,
                                                       "ready-callback-data", user_data,
                                                       NULL));
}

GPtrArray *
_snapd_get_prompting_requests_get_requests (SnapdGetPromptingRequests *self)
{
    return self->requests;
}

static SoupMessage *
generate_get_prompting_requests_request (SnapdRequest *request, GBytes **body)
{
    return soup_message_new ("GET", "http://snapd/v2/prompting/requests");
}

static gboolean
parse_get_prompting_requests_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) requests = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);

        SnapdPromptingRequest *request = _snapd_json_parse_prompting_request (node, error);
        if (request == NULL)
            return FALSE;

        g_ptr_array_add (requests, request);
    }

    self->requests = g_steal_pointer (&requests);

    return TRUE;
}

static void
snapd_get_prompting_requests_finalize (GObject *object)
{
    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (object);

    g_clear_pointer (&self->requests, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_prompting_requests_parent_class)->finalize (object);
}

static void
snapd_get_prompting_requests_class_init (SnapdGetPromptingRequestsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_prompting_requests_request;
   request_class->parse_response = parse_get_prompting_requests_response;
   gobject_class->finalize = snapd_get_prompting_requests_finalize;
}

static void
snapd_get_prompting_requests_init (SnapdGetPromptingRequests *self)
{
}
