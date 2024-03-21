/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdlib.h>
#include <string.h>

#include "snapd-json.h"

#include "snapd-error.h"
#include "snapd-app.h"
#include "snapd-category.h"
#include "snapd-media.h"
#include "snapd-screenshot.h"
#include "snapd-task.h"

void
_snapd_json_set_body (SoupMessage *message, JsonBuilder *builder, GBytes **body)
{
#if SOUP_CHECK_VERSION (2, 99, 2)
    SoupMessageHeaders *request_headers = soup_message_get_request_headers (message);
#else
    SoupMessageHeaders *request_headers = message->request_headers;
#endif
    soup_message_headers_set_content_type (request_headers, "application/json", NULL);

    g_autoptr(JsonNode) json_root = json_builder_get_root (builder);
    g_autoptr(JsonGenerator) json_generator = json_generator_new ();
    json_generator_set_pretty (json_generator, TRUE);
    json_generator_set_root (json_generator, json_root);
    gsize data_length;
    g_autofree guchar *data = (guchar *) json_generator_to_data (json_generator, &data_length);
    *body = g_bytes_new_take (g_steal_pointer (&data), data_length);
}

gboolean
_snapd_json_get_bool (JsonObject *object, const gchar *name, gboolean default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_BOOLEAN)
        return json_node_get_boolean (node);
    else
        return default_value;
}

gint64
_snapd_json_get_int (JsonObject *object, const gchar *name, gint64 default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_INT64)
        return json_node_get_int (node);
    else
        return default_value;
}

const gchar *
_snapd_json_get_string (JsonObject *object, const gchar *name, const gchar *default_value)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == G_TYPE_STRING)
        return json_node_get_string (node);
    else
        return default_value;
}

JsonArray *
_snapd_json_get_array (JsonObject *object, const gchar *name)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == JSON_TYPE_ARRAY)
        return json_array_ref (json_node_get_array (node));
    else
        return json_array_new ();
}

JsonObject *
_snapd_json_get_object (JsonObject *object, const gchar *name)
{
    JsonNode *node = json_object_get_member (object, name);
    if (node != NULL && json_node_get_value_type (node) == JSON_TYPE_OBJECT)
        return json_node_get_object (node);
    else
        return NULL;
}

static gboolean
parse_date (const gchar *date_string, gint *year, gint *month, gint *day)
{
    /* Example: 2016-05-17 */
    if (strchr (date_string, '-') != NULL) {
        g_auto(GStrv) tokens = g_strsplit (date_string, "-", -1);
        if (g_strv_length (tokens) != 3)
            return FALSE;

        *year = atoi (tokens[0]);
        *month = atoi (tokens[1]);
        *day = atoi (tokens[2]);

        return TRUE;
    }
    /* Example: 20160517 */
    else if (strlen (date_string) == 8) {
        // FIXME: Implement
        return FALSE;
    }
    else
        return FALSE;
}

static gboolean
parse_time (const gchar *time_string, gint *hour, gint *minute, gdouble *seconds)
{
    /* Example: 09:36:53.682 or 09:36:53 or 09:36 */
    if (strchr (time_string, ':') != NULL) {
        g_auto(GStrv) tokens = g_strsplit (time_string, ":", 3);
        *hour = atoi (tokens[0]);
        if (tokens[1] == NULL)
            return FALSE;
        *minute = atoi (tokens[1]);
        if (tokens[2] != NULL)
            *seconds = g_ascii_strtod (tokens[2], NULL);
        else
            *seconds = 0.0;

        return TRUE;
    }
    /* Example: 093653.682 or 093653 or 0936 */
    else {
        // FIXME: Implement
        return FALSE;
    }
}

static gboolean
is_timezone_prefix (gchar c)
{
    return c == '+' || c == '-' || c == 'Z';
}

GDateTime *
_snapd_json_get_date_time (JsonObject *object, const gchar *name)
{
    const gchar *value = _snapd_json_get_string (object, name, NULL);
    if (value == NULL)
        return NULL;

    /* Example: 2016-05-17T09:36:53+12:00 */
    g_auto(GStrv) tokens = g_strsplit (value, "T", 2);
    gint year = 0, month = 0, day = 0;
    if (!parse_date (tokens[0], &year, &month, &day))
        return NULL;
    g_autoptr(GTimeZone) timezone = NULL;
    gint hour = 0, minute = 0;
    gdouble seconds = 0.0;
    if (tokens[1] != NULL) {
        gchar *timezone_start;

        /* Timezone is either Z (UTC) +hh:mm or -hh:mm */
        timezone_start = tokens[1];
        while (*timezone_start != '\0' && !is_timezone_prefix (*timezone_start))
            timezone_start++;
        if (*timezone_start != '\0') {
#ifdef GLIB_VERSION_2_68
            timezone = g_time_zone_new_identifier (timezone_start);
            if (timezone == NULL)
                timezone = g_time_zone_new_utc ();
#else
            timezone = g_time_zone_new (timezone_start);
#endif
        }

        /* Strip off timezone */
        *timezone_start = '\0';

        if (!parse_time (tokens[1], &hour, &minute, &seconds))
            return NULL;
    }

    if (timezone == NULL)
        timezone = g_time_zone_new_local ();

    return g_date_time_new (timezone, year, month, day, hour, minute, seconds);
}

