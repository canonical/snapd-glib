/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-post-assertions.h"

#include "snapd-json.h"

struct _SnapdPostAssertions
{
    SnapdRequest parent_instance;
    GStrv assertions;
};

G_DEFINE_TYPE (SnapdPostAssertions, snapd_post_assertions, snapd_request_get_type ())

SnapdPostAssertions *
_snapd_post_assertions_new (GStrv assertions,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostAssertions *request;

    request = SNAPD_POST_ASSERTIONS (g_object_new (snapd_post_assertions_get_type (),
                                                   "cancellable", cancellable,
                                                   "ready-callback", callback,
                                                   "ready-callback-data", user_data,
                                                   NULL));
    request->assertions = g_strdupv (assertions);

    return request;
}

static SoupMessage *
generate_post_assertions_request (SnapdRequest *request)
{
    SnapdPostAssertions *r = SNAPD_POST_ASSERTIONS (request);
    SoupMessage *message;
    int i;

    message = soup_message_new ("POST", "http://snapd/v2/assertions");

    soup_message_headers_set_content_type (message->request_headers, "application/x.ubuntu.assertion", NULL); //FIXME
    for (i = 0; r->assertions[i]; i++) {
        if (i != 0)
            soup_message_body_append (message->request_body, SOUP_MEMORY_TEMPORARY, "\n\n", 2);
        soup_message_body_append (message->request_body, SOUP_MEMORY_TEMPORARY, r->assertions[i], strlen (r->assertions[i]));
    }
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);

    return message;
}

static gboolean
parse_post_assertions_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    g_autoptr(JsonObject) response = NULL;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_post_assertions_finalize (GObject *object)
{
    SnapdPostAssertions *request = SNAPD_POST_ASSERTIONS (object);

    g_clear_pointer (&request->assertions, g_strfreev);

    G_OBJECT_CLASS (snapd_post_assertions_parent_class)->finalize (object);
}

static void
snapd_post_assertions_class_init (SnapdPostAssertionsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_assertions_request;
   request_class->parse_response = parse_post_assertions_response;
   gobject_class->finalize = snapd_post_assertions_finalize;
}

static void
snapd_post_assertions_init (SnapdPostAssertions *request)
{
}
