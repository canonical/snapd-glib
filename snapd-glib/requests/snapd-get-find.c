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
    gchar *common_id;
    gchar *query;
    gchar *name;
    gchar *select;
    gchar *section;
    gchar *scope;
    gchar *suggested_currency;
    GPtrArray *snaps;
};

G_DEFINE_TYPE (SnapdGetFind, snapd_get_find, snapd_request_get_type ())

SnapdGetFind *
_snapd_get_find_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetFind *self = SNAPD_GET_FIND (g_object_new (snapd_get_find_get_type (),
                                                       "cancellable", cancellable,
                                                       "ready-callback", callback,
                                                       "ready-callback-data", user_data,
                                                       NULL));

    return self;
}

void
_snapd_get_find_set_common_id (SnapdGetFind *self, const gchar *common_id)
{
    g_free (self->common_id);
    self->common_id = g_strdup (common_id);
}

void
_snapd_get_find_set_query (SnapdGetFind *self, const gchar *query)
{
    g_free (self->query);
    self->query = g_strdup (query);
}

void
_snapd_get_find_set_name (SnapdGetFind *self, const gchar *name)
{
    g_free (self->name);
    self->name = g_strdup (name);
}

void
_snapd_get_find_set_select (SnapdGetFind *self, const gchar *select)
{
    g_free (self->select);
    self->select = g_strdup (select);
}

void
_snapd_get_find_set_section (SnapdGetFind *self, const gchar *section)
{
    g_free (self->section);
    self->section = g_strdup (section);
}

void
_snapd_get_find_set_scope (SnapdGetFind *self, const gchar *scope)
{
    g_free (self->scope);
    self->scope = g_strdup (scope);
}

GPtrArray *
_snapd_get_find_get_snaps (SnapdGetFind *self)
{
    return self->snaps;
}

const gchar *
_snapd_get_find_get_suggested_currency (SnapdGetFind *self)
{
    return self->suggested_currency;
}

static SoupMessage *
generate_get_find_request (SnapdRequest *request)
{
    SnapdGetFind *self = SNAPD_GET_FIND (request);

    g_autoptr(GPtrArray) query_attributes = g_ptr_array_new_with_free_func (g_free);
    if (self->common_id != NULL) {
        g_autoptr(GString) attr = g_string_new ("common-id=");
        g_string_append_uri_escaped (attr, self->common_id, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->query != NULL) {
        g_autoptr(GString) attr = g_string_new ("q=");
        g_string_append_uri_escaped (attr, self->query, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->name != NULL) {
        g_autoptr(GString) attr = g_string_new ("name=");
        g_string_append_uri_escaped (attr, self->name, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->select != NULL) {
        g_autoptr(GString) attr = g_string_new ("select=");
        g_string_append_uri_escaped (attr, self->select, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->section != NULL) {
        g_autoptr(GString) attr = g_string_new ("section=");
        g_string_append_uri_escaped (attr, self->section, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }
    if (self->scope != NULL) {
        g_autoptr(GString) attr = g_string_new ("scope=");
        g_string_append_uri_escaped (attr, self->scope, NULL, TRUE);
        g_ptr_array_add (query_attributes, g_strdup (attr->str));
    }

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/find");
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
parse_get_find_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetFind *self = SNAPD_GET_FIND (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (message, maintenance, NULL, error);
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
    self->suggested_currency = g_strdup (_snapd_json_get_string (response, "suggested-currency", NULL));

    return TRUE;
}

static void
snapd_get_find_finalize (GObject *object)
{
    SnapdGetFind *self = SNAPD_GET_FIND (object);

    g_free (self->common_id);
    g_free (self->query);
    g_free (self->name);
    g_free (self->select);
    g_free (self->section);
    g_free (self->scope);
    g_free (self->suggested_currency);
    g_clear_pointer (&self->snaps, g_ptr_array_unref);

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
snapd_get_find_init (SnapdGetFind *self)
{
}