static void
parse_error_response (JsonObject *root, JsonNode **error_value, GError **error)
{
    JsonObject *result = _snapd_json_get_object (root, "result");
    gint64 status_code = _snapd_json_get_int (root, "status-code", 0);
    const gchar *kind = result != NULL ? _snapd_json_get_string (result, "kind", NULL) : NULL;
    const gchar *message = result != NULL ? _snapd_json_get_string (result, "message", NULL) : NULL;

    if (error_value != NULL) {
        *error_value = result != NULL ? json_object_get_member (result, "value") : NULL;
        if (*error_value != NULL)
            json_node_ref (*error_value);
    }

    if (g_strcmp0 (kind, "login-required") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_AUTH_DATA_REQUIRED,
                             message);
    else if (g_strcmp0 (kind, "invalid-auth-data") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_AUTH_DATA_INVALID,
                             message);
    else if (g_strcmp0 (kind, "two-factor-required") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_TWO_FACTOR_REQUIRED,
                             message);
    else if (g_strcmp0 (kind, "two-factor-failed") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_TWO_FACTOR_INVALID,
                             message);
    else if (g_strcmp0 (kind, "terms-not-accepted") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_TERMS_NOT_ACCEPTED,
                             message);
    else if (g_strcmp0 (kind, "no-payment-methods") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_PAYMENT_NOT_SETUP,
                             message);
    else if (g_strcmp0 (kind, "payment-declined") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_PAYMENT_DECLINED,
                             message);
    else if (g_strcmp0 (kind, "snap-already-installed") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_ALREADY_INSTALLED,
                             message);
    else if (g_strcmp0 (kind, "snap-not-installed") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NOT_INSTALLED,
                             message);
    else if (g_strcmp0 (kind, "snap-not-found") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NOT_FOUND,
                             message);
    else if (g_strcmp0 (kind, "snap-local") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NOT_IN_STORE,
                             message);
    else if (g_strcmp0 (kind, "snap-no-update-available") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NO_UPDATE_AVAILABLE,
                             message);
    else if (g_strcmp0 (kind, "password-policy") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_PASSWORD_POLICY_ERROR,
                             message);
    else if (g_strcmp0 (kind, "snap-needs-devmode") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NEEDS_DEVMODE,
                             message);
    else if (g_strcmp0 (kind, "snap-needs-classic") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NEEDS_CLASSIC,
                             message);
    else if (g_strcmp0 (kind, "snap-needs-classic-system") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NEEDS_CLASSIC_SYSTEM,
                             message);
    else if (g_strcmp0 (kind, "bad-query") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_BAD_QUERY,
                             message);
    else if (g_strcmp0 (kind, "network-timeout") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NETWORK_TIMEOUT,
                             message);
    else if (g_strcmp0 (kind, "auth-cancelled") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_AUTH_CANCELLED,
                             message);
    else if (g_strcmp0 (kind, "snap-not-classic") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NOT_CLASSIC,
                             message);
    else if (g_strcmp0 (kind, "snap-revision-not-available") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_REVISION_NOT_AVAILABLE,
                             message);
    else if (g_strcmp0 (kind, "snap-channel-not-available") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_CHANNEL_NOT_AVAILABLE,
                             message);
    else if (g_strcmp0 (kind, "snap-not-a-snap") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_NOT_A_SNAP,
                             message);
    else if (g_strcmp0 (kind, "dns-failure") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_DNS_FAILURE,
                             message);
    else if (g_strcmp0 (kind, "option-not-found") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_OPTION_NOT_FOUND,
                             message);
    else if (g_strcmp0 (kind, "unsuccessful") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_UNSUCCESSFUL,
                             message);
    else if (g_strcmp0 (kind, "app-not-found") == 0)
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_APP_NOT_FOUND,
                             message);
    else if (g_strcmp0 (kind, "snap-architecture-not-available") == 0)
         g_set_error_literal (error,
                              SNAPD_ERROR,
                              SNAPD_ERROR_ARCHITECTURE_NOT_AVAILABLE,
                              message);
    else if (g_strcmp0 (kind, "snap-change-conflict") == 0)
         g_set_error_literal (error,
                              SNAPD_ERROR,
                              SNAPD_ERROR_CHANGE_CONFLICT,
                              message);
    else if (g_strcmp0 (kind, "interfaces-unchanged") == 0)
         g_set_error_literal (error,
                              SNAPD_ERROR,
                              SNAPD_ERROR_INTERFACES_UNCHANGED,
                              message);
    else {
        switch (status_code) {
        case SOUP_STATUS_BAD_REQUEST:
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_BAD_REQUEST,
                         "%s: %s", kind, message);
            break;
        case SOUP_STATUS_UNAUTHORIZED:
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_AUTH_DATA_REQUIRED,
                         "%s: %s", kind, message);
            break;
        case SOUP_STATUS_FORBIDDEN:
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_PERMISSION_DENIED,
                         "%s: %s", kind, message);
            break;
        case SOUP_STATUS_NOT_FOUND:
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_NOT_FOUND,
                         "%s: %s", kind, message);
            break;
        /* Other response codes currently produced by snapd:
        case SOUP_STATUS_METHOD_NOT_ALLOWED:
        case SOUP_STATUS_NOT_IMPLEMENTED:
        case SOUP_STATUS_CONFLICT:
         */
        default:
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_FAILED,
                         "status-code=%" G_GINT64_FORMAT " kind=%s message=%s", status_code, kind, message);
        }
    }
}

