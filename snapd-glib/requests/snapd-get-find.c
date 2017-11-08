/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-find.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetFind
{
    SnapdRequest parent_instance;
    SnapdFindFlags flags;
    gchar *query;
    gchar *section;
    gchar *suggested_currency;
    GPtrArray *snaps;
};

G_DEFINE_TYPE (SnapdGetFind, snapd_get_find, snapd_request_get_type ())

SnapdGetFind *
_snapd_get_find_new (SnapdFindFlags flags, const gchar *section, const gchar *query, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetFind *request;

    request = SNAPD_GET_FIND (g_object_new (snapd_get_find_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
    request->flags = flags;
    request->section = g_strdup (section);
    request->query = g_strdup (query);

    return request;
}

GPtrArray *
_snapd_get_find_get_snaps (SnapdGetFind *request)
{
    return request->snaps;
}

const gchar *
_snapd_get_find_get_suggested_currency (SnapdGetFind *request)
{
    return request->suggested_currency;
}

static SoupMessage *
generate_get_find_request (SnapdRequest *request)
{
    SnapdGetFind *r = SNAPD_GET_FIND (request);
    g_autoptr(GPtrArray) query_attributes = NULL;
    g_autoptr(GString) path = NULL;

    query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (r->query != NULL) {
        g_autofree gchar *escaped = soup_uri_encode (r->query, NULL);
        if ((r->flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
            g_ptr_array_add (query_attributes, g_strdup_printf ("name=%s", escaped));
        else
            g_ptr_array_add (query_attributes, g_strdup_printf ("q=%s", escaped));
    }

    if ((r->flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=private"));
    else if ((r->flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        g_ptr_array_add (query_attributes, g_strdup_printf ("select=refresh"));

    if (r->section != NULL) {
        g_autofree gchar *escaped = soup_uri_encode (r->section, NULL);
        g_ptr_array_add (query_attributes, g_strdup_printf ("section=%s", escaped));
    }

    path = g_string_new ("http://snapd/v2/find");
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
parse_get_find_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetFind *r = SNAPD_GET_FIND (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) snaps = NULL;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    snaps = _snapd_json_parse_snap_array (result, error);
    if (snaps == NULL)
        return FALSE;

    r->snaps = g_steal_pointer (&snaps);
    r->suggested_currency = g_strdup (_snapd_json_get_string (response, "suggested-currency", NULL));

    return TRUE;
}

static void
snapd_get_find_finalize (GObject *object)
{
    SnapdGetFind *request = SNAPD_GET_FIND (object);

    g_free (request->query);
    g_free (request->section);
    g_free (request->suggested_currency);
    g_clear_pointer (&request->snaps, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_find_parent_class)->finalize (object);
}

static void
snapd_get_find_class_init (SnapdGetFindClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_find_request;
   request_class->parse_response = parse_get_find_response;
   gobject_class->finalize = snapd_get_find_finalize;
}

static void
snapd_get_find_init (SnapdGetFind *request)
{
}
