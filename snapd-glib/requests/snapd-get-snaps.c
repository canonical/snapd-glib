/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-snaps.h"

#include "snapd-json.h"

struct _SnapdGetSnaps
{
    SnapdRequest parent_instance;
    gchar *select;
    GStrv names;
    GPtrArray *snaps;
};

G_DEFINE_TYPE (SnapdGetSnaps, snapd_get_snaps, snapd_request_get_type ())

SnapdGetSnaps *
_snapd_get_snaps_new (GCancellable *cancellable, GStrv names, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSnaps *self = SNAPD_GET_SNAPS (g_object_new (snapd_get_snaps_get_type (),
                                                         "cancellable", cancellable,
                                                         "ready-callback", callback,
                                                         "ready-callback-data", user_data,
                                                         NULL));
   if (names != NULL && names[0] != NULL)
       self->names = g_strdupv (names);

   return self;
}

void
_snapd_get_snaps_set_select (SnapdGetSnaps *self, const gchar *select)
{
    g_free (self->select);
    self->select = g_strdup (select);
}

GPtrArray *
_snapd_get_snaps_get_snaps (SnapdGetSnaps *self)
{
    return self->snaps;
}

static SoupMessage *
generate_get_snaps_request (SnapdRequest *request)
{
    SnapdGetSnaps *self = SNAPD_GET_SNAPS (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->select != NULL)
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=%s", self->select));
    if (self->names != NULL) {
        g_autofree gchar *names_list = g_strjoinv (",", self->names);
        g_ptr_array_add (query_attributes, g_strdup_printf ("snaps=%s", names_list));
    }

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/snaps");
    if (query_attributes->len > 0) {
        g_string_append_c (path, '?');
        for (guint i = 0; i < query_attributes->len; i++) {
            if (i != 0)
                g_string_append_c (path, '&');
            g_string_append (path, (gchar *) query_attributes->pdata[i]);
        }
    }

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_snaps_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetSnaps *self = SNAPD_GET_SNAPS (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) snaps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        SnapdSnap *snap;

        snap = _snapd_json_parse_snap (node, error);
        if (snap == NULL)
            return FALSE;

        g_ptr_array_add (snaps, snap);
    }

    self->snaps = g_steal_pointer (&snaps);

    return TRUE;
}

static void
snapd_get_snaps_finalize (GObject *object)
{
    SnapdGetSnaps *self = SNAPD_GET_SNAPS (object);

    g_clear_pointer (&self->select, g_free);
    g_clear_pointer (&self->names, g_strfreev);
    g_clear_pointer (&self->snaps, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_snaps_parent_class)->finalize (object);
}

static void
snapd_get_snaps_class_init (SnapdGetSnapsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_snaps_request;
   request_class->parse_response = parse_get_snaps_response;
   gobject_class->finalize = snapd_get_snaps_finalize;
}

static void
snapd_get_snaps_init (SnapdGetSnaps *self)
{
}