JsonObject *
_snapd_json_parse_response (const gchar *content_type, GBytes *body, SnapdMaintenance **maintenance, JsonNode **error_value, GError **error)
{
    if (content_type == NULL) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd returned no content type");
        return NULL;
    }
    if (g_strcmp0 (content_type, "application/json") != 0) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd returned unexpected content type %s", content_type);
        return NULL;
    }

    g_autoptr(JsonParser) parser = json_parser_new ();
    g_autoptr(GError) error_local = NULL;
    if (!json_parser_load_from_data (parser, g_bytes_get_data (body, NULL), g_bytes_get_size (body), &error_local)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "Unable to parse snapd response: %s", error_local->message);
        return NULL;
    }

    if (!JSON_NODE_HOLDS_OBJECT (json_parser_get_root (parser))) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd response does is not a valid JSON object");
        return NULL;
    }
    JsonObject *root = json_node_get_object (json_parser_get_root (parser));

    JsonNode *type_node = json_object_get_member (root, "type");
    if (type_node == NULL || json_node_get_value_type (type_node) != G_TYPE_STRING) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd response does not have a type");
        return NULL;
    }

    if (json_object_has_member (root, "maintenance") && maintenance != NULL) {
        JsonObject *m = json_object_get_object_member (root, "maintenance");

        const gchar *kind = _snapd_json_get_string (m, "kind", NULL);
        SnapdMaintenanceKind maintenance_kind = SNAPD_MAINTENANCE_KIND_UNKNOWN;
        if (g_strcmp0 (kind, "daemon-restart") == 0)
            maintenance_kind = SNAPD_MAINTENANCE_KIND_DAEMON_RESTART;
        else if (g_strcmp0 (kind, "system-restart") == 0)
            maintenance_kind = SNAPD_MAINTENANCE_KIND_SYSTEM_RESTART;

        *maintenance = g_object_new (SNAPD_TYPE_MAINTENANCE,
                                     "kind", maintenance_kind,
                                     "message", _snapd_json_get_string (m, "message", NULL),
                                     NULL);
    }

    if (strcmp (json_node_get_string (type_node), "error") == 0) {
        parse_error_response (root, error_value, error);
        return NULL;
    }

    return json_object_ref (root);
}

JsonNode *
_snapd_json_get_sync_result (JsonObject *response, GError **error)
{
    const gchar *type = json_object_get_string_member (response, "type");
    if (strcmp (type, "sync") != 0) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected response '%s' returned for sync request", type);
        return NULL;
    }

    if (!json_object_has_member (response, "result")) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "No result returned");
        return NULL;
    }

    return json_node_ref (json_object_get_member (response, "result"));
}

JsonObject *
_snapd_json_get_sync_result_o (JsonObject *response, GError **error)
{
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = _snapd_json_get_sync_result (response, error);*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return NULL;

    if (!JSON_NODE_HOLDS_OBJECT (result)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Result is not an object");
        json_node_unref (result);
        return NULL;
    }

    JsonObject *r = json_object_ref (json_node_get_object (result));
    json_node_unref (result);
    return r;
}

JsonArray *
_snapd_json_get_sync_result_a (JsonObject *response, GError **error)
{
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = _snapd_json_get_sync_result (response, error);*/
    JsonNode *result = _snapd_json_get_sync_result (response, error);
    if (result == NULL)
        return NULL;

    if (!JSON_NODE_HOLDS_ARRAY (result)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Result is not an array");
        json_node_unref (result);
        return NULL;
    }

    JsonArray *r = json_array_ref (json_node_get_array (result));
    json_node_unref (result);
    return r;
}

gchar *
_snapd_json_get_async_result (JsonObject *response, GError **error)
{
    const gchar *type = json_object_get_string_member (response, "type");
    if (strcmp (type, "async") != 0) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected response '%s' returned for async request", type);
        return NULL;
    }

    JsonNode *change_node = json_object_get_member (response, "change");
    if (change_node == NULL || json_node_get_value_type (change_node) != G_TYPE_STRING) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "No change returned for async request");
        return NULL;
    }

    return g_strdup (json_node_get_string (change_node));
}

static GStrv
create_str_array_from_jsonarray (JsonArray *data) {
    if (data == NULL)
        return NULL;

    guint len = json_array_get_length (data);
    GStrv list = g_malloc0_n (len, 1 + sizeof(gchar *));

    for (guint i = 0; i <len; i++) {
        JsonNode *node = json_array_get_element (data, i);
        if (node == NULL || json_node_get_value_type (node) != G_TYPE_STRING) {
            g_strfreev (list);
            return NULL;
        }
        list[i] = g_strdup(json_node_get_string (node));
    }
    return list;
}

SnapdChange *
_snapd_json_parse_change (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected change type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(JsonArray) array = _snapd_json_get_array (object, "tasks");
    g_autoptr(GPtrArray) tasks = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected task type");
            return NULL;
        }
        JsonObject *object = json_node_get_object (node);
        JsonObject *progress = _snapd_json_get_object (object, "progress");
        g_autoptr(GDateTime) spawn_time = _snapd_json_get_date_time (object, "spawn-time");
        g_autoptr(GDateTime) ready_time = _snapd_json_get_date_time (object, "ready-time");

        g_autoptr(SnapdTask) t = g_object_new (SNAPD_TYPE_TASK,
                                               "id", _snapd_json_get_string (object, "id", NULL),
                                               "kind", _snapd_json_get_string (object, "kind", NULL),
                                               "summary", _snapd_json_get_string (object, "summary", NULL),
                                               "status", _snapd_json_get_string (object, "status", NULL),
                                               "progress-label", progress != NULL ? _snapd_json_get_string (progress, "label", NULL) : NULL,
                                               "progress-done", progress != NULL ? _snapd_json_get_int (progress, "done", 0) : 0,
                                               "progress-total", progress != NULL ? _snapd_json_get_int (progress, "total", 0) : 0,
                                               "spawn-time", spawn_time,
                                               "ready-time", ready_time,
                                               NULL);
        g_ptr_array_add (tasks, g_steal_pointer (&t));
    }

    g_autoptr(GDateTime) main_spawn_time = _snapd_json_get_date_time (object, "spawn-time");
    g_autoptr(GDateTime) main_ready_time = _snapd_json_get_date_time (object, "ready-time");

    g_autoptr(SnapdChangeData) data = NULL;
    JsonObject *autorefresh_data = _snapd_json_get_object (object, "data");
    const gchar *kind = _snapd_json_get_string (object, "kind", "");

    // Currently, only one kind of Change can have DATA field, so this is quite "hardcoded". When more
    // Change kinds have DATA field, new subclasses of SNAPD_TYPE_CHANGE_DATA must be created, and
    // the code in QT's snapd-change-data.c file must be completed to manage the new types.
    if (g_str_equal (kind, "auto-refresh") && (autorefresh_data != NULL)) {

        g_autoptr(JsonArray) snap_names_json = _snapd_json_get_array (autorefresh_data, "snap-names");
        g_auto(GStrv) snap_names = create_str_array_from_jsonarray (snap_names_json);

        g_autoptr(JsonArray) refresh_forced_json = _snapd_json_get_array (autorefresh_data, "refresh-forced");
        g_auto(GStrv) refresh_forced = create_str_array_from_jsonarray (refresh_forced_json);

        data = g_object_new (SNAPD_TYPE_AUTOREFRESH_CHANGE_DATA,
                             "snap-names", snap_names,
                             "refresh-forced", refresh_forced,
                             NULL);
    }

    return g_object_new (SNAPD_TYPE_CHANGE,
                         "id", _snapd_json_get_string (object, "id", NULL),
                         "kind", _snapd_json_get_string (object, "kind", NULL),
                         "summary", _snapd_json_get_string (object, "summary", NULL),
                         "status", _snapd_json_get_string (object, "status", NULL),
                         "tasks", tasks,
                         "ready", _snapd_json_get_bool (object, "ready", FALSE),
                         "spawn-time", main_spawn_time,
                         "ready-time", main_ready_time,
                         "error", _snapd_json_get_string (object, "err", NULL),
                         "data", data,
                         NULL);
}

