/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-get-system-info.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetSystemInfo
{
    SnapdRequest parent_instance;
    SnapdSystemInformation *system_information;
};

G_DEFINE_TYPE (SnapdGetSystemInfo, snapd_get_system_info, snapd_request_get_type ())

SnapdGetSystemInfo *
_snapd_get_system_info_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_SYSTEM_INFO (g_object_new (snapd_get_system_info_get_type (),
                                                "cancellable", cancellable,
                                                "ready-callback", callback,
                                                "ready-callback-data", user_data,
                                                NULL));
}

SnapdSystemInformation *
_snapd_get_system_info_get_system_information (SnapdGetSystemInfo *self)
{
    return self->system_information;
}

static SoupMessage *
generate_get_system_info_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/system-info");
}

static gboolean
parse_get_system_info_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetSystemInfo *self = SNAPD_GET_SYSTEM_INFO (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(SnapdSystemInformation) system_information = _snapd_json_parse_system_information (result, error);
    json_node_unref (result);
    if (system_information == NULL)
        return FALSE;

    self->system_information = g_steal_pointer (&system_information);

    return TRUE;
}

static void
snapd_get_system_info_finalize (GObject *object)
{
    SnapdGetSystemInfo *self = SNAPD_GET_SYSTEM_INFO (object);

    g_clear_object (&self->system_information);

    G_OBJECT_CLASS (snapd_get_system_info_parent_class)->finalize (object);
}

static void
snapd_get_system_info_class_init (SnapdGetSystemInfoClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_system_info_request;
   request_class->parse_response = parse_get_system_info_response;
   gobject_class->finalize = snapd_get_system_info_finalize;
}

static void
snapd_get_system_info_init (SnapdGetSystemInfo *self)
{
}
