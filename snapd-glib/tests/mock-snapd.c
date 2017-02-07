/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <ctype.h>

#include "mock-snapd.h"

typedef struct
{
    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *status;
    gchar *spawn_time;
    gchar *ready_time;
    int task_index;
    GList *tasks;
    JsonNode *data;
} MockChange;

typedef struct
{
    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *status;
    gchar *progress_label;
    int progress_done;
    int progress_total;
    gchar *spawn_time;
    gchar *ready_time;
} MockTask;

struct _MockSnapd
{
    GObject parent_instance;

    GSocket *client_socket;
    GSocket *server_socket;
    GSource *read_source;
    GByteArray *buffer;
    gsize n_read;
    GList *accounts;
    GList *snaps;
    gchar *store;
    GList *store_sections;
    GList *store_snaps;
    GList *plugs;
    GList *slots;
    int change_index;
    GList *changes;
    gchar *suggested_currency;
    gchar *spawn_time;
    gchar *ready_time;
};

G_DEFINE_TYPE (MockSnapd, mock_snapd, G_TYPE_OBJECT)

static void
mock_app_free (MockApp *app)
{
    g_free (app->name);
    g_list_free_full (app->aliases, g_free);
}

static void
mock_price_free (MockPrice *price)
{
    g_free (price->currency);
    g_slice_free (MockPrice, price);
}

static void
mock_screenshot_free (MockScreenshot *screenshot)
{
    g_free (screenshot->url);
    g_slice_free (MockScreenshot, screenshot);
}

static void
mock_plug_free (MockPlug *plug)
{
    g_free (plug->name);
    g_free (plug->interface);
    g_free (plug->label);
    g_slice_free (MockPlug, plug);
}

static void
mock_slot_free (MockSlot *slot)
{
    g_free (slot->name);
    g_free (slot->interface);
    g_free (slot->label);
    g_slice_free (MockSlot, slot);
}

static void
mock_snap_free (MockSnap *snap)
{
    g_list_free_full (snap->apps, (GDestroyNotify) mock_app_free);
    g_free (snap->channel);
    g_free (snap->confinement);
    g_free (snap->description);
    g_free (snap->developer);
    g_free (snap->icon);
    g_free (snap->id);
    g_free (snap->install_date);
    g_free (snap->name);
    g_list_free_full (snap->prices, (GDestroyNotify) mock_price_free);
    g_list_free_full (snap->screenshots, (GDestroyNotify) mock_screenshot_free);
    g_free (snap->revision);
    g_free (snap->status);
    g_free (snap->summary);
    g_free (snap->tracking_channel);
    g_free (snap->type);
    g_free (snap->version);
    g_list_free_full (snap->store_sections, g_free);
    g_list_free_full (snap->plugs, (GDestroyNotify) mock_plug_free);
    g_list_free_full (snap->slots, (GDestroyNotify) mock_slot_free);
    g_slice_free (MockSnap, snap);
}

MockSnapd *
mock_snapd_new (void)
{
    return g_object_new (MOCK_TYPE_SNAPD, NULL);
}

GSocket *
mock_snapd_get_client_socket (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);
    return snapd->client_socket;
}

void
mock_snapd_set_store (MockSnapd *snapd, const gchar *name)
{
    g_free (snapd->store);
    snapd->store = g_strdup (name);
}

void
mock_snapd_set_suggested_currency (MockSnapd *snapd, const gchar *currency)
{
    g_free (snapd->suggested_currency);
    snapd->suggested_currency = g_strdup (currency);
}

void
mock_snapd_set_spawn_time (MockSnapd *snapd, const gchar *spawn_time)
{
    g_free (snapd->spawn_time);
    snapd->spawn_time = g_strdup (spawn_time);
}

void
mock_snapd_set_ready_time (MockSnapd *snapd, const gchar *ready_time)
{
    g_free (snapd->ready_time);
    snapd->ready_time = g_strdup (ready_time);
}

MockAccount *
mock_snapd_add_account (MockSnapd *snapd, const gchar *username, const gchar *password, const gchar *otp)
{
    MockAccount *account;

    account = g_slice_new0 (MockAccount);
    account->username = g_strdup (username);
    account->password = g_strdup (password);
    account->otp = g_strdup (otp);
    account->macaroon = g_strdup_printf ("MACAROON-%s", username);
    account->discharges = g_malloc (sizeof (gchar *) * 2);
    account->discharges[0] = g_strdup_printf ("DISCHARGE-%s", username);
    account->discharges[1] = NULL;
    snapd->accounts = g_list_append (snapd->accounts, account);

    return account;
}

static MockSnap *
mock_snap_new (const gchar *name)
{
    MockSnap *snap;

    snap = g_slice_new0 (MockSnap);
    snap->channel = g_strdup ("CHANNEL");
    snap->confinement = g_strdup ("strict");
    snap->description = g_strdup ("DESCRIPTION");
    snap->developer = g_strdup ("DEVELOPER");
    snap->icon = g_strdup ("ICON");
    snap->id = g_strdup ("ID");
    snap->name = g_strdup (name);
    snap->revision = g_strdup ("REVISION");
    snap->status = g_strdup ("active");
    snap->summary = g_strdup ("SUMMARY");
    snap->type = g_strdup ("app");
    snap->version = g_strdup ("VERSION");

    return snap;
}

MockSnap *
mock_account_add_private_snap (MockAccount *account, const gchar *name)
{
    MockSnap *snap;

    snap = mock_snap_new (name);
    snap->download_size = 65535;
    snap->private = TRUE;
    account->private_snaps = g_list_append (account->private_snaps, snap);

    return snap;
}

static MockAccount *
find_account (MockSnapd *snapd, const gchar *username)
{
    GList *link;

    for (link = snapd->accounts; link; link = link->next) {
        MockAccount *account = link->data;
        if (strcmp (account->username, username) == 0)
            return account;
    }

    return NULL;
}

static gboolean
discharges_match (gchar **discharges0, gchar **discharges1)
{
    int i;

    for (i = 0; discharges0[i] && discharges1[i]; i++)
        if (strcmp (discharges0[i], discharges1[i]) != 0)
            return FALSE;

    if (discharges0[i] != discharges1[i])
        return FALSE;

    return discharges0[i] == NULL;
}

static MockAccount *
find_account_by_macaroon (MockSnapd *snapd, const gchar *macaroon, gchar **discharges)
{
    GList *link;

    for (link = snapd->accounts; link; link = link->next) {
        MockAccount *account = link->data;
        if (strcmp (account->macaroon, macaroon) == 0 &&
            discharges_match (account->discharges, discharges))
            return account;
    }

    return NULL;
}

static void
mock_account_free (MockAccount *account)
{
    g_free (account->username);
    g_free (account->password);
    g_free (account->otp);
    g_free (account->macaroon);
    g_strfreev (account->discharges);
    g_list_free_full (account->private_snaps, (GDestroyNotify) mock_snap_free);
    g_slice_free (MockAccount, account);
}

