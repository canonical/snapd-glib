/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-put-snap-conf.h"

#include "snapd-json.h"

struct _SnapdPutSnapConf
{
    SnapdRequestAsync parent_instance;
    gchar *name;
    GHashTable *key_values;
};

G_DEFINE_TYPE (SnapdPutSnapConf, snapd_put_snap_conf, snapd_request_async_get_type ())

SnapdPutSnapConf *
_snapd_put_snap_conf_new (const gchar *name, GHashTable *key_values, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPutSnapConf *self = SNAPD_PUT_SNAP_CONF (g_object_new (snapd_put_snap_conf_get_type (),
                                                                "cancellable", cancellable,
                                                                "ready-callback", callback,
                                                                "ready-callback-data", user_data,
                                                                NULL));
    self->name = g_strdup (name);
    self->key_values = g_hash_table_ref (key_values);

    return self;
}

static SoupMessage *
generate_put_snap_conf_request (SnapdRequest *request)
{
    SnapdPutSnapConf *self = SNAPD_PUT_SNAP_CONF (request);

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/snaps/");
    g_string_append_uri_escaped (path, self->name, NULL, TRUE);
    g_string_append (path, "/conf");
    SoupMessage *message = soup_message_new ("PUT", path->str);

    g_autoptr(JsonBuilder) builder = json_builder_new ();
    json_builder_begin_object (builder);
    GHashTableIter iter;
    g_hash_table_iter_init (&iter, self->key_values);
    gpointer key, value;
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        const gchar *conf_key = key;
        GVariant *conf_value = value;

        json_builder_set_member_name (builder, conf_key);
        json_builder_add_value (builder, json_gvariant_serialize (conf_value));
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
snapd_put_snap_conf_finalize (GObject *object)
{
    SnapdPutSnapConf *self = SNAPD_PUT_SNAP_CONF (object);

    g_clear_pointer (&self->name, g_free);
    g_clear_pointer (&self->key_values, g_hash_table_unref);

    G_OBJECT_CLASS (snapd_put_snap_conf_parent_class)->finalize (object);
}

static void
snapd_put_snap_conf_class_init (SnapdPutSnapConfClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_put_snap_conf_request;
   gobject_class->finalize = snapd_put_snap_conf_finalize;
}

static void
snapd_put_snap_conf_init (SnapdPutSnapConf *self)
{
}