static SnapdConfinement
parse_confinement (const gchar *value)
{
    if (strcmp (value, "strict") == 0)
        return SNAPD_CONFINEMENT_STRICT;
    else if (strcmp (value, "classic") == 0)
        return SNAPD_CONFINEMENT_CLASSIC;
    else if (strcmp (value, "devmode") == 0)
        return SNAPD_CONFINEMENT_DEVMODE;
    else
        return SNAPD_CONFINEMENT_UNKNOWN;
}

SnapdSystemInformation *
_snapd_json_parse_system_information (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected system information type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    const gchar *architecture = _snapd_json_get_string (object, "architecture", "");
    const gchar *confinement_string = _snapd_json_get_string (object, "confinement", "");
    SnapdSystemConfinement confinement = SNAPD_SYSTEM_CONFINEMENT_UNKNOWN;
    if (strcmp (confinement_string, "strict") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_STRICT;
    else if (strcmp (confinement_string, "partial") == 0)
        confinement = SNAPD_SYSTEM_CONFINEMENT_PARTIAL;
    JsonObject *os_release = _snapd_json_get_object (object, "os-release");
    JsonObject *locations = _snapd_json_get_object (object, "locations");
    JsonObject *refresh = _snapd_json_get_object (object, "refresh");
    JsonObject *sandbox_features = _snapd_json_get_object (object, "sandbox-features");
    g_autoptr(GHashTable) sandbox_features_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_strfreev);
    if (sandbox_features != NULL) {
        JsonObjectIter iter;
        json_object_iter_init (&iter, sandbox_features);
        const gchar *name;
        JsonNode *features_node;
        while (json_object_iter_next (&iter, &name, &features_node)) {
            if (json_node_get_value_type (features_node) != JSON_TYPE_ARRAY) {
                g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected sandbox features type");
                return FALSE;
            }

            JsonArray *features_array = json_node_get_array (features_node);
            g_autoptr(GPtrArray) features_ptr_array = g_ptr_array_new ();
            for (guint i = 0; i < json_array_get_length (features_array); i++) {
                JsonNode *node = json_array_get_element (features_array, i);

                if (json_node_get_value_type (node) != G_TYPE_STRING) {
                    g_set_error (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected sandbox feature type");
                    return FALSE;
                }

                g_ptr_array_add (features_ptr_array, (gpointer) json_node_get_string (node));
            }
            g_ptr_array_add (features_ptr_array, NULL);
            g_hash_table_insert (sandbox_features_hash, g_strdup (name), g_strdupv ((GStrv) features_ptr_array->pdata));
        }
    }

    g_autoptr(GDateTime) refresh_hold = _snapd_json_get_date_time (refresh, "hold");
    g_autoptr(GDateTime) refresh_last = _snapd_json_get_date_time (refresh, "last");
    g_autoptr(GDateTime) refresh_next = _snapd_json_get_date_time (refresh, "next");

    return g_object_new (SNAPD_TYPE_SYSTEM_INFORMATION,
                         "architecture", architecture,
                         "binaries-directory", locations != NULL ? _snapd_json_get_string (locations, "snap-bin-dir", NULL) : NULL,
                         "build-id", _snapd_json_get_string (object, "build-id", NULL),
                         "confinement", confinement,
                         "kernel-version", _snapd_json_get_string (object, "kernel-version", NULL),
                         "managed", _snapd_json_get_bool (object, "managed", FALSE),
                         "mount-directory", locations != NULL ? _snapd_json_get_string (locations, "snap-mount-dir", NULL) : NULL,
                         "on-classic", _snapd_json_get_bool (object, "on-classic", FALSE),
                         "os-id", os_release != NULL ? _snapd_json_get_string (os_release, "id", NULL) : NULL,
                         "os-version", os_release != NULL ? _snapd_json_get_string (os_release, "version-id", NULL) : NULL,
                         "sandbox-features", sandbox_features_hash,
                         "series", _snapd_json_get_string (object, "series", NULL),
                         "store", _snapd_json_get_string (object, "store", NULL),
                         "version", _snapd_json_get_string (object, "version", NULL),
                         "refresh-hold", refresh_hold,
                         "refresh-last", refresh_last,
                         "refresh-next", refresh_next,
                         "refresh-schedule", _snapd_json_get_string (refresh, "schedule", NULL),
                         "refresh-timer", _snapd_json_get_string (refresh, "timer", NULL),
                         NULL);
}

SnapdSnap *
_snapd_json_parse_snap (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected snap type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    const gchar *name = _snapd_json_get_string (object, "name", NULL);

    SnapdConfinement confinement = parse_confinement (_snapd_json_get_string (object, "confinement", ""));

    const gchar *snap_type_string = _snapd_json_get_string (object, "type", "");
    SnapdSnapType snap_type = SNAPD_SNAP_TYPE_UNKNOWN;
    if (strcmp (snap_type_string, "app") == 0)
        snap_type = SNAPD_SNAP_TYPE_APP;
    else if (strcmp (snap_type_string, "kernel") == 0)
        snap_type = SNAPD_SNAP_TYPE_KERNEL;
    else if (strcmp (snap_type_string, "gadget") == 0)
        snap_type = SNAPD_SNAP_TYPE_GADGET;
    else if (strcmp (snap_type_string, "os") == 0)
        snap_type = SNAPD_SNAP_TYPE_OS;
    else if (strcmp (snap_type_string, "core") == 0)
        snap_type = SNAPD_SNAP_TYPE_CORE;
    else if (strcmp (snap_type_string, "base") == 0)
        snap_type = SNAPD_SNAP_TYPE_BASE;
    else if (strcmp (snap_type_string, "snapd") == 0)
        snap_type = SNAPD_SNAP_TYPE_SNAPD;

    const gchar *snap_status_string = _snapd_json_get_string (object, "status", "");
    SnapdSnapStatus snap_status = SNAPD_SNAP_STATUS_UNKNOWN;
    if (strcmp (snap_status_string, "available") == 0)
        snap_status = SNAPD_SNAP_STATUS_AVAILABLE;
    else if (strcmp (snap_status_string, "priced") == 0)
        snap_status = SNAPD_SNAP_STATUS_PRICED;
    else if (strcmp (snap_status_string, "installed") == 0)
        snap_status = SNAPD_SNAP_STATUS_INSTALLED;
    else if (strcmp (snap_status_string, "active") == 0)
        snap_status = SNAPD_SNAP_STATUS_ACTIVE;

    g_autoptr(JsonArray) apps = _snapd_json_get_array (object, "apps");
    g_autoptr(GPtrArray) apps_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (apps); i++) {
        JsonNode *node = json_array_get_element (apps, i);

        SnapdApp *app = _snapd_json_parse_app (node, name, error);
        if (app == NULL)
            return NULL;

        g_ptr_array_add (apps_array, app);
    }

    g_autoptr(JsonArray) categories = _snapd_json_get_array (object, "categories");
    g_autoptr(GPtrArray) categories_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (categories); i++) {
        JsonNode *node = json_array_get_element (categories, i);

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected categories type");
            return NULL;
        }

        JsonObject *c = json_node_get_object (node);
        g_autoptr(SnapdCategory) category = g_object_new (SNAPD_TYPE_CATEGORY,
                                                          "featured", _snapd_json_get_bool (c, "featured", FALSE),
                                                          "name", _snapd_json_get_string (c, "name", NULL),
                                                          NULL);
        g_ptr_array_add (categories_array, g_steal_pointer (&category));
    }

    JsonObject *channels = _snapd_json_get_object (object, "channels");
    g_autoptr(GPtrArray) channels_array = g_ptr_array_new_with_free_func (g_object_unref);
    if (channels != NULL) {
        JsonObjectIter iter;
        json_object_iter_init (&iter, channels);
        const gchar *name;
        JsonNode *channel_node;
        while (json_object_iter_next (&iter, &name, &channel_node)) {
            if (json_node_get_value_type (channel_node) != JSON_TYPE_OBJECT) {
                g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected channel type");
                return NULL;
            }
            JsonObject *c = json_node_get_object (channel_node);

            SnapdConfinement confinement = parse_confinement (_snapd_json_get_string (c, "confinement", ""));
            g_autoptr(GDateTime) released_at = _snapd_json_get_date_time (c, "released-at");

            g_autoptr(SnapdChannel) channel = g_object_new (SNAPD_TYPE_CHANNEL,
                                                            "confinement", confinement,
                                                            "epoch", _snapd_json_get_string (c, "epoch", NULL),
                                                            "name", _snapd_json_get_string (c, "channel", NULL),
                                                            "released-at", released_at,
                                                            "revision", _snapd_json_get_string (c, "revision", NULL),
                                                            "size", _snapd_json_get_int (c, "size", 0),
                                                            "version", _snapd_json_get_string (c, "version", NULL),
                                                            NULL);
            g_ptr_array_add (channels_array, g_steal_pointer (&channel));
        }
    }

    g_autoptr(JsonArray) common_ids = _snapd_json_get_array (object, "common-ids");
    g_autoptr(GPtrArray) common_ids_array = g_ptr_array_new ();
    for (guint i = 0; i < json_array_get_length (common_ids); i++) {
        JsonNode *node = json_array_get_element (common_ids, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected common ID type");
            return NULL;
        }

        g_ptr_array_add (common_ids_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (common_ids_array, NULL);

    g_autoptr(GDateTime) install_date = _snapd_json_get_date_time (object, "install-date");
    g_autoptr(GDateTime) hold = _snapd_json_get_date_time (object, "hold");

    JsonObject *prices = _snapd_json_get_object (object, "prices");
    g_autoptr(GPtrArray) prices_array = g_ptr_array_new_with_free_func (g_object_unref);
    if (prices != NULL) {
        JsonObjectIter iter;
        json_object_iter_init (&iter, prices);
        const gchar *currency;
        JsonNode *amount_node;
        while (json_object_iter_next (&iter, &currency, &amount_node)) {
            if (json_node_get_value_type (amount_node) != G_TYPE_DOUBLE) {
                g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected price type");
                return NULL;
            }

            g_autoptr(SnapdPrice) price = g_object_new (SNAPD_TYPE_PRICE,
                                                        "amount", json_node_get_double (amount_node),
                                                        "currency", currency,
                                                        NULL);
            g_ptr_array_add (prices_array, g_steal_pointer (&price));
        }
    }

    g_autoptr(JsonArray) media = _snapd_json_get_array (object, "media");
    g_autoptr(GPtrArray) media_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (media); i++) {
        JsonNode *node = json_array_get_element (media, i);

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected media type");
            return NULL;
        }

        JsonObject *s = json_node_get_object (node);
        g_autoptr(SnapdMedia) media = g_object_new (SNAPD_TYPE_MEDIA,
                                                    "type", _snapd_json_get_string (s, "type", NULL),
                                                    "url", _snapd_json_get_string (s, "url", NULL),
                                                    "width", (guint) _snapd_json_get_int (s, "width", 0),
                                                    "height", (guint) _snapd_json_get_int (s, "height", 0),
                                                    NULL);
        g_ptr_array_add (media_array, g_steal_pointer (&media));
    }

    g_autoptr(GPtrArray) screenshots_array = g_ptr_array_new_with_free_func (g_object_unref);

    /* The tracks field was originally incorrectly named, fixed in snapd 61ad9ed (2.29.5) */
    g_autoptr(JsonArray) tracks = NULL;
    if (json_object_has_member (object, "Tracks"))
        tracks = _snapd_json_get_array (object, "Tracks");
    else
        tracks = _snapd_json_get_array (object, "tracks");
    g_autoptr(GPtrArray) track_array = g_ptr_array_new ();
    for (guint i = 0; i < json_array_get_length (tracks); i++) {
        JsonNode *node = json_array_get_element (tracks, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected track type");
            return NULL;
        }

        g_ptr_array_add (track_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (track_array, NULL);

    /* The developer field originally contained the publisher username */
    const gchar *publisher_username = _snapd_json_get_string (object, "developer", NULL);
    JsonObject *publisher = _snapd_json_get_object (object, "publisher");
    const gchar *publisher_display_name = NULL;
    const gchar *publisher_id = NULL;
    SnapdPublisherValidation publisher_validation = SNAPD_PUBLISHER_VALIDATION_UNKNOWN;
    if (publisher != NULL) {
        publisher_display_name = _snapd_json_get_string (publisher, "display-name", NULL);
        publisher_id = _snapd_json_get_string (publisher, "id", NULL);
        publisher_username = _snapd_json_get_string (publisher, "username", publisher_username);
        const gchar *validation = _snapd_json_get_string (publisher, "validation", NULL);
        if (g_strcmp0 (validation, "unproven") == 0)
            publisher_validation = SNAPD_PUBLISHER_VALIDATION_UNPROVEN;
        else if (g_strcmp0 (validation, "starred") == 0)
            publisher_validation = SNAPD_PUBLISHER_VALIDATION_STARRED;
        else if (g_strcmp0 (validation, "verified") == 0)
            publisher_validation = SNAPD_PUBLISHER_VALIDATION_VERIFIED;
        else
            publisher_validation = SNAPD_PUBLISHER_VALIDATION_UNKNOWN;
    }

    JsonObject *refresh_inhibit = _snapd_json_get_object (object, "refresh-inhibit");
    g_autoptr(GDateTime) proceed_time = NULL;
    if (refresh_inhibit != NULL)
        proceed_time = _snapd_json_get_date_time (refresh_inhibit, "proceed-time");

    return g_object_new (SNAPD_TYPE_SNAP,
                         "apps", apps_array,
                         "base", _snapd_json_get_string (object, "base", NULL),
                         "broken", _snapd_json_get_string (object, "broken", NULL),
                         "categories", categories_array,
                         "channel", _snapd_json_get_string (object, "channel", NULL),
                         "channels", channels_array,
                         "common-ids", (GStrv) common_ids_array->pdata,
                         "confinement", confinement,
                         "contact", _snapd_json_get_string (object, "contact", NULL),
                         "description", _snapd_json_get_string (object, "description", NULL),
                         "devmode", _snapd_json_get_bool (object, "devmode", FALSE),
                         "download-size", _snapd_json_get_int (object, "download-size", 0),
                         "hold", hold,
                         "icon", _snapd_json_get_string (object, "icon", NULL),
                         "id", _snapd_json_get_string (object, "id", NULL),
                         "install-date", install_date,
                         "installed-size", _snapd_json_get_int (object, "installed-size", 0),
                         "jailmode", _snapd_json_get_bool (object, "jailmode", FALSE),
                         "license", _snapd_json_get_string (object, "license", NULL),
                         "media", media_array,
                         "mounted-from", _snapd_json_get_string (object, "mounted-from", NULL),
                         "name", name,
                         "prices", prices_array,
                         "private", _snapd_json_get_bool (object, "private", FALSE),
                         "publisher-id", publisher_id,
                         "publisher-username", publisher_username,
                         "publisher-display-name", publisher_display_name,
                         "publisher-validation", publisher_validation,
                         "revision", _snapd_json_get_string (object, "revision", NULL),
                         "screenshots", screenshots_array,
                         "snap-type", snap_type,
                         "status", snap_status,
                         "store-url", _snapd_json_get_string (object, "store-url", NULL),
                         "summary", _snapd_json_get_string (object, "summary", NULL),
                         "title", _snapd_json_get_string (object, "title", NULL),
                         "tracking-channel", _snapd_json_get_string (object, "tracking-channel", NULL),
                         "tracks", (GStrv) track_array->pdata,
                         "trymode", _snapd_json_get_bool (object, "trymode", FALSE),
                         "version", _snapd_json_get_string (object, "version", NULL),
                         "website", _snapd_json_get_string (object, "website", NULL),
                         "proceed-time", proceed_time,
                         NULL);
}

SnapdApp *
_snapd_json_parse_app (JsonNode *node, const gchar *snap_name, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected app type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    const gchar *daemon = _snapd_json_get_string (object, "daemon", NULL);
    SnapdDaemonType daemon_type = SNAPD_DAEMON_TYPE_NONE;
    if (daemon == NULL)
        daemon_type = SNAPD_DAEMON_TYPE_NONE;
    else if (strcmp (daemon, "simple") == 0)
        daemon_type = SNAPD_DAEMON_TYPE_SIMPLE;
    else if (strcmp (daemon, "forking") == 0)
        daemon_type = SNAPD_DAEMON_TYPE_FORKING;
    else if (strcmp (daemon, "oneshot") == 0)
        daemon_type = SNAPD_DAEMON_TYPE_ONESHOT;
    else if (strcmp (daemon, "dbus") == 0)
        daemon_type = SNAPD_DAEMON_TYPE_DBUS;
    else if (strcmp (daemon, "notify") == 0)
        daemon_type = SNAPD_DAEMON_TYPE_NOTIFY;
    else
        daemon_type = SNAPD_DAEMON_TYPE_UNKNOWN;

    const gchar *app_snap_name = _snapd_json_get_string (object, "snap", NULL);
    return g_object_new (SNAPD_TYPE_APP,
                         "name", _snapd_json_get_string (object, "name", NULL),
                         "active", _snapd_json_get_bool (object, "active", FALSE),
                         "common-id", _snapd_json_get_string (object, "common-id", NULL),
                         "daemon-type", daemon_type,
                         "desktop-file", _snapd_json_get_string (object, "desktop-file", NULL),
                         "enabled", _snapd_json_get_bool (object, "enabled", FALSE),
                         "snap", snap_name ? snap_name : app_snap_name,
                         NULL);
}

SnapdAlias *
_snapd_json_parse_alias (JsonNode *node, const gchar *snap_name, const gchar *name, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected alias type");
        return FALSE;
    }
    JsonObject *object = json_node_get_object (node);

    const gchar *status_string = _snapd_json_get_string (object, "status", NULL);
    SnapdAliasStatus status = SNAPD_ALIAS_STATUS_UNKNOWN;
    if (strcmp (status_string, "disabled") == 0)
        status = SNAPD_ALIAS_STATUS_DISABLED;
    else if (strcmp (status_string, "auto") == 0)
        status = SNAPD_ALIAS_STATUS_AUTO;
    else if (strcmp (status_string, "manual") == 0)
        status = SNAPD_ALIAS_STATUS_MANUAL;
    else
        status = SNAPD_ALIAS_STATUS_UNKNOWN;

    return g_object_new (SNAPD_TYPE_ALIAS,
                         "snap", snap_name,
                         "app-auto", _snapd_json_get_string (object, "auto", NULL),
                         "app-manual", _snapd_json_get_string (object, "manual", NULL),
                         "command", _snapd_json_get_string (object, "command", NULL),
                         "name", name,
                         "status", status,
                         NULL);
}