MockSnap *
mock_snapd_add_snap (MockSnapd *snapd, const gchar *name)
{
    MockSnap *snap;

    snap = mock_snap_new (name);
    snap->installed_size = 65535;
    snap->install_date = g_strdup ("2017-01-01T00:00:00+12:00");
    snapd->snaps = g_list_append (snapd->snaps, snap);

    return snap;
}

MockSnap *
mock_snapd_find_snap (MockSnapd *snapd, const gchar *name)
{
    GList *link;

    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0)
            return snap;
    }

    return NULL;
}

void
mock_snapd_add_store_section (MockSnapd *snapd, const gchar *name)
{
    snapd->store_sections = g_list_append (snapd->store_sections, g_strdup (name));
}

MockSnap *
mock_snapd_add_store_snap (MockSnapd *snapd, const gchar *name)
{
    MockSnap *snap;

    snap = mock_snap_new (name);
    snap->download_size = 65535;
    snapd->store_snaps = g_list_append (snapd->store_snaps, snap);

    return snap;
}

static MockSnap *
mock_snapd_find_store_snap_by_name (MockSnapd *snapd, const gchar *name, const gchar *channel)
{
    GList *link;

    for (link = snapd->store_snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0 &&
            (channel == NULL || g_strcmp0 (snap->channel, channel) == 0))
            return snap;
    }

    return NULL;
}

static MockSnap *
mock_snapd_find_store_snap_by_id (MockSnapd *snapd, const gchar *id)
{
    GList *link;

    for (link = snapd->store_snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->id, id) == 0)
            return snap;
    }

    return NULL;
}

MockApp *
mock_snap_add_app (MockSnap *snap, const gchar *name)
{
    MockApp *app;

    app = g_slice_new0 (MockApp);
    app->name = g_strdup (name);
    snap->apps = g_list_append (snap->apps, app);

    return app;
}

void
mock_app_add_alias (MockApp *app, const gchar *alias)
{
    app->aliases = g_list_append (app->aliases, g_strdup (alias));
}

void
mock_snap_set_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->channel);
    snap->channel = g_strdup (channel);
}

void
mock_snap_set_confinement (MockSnap *snap, const gchar *confinement)
{
    g_free (snap->confinement);
    snap->confinement = g_strdup (confinement);
}

void
mock_snap_set_description (MockSnap *snap, const gchar *description)
{
    g_free (snap->description);
    snap->description = g_strdup (description);
}

void
mock_snap_set_developer (MockSnap *snap, const gchar *developer)
{
    g_free (snap->developer);
    snap->developer = g_strdup (developer);
}

void
mock_snap_set_icon (MockSnap *snap, const gchar *icon)
{
    g_free (snap->icon);
    snap->icon = g_strdup (icon);
}

void
mock_snap_set_id (MockSnap *snap, const gchar *id)
{
    g_free (snap->id);
    snap->id = g_strdup (id);
}

void
mock_snap_set_install_date (MockSnap *snap, const gchar *install_date)
{
    g_free (snap->install_date);
    snap->install_date = g_strdup (install_date);
}

MockPrice *
mock_snap_add_price (MockSnap *snap, gdouble amount, const gchar *currency)
{
    MockPrice *price;

    price = g_slice_new0 (MockPrice);
    price->amount = amount;
    price->currency = g_strdup (currency);
    snap->prices = g_list_append (snap->prices, price);

    return price;
}

static gdouble
mock_snap_find_price (MockSnap *snap, const gchar *currency)
{
    GList *link;

    for (link = snap->prices; link; link = link->next) {
        MockPrice *price = link->data;
        if (strcmp (price->currency, currency) == 0)
            return price->amount;
    }

    return 0.0;
}

void
mock_snap_set_revision (MockSnap *snap, const gchar *revision)
{
    g_free (snap->revision);
    snap->revision = g_strdup (revision);
}

MockScreenshot *
mock_snap_add_screenshot (MockSnap *snap, const gchar *url, int width, int height)
{
    MockScreenshot *screenshot;

    screenshot = g_slice_new0 (MockScreenshot);
    screenshot->url = g_strdup (url);
    screenshot->width = width;
    screenshot->height = height;
    snap->screenshots = g_list_append (snap->screenshots, screenshot);

    return screenshot;
}

void
mock_snap_set_status (MockSnap *snap, const gchar *status)
{
    g_free (snap->status);
    snap->status = g_strdup (status);
}

void
mock_snap_set_summary (MockSnap *snap, const gchar *summary)
{
    g_free (snap->summary);
    snap->summary = g_strdup (summary);
}

void
mock_snap_set_tracking_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->tracking_channel);
    snap->tracking_channel = g_strdup (channel);
}

void
mock_snap_set_type (MockSnap *snap, const gchar *type)
{
    g_free (snap->type);
    snap->type = g_strdup (type);
}

void
mock_snap_set_version (MockSnap *snap, const gchar *version)
{
    g_free (snap->version);
    snap->version = g_strdup (version);
}

void
mock_snap_add_store_section (MockSnap *snap, const gchar *name)
{
    snap->store_sections = g_list_append (snap->store_sections, g_strdup (name));
}

MockPlug *
mock_snap_add_plug (MockSnap *snap, const gchar *name)
{
    MockPlug *plug;

    plug = g_slice_new0 (MockPlug);
    plug->snap = snap;
    plug->name = g_strdup (name);
    plug->interface = g_strdup ("INTERFACE");
    plug->label = g_strdup ("LABEL");
    snap->plugs = g_list_append (snap->plugs, plug);

    return plug;
}

static MockPlug *
find_plug (MockSnap *snap, const gchar *name)
{
    GList *link;

    for (link = snap->plugs; link; link = link->next) {
        MockPlug *plug = link->data;
        if (strcmp (plug->name, name) == 0)
            return plug;
    }

    return NULL;
}

MockSlot *
mock_snap_add_slot (MockSnap *snap, const gchar *name)
{
    MockSlot *slot;

    slot = g_slice_new0 (MockSlot);
    slot->snap = snap;
    slot->name = g_strdup (name);
    slot->interface = g_strdup ("INTERFACE");
    slot->label = g_strdup ("LABEL");
    snap->slots = g_list_append (snap->slots, slot);

    return slot;
}

static MockSlot *
find_slot (MockSnap *snap, const gchar *name)
{
    GList *link;

    for (link = snap->slots; link; link = link->next) {
        MockSlot *slot = link->data;
        if (strcmp (slot->name, name) == 0)
            return slot;
    }

    return NULL;
}

