/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-notices.h"

#include "snapd-json.h"

struct _SnapdGetNotices
{
    SnapdRequest parent_instance;
    gchar *user_id;
    gchar *users;
    gchar *types;
    gchar *keys;
    GDateTime *since_date_time;
    GTimeSpan timeout;
    int since_date_time_nanoseconds;

    GPtrArray *notices;
};

G_DEFINE_TYPE (SnapdGetNotices, snapd_get_notices, snapd_request_get_type ())

SnapdGetNotices *
_snapd_get_notices_new (gchar               *user_id,
                        gchar               *users,
                        gchar               *types,
                        gchar               *keys,
                        GDateTime           *since_date_time,
                        int                  since_date_time_nanoseconds,
                        GTimeSpan            timeout,
                        GCancellable        *cancellable,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
    SnapdGetNotices *self = SNAPD_GET_NOTICES (g_object_new (snapd_get_notices_get_type (),
                                               "cancellable", cancellable,
                                               "ready-callback", callback,
                                               "ready-callback-data", user_data,
                                               NULL));

    self->user_id = g_strdup (user_id);
    self->users = g_strdup (users);
    self->types = g_strdup (types);
    self->keys = g_strdup (keys);
    self->since_date_time = since_date_time == NULL ? NULL : g_date_time_ref (since_date_time);
    self->timeout = timeout;
    self->since_date_time_nanoseconds = since_date_time_nanoseconds;
    return self;
}

static void
add_uri_parameter_base (GString *query, const gchar *name)
{
    if (query->len != 0)
        g_string_append (query, "&");
    g_string_append (query, name);
    g_string_append (query, "=");
}

static void
add_uri_parameter (GString *query, const gchar *name, const gchar *value)
{
    if ((value == NULL) || (*value == 0))
        return;
    add_uri_parameter_base (query, name);
    g_autofree gchar *value_escaped = g_uri_escape_string (value, NULL, TRUE);
    g_string_append (query, value_escaped);
}

static SoupMessage *
generate_get_snap_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetNotices *self = SNAPD_GET_NOTICES (request);
    g_autoptr(GString) query = g_string_new ("");
    add_uri_parameter (query, "user-id", self->user_id);
    add_uri_parameter (query, "users", self->users);
    add_uri_parameter (query, "types", self->types);
    add_uri_parameter (query, "keys", self->keys);
    if (self->since_date_time != NULL) {
        g_autofree gchar *date_time = NULL;
        if (self->since_date_time_nanoseconds == -1) {
            date_time = g_date_time_format (self->since_date_time, "%FT%T.%f%:z");
        } else {
            if (self->since_date_time_nanoseconds == 0) {
                date_time = g_date_time_format (self->since_date_time, "%FT%T%:z");
            } else {
                // create a temporal date-time string with a %09d between seconds and timezone
                g_autofree gchar *date_time_tmp = g_date_time_format (self->since_date_time, "%FT%T.%%09d%:z");
                // Now use that %s to include the nanoseconds
                date_time = g_strdup_printf(date_time_tmp, self->since_date_time_nanoseconds);
            }
        }
        add_uri_parameter (query, "after", date_time);
        self->since_date_time_nanoseconds = -1;
    }
    if (self->timeout != 0) {
        add_uri_parameter_base (query, "timeout");
        // timeout in microseconds
        g_string_append_printf (query, "%luus", self->timeout);
    }

    if (query->len != 0)
        g_string_prepend (query, "?");
    g_string_prepend (query, "http://snapd/v2/notices");
    return soup_message_new ("GET", query->str);
}

static gboolean
parse_get_snap_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetNotices *self = SNAPD_GET_NOTICES (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) notices = _snapd_json_parse_notices (result, error);
    json_node_unref (result);
    if (notices == NULL)
        return FALSE;
    g_clear_pointer (&self->notices, g_ptr_array_unref);
    self->notices = g_steal_pointer (&notices);

    return TRUE;
}

static void
snapd_get_notices_finalize (GObject *object)
{
    SnapdGetNotices *self = SNAPD_GET_NOTICES (object);

    g_clear_pointer (&self->user_id, g_free);
    g_clear_pointer (&self->users, g_free);
    g_clear_pointer (&self->types, g_free);
    g_clear_pointer (&self->keys, g_free);
    g_clear_pointer (&self->since_date_time, g_date_time_unref);
    g_clear_pointer (&self->notices, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_notices_parent_class)->finalize (object);
}

GPtrArray *
_snapd_get_notices_get_notices (SnapdGetNotices *self)
{
    g_return_val_if_fail (SNAPD_IS_GET_NOTICES (self), NULL);
    return self->notices;
}

static void
snapd_get_notices_class_init (SnapdGetNoticesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_snap_request;
   request_class->parse_response = parse_get_snap_response;
   gobject_class->finalize = snapd_get_notices_finalize;
}

static void
snapd_get_notices_init (SnapdGetNotices *self)
{
    self->since_date_time_nanoseconds = -1;
}
