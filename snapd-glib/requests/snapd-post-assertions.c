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
    SnapdPostAssertions *self = SNAPD_POST_ASSERTIONS (g_object_new (snapd_post_assertions_get_type (),
                                                                     "cancellable", cancellable,
                                                                     "ready-callback", callback,
                                                                     "ready-callback-data", user_data,
                                                                     NULL));
    self->assertions = g_strdupv (assertions);

    return self;
}

static SoupMessage *
generate_post_assertions_request (SnapdRequest *request)
{
    SnapdPostAssertions *self = SNAPD_POST_ASSERTIONS (request);

    SoupMessage *message = soup_message_new ("POST", "http://snapd/v2/assertions");

    soup_message_headers_set_content_type (message->request_headers, "application/x.ubuntu.assertion", NULL); //FIXME
    for (int i = 0; self->assertions[i]; i++) {
        if (i != 0)
            soup_message_body_append (message->request_body, SOUP_MEMORY_TEMPORARY, "\n\n", 2);
        soup_message_body_append (message->request_body, SOUP_MEMORY_TEMPORARY, self->assertions[i], strlen (self->assertions[i]));
    }
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);

    return message;
}

static gboolean
parse_post_assertions_response (SnapdRequest *request, SoupMessage *message, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_post_assertions_finalize (GObject *object)
{
    SnapdPostAssertions *self = SNAPD_POST_ASSERTIONS (object);

    g_clear_pointer (&self->assertions, g_strfreev);

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
snapd_post_assertions_init (SnapdPostAssertions *self)
{
}
