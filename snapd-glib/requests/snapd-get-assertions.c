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
    GStrv assertions;
};

G_DEFINE_TYPE (SnapdGetAssertions, snapd_get_assertions, snapd_request_get_type ())

SnapdGetAssertions *
_snapd_get_assertions_new (const gchar *type, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetAssertions *self = SNAPD_GET_ASSERTIONS (g_object_new (snapd_get_assertions_get_type (),
                                                                   "cancellable", cancellable,
                                                                   "ready-callback", callback,
                                                                   "ready-callback-data", user_data,
                                                                   NULL));
    self->type = g_strdup (type);

    return self;
}

GStrv
_snapd_get_assertions_get_assertions (SnapdGetAssertions *self)
{
    return self->assertions;
}

static SoupMessage *
generate_get_assertions_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetAssertions *self = SNAPD_GET_ASSERTIONS (request);

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/assertions/");
    g_string_append_uri_escaped (path, self->type, NULL, TRUE);

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_assertions_response (SnapdRequest *request, SoupMessage *message, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetAssertions *self = SNAPD_GET_ASSERTIONS (request);

    const gchar *content_type = soup_message_headers_get_content_type (message->response_headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonObject) response = _snapd_json_parse_response (message, body, maintenance, NULL, error);
        if (response == NULL)
            return FALSE;
        g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o (response, error);
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

    g_autoptr(GPtrArray) assertions = g_ptr_array_new ();
    gsize data_length, offset = 0;
    const gchar *data = g_bytes_get_data (body, &data_length);
    while (offset < data_length) {
        /* Headers terminated by double newline */
        gsize assertion_start = offset;
        while (offset < data_length && !g_str_has_prefix (data + offset, "\n\n"))
            offset++;
        offset += 2;

        /* Make a temporary assertion object to decode body length header */
        g_autofree gchar *content = g_strndup (data + assertion_start, offset - assertion_start);
        g_autoptr(SnapdAssertion) assertion = snapd_assertion_new (content);
        g_autofree gchar *body_length_header = snapd_assertion_get_header (assertion, "body-length");

        /* Skip over body */
        gsize body_length = body_length_header != NULL ? strtoul (body_length_header, NULL, 10) : 0;
        if (body_length > 0)
            offset += body_length + 2;

        /* Find end of signature */
        while (offset < data_length && !g_str_has_prefix (data + offset, "\n\n"))
            offset++;
        gsize assertion_end = offset;
        offset += 2;

        g_ptr_array_add (assertions, g_strndup (data + assertion_start, assertion_end - assertion_start));
    }
    g_ptr_array_add (assertions, NULL);

    self->assertions = g_steal_pointer ((GStrv *)&assertions->pdata);

    return TRUE;
}

static void
snapd_get_assertions_finalize (GObject *object)
{
    SnapdGetAssertions *self = SNAPD_GET_ASSERTIONS (object);

    g_clear_pointer (&self->type, g_free);
    g_clear_pointer (&self->assertions, g_strfreev);

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
snapd_get_assertions_init (SnapdGetAssertions *self)
{
}
