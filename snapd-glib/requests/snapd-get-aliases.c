/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>

#include "snapd-get-aliases.h"

#include "snapd-alias.h"
#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetAliases
{
    SnapdRequest parent_instance;
    GPtrArray *aliases;
};

G_DEFINE_TYPE (SnapdGetAliases, snapd_get_aliases, snapd_request_get_type ())

SnapdGetAliases *
_snapd_get_aliases_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_ALIASES (g_object_new (snapd_get_aliases_get_type (),
                                            "cancellable", cancellable,
                                            "ready-callback", callback,
                                            "ready-callback-data", user_data,
                                            NULL));
}

GPtrArray *
_snapd_get_aliases_get_aliases (SnapdGetAliases *request)
{
    return request->aliases;
}

static SoupMessage *
generate_get_aliases_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/aliases");
}

static gboolean
parse_get_aliases_response (SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetAliases *r = SNAPD_GET_ALIASES (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonObject) result = NULL;
    g_autoptr(GPtrArray) aliases = NULL;
    JsonObjectIter snap_iter;
    const gchar *snap;
    JsonNode *snap_node;

    response = _snapd_json_parse_response (message, maintenance, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_o (response, error);
    if (result == NULL)
        return FALSE;

    aliases = g_ptr_array_new_with_free_func (g_object_unref);
    json_object_iter_init (&snap_iter, result);
    while (json_object_iter_next (&snap_iter, &snap, &snap_node)) {
        JsonObjectIter alias_iter;
        const gchar *name;
        JsonNode *alias_node;

        if (json_node_get_value_type (snap_node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected alias type");
            return FALSE;
        }

        json_object_iter_init (&alias_iter, json_node_get_object (snap_node));
        while (json_object_iter_next (&alias_iter, &name, &alias_node)) {
            JsonObject *o;
            SnapdAliasStatus status = SNAPD_ALIAS_STATUS_UNKNOWN;
            const gchar *status_string;
            g_autoptr(SnapdAlias) alias = NULL;

            if (json_node_get_value_type (alias_node) != JSON_TYPE_OBJECT) {
                g_set_error (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected alias type");
                return FALSE;
            }

            o = json_node_get_object (alias_node);
            status_string = _snapd_json_get_string (o, "status", NULL);
            if (strcmp (status_string, "disabled") == 0)
                status = SNAPD_ALIAS_STATUS_DISABLED;
            else if (strcmp (status_string, "auto") == 0)
                status = SNAPD_ALIAS_STATUS_AUTO;
            else if (strcmp (status_string, "manual") == 0)
                status = SNAPD_ALIAS_STATUS_MANUAL;
            else
                status = SNAPD_ALIAS_STATUS_UNKNOWN;

            alias = g_object_new (SNAPD_TYPE_ALIAS,
                                  "snap", snap,
                                  "app-auto", _snapd_json_get_string (o, "auto", NULL),
                                  "app-manual", _snapd_json_get_string (o, "manual", NULL),
                                  "command", _snapd_json_get_string (o, "command", NULL),
                                  "name", name,
                                  "status", status,
                                  NULL);
            g_ptr_array_add (aliases, g_steal_pointer (&alias));
        }
    }

    r->aliases = g_steal_pointer (&aliases);

    return TRUE;
}

static void
snapd_get_aliases_finalize (GObject *object)
{
    SnapdGetAliases *request = SNAPD_GET_ALIASES (object);

    g_clear_pointer (&request->aliases, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_aliases_parent_class)->finalize (object);
}

static void
snapd_get_aliases_class_init (SnapdGetAliasesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_aliases_request;
   request_class->parse_response = parse_get_aliases_response;
   gobject_class->finalize = snapd_get_aliases_finalize;
}

static void
snapd_get_aliases_init (SnapdGetAliases *request)
{
}
