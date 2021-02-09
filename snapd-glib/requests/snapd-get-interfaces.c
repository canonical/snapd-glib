/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-interfaces.h"

#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-plug.h"
#include "snapd-slot.h"

struct _SnapdGetInterfaces
{
    SnapdRequest parent_instance;

    gchar **names;
    gboolean include_docs;
    gboolean include_plugs;
    gboolean include_slots;
    gboolean only_connected;

    GPtrArray *interfaces;
};

G_DEFINE_TYPE (SnapdGetInterfaces, snapd_get_interfaces, snapd_request_get_type ())

SnapdGetInterfaces *
_snapd_get_interfaces_new (gchar **names, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetInterfaces *self = g_object_new (snapd_get_interfaces_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL);

    if (names != NULL && names[0] != NULL)
        self->names = g_strdupv (names);

    return self;
}

void
_snapd_get_interfaces_set_include_docs (SnapdGetInterfaces *self, gboolean include_docs)
{
    self->include_docs = include_docs;
}

void
_snapd_get_interfaces_set_include_plugs (SnapdGetInterfaces *self, gboolean include_plugs)
{
    self->include_plugs = include_plugs;
}

void
_snapd_get_interfaces_set_include_slots (SnapdGetInterfaces *self, gboolean include_slots)
{
    self->include_slots = include_slots;
}

void
_snapd_get_interfaces_set_only_connected (SnapdGetInterfaces *self, gboolean only_connected)
{
    self->only_connected = only_connected;
}

GPtrArray *
_snapd_get_interfaces_get_interfaces (SnapdGetInterfaces *self)
{
    return self->interfaces;
}

static SoupMessage *
generate_get_interfaces_request (SnapdRequest *request)
{
    SnapdGetInterfaces *self = SNAPD_GET_INTERFACES (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->names != NULL) {
        g_autofree gchar *names_list = g_strjoinv (",", self->names);
        g_ptr_array_add (query_attributes, g_strdup_printf ("names=%s", names_list));
    }
    if (self->include_docs)
        g_ptr_array_add (query_attributes, g_strdup ("doc=true"));
    if (self->include_plugs)
        g_ptr_array_add (query_attributes, g_strdup ("plugs=true"));
    if (self->include_slots)
        g_ptr_array_add (query_attributes, g_strdup ("slots=true"));
    g_ptr_array_add (query_attributes, g_strdup_printf ("select=%s", self->only_connected ? "connected" : "all"));

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/interfaces?");
    for (guint i = 0; i < query_attributes->len; i++) {
        if (i != 0)
            g_string_append_c (path, '&');
        g_string_append (path, (gchar *) query_attributes->pdata[i]);
    }

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_interfaces_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetInterfaces *self = SNAPD_GET_INTERFACES (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, NULL, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) interfaces = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        SnapdInterface *interface;

        interface = _snapd_json_parse_interface (node, error);
        if (interface == NULL)
            return FALSE;

        g_ptr_array_add (interfaces, interface);
    }

    self->interfaces = g_steal_pointer (&interfaces);

    return TRUE;
}

static void
snapd_get_interfaces_finalize (GObject *object)
{
    SnapdGetInterfaces *self = SNAPD_GET_INTERFACES (object);

    g_clear_pointer (&self->names, g_strfreev);
    g_clear_pointer (&self->interfaces, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_interfaces_parent_class)->finalize (object);
}

static void
snapd_get_interfaces_class_init (SnapdGetInterfacesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_interfaces_request;
   request_class->parse_response = parse_get_interfaces_response;
   gobject_class->finalize = snapd_get_interfaces_finalize;
}

static void
snapd_get_interfaces_init (SnapdGetInterfaces *self)
{
}
