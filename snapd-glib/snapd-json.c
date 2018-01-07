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
#include "snapd-screenshot.h"
#include "snapd-task.h"

void
_snapd_json_set_body (SoupMessage *message, JsonBuilder *builder)
{
    g_autoptr(JsonNode) json_root = NULL;
    g_autoptr(JsonGenerator) json_generator = NULL;
    g_autofree gchar *data = NULL;
    gsize data_length;

    json_root = json_builder_get_root (builder);
    json_generator = json_generator_new ();
    json_generator_set_pretty (json_generator, TRUE);
    json_generator_set_root (json_generator, json_root);
    data = json_generator_to_data (json_generator, &data_length);

    soup_message_headers_set_content_type (message->request_headers, "application/json", NULL);
    soup_message_body_append_take (message->request_body, g_steal_pointer (&data), data_length);
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);
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
        g_auto(GStrv) tokens = NULL;

        tokens = g_strsplit (date_string, "-", -1);
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
        g_auto(GStrv) tokens = NULL;

        tokens = g_strsplit (time_string, ":", 3);
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

static GDateTime *
parse_date_time (JsonObject *object, const gchar *name)
{
    const gchar *value;
    g_auto(GStrv) tokens = NULL;
    g_autoptr(GTimeZone) timezone = NULL;
    gint year = 0, month = 0, day = 0, hour = 0, minute = 0;
    gdouble seconds = 0.0;

    value = _snapd_json_get_string (object, name, NULL);
    if (value == NULL)
        return NULL;

    /* Example: 2016-05-17T09:36:53+12:00 */
    tokens = g_strsplit (value, "T", 2);
    if (!parse_date (tokens[0], &year, &month, &day))
        return NULL;
    if (tokens[1] != NULL) {
        gchar *timezone_start;

        /* Timezone is either Z (UTC) +hh:mm or -hh:mm */
        timezone_start = tokens[1];
        while (*timezone_start != '\0' && !is_timezone_prefix (*timezone_start))
            timezone_start++;
        if (*timezone_start != '\0')
            timezone = g_time_zone_new (timezone_start);

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
parse_error_response (JsonObject *root, GError **error)
{
    const gchar *kind = NULL, *message = NULL;
    gint64 status_code;
    JsonObject *result;

    result = _snapd_json_get_object (root, "result");
    status_code = _snapd_json_get_int (root, "status-code", 0);
    kind = result != NULL ? _snapd_json_get_string (result, "kind", NULL) : NULL;
    message = result != NULL ? _snapd_json_get_string (result, "message", NULL) : NULL;

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
    else {
        switch (status_code) {
        case SOUP_STATUS_BAD_REQUEST:
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_BAD_REQUEST,
                                 message);
            break;
        case SOUP_STATUS_UNAUTHORIZED:
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_AUTH_DATA_REQUIRED,
                                 message);
            break;
        case SOUP_STATUS_FORBIDDEN:
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_PERMISSION_DENIED,
                                 message);
            break;
        /* Other response codes currently produced by snapd:
        case SOUP_STATUS_NOT_FOUND:
        case SOUP_STATUS_METHOD_NOT_ALLOWED:
        case SOUP_STATUS_NOT_IMPLEMENTED:
        case SOUP_STATUS_CONFLICT:
         */
        default:
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_FAILED,
                                 message);
        }
    }
}