static MockChange *
add_change (MockSnapd *snapd, JsonNode *data)
{
    MockChange *change;

    change = g_slice_new0 (MockChange);
    snapd->change_index++;
    change->id = g_strdup_printf ("%d", snapd->change_index);
    change->kind = g_strdup ("KIND");
    change->summary = g_strdup ("SUMMARY");
    change->status = g_strdup ("STATUS");
    change->spawn_time = g_strdup (snapd->spawn_time);
    change->ready_time = g_strdup (snapd->ready_time);
    change->task_index = snapd->change_index * 100;
    if (data != NULL)
        change->data = data;
    snapd->changes = g_list_append (snapd->changes, change);

    return change;
}

static MockChange *
get_change (MockSnapd *snapd, const gchar *id)
{
    GList *link;

    for (link = snapd->changes; link; link = link->next) {
        MockChange *change = link->data;
        if (strcmp (change->id, id) == 0)
            return change;
    }

    return NULL;
}

static MockTask *
add_task (MockChange *change, const gchar *kind)
{
    MockTask *task;

    task = g_slice_new0 (MockTask);
    task->id = g_strdup_printf ("%d", change->task_index);
    change->task_index++;
    task->kind = g_strdup (kind);
    task->summary = g_strdup ("SUMMARY");
    task->status = g_strdup ("STATUS");
    task->progress_label = g_strdup ("LABEL");
    task->progress_done = 0;
    task->progress_total = 1;
    change->tasks = g_list_append (change->tasks, task);

    return task;
}

static void
mock_task_free (MockTask *task)
{
    g_free (task->id);
    g_free (task->kind);
    g_free (task->summary);
    g_free (task->status);
    g_free (task->progress_label);  
    g_free (task->spawn_time);
    g_free (task->ready_time);
    g_slice_free (MockTask, task);
}

static void
mock_change_free (MockChange *change)
{
    g_free (change->id);
    g_free (change->kind);
    g_free (change->summary);
    g_free (change->status);
    g_free (change->spawn_time);
    g_free (change->ready_time);
    g_list_free_full (change->tasks, (GDestroyNotify) mock_task_free);
    if (change->data)
        json_node_unref (change->data);
    g_slice_free (MockChange, change);
}

static gboolean
read_data (MockSnapd *snapd, gsize size)
{
    gssize n_read;
    g_autoptr(GError) error = NULL;

    if (snapd->n_read + size > snapd->buffer->len)
        g_byte_array_set_size (snapd->buffer, snapd->n_read + size);
    n_read = g_socket_receive (snapd->server_socket,
                               (gchar *) (snapd->buffer->data + snapd->n_read),
                               size,
                               NULL,
                               &error);
    if (n_read < 0) {
        g_printerr ("read error: %s\n", error->message);
        return FALSE;
    }

    snapd->n_read += n_read;

    return TRUE;
}

static void
send_response (MockSnapd *snapd, guint status_code, const gchar *reason_phrase, const gchar *content_type, const guint8 *content, gsize content_length)
{
    g_autoptr(SoupMessageHeaders) headers = NULL;
    g_autoptr(GString) message = NULL;
    gssize n_written;
    g_autoptr(GError) error = NULL;

    message = g_string_new (NULL);
    g_string_append_printf (message, "HTTP/1.1 %u %s\r\n", status_code, reason_phrase);
    g_string_append_printf (message, "Content-Type: %s\r\n", content_type);
    g_string_append_printf (message, "Content-Length: %zi\r\n", content_length);
    g_string_append (message, "\r\n");

    n_written = g_socket_send (snapd->server_socket, message->str, message->len, NULL, &error);
    g_assert (n_written == message->len);
    n_written = g_socket_send (snapd->server_socket, (const gchar *) content, content_length, NULL, &error);
    g_assert (n_written == content_length);
}

static void
send_json_response (MockSnapd *snapd, guint status_code, const gchar *reason_phrase, JsonNode *node)
{
    g_autoptr(JsonGenerator) generator = NULL;
    g_autofree gchar *data;
    gsize data_length;

    generator = json_generator_new ();
    json_generator_set_root (generator, node);
    data = json_generator_to_data (generator, &data_length);

    send_response (snapd, status_code, reason_phrase, "application/json", (guint8*) data, data_length);
}

static JsonNode *
make_response (const gchar *type, guint status_code, const gchar *status, JsonNode *result, const gchar *change_id, const gchar *suggested_currency) // FIXME: sources
{
    g_autoptr(JsonBuilder) builder = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, type);
    json_builder_set_member_name (builder, "status-code");
    json_builder_add_int_value (builder, status_code);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, status);
    json_builder_set_member_name (builder, "result");
    if (result != NULL)
        json_builder_add_value (builder, result);
     else
        json_builder_add_null_value (builder); // FIXME: Snapd sets it to null, but it might be a bug?
    if (change_id != NULL) {
        json_builder_set_member_name (builder, "change");
        json_builder_add_string_value (builder, change_id);
    }
    if (suggested_currency != NULL) {
        json_builder_set_member_name (builder, "suggested-currency");
        json_builder_add_string_value (builder, suggested_currency);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static void
send_sync_response (MockSnapd *snapd, guint status_code, const gchar *reason_phrase, JsonNode *result, const gchar *suggested_currency)
{
    g_autoptr(JsonNode) response = NULL;

    response = make_response ("sync", status_code, reason_phrase, result, NULL, suggested_currency);
    send_json_response (snapd, status_code, reason_phrase, response);
}

static void
send_async_response (MockSnapd *snapd, guint status_code, const gchar *reason_phrase, const gchar *change_id)
{
    g_autoptr(JsonNode) response = NULL;

    response = make_response ("async", status_code, reason_phrase, NULL, change_id, NULL);
    send_json_response (snapd, status_code, reason_phrase, response);
}

static void
send_error_response (MockSnapd *snapd, guint status_code, const gchar *reason_phrase, const gchar *message, const gchar *kind)
{
    g_autoptr(JsonBuilder) builder = NULL;
    g_autoptr(JsonNode) response = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "message");
    json_builder_add_string_value (builder, message);
    if (kind != NULL) {
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, kind);
    }
    json_builder_end_object (builder);

    response = make_response ("error", status_code, reason_phrase, json_builder_get_root (builder), NULL, NULL);
    send_json_response (snapd, status_code, reason_phrase, response);
}

static void
send_error_bad_request (MockSnapd *snapd, const gchar *message, const gchar *kind)
{
    send_error_response (snapd, 400, "Bad Request", message, kind);
}

static void
send_error_unauthorized (MockSnapd *snapd, const gchar *message, const gchar *kind)
{
    send_error_response (snapd, 401, "Unauthorized", message, kind);
}

static void
send_error_not_found (MockSnapd *snapd, const gchar *message)
{
    send_error_response (snapd, 404, "Not Found", message, NULL);
}

static void
send_error_method_not_allowed (MockSnapd *snapd, const gchar *message)
{
    send_error_response (snapd, 405, "Method Not Allowed", message, NULL);
}

