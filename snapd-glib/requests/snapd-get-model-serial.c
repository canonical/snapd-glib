/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-model-serial.h"

#include "snapd-error.h"

struct _SnapdGetModelSerial
{
    SnapdRequest parent_instance;
    gchar *serial_assertion;
};

G_DEFINE_TYPE (SnapdGetModelSerial, snapd_get_model_serial, snapd_request_get_type ())

SnapdGetModelSerial *
_snapd_get_model_serial_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_MODEL_SERIAL (g_object_new (snapd_get_model_serial_get_type (),
                                                 "cancellable", cancellable,
                                                 "ready-callback", callback,
                                                 "ready-callback-data", user_data,
                                                 NULL));
}

const gchar *
_snapd_get_model_serial_get_serial_assertion (SnapdGetModelSerial *self)
{
    return self->serial_assertion;
}

static SoupMessage *
generate_get_model_serial_request (SnapdRequest *request, GBytes **body)
{
    return soup_message_new ("GET", "http://snapd/v2/model/serial");
}

static gboolean
parse_get_model_serial_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetModelSerial *self = SNAPD_GET_MODEL_SERIAL (request);

    if (g_strcmp0 (content_type, "application/x.ubuntu.assertion") != 0) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Got unknown content type '%s' retrieving serial assertion", content_type);
        return FALSE;
    }

    gsize data_length;
    const gchar *data = g_bytes_get_data (body, &data_length);
    self->serial_assertion = g_strndup (data, data_length);

    return TRUE;
}

static void
snapd_get_model_serial_finalize (GObject *object)
{
    SnapdGetModelSerial *self = SNAPD_GET_MODEL_SERIAL (object);

    g_clear_pointer (&self->serial_assertion, g_free);

    G_OBJECT_CLASS (snapd_get_model_serial_parent_class)->finalize (object);
}

static void
snapd_get_model_serial_class_init (SnapdGetModelSerialClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_model_serial_request;
   request_class->parse_response = parse_get_model_serial_response;
   gobject_class->finalize = snapd_get_model_serial_finalize;
}

static void
snapd_get_model_serial_init (SnapdGetModelSerial *self)
{
}