JsonObject *
_snapd_json_parse_response (SoupMessage *message, GError **error)
{
    const gchar *content_type;
    g_autoptr(JsonParser) parser = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;
    g_autoptr(GError) error_local = NULL;
    JsonObject *root;
    JsonNode *type_node;

    content_type = soup_message_headers_get_content_type (message->response_headers, NULL);
    if (content_type == NULL) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd returned no content type");
        return NULL;
    }
    if (g_strcmp0 (content_type, "application/json") != 0) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd returned unexpected content type %s", content_type);
        return NULL;
    }

    parser = json_parser_new ();
    buffer = soup_message_body_flatten (message->response_body);
    if (!json_parser_load_from_data (parser, buffer->data, buffer->length, &error_local)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "Unable to parse snapd response: %s", error_local->message);
        return NULL;
    }

    if (!JSON_NODE_HOLDS_OBJECT (json_parser_get_root (parser))) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd response does is not a valid JSON object");
        return NULL;
    }
    root = json_node_get_object (json_parser_get_root (parser));

    type_node = json_object_get_member (root, "type");
    if (type_node == NULL || json_node_get_value_type (type_node) != G_TYPE_STRING) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_BAD_RESPONSE, "snapd response does not have a type");
        return NULL;
    }

    if (strcmp (json_node_get_string (type_node), "error") == 0) {
        parse_error_response (root, error);
        return NULL;
    }

    return json_object_ref (root);
}

static JsonNode *
parse_sync_response (JsonObject *response, GError **error)
{
    const gchar *type;

    type = json_object_get_string_member (response, "type");
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
    /*g_autoptr(JsonNode) result = parse_sync_response (response, error);*/
    JsonObject *r;
    JsonNode *result = parse_sync_response (response, error);
    if (result == NULL)
        return NULL;

    if (!JSON_NODE_HOLDS_OBJECT (result)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Result is not an object");
        json_node_unref (result);
        return NULL;
    }

    r = json_object_ref (json_node_get_object (result));
    json_node_unref (result);
    return r;
}

JsonArray *
_snapd_json_get_sync_result_a (JsonObject *response, GError **error)
{
    /* FIXME: Needs json-glib to be fixed to use json_node_unref */
    /*g_autoptr(JsonNode) result = parse_sync_response (response, error);*/
    JsonArray *r;
    JsonNode *result = parse_sync_response (response, error);
    if (result == NULL)
        return NULL;

    if (!JSON_NODE_HOLDS_ARRAY (result)) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Result is not an array");
        json_node_unref (result);
        return NULL;
    }

    r = json_array_ref (json_node_get_array (result));
    json_node_unref (result);
    return r;
}

gchar *
_snapd_json_get_async_result (JsonObject *response, GError **error)
{
    const gchar *type;
    JsonNode *change_node;

    type = json_object_get_string_member (response, "type");
    if (strcmp (type, "async") != 0) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected response '%s' returned for async request", type);
        return NULL;
    }

    change_node = json_object_get_member (response, "change");
    if (change_node == NULL || json_node_get_value_type (change_node) != G_TYPE_STRING) {
        g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "No change returned for async request");
        return NULL;
    }

    return g_strdup (json_node_get_string (change_node));
}

SnapdChange *
_snapd_json_parse_change (JsonObject *object, GError **error)
{
    g_autoptr(JsonArray) array = NULL;
    guint i;
    g_autoptr(GPtrArray) tasks = NULL;
    g_autoptr(GDateTime) main_spawn_time = NULL;
    g_autoptr(GDateTime) main_ready_time = NULL;

    array = _snapd_json_get_array (object, "tasks");
    tasks = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        JsonObject *object, *progress;
        g_autoptr(GDateTime) spawn_time = NULL;
        g_autoptr(GDateTime) ready_time = NULL;
        g_autoptr(SnapdTask) t = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error,
                         SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Unexpected task type");
            return NULL;
        }
        object = json_node_get_object (node);
        progress = _snapd_json_get_object (object, "progress");
        spawn_time = parse_date_time (object, "spawn-time");
        ready_time = parse_date_time (object, "ready-time");

        t = g_object_new (SNAPD_TYPE_TASK,
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

    main_spawn_time = parse_date_time (object, "spawn-time");
    main_ready_time = parse_date_time (object, "ready-time");

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

static GPtrArray *
_snapd_json_parse_apps (const gchar *snap_name, JsonArray *apps, GError **error)
{
    g_autoptr(GPtrArray) apps_array = NULL;
    guint i;

    apps_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (apps); i++) {
        JsonNode *node = json_array_get_element (apps, i);
        JsonObject *a;
        const gchar *daemon;
        g_autoptr(SnapdApp) app = NULL;
        SnapdDaemonType daemon_type = SNAPD_DAEMON_TYPE_NONE;
        const gchar *app_snap_name;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected app type");
            return NULL;
        }

        a = json_node_get_object (node);

        daemon = _snapd_json_get_string (a, "daemon", NULL);
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

        app_snap_name = _snapd_json_get_string (a, "snap", NULL);
        app = g_object_new (SNAPD_TYPE_APP,
                            "name", _snapd_json_get_string (a, "name", NULL),
                            "active", _snapd_json_get_bool (a, "active", FALSE),
                            "daemon-type", daemon_type,
                            "desktop-file", _snapd_json_get_string (a, "desktop-file", NULL),
                            "enabled", _snapd_json_get_bool (a, "enabled", FALSE),
                            "snap", app_snap_name != NULL ? app_snap_name : snap_name,
                            NULL);
        g_ptr_array_add (apps_array, g_steal_pointer (&app));
    }

    return g_steal_pointer (&apps_array);
}

