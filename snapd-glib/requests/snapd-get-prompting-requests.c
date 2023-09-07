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
    gboolean follow;
    SnapdGetPromptingRequestsRequestCallback request_callback;
    gpointer request_callback_data;
    GDestroyNotify request_callback_destroy_notify;
    GPtrArray *requests;
};

G_DEFINE_TYPE (SnapdGetPromptingRequests, snapd_get_prompting_requests, snapd_request_get_type ())

SnapdGetPromptingRequests *
_snapd_get_prompting_requests_new (gboolean follow, SnapdGetPromptingRequestsRequestCallback request_callback, gpointer request_callback_data, GDestroyNotify request_callback_destroy_notify, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (g_object_new (snapd_get_prompting_requests_get_type (),
                                                                    "cancellable", cancellable,
                                                                    "ready-callback", callback,
                                                                    "ready-callback-data", user_data,
                                                                    NULL));

    self->follow = follow;
    self->request_callback = request_callback;
    self->request_callback_data = request_callback_data;
    self->request_callback_destroy_notify = request_callback_destroy_notify;

    return self;
}

GPtrArray *
_snapd_get_prompting_requests_get_requests (SnapdGetPromptingRequests *self)
{
    return self->requests;
}

static SoupMessage *
generate_get_prompting_requests_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->follow) {
        g_ptr_array_add (query_attributes, g_strdup ("follow=true"));
    }

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/prompting/requests");
    if (query_attributes->len > 0) {
        g_string_append_c (path, '?');
        for (guint i = 0; i < query_attributes->len; i++) {
            if (i != 0)
                g_string_append_c (path, '&');
            g_string_append (path, (gchar *) query_attributes->pdata[i]);
        }
    }

    return soup_message_new ("GET", path->str);
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

static gboolean
parse_get_prompting_requests_json_seq (SnapdRequest *request, JsonNode *seq, GError **error)
{
    g_autoptr(SnapdPromptingRequest) prompting_request = NULL;

    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (request);

    if (!JSON_NODE_HOLDS_OBJECT (seq)) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected prompt request type");
        return FALSE;
    }

    prompting_request = _snapd_json_parse_prompting_request (seq, error);
    if (prompting_request == NULL)
        return FALSE;

    if (self->request_callback != NULL) {
        self->request_callback (self, prompting_request, self->request_callback_data);
    } else {
        g_ptr_array_add (self->requests, g_steal_pointer (&prompting_request));
    }

    return TRUE;
}

static void
snapd_get_prompting_requests_finalize (GObject *object)
{
    SnapdGetPromptingRequests *self = SNAPD_GET_PROMPTING_REQUESTS (object);

    if (self->request_callback_destroy_notify) {
        self->request_callback_destroy_notify (self->request_callback_data);
    }
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
   request_class->parse_json_seq = parse_get_prompting_requests_json_seq;
   gobject_class->finalize = snapd_get_prompting_requests_finalize;
}

static void
snapd_get_prompting_requests_init (SnapdGetPromptingRequests *self)
{
}
