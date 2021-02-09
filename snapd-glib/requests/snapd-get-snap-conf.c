/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-snap-conf.h"

#include "snapd-json.h"

struct _SnapdGetSnapConf
{
    SnapdRequest parent_instance;
    gchar *name;
    GStrv keys;
    GHashTable *conf;
};

G_DEFINE_TYPE (SnapdGetSnapConf, snapd_get_snap_conf, snapd_request_get_type ())

SnapdGetSnapConf *
_snapd_get_snap_conf_new (const gchar *name, GStrv keys, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSnapConf *self = SNAPD_GET_SNAP_CONF (g_object_new (snapd_get_snap_conf_get_type (),
                                                                "cancellable", cancellable,
                                                                "ready-callback", callback,
                                                                "ready-callback-data", user_data,
                                                                NULL));
    self->name = g_strdup (name);
    if (keys != NULL && keys[0] != NULL)
        self->keys = g_strdupv (keys);

    return self;
}

GHashTable *
_snapd_get_snap_conf_get_conf (SnapdGetSnapConf *self)
{
    return self->conf;
}

static SoupMessage *
generate_get_snap_conf_request (SnapdRequest *request)
{
    SnapdGetSnapConf *self = SNAPD_GET_SNAP_CONF (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->keys != NULL) {
        g_autofree gchar *keys_list = g_strjoinv (",", self->keys);
        g_ptr_array_add (query_attributes, g_strdup_printf ("keys=%s", keys_list));
    }

    g_autoptr(GString) path = g_string_new ("");
    g_autofree gchar *escaped = soup_uri_encode (self->name, NULL);
    g_string_append_printf (path, "http://snapd/v2/snaps/%s/conf", escaped);
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
parse_get_snap_conf_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetSnapConf *self = SNAPD_GET_SNAP_CONF (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonObject) result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    self->conf = _snapd_json_parse_object (result, error);
    if (self->conf == NULL)
        return FALSE;

    return TRUE;
}

static void
snapd_get_snap_conf_finalize (GObject *object)
{
    SnapdGetSnapConf *self = SNAPD_GET_SNAP_CONF (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->keys, g_strfreev);
    g_clear_pointer (&self->conf, g_hash_table_unref);

    G_OBJECT_CLASS (snapd_get_snap_conf_parent_class)->finalize (object);
}

static void
snapd_get_snap_conf_class_init (SnapdGetSnapConfClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_snap_conf_request;
   request_class->parse_response = parse_get_snap_conf_response;
   gobject_class->finalize = snapd_get_snap_conf_finalize;
}

static void
snapd_get_snap_conf_init (SnapdGetSnapConf *self)
{
}