SnapdUserInformation *
_snapd_json_parse_user_information (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected user information type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(JsonArray) ssh_keys = _snapd_json_get_array (object, "ssh-keys");
    g_autoptr(GPtrArray) ssh_key_array = g_ptr_array_new ();
    for (guint i = 0; i < json_array_get_length (ssh_keys); i++) {
        JsonNode *node = json_array_get_element (ssh_keys, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected SSH key type");
            return NULL;
        }

        g_ptr_array_add (ssh_key_array, (gpointer) json_node_get_string (node));
    }

    g_ptr_array_add (ssh_key_array, NULL);

    g_autoptr(SnapdAuthData) auth_data = NULL;
    if (json_object_has_member (object, "macaroon")) {
        g_autoptr(JsonArray) discharges = _snapd_json_get_array (object, "discharges");
        g_autoptr(GPtrArray) discharge_array = g_ptr_array_new ();
        for (guint i = 0; i < json_array_get_length (discharges); i++) {
            JsonNode *node = json_array_get_element (discharges, i);

            if (json_node_get_value_type (node) != G_TYPE_STRING) {
                g_set_error (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected discharge type");
                return FALSE;
            }

            g_ptr_array_add (discharge_array, (gpointer) json_node_get_string (node));
        }
        g_ptr_array_add (discharge_array, NULL);
        auth_data = snapd_auth_data_new (_snapd_json_get_string (object, "macaroon", NULL), (GStrv) discharge_array->pdata);
    }

    return g_object_new (SNAPD_TYPE_USER_INFORMATION,
                         "id", _snapd_json_get_int (object, "id", -1),
                         "username", _snapd_json_get_string (object, "username", NULL),
                         "email", _snapd_json_get_string (object, "email", NULL),
                         "ssh-keys", (GStrv) ssh_key_array->pdata,
                         "auth-data", auth_data,
                         NULL);
}

GHashTable *
_snapd_json_parse_object (JsonObject *object, GError **error)
{
    g_autoptr(GHashTable) attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);

    JsonObjectIter iter;
    json_object_iter_init (&iter, object);
    const gchar *attribute_name;
    JsonNode *node;
    while (json_object_iter_next (&iter, &attribute_name, &node)) {
        GVariant *variant = json_gvariant_deserialize (node, NULL, error);
        if (variant == NULL)
            return NULL;
        g_hash_table_insert (attributes, g_strdup (attribute_name), g_variant_ref_sink (variant));
    }

    return g_steal_pointer (&attributes);
}

