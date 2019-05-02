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
    SnapdGetApps *request;

    request = SNAPD_GET_APPS (g_object_new (snapd_get_apps_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
   if (snaps != NULL && snaps[0] != NULL)
       request->snaps = g_strdupv (snaps);

    return request;
}

void
_snapd_get_apps_set_select (SnapdGetApps *request, const gchar *select)
{
    g_free (request->select);
    request->select = g_strdup (select);
}

GPtrArray *
_snapd_get_apps_get_apps (SnapdGetApps *request)
{
    return request->apps;
}

static SoupMessage *
generate_get_apps_request (SnapdRequest *request)
{
    SnapdGetApps *r = SNAPD_GET_APPS (request);
    g_autoptr(GPtrArray) query_attributes = NULL;
    g_autoptr(GString) path = NULL;

    query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (r->select != NULL) {
        g_autofree gchar *escaped = soup_uri_encode (r->select, NULL);
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=%s", escaped));
    }
    if (r->snaps != NULL) {
        g_autofree gchar *snaps_list = g_strjoinv (",", r->snaps);
        g_ptr_array_add (query_attributes, g_strdup_printf ("names=%s", snaps_list));
    }

    path = g_string_new ("http://snapd/v2/apps");
    if (query_attributes->len > 0) {
        guint i;

        g_string_append_c (path, '?');
        for (i = 0; i < query_attributes->len; i++) {
            if (i != 0)
                g_string_append_c (path, '&');
            g_string_append (path, (gchar *) query_attributes->pdata[i]);
        }
    }

    return soup_message_new ("GET", path->str);
}

static gboolean
parse_get_apps_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetApps *r = SNAPD_GET_APPS (request);
    g_autoptr(JsonObject) response = NULL;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result;
    GPtrArray *apps;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    apps = _snapd_json_parse_app_array (result, error);
    json_node_unref (result);
    if (apps == NULL)
        return FALSE;

    r->apps = g_steal_pointer (&apps);

    return TRUE;
}

static void
snapd_get_apps_finalize (GObject *object)
{
    SnapdGetApps *request = SNAPD_GET_APPS (object);

    g_clear_pointer (&request->select, g_free);
    g_clear_pointer (&request->snaps, g_strfreev);
    g_clear_pointer (&request->apps, g_ptr_array_unref);

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
snapd_get_apps_init (SnapdGetApps *request)
{
}