SnapdSnap *
_snapd_json_parse_snap (JsonObject *object, GError **error)
{
    const gchar *name;
    SnapdConfinement confinement;
    const gchar *snap_type_string;
    SnapdSnapType snap_type = SNAPD_SNAP_TYPE_UNKNOWN;
    const gchar *snap_status_string;
    SnapdSnapStatus snap_status = SNAPD_SNAP_STATUS_UNKNOWN;
    g_autoptr(JsonArray) apps = NULL;
    JsonObject *channels;
    g_autoptr(GDateTime) install_date = NULL;
    JsonObject *prices;
    g_autoptr(GPtrArray) apps_array = NULL;
    g_autoptr(GPtrArray) channels_array = NULL;
    g_autoptr(GPtrArray) prices_array = NULL;
    g_autoptr(JsonArray) screenshots = NULL;
    g_autoptr(GPtrArray) screenshots_array = NULL;
    g_autoptr(JsonArray) tracks = NULL;
    g_autoptr(GPtrArray) track_array = NULL;
    guint i;

    name = _snapd_json_get_string (object, "name", NULL);

    confinement = parse_confinement (_snapd_json_get_string (object, "confinement", ""));

    snap_type_string = _snapd_json_get_string (object, "type", "");
    if (strcmp (snap_type_string, "app") == 0)
        snap_type = SNAPD_SNAP_TYPE_APP;
    else if (strcmp (snap_type_string, "kernel") == 0)
        snap_type = SNAPD_SNAP_TYPE_KERNEL;
    else if (strcmp (snap_type_string, "gadget") == 0)
        snap_type = SNAPD_SNAP_TYPE_GADGET;
    else if (strcmp (snap_type_string, "os") == 0)
        snap_type = SNAPD_SNAP_TYPE_OS;

    snap_status_string = _snapd_json_get_string (object, "status", "");
    if (strcmp (snap_status_string, "available") == 0)
        snap_status = SNAPD_SNAP_STATUS_AVAILABLE;
    else if (strcmp (snap_status_string, "priced") == 0)
        snap_status = SNAPD_SNAP_STATUS_PRICED;
    else if (strcmp (snap_status_string, "installed") == 0)
        snap_status = SNAPD_SNAP_STATUS_INSTALLED;
    else if (strcmp (snap_status_string, "active") == 0)
        snap_status = SNAPD_SNAP_STATUS_ACTIVE;

    apps = _snapd_json_get_array (object, "apps");
    apps_array = _snapd_json_parse_apps (name, apps, error);
    if (apps_array == NULL)
        return NULL;

    channels = _snapd_json_get_object (object, "channels");
    channels_array = g_ptr_array_new_with_free_func (g_object_unref);
    if (channels != NULL) {
        JsonObjectIter iter;
        const gchar *name;
        JsonNode *channel_node;

        json_object_iter_init (&iter, channels);
        while (json_object_iter_next (&iter, &name, &channel_node)) {
            JsonObject *c;
            SnapdConfinement confinement;
            g_autoptr(SnapdChannel) channel = NULL;

            if (json_node_get_value_type (channel_node) != JSON_TYPE_OBJECT) {
                g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected channel type");
                return NULL;
            }
            c = json_node_get_object (channel_node);

            confinement = parse_confinement (_snapd_json_get_string (c, "confinement", ""));

            channel = g_object_new (SNAPD_TYPE_CHANNEL,
                                    "confinement", confinement,
                                    "epoch", _snapd_json_get_string (c, "epoch", NULL),
                                    "name", _snapd_json_get_string (c, "channel", NULL),
                                    "revision", _snapd_json_get_string (c, "revision", NULL),
                                    "size", _snapd_json_get_int (c, "size", 0),
                                    "version", _snapd_json_get_string (c, "version", NULL),
                                    NULL);
            g_ptr_array_add (channels_array, g_steal_pointer (&channel));
        }
    }

    install_date = parse_date_time (object, "install-date");

    prices = _snapd_json_get_object (object, "prices");
    prices_array = g_ptr_array_new_with_free_func (g_object_unref);
    if (prices != NULL) {
        JsonObjectIter iter;
        const gchar *currency;
        JsonNode *amount_node;

        json_object_iter_init (&iter, prices);
        while (json_object_iter_next (&iter, &currency, &amount_node)) {
            g_autoptr(SnapdPrice) price = NULL;

            if (json_node_get_value_type (amount_node) != G_TYPE_DOUBLE) {
                g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected price type");
                return NULL;
            }

            price = g_object_new (SNAPD_TYPE_PRICE,
                                  "amount", json_node_get_double (amount_node),
                                  "currency", currency,
                                  NULL);
            g_ptr_array_add (prices_array, g_steal_pointer (&price));
        }
    }

    screenshots = _snapd_json_get_array (object, "screenshots");
    screenshots_array = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (screenshots); i++) {
        JsonNode *node = json_array_get_element (screenshots, i);
        JsonObject *s;
        g_autoptr(SnapdScreenshot) screenshot = NULL;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected screenshot type");
            return NULL;
        }

        s = json_node_get_object (node);
        screenshot = g_object_new (SNAPD_TYPE_SCREENSHOT,
                                   "url", _snapd_json_get_string (s, "url", NULL),
                                   "width", (guint) _snapd_json_get_int (s, "width", 0),
                                   "height", (guint) _snapd_json_get_int (s, "height", 0),
                                   NULL);
        g_ptr_array_add (screenshots_array, g_steal_pointer (&screenshot));
    }

    /* The tracks field was originally incorrectly named, fixed in snapd 61ad9ed (2.29.5) */
    if (json_object_has_member (object, "Tracks"))
        tracks = _snapd_json_get_array (object, "Tracks");
    else
        tracks = _snapd_json_get_array (object, "tracks");
    track_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (tracks); i++) {
        JsonNode *node = json_array_get_element (tracks, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected track type");
            return NULL;
        }

        g_ptr_array_add (track_array, (gpointer) json_node_get_string (node));
    }
    g_ptr_array_add (track_array, NULL);

    return g_object_new (SNAPD_TYPE_SNAP,
                         "apps", apps_array,
                         "broken", _snapd_json_get_string (object, "broken", NULL),
                         "channel", _snapd_json_get_string (object, "channel", NULL),
                         "channels", channels_array,
                         "confinement", confinement,
                         "contact", _snapd_json_get_string (object, "contact", NULL),
                         "description", _snapd_json_get_string (object, "description", NULL),
                         "developer", _snapd_json_get_string (object, "developer", NULL),
                         "devmode", _snapd_json_get_bool (object, "devmode", FALSE),
                         "download-size", _snapd_json_get_int (object, "download-size", 0),
                         "icon", _snapd_json_get_string (object, "icon", NULL),
                         "id", _snapd_json_get_string (object, "id", NULL),
                         "install-date", install_date,
                         "installed-size", _snapd_json_get_int (object, "installed-size", 0),
                         "jailmode", _snapd_json_get_bool (object, "jailmode", FALSE),
                         "license", _snapd_json_get_string (object, "license", NULL),
                         "name", name,
                         "prices", prices_array,
                         "private", _snapd_json_get_bool (object, "private", FALSE),
                         "revision", _snapd_json_get_string (object, "revision", NULL),
                         "screenshots", screenshots_array,
                         "snap-type", snap_type,
                         "status", snap_status,
                         "summary", _snapd_json_get_string (object, "summary", NULL),
                         "title", _snapd_json_get_string (object, "title", NULL),
                         "tracking-channel", _snapd_json_get_string (object, "tracking-channel", NULL),
                         "tracks", (gchar **) track_array->pdata,
                         "trymode", _snapd_json_get_bool (object, "trymode", FALSE),
                         "version", _snapd_json_get_string (object, "version", NULL),
                         NULL);
}