GHashTable *
_snapd_json_parse_attributes (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected attributes type");
        return NULL;
    }

    return _snapd_json_parse_object (json_node_get_object (node), error);
}

SnapdSlot *
_snapd_json_parse_slot (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected slot type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(JsonArray) connections = _snapd_json_get_array (object, "connections");
    g_autoptr(GPtrArray) plug_refs = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (connections); i++) {
        JsonNode *node = json_array_get_element (connections, i);

        SnapdPlugRef *plug_ref = _snapd_json_parse_plug_ref (node, error);
        if (plug_ref == NULL)
            return NULL;
        g_ptr_array_add (plug_refs, plug_ref);
    }

    g_autoptr(GHashTable) attributes = NULL;
    if (json_object_has_member (object, "attrs")) {
        attributes = _snapd_json_parse_attributes (json_object_get_member (object, "attrs"), error);
        if (attributes == NULL)
            return FALSE;
    }
    else
        attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);

    return g_object_new (SNAPD_TYPE_SLOT,
                         "name", _snapd_json_get_string (object, "slot", NULL),
                         "snap", _snapd_json_get_string (object, "snap", NULL),
                         "interface", _snapd_json_get_string (object, "interface", NULL),
                         "label", _snapd_json_get_string (object, "label", NULL),
                         "connections", plug_refs,
                         "attributes", attributes,
                         // FIXME: apps
                         NULL);
}