static void
handle_system_info (MockSnapd *snapd, const gchar *method)
{
    g_autoptr(JsonBuilder) builder = NULL;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "os-release");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, "OS-ID");
    json_builder_set_member_name (builder, "version-id");
    json_builder_add_string_value (builder, "OS-VERSION");
    json_builder_end_object (builder);
    json_builder_set_member_name (builder, "series");
    json_builder_add_string_value (builder, "SERIES");
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, "VERSION");
    if (snapd->store) {
        json_builder_set_member_name (builder, "store");
        json_builder_add_string_value (builder, snapd->store);
    }
    json_builder_end_object (builder);

    send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
}

static void
send_macaroon (MockSnapd *snapd, MockAccount *account)
{
    g_autoptr(JsonBuilder) builder = NULL;
    int i;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "macaroon");
    json_builder_add_string_value (builder, account->macaroon);
    json_builder_set_member_name (builder, "discharges");
    json_builder_begin_array (builder);
    for (i = 0; account->discharges[i]; i++)
        json_builder_add_string_value (builder, account->discharges[i]);
    json_builder_end_array (builder);
    json_builder_end_object (builder);

    send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
}

static void
handle_login (MockSnapd *snapd, const gchar *method, JsonNode *request)
{
    JsonObject *o;
    const gchar *username, *password, *otp = NULL;
    MockAccount *account;

    if (strcmp (method, "POST") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    o = json_node_get_object (request);
    username = json_object_get_string_member (o, "username");
    password = json_object_get_string_member (o, "password");
    if (json_object_has_member (o, "otp"))
        otp = json_object_get_string_member (o, "otp");

    if (strstr (username, "@") == NULL) {
        send_error_bad_request (snapd, "please use a valid email address.", "invalid-auth-data");
        return;
    }

    account = find_account (snapd, username);
    if (account == NULL) {
        send_error_unauthorized (snapd, "cannot authenticate to snap store: Provided email/password is not correct.", "login-required");
        return;
    }

    if (strcmp (account->password, password) != 0) {
        send_error_unauthorized (snapd, "cannot authenticate to snap store: Provided email/password is not correct.", "login-required");
        return;
    }

    if (account->otp != NULL) {
        if (otp == NULL) {
            send_error_unauthorized (snapd, "two factor authentication required", "two-factor-required");
            return;
        }
        if (strcmp (account->otp, otp) != 0) {
            send_error_unauthorized (snapd, "two factor authentication failed", "two-factor-failed");
            return;
        }
    }

    send_macaroon (snapd, account);
}

static JsonNode *
make_snap_node (MockSnap *snap)
{
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *resource = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snap->apps != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "apps");
        json_builder_begin_array (builder);
        for (link = snap->apps; link; link = link->next) {
            MockApp *app = link->data;
            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "name");
            json_builder_add_string_value (builder, app->name);
            if (app->aliases != NULL) {
                GList *link2;
                json_builder_set_member_name (builder, "aliases");
                json_builder_begin_array (builder);
                for (link2 = app->aliases; link2; link2 = link2->next)
                    json_builder_add_string_value (builder, link2->data);
                json_builder_end_array (builder);
            }
            json_builder_end_object (builder);
        }
        json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "channel");
    json_builder_add_string_value (builder, snap->channel);
    json_builder_set_member_name (builder, "confinement");
    json_builder_add_string_value (builder, snap->confinement);
    json_builder_set_member_name (builder, "description");
    json_builder_add_string_value (builder, snap->description);
    json_builder_set_member_name (builder, "developer");
    json_builder_add_string_value (builder, snap->developer);
    json_builder_set_member_name (builder, "devmode");
    json_builder_add_boolean_value (builder, snap->devmode);
    if (snap->download_size > 0) {
        json_builder_set_member_name (builder, "download-size");
        json_builder_add_int_value (builder, snap->download_size);
    }
    json_builder_set_member_name (builder, "icon");
    json_builder_add_string_value (builder, snap->icon);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, snap->id);
    if (snap->install_date != NULL) {
        json_builder_set_member_name (builder, "install-date");
        json_builder_add_string_value (builder, snap->install_date);
    }
    if (snap->installed_size > 0) {
        json_builder_set_member_name (builder, "installed-size");
        json_builder_add_int_value (builder, snap->installed_size);
    }
    json_builder_set_member_name (builder, "name");
    json_builder_add_string_value (builder, snap->name);
    if (snap->prices != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "prices");
        json_builder_begin_object (builder);
        for (link = snap->prices; link; link = link->next) {
            MockPrice *price = link->data;

            json_builder_set_member_name (builder, price->currency);
            json_builder_add_double_value (builder, price->amount);
        }
        json_builder_end_object (builder);
    }
    json_builder_set_member_name (builder, "private");
    json_builder_add_boolean_value (builder, snap->private);
    json_builder_set_member_name (builder, "resource");
    resource = g_strdup_printf ("/v2/snaps/%s", snap->name);
    json_builder_add_string_value (builder, resource);
    json_builder_set_member_name (builder, "revision");
    json_builder_add_string_value (builder, snap->revision);
    if (snap->screenshots != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "screenshots");
        json_builder_begin_array (builder);
        for (link = snap->screenshots; link; link = link->next) {
            MockScreenshot *screenshot = link->data;

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "url");
            json_builder_add_string_value (builder, screenshot->url);
            if (screenshot->width > 0 && screenshot->height > 0) {
                json_builder_set_member_name (builder, "width");
                json_builder_add_int_value (builder, screenshot->width);
                json_builder_set_member_name (builder, "height");
                json_builder_add_int_value (builder, screenshot->height);
            }
            json_builder_end_object (builder);
        }
        json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, snap->status);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, snap->summary);
    if (snap->tracking_channel) {
        json_builder_set_member_name (builder, "tracking-channel");
        json_builder_add_string_value (builder, snap->tracking_channel);
    }
    json_builder_set_member_name (builder, "trymode");
    json_builder_add_boolean_value (builder, snap->trymode);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, snap->type);
    json_builder_set_member_name (builder, "version");
    json_builder_add_string_value (builder, snap->version);
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static GList *
get_refreshable_snaps (MockSnapd *snapd)
{
    GList *link, *refreshable_snaps = NULL;

    for (link = snapd->store_snaps; link; link = link->next) {
        MockSnap *store_snap = link->data;
        MockSnap *snap;

        snap = mock_snapd_find_snap (snapd, store_snap->name);
        if (snap != NULL && strcmp (store_snap->revision, snap->revision) > 0)
            refreshable_snaps = g_list_append (refreshable_snaps, store_snap);
    }

    return refreshable_snaps;
}

