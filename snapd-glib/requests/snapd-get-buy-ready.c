/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-buy-ready.h"

#include "snapd-json.h"

struct _SnapdGetBuyReady
{
    SnapdRequest parent_instance;
};

G_DEFINE_TYPE (SnapdGetBuyReady, snapd_get_buy_ready, snapd_request_get_type ())

SnapdGetBuyReady *
_snapd_get_buy_ready_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_BUY_READY (g_object_new (snapd_get_buy_ready_get_type (),
                                              "cancellable", cancellable,
                                              "ready-callback", callback,
                                              "ready-callback-data", user_data,
                                              NULL));
}

static SoupMessage *
generate_get_buy_ready_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/buy/ready");
}

static void
parse_get_buy_ready_response (SnapdRequest *request, SoupMessage *message)
{
    g_autoptr(JsonObject) response = NULL;
    GError *error = NULL;

    response = _snapd_json_parse_response (message, &error);
    if (response == NULL) {
        _snapd_request_complete (request, error);
        return;
    }

    _snapd_request_complete (request, NULL);
}

static void
snapd_get_buy_ready_class_init (SnapdGetBuyReadyClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);

   request_class->generate_request = generate_get_buy_ready_request;
   request_class->parse_response = parse_get_buy_ready_response;
}

static void
snapd_get_buy_ready_init (SnapdGetBuyReady *request)
{
}