SnapdPlug *
_snapd_json_parse_plug (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected plug type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(JsonArray) connections = _snapd_json_get_array (object, "connections");
    g_autoptr(GPtrArray) slot_refs = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (connections); i++) {
        JsonNode *node = json_array_get_element (connections, i);

        SnapdSlotRef *slot_ref = _snapd_json_parse_slot_ref (node, error);
        if (slot_ref == NULL)
            return NULL;
        g_ptr_array_add (slot_refs, slot_ref);
    }

    g_autoptr(GHashTable) attributes = NULL;
    if (json_object_has_member (object, "attrs")) {
        attributes = _snapd_json_parse_attributes (json_object_get_member (object, "attrs"), error);
        if (attributes == NULL)
            return FALSE;
    }
    else
        attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);

    return g_object_new (SNAPD_TYPE_PLUG,
                         "name", _snapd_json_get_string (object, "plug", NULL),
                         "snap", _snapd_json_get_string (object, "snap", NULL),
                         "interface", _snapd_json_get_string (object, "interface", NULL),
                         "label", _snapd_json_get_string (object, "label", NULL),
                         "connections", slot_refs,
                         "attributes", attributes,
                         // FIXME: apps
                         NULL);
}

SnapdSlotRef *
_snapd_json_parse_slot_ref (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected slot ref type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    return g_object_new (SNAPD_TYPE_SLOT_REF,
                         "slot", _snapd_json_get_string (object, "slot", NULL),
                         "snap", _snapd_json_get_string (object, "snap", NULL),
                         NULL);
}

