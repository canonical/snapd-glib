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
_snapd_get_system_info_get_system_information (SnapdGetSystemInfo *request)
{
    return request->system_information;
}

static SoupMessage *
generate_get_system_info_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/system-info");
}

static gboolean
parse_get_system_info_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetSystemInfo *r = SNAPD_GET_SYSTEM_INFO (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(SnapdSystemInformation) system_information = NULL;
    const gchar *confinement_string;
    SnapdSystemConfinement confinement = SNAPD_SYSTEM_CONFINEMENT_UNKNOWN;
    JsonObject *os_release, *locations;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    confinement_string = _snapd_json_get_string (result, "confinement", "");
    if (strcmp (confinement_string, "strict") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_STRICT;
    else if (strcmp (confinement_string, "partial") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_PARTIAL;
    os_release = _snapd_json_get_object (result, "os-release");
    locations  = _snapd_json_get_object (result, "locations");
    system_information = g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                                       "binaries-directory", locations != NULL ? _snapd_json_get_string (locations, "snap-bin-dir", NULL) : NULL,
                                       "build-id", _snapd_json_get_string (result, "build-id", NULL),
                                       "confinement", confinement,
                                       "kernel-version", _snapd_json_get_string (result, "kernel-version", NULL),
                                       "managed", _snapd_json_get_bool (result, "managed", FALSE),
                                       "mount-directory", locations != NULL ? _snapd_json_get_string (locations, "snap-mount-dir", NULL) : NULL,
                                       "on-classic", _snapd_json_get_bool (result, "on-classic", FALSE),
                                       "os-id", os_release != NULL ? _snapd_json_get_string (os_release, "id", NULL) : NULL,
                                       "os-version", os_release != NULL ? _snapd_json_get_string (os_release, "version-id", NULL) : NULL,
                                       "series", _snapd_json_get_string (result, "series", NULL),
                                       "store", _snapd_json_get_string (result, "store", NULL),
                                       "version", _snapd_json_get_string (result, "version", NULL),
                                       NULL);
    r->system_information = g_steal_pointer (&system_information);

    return TRUE;
}

static void
snapd_get_system_info_finalize (GObject *object)
{
    SnapdGetSystemInfo *request = SNAPD_GET_SYSTEM_INFO (object);

    g_clear_object (&request->system_information);

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
snapd_get_system_info_init (SnapdGetSystemInfo *request)
{
}