static void
handle_snaps (MockSnapd *snapd, const gchar *method, JsonNode *request)
{
    if (strcmp (method, "GET") == 0) {
        g_autoptr(JsonBuilder) builder = NULL;
        GList *link;

        builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;
            json_builder_add_value (builder, make_snap_node (snap));
        }
        json_builder_end_array (builder);

        send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
    }
    else if (strcmp (method, "POST") == 0) {
        JsonObject *o;
        const gchar *action;

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");
        if (strcmp (action, "refresh") == 0) {
            g_autoptr(GList) refreshable_snaps = NULL;
            g_autoptr(JsonBuilder) builder = NULL;
            GList *link;
            MockChange *change;

            refreshable_snaps = get_refreshable_snaps (snapd);

            builder = json_builder_new ();
            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap-names");
            json_builder_begin_array (builder);
            for (link = refreshable_snaps; link; link = link->next) {
                MockSnap *snap = link->data;
                json_builder_add_string_value (builder, snap->name);
            }
            json_builder_end_array (builder);
            json_builder_end_object (builder);          

            change = add_change (snapd, json_builder_get_root (builder));
            send_async_response (snapd, 202, "Accepted", change->id);
        }
        else {
            send_error_bad_request (snapd, "unsupported multi-snap operation", NULL);
            return;
        }
    }
    else {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }
}

static void
handle_snap (MockSnapd *snapd, const gchar *method, const gchar *name, JsonNode *request)
{
    if (strcmp (method, "GET") == 0) {
        MockSnap *snap;

        snap = mock_snapd_find_snap (snapd, name);
        if (snap != NULL)
            send_sync_response (snapd, 200, "OK", make_snap_node (snap), NULL);
        else
            send_error_not_found (snapd, "cannot find snap");
    }
    else if (strcmp (method, "POST") == 0) {
        JsonObject *o;
        const gchar *action, *channel = NULL;

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");
        if (json_object_has_member (o, "channel"))
            channel = json_object_get_string_member (o, "channel");

        if (strcmp (action, "install") == 0) {
            MockSnap *snap;

            snap = mock_snapd_find_snap (snapd, name);
            if (snap == NULL) {
                snap = mock_snapd_find_store_snap_by_name (snapd, name, channel);
                if (snap != NULL)
                {
                    MockSnap *snap;
                    MockChange *change;

                    snap = mock_snapd_find_snap (snapd, name);
                    if (snap == NULL)
                        snap = mock_snapd_add_snap (snapd, name);
                    g_free (snap->channel);
                    snap->channel = g_strdup (channel);

                    change = add_change (snapd, NULL);
                    add_task (change, "install");
                    send_async_response (snapd, 202, "Accepted", change->id);
                }
                else
                    send_error_bad_request (snapd, "cannot install, snap not found", NULL);
            }
            else
                send_error_bad_request (snapd, "snap is already installed", "snap-already-installed");
        }
        else if (strcmp (action, "refresh") == 0) {
            MockSnap *snap;

            snap = mock_snapd_find_snap (snapd, name);
            if (snap != NULL)
            {
                MockSnap *store_snap;

                /* Find if we have a store snap with a newer revision */
                store_snap = mock_snapd_find_store_snap_by_name (snapd, name, channel);
                if (store_snap != NULL && strcmp (store_snap->revision, snap->revision) > 0) {
                    MockChange *change;

                    g_free (snap->channel);
                    snap->channel = g_strdup (channel);

                    change = add_change (snapd, NULL);
                    add_task (change, "refresh");
                    send_async_response (snapd, 202, "Accepted", change->id);
                }
                else
                    send_error_bad_request (snapd, "snap has no updates available", "snap-no-update-available");
            }
            else
                send_error_bad_request (snapd, "cannot refresh: cannot find snap", NULL);
        }
        else if (strcmp (action, "remove") == 0) {
            MockSnap *snap;

            snap = mock_snapd_find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                snapd->snaps = g_list_remove (snapd->snaps, snap);
                mock_snap_free (snap);

                change = add_change (snapd, NULL);
                add_task (change, "remove");
                send_async_response (snapd, 202, "Accepted", change->id);
            }
            else
                send_error_bad_request (snapd, "snap is not installed", "snap-not-installed");
        }
        else if (strcmp (action, "enable") == 0) {
            MockSnap *snap;

            snap = mock_snapd_find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                if (!snap->disabled) {
                    send_error_bad_request (snapd, "cannot enable: snap is already enabled", NULL);
                    return;
                }
                snap->disabled = FALSE;

                change = add_change (snapd, NULL);
                add_task (change, "enable");
                send_async_response (snapd, 202, "Accepted", change->id);
            }
            else
                send_error_bad_request (snapd, "cannot enable: cannot find snap", NULL);
        }
        else if (strcmp (action, "disable") == 0) {
            MockSnap *snap;

            snap = mock_snapd_find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                if (snap->disabled) {
                    send_error_bad_request (snapd, "cannot disable: snap is already disabled", NULL);
                    return;
                }
                snap->disabled = TRUE;

                change = add_change (snapd, NULL);
                add_task (change, "disable");
                send_async_response (snapd, 202, "Accepted", change->id);
            }
            else
                send_error_bad_request (snapd, "cannot disable: cannot find snap", NULL);
        }
        else
            send_error_bad_request (snapd, "unknown action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, "method not allowed");
}

static void
handle_icon (MockSnapd *snapd, const gchar *method, const gchar *path)
{
    MockSnap *snap;
    g_autofree gchar *name = NULL;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    if (!g_str_has_suffix (path, "/icon")) {
        send_error_not_found (snapd, "not found");
        return;
    }
    name = g_strndup (path, strlen (path) - strlen ("/icon"));

    snap = mock_snapd_find_snap (snapd, name);
    if (snap != NULL)
        send_response (snapd, 200, "OK", "image/png", (const guint8 *) "ICON", 4);
    else
        send_error_not_found (snapd, "cannot find snap");
}

