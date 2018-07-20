/*
 * Copyright (C) 2018 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-interface-info.h"

#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-plug.h"
#include "snapd-slot.h"

struct _SnapdGetInterfaceInfo
{
    SnapdRequest parent_instance;

    gchar **names;
    gboolean include_docs;
    gboolean include_plugs;
    gboolean include_slots;
    gboolean only_connected;

    GPtrArray *interfaces;
};

G_DEFINE_TYPE (SnapdGetInterfaceInfo, snapd_get_interface_info, snapd_request_get_type ())

SnapdGetInterfaceInfo *
_snapd_get_interface_info_new (gchar **names,
                               GCancellable *cancellable,
                               GAsyncReadyCallback callback,
                               gpointer user_data)
{
    SnapdGetInterfaceInfo *request;

    request = g_object_new (snapd_get_interface_info_get_type (),
                            "cancellable", cancellable,
                            "ready-callback", callback,
                            "ready-callback-data", user_data,
                            NULL);

    if (names != NULL && names[0] != NULL)
        request->names = g_strdupv (names);

    return request;
}

void
_snapd_get_interface_info_set_include_docs (SnapdGetInterfaceInfo *request,
                                            gboolean include_docs)
{
    request->include_docs = include_docs;
}

void
_snapd_get_interface_info_set_include_plugs (SnapdGetInterfaceInfo *request,
                                             gboolean include_plugs)
{
    request->include_plugs = include_plugs;
}

void
_snapd_get_interface_info_set_include_slots (SnapdGetInterfaceInfo *request,
                                             gboolean include_slots)
{
    request->include_slots = include_slots;
}

void
_snapd_get_interface_info_set_only_connected (SnapdGetInterfaceInfo *request,
                                              gboolean only_connected)
{
    request->only_connected = only_connected;
}

GPtrArray *
_snapd_get_interface_info_get_interfaces (SnapdGetInterfaceInfo *request)
{
    return request->interfaces;
}

static SoupMessage *
generate_get_interfaces_request (SnapdRequest *request)
{
    SnapdGetInterfaceInfo *r = SNAPD_GET_INTERFACE_INFO (request);
    g_autoptr(GPtrArray) query_attributes = NULL;
    g_autoptr(GString) path = NULL;
    guint i;

    query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (r->names != NULL) {
        g_autofree gchar *names_list = g_strjoinv (",", r->names);
        g_ptr_array_add (query_attributes, g_strdup_printf ("names=%s", names_list));
    }
    if (r->include_docs)
        g_ptr_array_add (query_attributes, g_strdup ("doc=true"));
    if (r->include_plugs)
        g_ptr_array_add (query_attributes, g_strdup ("plugs=true"));
    if (r->include_slots)
        g_ptr_array_add (query_attributes, g_strdup ("slots=true"));
    g_ptr_array_add (query_attributes, g_strdup_printf ("select=%s", r->only_connected ? "connected" : "all"));

    path = g_string_new ("http://snapd/v2/interfaces?");
    for (i = 0; i < query_attributes->len; i++) {
        if (i != 0)
            g_string_append_c (path, '&');
        g_string_append (path, (gchar *) query_attributes->pdata[i]);
    }

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_interfaces_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetInterfaceInfo *r = SNAPD_GET_INTERFACE_INFO (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    r->interfaces = _snapd_json_parse_interface_array (result, error);
    if (r->interfaces == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_get_interface_info_finalize (GObject *object)
{
    SnapdGetInterfaceInfo *request = SNAPD_GET_INTERFACE_INFO (object);

    g_clear_pointer (&request->names, g_strfreev);
    g_clear_pointer (&request->interfaces, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_interface_info_parent_class)->finalize (object);
}

static void
snapd_get_interface_info_class_init (SnapdGetInterfaceInfoClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_interfaces_request;
   request_class->parse_response = parse_get_interfaces_response;
   gobject_class->finalize = snapd_get_interface_info_finalize;
}

static void
snapd_get_interface_info_init (SnapdGetInterfaceInfo *request)
{
}
