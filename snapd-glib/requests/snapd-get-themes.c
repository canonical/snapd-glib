/*
 * Copyright (C) 2021 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-get-themes.h"

#include "snapd-client.h"
#include "snapd-json.h"

struct _SnapdGetThemes
{
    SnapdRequest parent_instance;

    GStrv gtk_theme_names;
    GStrv icon_theme_names;
    GStrv sound_theme_names;

    GHashTable *gtk_theme_status;
    GHashTable *icon_theme_status;
    GHashTable *sound_theme_status;
};

G_DEFINE_TYPE (SnapdGetThemes, snapd_get_themes, snapd_request_get_type ())

SnapdGetThemes *
_snapd_get_themes_new (GStrv gtk_theme_names, GStrv icon_theme_names, GStrv sound_theme_names, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetThemes *self = SNAPD_GET_THEMES (g_object_new (snapd_get_themes_get_type (),
                                                           "cancellable", cancellable,
                                                           "ready-callback", callback,
                                                           "ready-callback-data", user_data,
                                                           NULL));

    self->gtk_theme_names = g_strdupv (gtk_theme_names);
    self->icon_theme_names = g_strdupv (icon_theme_names);
    self->sound_theme_names = g_strdupv (sound_theme_names);

    return self;
}

GHashTable *
_snapd_get_themes_get_gtk_theme_status (SnapdGetThemes *self)
{
    return self->gtk_theme_status;
}

GHashTable *
_snapd_get_themes_get_icon_theme_status (SnapdGetThemes *self)
{
    return self->icon_theme_status;
}

GHashTable *
_snapd_get_themes_get_sound_theme_status (SnapdGetThemes *self)
{
    return self->sound_theme_status;
}

static void
add_theme_names (GString *path, gboolean *first_param, const char *theme_type, GStrv theme_names)
{
    if (theme_names == NULL)
        return;

    for (gchar * const *name = theme_names; *name != NULL; name++) {
        g_string_append_c (path, *first_param ? '?' : '&');
        *first_param = FALSE;
        g_string_append (path, theme_type);
        g_string_append_uri_escaped (path, *name, NULL, TRUE);
    }
}

static SoupMessage *
generate_get_themes_request (SnapdRequest *request, GBytes **body)
{
    SnapdGetThemes *self = SNAPD_GET_THEMES (request);

    g_autoptr(GString) path = g_string_new ("http://snapd/v2/accessories/themes");
    gboolean first_param = TRUE;

    add_theme_names (path, &first_param, "gtk-theme=", self->gtk_theme_names);
    add_theme_names (path, &first_param, "icon-theme=", self->icon_theme_names);
    add_theme_names (path, &first_param, "sound-theme=", self->sound_theme_names);

    return soup_message_new ("GET", path->str);
}

static GHashTable *
parse_theme_status(JsonNode *status_object) {
    g_autoptr(GHashTable) status = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

    if (status_object == NULL || json_node_get_value_type (status_object) != JSON_TYPE_OBJECT)
        return NULL;

    JsonObjectIter iter;
    json_object_iter_init (&iter, json_node_get_object (status_object));
    const char *theme_name;
    JsonNode *node;
    while (json_object_iter_next (&iter, &theme_name, &node)) {
        const char *value = json_node_get_string (node);
        SnapdThemeStatus theme_status = SNAPD_THEME_STATUS_UNAVAILABLE;

        if (g_strcmp0 (value, "installed") == 0) {
            theme_status = SNAPD_THEME_STATUS_INSTALLED;
        } else if (g_strcmp0 (value, "available") == 0) {
            theme_status = SNAPD_THEME_STATUS_AVAILABLE;
        } else if (g_strcmp0 (value, "unavailable") == 0) {
            theme_status = SNAPD_THEME_STATUS_UNAVAILABLE;
        }
        g_hash_table_insert (status, g_strdup (theme_name), GINT_TO_POINTER (theme_status));
    }

    return g_steal_pointer(&status);
}

static gboolean
parse_get_themes_response (SnapdRequest *request, guint status_code, const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, GError **error)
{
    SnapdGetThemes *self = SNAPD_GET_THEMES (request);

    g_autoptr(JsonObject) response = _snapd_json_parse_response (content_type, body, maintenance, NULL, error);
    if (response == NULL)
        return FALSE;
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = NULL;*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return FALSE;

    JsonObject *object = json_node_get_object (result);

    self->gtk_theme_status = parse_theme_status (json_object_get_member (object, "gtk-themes"));
    self->icon_theme_status = parse_theme_status (json_object_get_member (object, "icon-themes"));
    self->sound_theme_status = parse_theme_status (json_object_get_member (object, "sound-themes"));

    return TRUE;
}

static void
snapd_get_themes_finalize (GObject *object)
{
    SnapdGetThemes *self = SNAPD_GET_THEMES (object);

    g_clear_pointer (&self->gtk_theme_names, g_strfreev);
    g_clear_pointer (&self->icon_theme_names, g_strfreev);
    g_clear_pointer (&self->sound_theme_names, g_strfreev);

    g_clear_pointer (&self->gtk_theme_status, g_hash_table_unref);
    g_clear_pointer (&self->icon_theme_status, g_hash_table_unref);
    g_clear_pointer (&self->sound_theme_status, g_hash_table_unref);

    G_OBJECT_CLASS (snapd_get_themes_parent_class)->finalize (object);
}

static void
snapd_get_themes_class_init (SnapdGetThemesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_get_themes_request;
   request_class->parse_response = parse_get_themes_response;
   gobject_class->finalize = snapd_get_themes_finalize;
}

static void
snapd_get_themes_init (SnapdGetThemes *self)
{
}