static void
handle_interfaces (MockSnapd *snapd, const gchar *method, JsonNode *request)
{
    if (strcmp (method, "GET") == 0) {
        g_autoptr(JsonBuilder) builder = NULL;
        GList *link;
        g_autoptr (GList) connected_plugs = NULL;

        builder = json_builder_new ();
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "plugs");
        json_builder_begin_array (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;
            GList *l;

            for (l = snap->plugs; l; l = l->next) {
                MockPlug *plug = l->data;
                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "snap");
                json_builder_add_string_value (builder, snap->name);
                json_builder_set_member_name (builder, "plug");
                json_builder_add_string_value (builder, plug->name);
                json_builder_set_member_name (builder, "interface");
                json_builder_add_string_value (builder, plug->interface);
                json_builder_set_member_name (builder, "label");
                json_builder_add_string_value (builder, plug->label);
                if (plug->connection) {
                    json_builder_set_member_name (builder, "connections");
                    json_builder_begin_array (builder);
                    json_builder_begin_object (builder);
                    json_builder_set_member_name (builder, "snap");
                    json_builder_add_string_value (builder, plug->connection->snap->name);
                    json_builder_set_member_name (builder, "slot");
                    json_builder_add_string_value (builder, plug->connection->name);
                    json_builder_end_object (builder);
                    json_builder_end_array (builder);
                    connected_plugs = g_list_append (connected_plugs, plug);
                }
                json_builder_end_object (builder);
            }
        }
        json_builder_end_array (builder);
        json_builder_set_member_name (builder, "slots");
        json_builder_begin_array (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;
            GList *l;

            for (l = snap->slots; l; l = l->next) {
                MockSlot *slot = l->data;
                GList *l2;
                g_autoptr(GList) plugs = NULL;

                for (l2 = connected_plugs; l2; l2 = l2->next) {
                    MockPlug *plug = l2->data;
                    if (plug->connection == slot)
                        plugs = g_list_append (plugs, plug);
                }

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "snap");
                json_builder_add_string_value (builder, snap->name);
                json_builder_set_member_name (builder, "slot");
                json_builder_add_string_value (builder, slot->name);
                json_builder_set_member_name (builder, "interface");
                json_builder_add_string_value (builder, slot->interface);
                json_builder_set_member_name (builder, "label");
                json_builder_add_string_value (builder, slot->label);
                if (plugs != NULL) {
                    json_builder_set_member_name (builder, "connections");
                    json_builder_begin_array (builder);
                    for (l2 = plugs; l2; l2 = l2->next) {
                        MockPlug *plug = l2->data;
                        json_builder_begin_object (builder);
                        json_builder_set_member_name (builder, "snap");
                        json_builder_add_string_value (builder, plug->snap->name);
                        json_builder_set_member_name (builder, "plug");
                        json_builder_add_string_value (builder, plug->name);
                        json_builder_end_object (builder);
                    }
                    json_builder_end_array (builder);
                }
                json_builder_end_object (builder);
            }
        }
        json_builder_end_array (builder);
        json_builder_end_object (builder);

        send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
    }
    else if (strcmp (method, "POST") == 0) {
        const gchar *action;
        JsonObject *o;
        JsonArray *a;
        int i;
        g_autoptr(GList) plugs = NULL;
        g_autoptr(GList) slots = NULL;

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");

        a = json_object_get_array_member (o, "plugs");
        for (i = 0; i < json_array_get_length (a); i++) {
            JsonObject *po = json_array_get_object_element (a, i);
            MockSnap *snap;
            MockPlug *plug;

            snap = mock_snapd_find_snap (snapd, json_object_get_string_member (po, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, "invalid snap", NULL);
                return;
            }
            plug = find_plug (snap, json_object_get_string_member (po, "plug"));
            if (plug == NULL) {
                send_error_bad_request (snapd, "invalid plug", NULL);
                return;
            }
            plugs = g_list_append (plugs, plug);
        }

        a = json_object_get_array_member (o, "slots");
        for (i = 0; i < json_array_get_length (a); i++) {
            JsonObject *so = json_array_get_object_element (a, i);
            MockSnap *snap;
            MockSlot *slot;

            snap = mock_snapd_find_snap (snapd, json_object_get_string_member (so, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, "invalid snap", NULL);
                return;
            }
            slot = find_slot (snap, json_object_get_string_member (so, "slot"));
            if (slot == NULL) {
                send_error_bad_request (snapd, "invalid slot", NULL);
                return;
            }
            slots = g_list_append (slots, slot);
        }

        if (strcmp (action, "connect") == 0) {
            MockChange *change;
            GList *link;

            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, "at least one plug and slot is required", NULL);
                return;
            }

            for (link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                plug->connection = slots->data;
            }

            change = add_change (snapd, NULL);
            add_task (change, "connect-snap");
            send_async_response (snapd, 202, "Accepted", change->id);
        }
        else if (strcmp (action, "disconnect") == 0) {
            MockChange *change;
            GList *link;

            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, "at least one plug and slot is required", NULL);
                return;
            }

            for (link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                plug->connection = NULL;
            }

            change = add_change (snapd, NULL);
            add_task (change, "disconnect");
            send_async_response (snapd, 202, "Accepted", change->id);
        }
        else
            send_error_bad_request (snapd, "unsupported interface action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, "method not allowed");
}

static void
handle_changes (MockSnapd *snapd, const gchar *method, const gchar *change_id)
{
    MockChange *change;
    g_autoptr(JsonBuilder) builder = NULL;
    int progress_total, progress_done;
    gboolean is_ready;
    GList *link;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    change = get_change (snapd, change_id);
    if (change == NULL) {
        send_error_not_found (snapd, "cannot find change");
        return;
    }

    /* Make progress... */
    for (link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        if (task->progress_done < task->progress_total) {
            task->progress_done++;
            break;
        }
    }
    for (progress_done = 0, progress_total = 0, link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        progress_done += task->progress_done;
        progress_total += task->progress_total;
    }
    is_ready = progress_done >= progress_total;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, change->id);
    json_builder_set_member_name (builder, "kind");
    json_builder_add_string_value (builder, change->kind);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, change->summary);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, change->status);
    json_builder_set_member_name (builder, "tasks");
    json_builder_begin_array (builder);
    for (link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        json_builder_begin_object (builder);  
        json_builder_set_member_name (builder, "id");
        json_builder_add_string_value (builder, task->id);
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, task->kind);
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, task->summary);
        json_builder_set_member_name (builder, "status");
        json_builder_add_string_value (builder, task->status);
        json_builder_set_member_name (builder, "progress");
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "label");
        json_builder_add_string_value (builder, task->progress_label);
        json_builder_set_member_name (builder, "done");
        json_builder_add_int_value (builder, task->progress_done);
        json_builder_set_member_name (builder, "total");
        json_builder_add_int_value (builder, task->progress_total);
        json_builder_end_object (builder);
        if (task->spawn_time != NULL) {
            json_builder_set_member_name (builder, "spawn-time");
            json_builder_add_string_value (builder, task->spawn_time);
        }
        if (is_ready && task->ready_time != NULL) {
            json_builder_set_member_name (builder, "ready-time");
            json_builder_add_string_value (builder, task->ready_time);
        }
        json_builder_end_object (builder);
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "ready");
    json_builder_add_boolean_value (builder, is_ready);
    json_builder_set_member_name (builder, "spawn-time");
    json_builder_add_string_value (builder, change->spawn_time);
    if (is_ready && change->ready_time != NULL) {
        json_builder_set_member_name (builder, "ready-time");
        json_builder_add_string_value (builder, change->ready_time);
    }
    if (is_ready && change->data != NULL) {
        json_builder_set_member_name (builder, "data");
        json_builder_add_value (builder, json_node_ref (change->data));
    }
    json_builder_end_object (builder);

    send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
}

static gboolean
matches_query (MockSnap *snap, const gchar *query)
{
    return query != NULL && strstr (snap->name, query) != NULL;
}

static gboolean
matches_name (MockSnap *snap, const gchar *name)
{
    return name != NULL && strcmp (snap->name, name) == 0;
}

