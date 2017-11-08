/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_JSON_H__
#define __SNAPD_JSON_H__

#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "snapd-change.h"
#include "snapd-snap.h"
#include "snapd-user-information.h"

G_BEGIN_DECLS

void                  _snapd_json_set_body               (SoupMessage        *message,
                                                          JsonBuilder        *builder);

gboolean              _snapd_json_get_bool               (JsonObject         *object,
                                                          const gchar        *name,
                                                          gboolean            default_value);

gint64                _snapd_json_get_int                (JsonObject         *object,
                                                          const gchar        *name,
                                                          gint64              default_value);

const gchar          *_snapd_json_get_string             (JsonObject         *object,
                                                          const gchar        *name,
                                                          const gchar        *default_value);

JsonArray            *_snapd_json_get_array              (JsonObject         *object,
                                                          const gchar        *name);

JsonObject           *_snapd_json_get_object             (JsonObject         *object,
                                                          const gchar        *name);

JsonObject           *_snapd_json_parse_response         (SoupMessage        *message,
                                                          GError            **error);

JsonObject           *_snapd_json_get_sync_result_o      (JsonObject         *response,
                                                          GError            **error);

JsonArray            *_snapd_json_get_sync_result_a      (JsonObject         *response,
                                                          GError            **error);

gchar                *_snapd_json_get_async_result       (JsonObject         *response,
                                                          GError            **error);

SnapdChange          *_snapd_json_parse_change           (JsonObject         *object,
                                                          GError            **error);

SnapdSnap            *_snapd_json_parse_snap             (JsonObject         *object,
                                                          GError            **error);

GPtrArray            *_snapd_json_parse_snap_array       (JsonArray          *array,
                                                          GError            **error);

GPtrArray            *_snapd_json_parse_app_array        (JsonArray          *array,
                                                          GError            **error);

SnapdUserInformation *_snapd_json_parse_user_information (JsonObject         *object,
                                                          GError            **error);

G_END_DECLS

#endif /* __SNAPD_JSON_H__ */
