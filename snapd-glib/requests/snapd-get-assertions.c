/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdlib.h>

#include "snapd-get-assertions.h"

#include "snapd-assertion.h"
#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetAssertions
{
    SnapdRequest parent_instance;
    gchar *type;
    gchar **assertions;
};

G_DEFINE_TYPE (SnapdGetAssertions, snapd_get_assertions, snapd_request_get_type ())

SnapdGetAssertions *
_snapd_get_assertions_new (const gchar *type, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetAssertions *request;

    request = SNAPD_GET_ASSERTIONS (g_object_new (snapd_get_assertions_get_type (),
                                                  "cancellable", cancellable,
                                                  "ready-callback", callback,
                                                  "ready-callback-data", user_data,
                                                  NULL));
    request->type = g_strdup (type);

    return request;
}

gchar **
_snapd_get_assertions_get_assertions (SnapdGetAssertions *request)
{
    return request->assertions;
}

static SoupMessage *
generate_get_assertions_request (SnapdRequest *request)
{
    SnapdGetAssertions *r = SNAPD_GET_ASSERTIONS (request);
    g_autofree gchar *escaped = NULL, *path = NULL;

    escaped = soup_uri_encode (r->type, NULL);
    path = g_strdup_printf ("http://snapd/v2/assertions/%s", escaped);

    return soup_message_new ("GET", path);
}

static gboolean
parse_get_assertions_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetAssertions *r = SNAPD_GET_ASSERTIONS (request);
    const gchar *content_type;
    g_autoptr(GPtrArray) assertions = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;
    gsize offset = 0;

    content_type = soup_message_headers_get_content_type (message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = NULL;
        g_autoptr(JsonObject) result = NULL;

        response = _snapd_json_parse_response (message, maintenance, error);
        if (response == NULL)
            return FALSE;
        result = _snapd_json_get_sync_result_o (response, error);
        if (result == NULL)
            return FALSE;

        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unknown response");
        return FALSE;
    }

    if (message->status_code != SOUP_STATUS_OK) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Got response %u retrieving assertions", message->status_code);
        return FALSE;
    }

    if (g_strcmp0 (content_type, "application/x.ubuntu.assertion") != 0) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Got unknown content type '%s' retrieving assertions", content_type);
        return FALSE;
    }

    assertions = g_ptr_array_new ();
    buffer = soup_message_body_flatten (message->response_body);
    while (offset < buffer->length) {
        gsize assertion_start, assertion_end, body_length = 0;
        g_autofree gchar *body_length_header = NULL;
        g_autofree gchar *content = NULL;
        g_autoptr(SnapdAssertion) assertion = NULL;

        /* Headers terminated by double newline */
        assertion_start = offset;
        while (offset < buffer->length && !g_str_has_prefix (buffer->data + offset, "\n\n"))
            offset++;
        offset += 2;

        /* Make a temporary assertion object to decode body length header */
        content = g_strndup (buffer->data + assertion_start, offset - assertion_start);
        assertion = snapd_assertion_new (content);
        body_length_header = snapd_assertion_get_header (assertion, "body-length");

        /* Skip over body */
        body_length = body_length_header != NULL ? strtoul (body_length_header, NULL, 10) : 0;
        if (body_length > 0)
            offset += body_length + 2;

        /* Find end of signature */
        while (offset < buffer->length && !g_str_has_prefix (buffer->data + offset, "\n\n"))
            offset++;
        assertion_end = offset;
        offset += 2;

        g_ptr_array_add (assertions, g_strndup (buffer->data + assertion_start, assertion_end - assertion_start));
    }
    g_ptr_array_add (assertions, NULL);

    r->assertions = g_steal_pointer ((gchar ***)&assertions->pdata);

    return TRUE;
}

static void
snapd_get_assertions_finalize (GObject *object)
{
    SnapdGetAssertions *request = SNAPD_GET_ASSERTIONS (object);

    g_clear_pointer (&request->type, g_free);
    g_clear_pointer (&request->assertions, g_strfreev);

    G_OBJECT_CLASS (snapd_get_assertions_parent_class)->finalize (object);
}

static void
snapd_get_assertions_class_init (SnapdGetAssertionsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_assertions_request;
   request_class->parse_response = parse_get_assertions_response;
   gobject_class->finalize = snapd_get_assertions_finalize;
}

static void
snapd_get_assertions_init (SnapdGetAssertions *request)
{
}