static gboolean
in_section (MockSnap *snap, const gchar *section)
{
    GList *link;

    if (section == NULL)
        return TRUE;

    for (link = snap->store_sections; link; link = link->next) 
        if (strcmp (link->data, section) == 0)
            return TRUE;

    return FALSE;
}

static void
handle_find (MockSnapd *snapd, const gchar *method, MockAccount *account, const gchar *query)
{
    const gchar *i;
    g_autofree gchar *query_param = NULL;
    g_autofree gchar *name_param = NULL;
    g_autofree gchar *select_param = NULL;
    g_autofree gchar *section_param = NULL;  
    g_autoptr(JsonBuilder) builder = NULL;
    GList *snaps, *link;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    i = query;
    while (TRUE) {
        const gchar *start;
        g_autofree gchar *param_name = NULL;
        g_autofree gchar *param_value = NULL;

        while (isspace (*i)) i++;
        if (*i == '\0')
            break;

        start = i;
        while (*i && !(isspace (*i) || *i == '=')) i++;
        param_name = g_strndup (start, i - start);
        if (*i == '\0')
            break;
        i++;

        while (isspace (*i)) i++;
        if (*i == '\0')
            break;
        start = i;
        while (*i && *i != '&') i++;
        param_value = g_strndup (start, i - start);

        if (strcmp (param_name, "q") == 0) {
             g_free (query_param);
             query_param = g_steal_pointer (&param_value);
        }
        else if (strcmp (param_name, "name") == 0) {
             g_free (name_param);
             name_param = g_steal_pointer (&param_value);
        }
        else if (strcmp (param_name, "select") == 0) {
             g_free (select_param);
             select_param = g_steal_pointer (&param_value);
        }
        else if (strcmp (param_name, "section") == 0) {
             g_free (section_param);
             section_param = g_steal_pointer (&param_value);
        }

        if (*i != '&')
            break;
        i++;
    }

    if (g_strcmp0 (select_param, "refresh") == 0) {
        g_autoptr(GList) refreshable_snaps = NULL;

        if (query_param != NULL) {
            send_error_bad_request (snapd, "cannot use 'q' with 'select=refresh'", NULL);
            return;
        }
        if (name_param != NULL) {
            send_error_bad_request (snapd, "cannot use 'name' with 'select=refresh'", NULL);
            return;
        }

        builder = json_builder_new ();
        json_builder_begin_array (builder);
        refreshable_snaps = get_refreshable_snaps (snapd);
        for (link = refreshable_snaps; link; link = link->next)
            json_builder_add_value (builder, make_snap_node (link->data));
        json_builder_end_array (builder);

        send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), snapd->suggested_currency);
        return;
    }
    else if (g_strcmp0 (select_param, "private") == 0) {
        if (account == NULL) {
            send_error_bad_request (snapd, "you need to log in first", "login-required");
            return;
        }
        snaps = account->private_snaps;
    }
    else
        snaps = snapd->store_snaps;

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        if (in_section (snap, section_param) && (matches_query (snap, query_param) || matches_name (snap, name_param)))
            json_builder_add_value (builder, make_snap_node (snap));
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), snapd->suggested_currency);
}

static void
handle_buy_ready (MockSnapd *snapd, const gchar *method, MockAccount *account)
{
    JsonNode *result;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    if (account == NULL) {
        send_error_bad_request (snapd, "you need to log in first", "login-required");
        return;
    }

    if (!account->terms_accepted) {
        send_error_bad_request (snapd, "terms of service not accepted", "terms-not-accepted");
        return;
    }

    if (!account->has_payment_methods) {
        send_error_bad_request (snapd, "no payment methods", "no-payment-methods");
        return;
    }

    result = json_node_new (JSON_NODE_VALUE);
    json_node_set_boolean (result, TRUE);

    send_sync_response (snapd, 200, "OK", result, NULL);
}

