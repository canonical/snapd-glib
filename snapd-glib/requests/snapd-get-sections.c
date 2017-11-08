/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-sections.h"

#include "snapd-error.h"
#include "snapd-json.h"

struct _SnapdGetSections
{
    SnapdRequest parent_instance;
    gchar **sections;
};

G_DEFINE_TYPE (SnapdGetSections, snapd_get_sections, snapd_request_get_type ())

SnapdGetSections *
_snapd_get_sections_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_SECTIONS (g_object_new (snapd_get_sections_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
}

gchar **
_snapd_get_sections_get_sections (SnapdGetSections *request)
{
    return request->sections;
}

static SoupMessage *
generate_get_sections_request (SnapdRequest *request)
{
    return soup_message_new ("GET", "http://snapd/v2/sections");
}

static gboolean
parse_get_sections_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdGetSections *r = SNAPD_GET_SECTIONS (request);
    g_autoptr(JsonObject) response = NULL;
    g_autoptr(JsonArray) result = NULL;
    g_autoptr(GPtrArray) sections = NULL;
    guint i;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    sections = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected snap name type");
            return FALSE;
        }

        g_ptr_array_add (sections, g_strdup (json_node_get_string (node)));
    }
    g_ptr_array_add (sections, NULL);

    r->sections = g_steal_pointer (&sections->pdata);

    return TRUE;
}

static void
snapd_get_sections_finalize (GObject *object)
{
    SnapdGetSections *request = SNAPD_GET_SECTIONS (object);

    g_clear_pointer (&request->sections, g_strfreev);

    G_OBJECT_CLASS (snapd_get_sections_parent_class)->finalize (object);
}

static void
snapd_get_sections_class_init (SnapdGetSectionsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_sections_request;
   request_class->parse_response = parse_get_sections_response;
   gobject_class->finalize = snapd_get_sections_finalize;
}

static void
snapd_get_sections_init (SnapdGetSections *request)
{
}
