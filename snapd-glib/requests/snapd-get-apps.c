/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-apps.h"

#include "snapd-json.h"

struct _SnapdGetApps
{
    SnapdRequest parent_instance;
    gchar *select;
    GStrv snaps;
    GPtrArray *apps;
};

G_DEFINE_TYPE (SnapdGetApps, snapd_get_apps, snapd_request_get_type ())

SnapdGetApps *
_snapd_get_apps_new (GStrv snaps, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetApps *self = SNAPD_GET_APPS (g_object_new (snapd_get_apps_get_type (),
                                                       "cancellable", cancellable,
                                                       "ready-callback", callback,
                                                       "ready-callback-data", user_data,
                                                       NULL));
   if (snaps != NULL && snaps[0] != NULL)
       self->snaps = g_strdupv (snaps);

    return self;
}

void
_snapd_get_apps_set_select (SnapdGetApps *self, const gchar *select)
{
    g_free (self->select);
    self->select = g_strdup (select);
}

GPtrArray *
_snapd_get_apps_get_apps (SnapdGetApps *self)
{
    return self->apps;
}

static SoupMessage *
generate_get_apps_request (SnapdRequest *request)
{
    SnapdGetApps *self = SNAPD_GET_APPS (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->select != NULL) {
        g_autoptr(GString) attr = g_string_new("select=");
        g_string_append_uri_escaped (attr, self->select, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->snaps != NULL) {
        g_autoptr(GString) attr = g_string_new("names=");
        for (guint i = 0; self->snaps[i] != NULL; i++) {
            if (i != 0)
                g_string_append (attr, ",");
            g_string_append_uri_escaped (attr, self->snaps[i], NULL, TRUE);
        }
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/apps");
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
parse_get_apps_response (SnapdRequest *request, SoupMessage *message, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetApps *self = SNAPD_GET_APPS (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) apps = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);

        SnapdApp *app = _snapd_json_parse_app (node, NULL, error);
        if (app == NULL)
            return FALSE;

        g_ptr_array_add (apps, app);
    }

    self->apps = g_steal_pointer (&apps);

    return TRUE;
}

static void
snapd_get_apps_finalize (GObject *object)
{
    SnapdGetApps *self = SNAPD_GET_APPS (object);

    g_clear_pointer (&self->select, g_free);
    g_clear_pointer (&self->snaps, g_strfreev);
    g_clear_pointer (&self->apps, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_apps_parent_class)->finalize (object);
}

static void
snapd_get_apps_class_init (SnapdGetAppsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_apps_request;
   request_class->parse_response = parse_get_apps_response;
   gobject_class->finalize = snapd_get_apps_finalize;
}

static void
snapd_get_apps_init (SnapdGetApps *self)
{
}