SnapdPlugRef *
_snapd_json_parse_plug_ref (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected plug ref type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    return g_object_new (SNAPD_TYPE_PLUG_REF,
                         "plug", _snapd_json_get_string (object, "plug", NULL),
                         "snap", _snapd_json_get_string (object, "snap", NULL),
                         NULL);
}

SnapdConnection *
_snapd_json_parse_connection (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected connection type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(SnapdSlotRef) slot_ref = NULL;
    if (json_object_has_member (object, "slot")) {
        slot_ref = _snapd_json_parse_slot_ref (json_object_get_member (object, "slot"), error);
        if (slot_ref == NULL)
            return NULL;
    }
    g_autoptr(SnapdPlugRef) plug_ref = NULL;
    if (json_object_has_member (object, "plug")) {
        plug_ref = _snapd_json_parse_plug_ref (json_object_get_member (object, "plug"), error);
        if (plug_ref == NULL)
            return NULL;
    }

    g_autoptr(GHashTable) slot_attributes = NULL;
    if (json_object_has_member (object, "slot-attrs")) {
        slot_attributes = _snapd_json_parse_attributes (json_object_get_member (object, "slot-attrs"), error);
        if (slot_attributes == NULL)
            return FALSE;
    }
    else
        slot_attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);

    g_autoptr(GHashTable) plug_attributes = NULL;
    if (json_object_has_member (object, "plug-attrs")) {
        plug_attributes = _snapd_json_parse_attributes (json_object_get_member (object, "plug-attrs"), error);
        if (plug_attributes == NULL)
            return FALSE;
    }
    else
        plug_attributes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_variant_unref);

    return g_object_new (SNAPD_TYPE_CONNECTION,
                         "slot", slot_ref,
                         "plug", plug_ref,
                         "interface", _snapd_json_get_string (object, "interface", NULL),
                         "manual", _snapd_json_get_bool (object, "manual", FALSE),
                         "gadget", _snapd_json_get_bool (object, "gadget", FALSE),
                         "slot-attrs", slot_attributes,
                         "plug-attrs", plug_attributes,
                         NULL);
}

SnapdInterface *
_snapd_json_parse_interface (JsonNode *node, GError **error)
{
    if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
        g_set_error (error,
                     SNAPD_ERROR,
                     SNAPD_ERROR_READ_FAILED,
                     "Unexpected interface type");
        return NULL;
    }
    JsonObject *object = json_node_get_object (node);

    g_autoptr(JsonArray) plugs = _snapd_json_get_array (object, "plugs");
    g_autoptr(GPtrArray) plug_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (plugs); i++) {
        JsonNode *node = json_array_get_element (plugs, i);

        SnapdPlug *plug = _snapd_json_parse_plug (node, error);
        if (plug == NULL)
            return FALSE;

        g_ptr_array_add (plug_array, plug);
    }

    g_autoptr(JsonArray) slots = _snapd_json_get_array (object, "slots");
    g_autoptr(GPtrArray) slot_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (guint i = 0; i < json_array_get_length (slots); i++) {
        JsonNode *node = json_array_get_element (slots, i);

        SnapdSlot *slot = _snapd_json_parse_slot (node, error);
        if (slot == NULL)
            return FALSE;

        g_ptr_array_add (slot_array, slot);
    }

    return g_object_new (SNAPD_TYPE_INTERFACE,
                         "name", _snapd_json_get_string (object, "name", NULL),
                         "summary", _snapd_json_get_string (object, "summary", NULL),
                         "doc-url", _snapd_json_get_string (object, "doc-url", NULL),
                         "plugs", plug_array,
                         "slots", slot_array,
                         NULL);
}