static void
handle_buy (MockSnapd *snapd, const gchar *method, MockAccount *account, JsonNode *request)
{
    JsonObject *o;
    const gchar *snap_id, *currency;
    gdouble price;
    MockSnap *snap;
    gdouble amount;

    if (strcmp (method, "POST") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    if (account == NULL) {
        send_error_bad_request (snapd, "you need to log in first", "login-required");
        return;
    }

    if (!account->terms_accepted) {
        send_error_bad_request (snapd, "terms of service not accepted", "terms-not-accepted");
        return;
    }

    if (!account->has_payment_methods) {
        send_error_bad_request (snapd, "no payment methods", "no-payment-methods");
        return;
    }

    o = json_node_get_object (request);
    snap_id = json_object_get_string_member (o, "snap-id");
    price = json_object_get_double_member (o, "price");
    currency = json_object_get_string_member (o, "currency");

    snap = mock_snapd_find_store_snap_by_id (snapd, snap_id);
    if (snap == NULL) {
        send_error_not_found (snapd, "not found"); // FIXME: Check is error snapd returns
        return;
    }

    amount = mock_snap_find_price (snap, currency);
    if (amount == 0) {
        send_error_bad_request (snapd, "no price found", "payment-declined"); // FIXME: Check is error snapd returns
        return;
    }
    if (amount != price) {
        send_error_bad_request (snapd, "invalid price", "payment-declined"); // FIXME: Check is error snapd returns
        return;
    }

    send_sync_response (snapd, 200, "OK", NULL, NULL);
}

static void
handle_sections (MockSnapd *snapd, const gchar *method)
{
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (method, "GET") != 0) {
        send_error_method_not_allowed (snapd, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snapd->store_sections; link; link = link->next)
        json_builder_add_string_value (builder, link->data);
    json_builder_end_array (builder);

    send_sync_response (snapd, 200, "OK", json_builder_get_root (builder), NULL);
}

static gboolean
parse_macaroon (const gchar *authorization, gchar **macaroon, gchar ***discharges)
{
    g_autofree gchar *scheme = NULL;
    g_autofree gchar *m = NULL;
    g_autoptr(GPtrArray) d = NULL;
    const gchar *i, *start;

    if (authorization == NULL)
        return FALSE;

    i = authorization;
    while (isspace (*i)) i++;
    start = i;
    while (*i && !isspace (*i)) i++;
    scheme = g_strndup (start, i - start);
    if (strcmp (scheme, "Macaroon") != 0)
        return FALSE;

    d = g_ptr_array_new_with_free_func (g_free);
    while (TRUE) {
        g_autofree gchar *param_name = NULL;
        g_autofree gchar *param_value = NULL;

        while (isspace (*i)) i++;
        if (*i == '\0')
            break;

        start = i;
        while (*i && !(isspace (*i) || *i == '=')) i++;
        param_name = g_strndup (start, i - start);
        if (*i == '\0')
            break;
        i++;

        while (isspace (*i)) i++;
        if (*i == '\0')
            break;
        if (*i == '"') {
            i++;
            start = i;
            while (*i && *i != '"') i++;
            param_value = g_strndup (start, i - start);
            i++;
            while (isspace (*i)) i++;
            while (*i && *i != ',') i++;
        }
        else {
            start = i;
            while (*i && *i != ',') i++;
            param_value = g_strndup (start, i - start);
        }

        if (strcmp (param_name, "root") == 0) {
             g_free (m);
             m = g_steal_pointer (&param_value);
        }
        else if (strcmp (param_name, "discharge") == 0) {
             g_ptr_array_add (d, g_steal_pointer (&param_value));
        }

        if (*i != ',')
            break;
        i++;
    }
    g_ptr_array_add (d, NULL);

    if (m == NULL)
        return FALSE;

    *macaroon = g_steal_pointer (&m);
    *discharges = (gchar **) d->pdata;
    g_ptr_array_free (d, FALSE);
    d = NULL;
    return TRUE;
}

static void
handle_request (MockSnapd *snapd, const gchar *method, const gchar *path, SoupMessageHeaders *headers, const guint8 *content, gsize content_length)
{
    const gchar *content_type, *authorization;
    g_autoptr(JsonNode) json_content = NULL;
    MockAccount *account = NULL;

    content_type = soup_message_headers_get_content_type (headers, NULL);
    if (g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonParser) parser = NULL;
        g_autoptr(GError) error = NULL;

        parser = json_parser_new ();
        if (json_parser_load_from_data (parser, (const gchar *) content, content_length, &error))
            json_content = json_node_ref (json_parser_get_root (parser));
        else
            g_warning ("Failed to parse request: %s", error->message);
    }

    authorization = soup_message_headers_get_one (headers, "Authorization");
    if (authorization != NULL) {
        g_autofree gchar *macaroon = NULL;
        g_auto(GStrv) discharges = NULL;

        if (parse_macaroon (authorization, &macaroon, &discharges))
            account = find_account_by_macaroon (snapd, macaroon, discharges);
    }

    if (strcmp (path, "/v2/system-info") == 0)
        handle_system_info (snapd, method);
    else if (strcmp (path, "/v2/login") == 0)
        handle_login (snapd, method, json_content);
    else if (strcmp (path, "/v2/snaps") == 0)
        handle_snaps (snapd, method, json_content);
    else if (g_str_has_prefix (path, "/v2/snaps/"))
        handle_snap (snapd, method, path + strlen ("/v2/snaps/"), json_content);
    else if (g_str_has_prefix (path, "/v2/icons/"))
        handle_icon (snapd, method, path + strlen ("/v2/icons/"));
    else if (strcmp (path, "/v2/interfaces") == 0)
        handle_interfaces (snapd, method, json_content);
    else if (g_str_has_prefix (path, "/v2/changes/"))
        handle_changes (snapd, method, path + strlen ("/v2/changes/"));
    else if (g_str_has_prefix (path, "/v2/find?"))
        handle_find (snapd, method, account, path + strlen ("/v2/find?"));
    else if (strcmp (path, "/v2/buy/ready") == 0)
        handle_buy_ready (snapd, method, account);
    else if (strcmp (path, "/v2/buy") == 0)
        handle_buy (snapd, method, account, json_content);
    else if (strcmp (path, "/v2/sections") == 0)
        handle_sections (snapd, method);
    else
        send_error_not_found (snapd, "not found");
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, MockSnapd *snapd)
{
    g_autoptr(GError) error = NULL;

    if (!read_data (snapd, 1024))
        return G_SOURCE_REMOVE;

    while (TRUE) {
        guint8 *body;
        gsize header_length;
        g_autoptr(SoupMessageHeaders) headers = NULL;
        g_autofree gchar *method = NULL;
        g_autofree gchar *path = NULL;
        goffset content_length;

        /* Look for header divider */
        body = (guint8 *) g_strstr_len ((gchar *) snapd->buffer->data, snapd->n_read, "\r\n\r\n");
        if (body == NULL)
            return G_SOURCE_CONTINUE;
        body += 4;
        header_length = body - (guint8 *) snapd->buffer->data;

        /* Parse headers */
        headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_REQUEST);
        if (!soup_headers_parse_request ((gchar *) snapd->buffer->data, header_length, headers,
                                         &method, &path, NULL)) {
            g_socket_close (socket, NULL);
            return G_SOURCE_REMOVE;
        }

        content_length = soup_message_headers_get_content_length (headers);
        if (snapd->n_read < header_length + content_length)
            return G_SOURCE_CONTINUE;

        handle_request (snapd, method, path, headers, body, content_length);

        /* Move remaining data to the start of the buffer */
        g_byte_array_remove_range (snapd->buffer, 0, header_length + content_length);
        snapd->n_read -= header_length + content_length;
    }

    return G_SOURCE_CONTINUE;
}

static void
mock_snapd_finalize (GObject *object)
{
    MockSnapd *snapd = MOCK_SNAPD (object);

    g_clear_object (&snapd->client_socket);
    g_clear_object (&snapd->server_socket);
    if (snapd->read_source != NULL)
        g_source_destroy (snapd->read_source);
    g_clear_pointer (&snapd->read_source, g_source_unref);
    g_clear_pointer (&snapd->buffer, g_byte_array_unref);
    g_clear_pointer (&snapd->read_source, g_source_unref);
    g_list_free_full (snapd->accounts, (GDestroyNotify) mock_account_free);
    g_list_free_full (snapd->snaps, (GDestroyNotify) mock_snap_free);
    snapd->snaps = NULL;
    g_free (snapd->store);
    g_list_free_full (snapd->store_sections, g_free);
    snapd->store_sections = NULL;
    g_list_free_full (snapd->store_snaps, (GDestroyNotify) mock_snap_free);
    snapd->store_snaps = NULL;
    g_list_free_full (snapd->plugs, (GDestroyNotify) mock_plug_free);
    snapd->plugs = NULL;
    g_list_free_full (snapd->slots, (GDestroyNotify) mock_slot_free);
    snapd->slots = NULL;
    g_list_free_full (snapd->changes, (GDestroyNotify) mock_change_free);
    snapd->changes = NULL;
    g_clear_pointer (&snapd->suggested_currency, g_free);
    g_clear_pointer (&snapd->spawn_time, g_free);
    g_clear_pointer (&snapd->ready_time, g_free);  
}

static void
mock_snapd_class_init (MockSnapdClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = mock_snapd_finalize;
}

static void
mock_snapd_init (MockSnapd *snapd)
{
    int fds[2];

    socketpair (AF_UNIX, SOCK_STREAM, 0, fds);
    snapd->client_socket = g_socket_new_from_fd (fds[0], NULL);
    snapd->server_socket = g_socket_new_from_fd (fds[1], NULL);
    snapd->buffer = g_byte_array_new ();
    snapd->read_source = g_socket_create_source (snapd->server_socket, G_IO_IN, NULL);
    g_source_set_callback (snapd->read_source, (GSourceFunc) read_cb, snapd, NULL);
    g_source_attach (snapd->read_source, NULL);  
}
