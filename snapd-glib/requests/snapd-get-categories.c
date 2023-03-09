/*
 * Copyright (C) 2023 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-categories.h"

#include "snapd-error.h"
#include "snapd-category-details.h"
#include "snapd-json.h"

struct _SnapdGetCategories
{
    SnapdRequest parent_instance;
    GPtrArray *categories;
};

G_DEFINE_TYPE (SnapdGetCategories, snapd_get_categories, snapd_request_get_type ())

SnapdGetCategories *
_snapd_get_categories_new (GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    return SNAPD_GET_CATEGORIES (g_object_new (snapd_get_categories_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
}

GPtrArray *
_snapd_get_categories_get_categories (SnapdGetCategories *self)
{
    return self->categories;
}

static SoupMessage *
generate_get_categories_request (SnapdRequest *request, GBytes **body)
{
    return soup_message_new ("GET", "http://snapd/v2/categories");
}

static gboolean
parse_get_categories_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetCategories *self = SNAPD_GET_CATEGORIES (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    g_autoptr(JsonArray) result = _snapd_json_get_sync_result_a (response, error);
    if (result == NULL)
        return FALSE;

    g_autoptr(GPtrArray) categories = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (result); i++) {
        JsonNode *node = json_array_get_element (result, i);
        const gchar *name;
        SnapdCategoryDetails *details;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected snap category details type");
            return FALSE;
        }
        JsonObject *object = json_node_get_object (node);
        name = _snapd_json_get_string (object, "name", NULL);
        details = g_object_new (snapd_category_details_get_type (), "name", name, NULL);
        g_ptr_array_add (categories, details);
    }

    self->categories = g_steal_pointer (&categories);

    return TRUE;
}

static void
snapd_get_categories_finalize (GObject *object)
{
    SnapdGetCategories *self = SNAPD_GET_CATEGORIES (object);

    g_clear_pointer (&self->categories, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_get_categories_parent_class)->finalize (object);
}

static void
snapd_get_categories_class_init (SnapdGetCategoriesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_categories_request;
   request_class->parse_response = parse_get_categories_response;
   gobject_class->finalize = snapd_get_categories_finalize;
}

static void
snapd_get_categories_init (SnapdGetCategories *self)
{
}