GPtrArray *
_snapd_json_parse_snap_array (JsonArray *array, GError **error)
{
    g_autoptr(GPtrArray) snaps = NULL;
    guint i;

    snaps = g_ptr_array_new_with_free_func (g_object_unref);
    for (i = 0; i < json_array_get_length (array); i++) {
        JsonNode *node = json_array_get_element (array, i);
        SnapdSnap *snap;

        if (json_node_get_value_type (node) != JSON_TYPE_OBJECT) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected snap type");
            return NULL;
        }

        snap = _snapd_json_parse_snap (json_node_get_object (node), error);
        if (snap == NULL)
            return NULL;
        g_ptr_array_add (snaps, snap);
    }

    return g_steal_pointer (&snaps);
}

GPtrArray *
_snapd_json_parse_app_array (JsonArray *array, GError **error)
{
    return _snapd_json_parse_apps (NULL, array, error);
}

SnapdUserInformation *
_snapd_json_parse_user_information (JsonObject *object, GError **error)
{
    g_autoptr(JsonArray) ssh_keys = NULL;
    g_autoptr(GPtrArray) ssh_key_array = NULL;
    g_autoptr(SnapdAuthData) auth_data = NULL;
    guint i;

    ssh_keys = _snapd_json_get_array (object, "ssh-keys");
    ssh_key_array = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (ssh_keys); i++) {
        JsonNode *node = json_array_get_element (ssh_keys, i);

        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error (error, SNAPD_ERROR, SNAPD_ERROR_READ_FAILED, "Unexpected SSH key type");
            return NULL;
        }

        g_ptr_array_add (ssh_key_array, (gpointer) json_node_get_string (node));
    }

    g_ptr_array_add (ssh_key_array, NULL);

    if (json_object_has_member (object, "macaroon")) {
        g_autoptr(JsonArray) discharges = NULL;
        g_autoptr(GPtrArray) discharge_array = NULL;

        discharges = _snapd_json_get_array (object, "discharges");
        discharge_array = g_ptr_array_new ();
        for (i = 0; i < json_array_get_length (discharges); i++) {
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
        auth_data = snapd_auth_data_new (_snapd_json_get_string (object, "macaroon", NULL), (gchar **) discharge_array->pdata);
    }

    return g_object_new (SNAPD_TYPE_USER_INFORMATION,
                         "id", _snapd_json_get_int (object, "id", -1),
                         "username", _snapd_json_get_string (object, "username", NULL),
                         "email", _snapd_json_get_string (object, "email", NULL),
                         "ssh-keys", (gchar **) ssh_key_array->pdata,
                         "auth-data", auth_data,
                         NULL);
}
