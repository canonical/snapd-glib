/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>

#include <glib/gstdio.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "mock-snapd.h"

struct _MockSnapd
{
    GObject parent_instance;

    GMutex mutex;
    GCond condition;

    GThread *thread;
    GError **thread_init_error;
    GMainLoop *loop;
    GMainContext *context;

    gchar *dir_path;
    gchar *socket_path;
    gboolean close_on_request;
    gboolean decline_auth;
    GList *accounts;
    GList *users;
    GList *interfaces;
    GList *snaps;
    gchar *build_id;
    gchar *confinement;
    GHashTable *sandbox_features;
    gchar *store;
    gchar *maintenance_kind;
    gchar *maintenance_message;
    gboolean managed;
    gboolean on_classic;
    gchar *refresh_hold;
    gchar *refresh_last;
    gchar *refresh_next;
    gchar *refresh_schedule;
    gchar *refresh_timer;
    GList *store_sections;
    GList *store_snaps;
    GList *established_connections;
    GList *undesired_connections;
    GList *assertions;
    int change_index;
    GList *changes;
    gchar *suggested_currency;
    gchar *spawn_time;
    gchar *ready_time;
    SoupMessageHeaders *last_request_headers;
};

G_DEFINE_TYPE (MockSnapd, mock_snapd, G_TYPE_OBJECT)

struct _MockAccount
{
    int id;
    gchar *email;
    gchar *username;
    gchar *password;
    gchar *otp;
    GStrv ssh_keys;
    gchar *macaroon;
    GStrv discharges;
    gboolean sudoer;
    gboolean known;
    gboolean terms_accepted;
    gboolean has_payment_methods;
    GList *private_snaps;
};

struct _MockAlias
{
    gchar *name;
    gboolean automatic;
    gboolean enabled;
};

struct _MockApp
{
    gchar *name;
    gchar *common_id;
    gchar *daemon;
    gchar *desktop_file;
    gboolean enabled;
    gboolean active;
    GList *aliases;
};

struct _MockChange
{
    gchar *id;
    gchar *kind;
    gchar *summary;
    gchar *spawn_time;
    gchar *ready_time;
    int task_index;
    GList *tasks;
    JsonNode *data;
};

struct _MockChannel
{
    gchar *risk;
    gchar *branch;
    gchar *confinement;
    gchar *epoch;
    gchar *released_at;
    gchar *revision;
    int size;
    gchar *version;
};

struct _MockPrice
{
    gdouble amount;
    gchar *currency;
};

struct _MockMedia
{
    gchar *type;
    gchar *url;
    int width;
    int height;
};

struct _MockInterface
{
    gchar *name;
    gchar *summary;
    gchar *doc_url;
};

struct _MockPlug
{
    MockSnap *snap;
    gchar *name;
    MockInterface *interface;
    // FIXME: Attributes
    gchar *label;
};

struct _MockSlot
{
    MockSnap *snap;
    gchar *name;
    MockInterface *interface;
    // FIXME: Attributes
    gchar *label;
};

struct _MockConnection
{
    MockPlug *plug;
    MockSlot *slot;
    gboolean manual;
    gboolean gadget;
};

struct _MockSnap
{
    GList *apps;
    gchar *base;
    gchar *broken;
    gchar *channel;
    gchar *confinement;
    gchar *contact;
    gchar *description;
    gboolean devmode;
    int download_size;
    gchar *icon;
    gchar *icon_mime_type;
    GBytes *icon_data;
    gchar *id;
    gchar *install_date;
    int installed_size;
    gboolean jailmode;
    gchar *license;
    GList *media;
    gchar *mounted_from;
    gchar *name;
    GList *prices;
    gboolean is_private;
    gchar *publisher_display_name;
    gchar *publisher_id;
    gchar *publisher_username;
    gchar *publisher_validation;
    gchar *revision;
    gchar *status;
    gchar *summary;
    gchar *title;
    gchar *tracking_channel;
    GList *tracks;
    gboolean trymode;
    gchar *type;
    gchar *version;
    GList *store_sections;
    GList *plugs;
    GList *slots_;
    gboolean disabled;
    gboolean dangerous;
    gchar *snap_data;
    gchar *snap_path;
    gchar *error;
    gboolean restart_required;
    gboolean preferred;
    gboolean scope_is_wide;
};

struct _MockTask
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
    MockSnap *snap;
    gchar *snap_name;
    gchar *error;
};

struct _MockTrack
{
    gchar *name;
    GList *channels;
};

static void
mock_alias_free (MockAlias *alias)
{
    g_free (alias->name);
    g_slice_free (MockAlias, alias);
}

static void
mock_app_free (MockApp *app)
{
    g_free (app->name);
    g_free (app->common_id);
    g_free (app->daemon);
    g_free (app->desktop_file);
    g_list_free_full (app->aliases, (GDestroyNotify) mock_alias_free);
    g_slice_free (MockApp, app);
}

static void
mock_channel_free (MockChannel *channel)
{
    g_free (channel->risk);
    g_free (channel->branch);
    g_free (channel->confinement);
    g_free (channel->epoch);
    g_free (channel->released_at);
    g_free (channel->revision);
    g_free (channel->version);
    g_slice_free (MockChannel, channel);
}

static void
mock_track_free (MockTrack *track)
{
    g_free (track->name);
    g_list_free_full (track->channels, (GDestroyNotify) mock_channel_free);
    g_slice_free (MockTrack, track);
}

static void
mock_price_free (MockPrice *price)
{
    g_free (price->currency);
    g_slice_free (MockPrice, price);
}

static void
mock_media_free (MockMedia *media)
{
    g_free (media->type);
    g_free (media->url);
    g_slice_free (MockMedia, media);
}

static void
mock_interface_free (MockInterface *interface)
{
    g_free (interface->name);
    g_free (interface->summary);
    g_free (interface->doc_url);
    g_slice_free (MockInterface, interface);
}

static void
mock_plug_free (MockPlug *plug)
{
    g_free (plug->name);
    g_free (plug->label);
    g_slice_free (MockPlug, plug);
}

static void
mock_slot_free (MockSlot *slot)
{
    g_free (slot->name);
    g_free (slot->label);
    g_slice_free (MockSlot, slot);
}

static void
mock_connection_free (MockConnection *connection)
{
    g_slice_free (MockConnection, connection);
}

static void
mock_snap_free (MockSnap *snap)
{
    g_list_free_full (snap->apps, (GDestroyNotify) mock_app_free);
    g_free (snap->base);
    g_free (snap->broken);
    g_free (snap->channel);
    g_free (snap->confinement);
    g_free (snap->contact);
    g_free (snap->description);
    g_free (snap->icon);
    g_free (snap->icon_mime_type);
    g_bytes_unref (snap->icon_data);
    g_free (snap->id);
    g_free (snap->install_date);
    g_free (snap->license);
    g_list_free_full (snap->media, (GDestroyNotify) mock_media_free);
    g_free (snap->mounted_from);
    g_free (snap->name);
    g_list_free_full (snap->prices, (GDestroyNotify) mock_price_free);
    g_free (snap->publisher_display_name);
    g_free (snap->publisher_id);
    g_free (snap->publisher_username);
    g_free (snap->publisher_validation);
    g_free (snap->revision);
    g_free (snap->status);
    g_free (snap->summary);
    g_free (snap->title);
    g_free (snap->tracking_channel);
    g_list_free_full (snap->tracks, (GDestroyNotify) mock_track_free);
    g_free (snap->type);
    g_free (snap->version);
    g_list_free_full (snap->store_sections, g_free);
    g_list_free_full (snap->plugs, (GDestroyNotify) mock_plug_free);
    g_list_free_full (snap->slots_, (GDestroyNotify) mock_slot_free);
    g_free (snap->snap_data);
    g_free (snap->snap_path);
    g_free (snap->error);
    g_slice_free (MockSnap, snap);
}

MockSnapd *
mock_snapd_new (void)
{
    return g_object_new (MOCK_TYPE_SNAPD, NULL);
}

const gchar *
mock_snapd_get_socket_path (MockSnapd *snapd)
{
    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);
    return snapd->socket_path;
}

void
mock_snapd_set_close_on_request (MockSnapd *snapd, gboolean close_on_request)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    snapd->close_on_request = close_on_request;
}

void
mock_snapd_set_decline_auth (MockSnapd *snapd, gboolean decline_auth)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    snapd->decline_auth = decline_auth;
}

void
mock_snapd_set_maintenance (MockSnapd *snapd, const gchar *kind, const gchar *message)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));
    g_free (snapd->maintenance_kind);
    snapd->maintenance_kind = g_strdup (kind);
    g_free (snapd->maintenance_message);
    snapd->maintenance_message = g_strdup (message);
}

void
mock_snapd_set_build_id (MockSnapd *snapd, const gchar *build_id)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->build_id);
    snapd->build_id = g_strdup (build_id);
}

void
mock_snapd_set_confinement (MockSnapd *snapd, const gchar *confinement)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->confinement);
    snapd->confinement = g_strdup (confinement);
}

void
mock_snapd_add_sandbox_feature (MockSnapd *snapd, const gchar *backend, const gchar *feature)
{
    g_autoptr(GMutexLocker) locker = NULL;
    GPtrArray *backend_features;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    backend_features = g_hash_table_lookup (snapd->sandbox_features, backend);
    if (backend_features == NULL) {
        backend_features = g_ptr_array_new_with_free_func (g_free);
        g_hash_table_insert (snapd->sandbox_features, g_strdup (backend), backend_features);
    }
    g_ptr_array_add (backend_features, g_strdup (feature));
}

void
mock_snapd_set_store (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->store);
    snapd->store = g_strdup (name);
}

void
mock_snapd_set_managed (MockSnapd *snapd, gboolean managed)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    snapd->managed = managed;
}

void
mock_snapd_set_on_classic (MockSnapd *snapd, gboolean on_classic)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    snapd->on_classic = on_classic;
}

void
mock_snapd_set_refresh_hold (MockSnapd *snapd, const gchar *refresh_hold)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_hold);
    snapd->refresh_hold = g_strdup (refresh_hold);
}

void
mock_snapd_set_refresh_last (MockSnapd *snapd, const gchar *refresh_last)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_last);
    snapd->refresh_last = g_strdup (refresh_last);
}

void
mock_snapd_set_refresh_next (MockSnapd *snapd, const gchar *refresh_next)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_next);
    snapd->refresh_next = g_strdup (refresh_next);
}

void
mock_snapd_set_refresh_schedule (MockSnapd *snapd, const gchar *schedule)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_schedule);
    snapd->refresh_schedule = g_strdup (schedule);
}

void
mock_snapd_set_refresh_timer (MockSnapd *snapd, const gchar *timer)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->refresh_timer);
    snapd->refresh_timer = g_strdup (timer);
}

void
mock_snapd_set_suggested_currency (MockSnapd *snapd, const gchar *currency)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->suggested_currency);
    snapd->suggested_currency = g_strdup (currency);
}

void
mock_snapd_set_spawn_time (MockSnapd *snapd, const gchar *spawn_time)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->spawn_time);
    snapd->spawn_time = g_strdup (spawn_time);
}

void
mock_snapd_set_ready_time (MockSnapd *snapd, const gchar *ready_time)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    g_free (snapd->ready_time);
    snapd->ready_time = g_strdup (ready_time);
}

static MockAccount *
add_account (MockSnapd *snapd, const gchar *email, const gchar *username, const gchar *password)
{
    MockAccount *account;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    account = g_slice_new0 (MockAccount);
    account->email = g_strdup (email);
    account->username = g_strdup (username);
    account->password = g_strdup (password);
    account->macaroon = g_strdup_printf ("MACAROON-%s", username);
    account->discharges = g_malloc (sizeof (gchar *) * 2);
    account->discharges[0] = g_strdup_printf ("DISCHARGE-%s", username);
    account->discharges[1] = NULL;
    snapd->accounts = g_list_append (snapd->accounts, account);
    account->id = g_list_length (snapd->accounts);

    return account;
}

MockAccount *
mock_snapd_add_account (MockSnapd *snapd, const gchar *email, const gchar *username, const gchar *password)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    return add_account (snapd, email, username, password);
}

void
mock_account_set_terms_accepted (MockAccount *account, gboolean terms_accepted)
{
    account->terms_accepted = terms_accepted;
}

void
mock_account_set_has_payment_methods (MockAccount *account, gboolean has_payment_methods)
{
    account->has_payment_methods = has_payment_methods;
}

const gchar *
mock_account_get_macaroon (MockAccount *account)
{
    return account->macaroon;
}

GStrv
mock_account_get_discharges (MockAccount *account)
{
    return account->discharges;
}

gboolean
mock_account_get_sudoer (MockAccount *account)
{
    return account->sudoer;
}

gboolean
mock_account_get_known (MockAccount *account)
{
    return account->known;
}

void
mock_account_set_otp (MockAccount *account, const gchar *otp)
{
    g_clear_pointer (&account->otp, g_free);
    account->otp = g_strdup (otp);
}

void
mock_account_set_ssh_keys (MockAccount *account, GStrv ssh_keys)
{
    g_clear_pointer (&account->ssh_keys, g_strfreev);
    account->ssh_keys = g_strdupv (ssh_keys);
}

static MockAccount *
find_account_by_username (MockSnapd *snapd, const gchar *username)
{
    GList *link;

    for (link = snapd->accounts; link; link = link->next) {
        MockAccount *user = link->data;
        if (strcmp (user->username, username) == 0)
            return user;
    }

    return NULL;
}

MockAccount *
mock_snapd_find_account_by_username (MockSnapd *snapd, const gchar *username)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    return find_account_by_username (snapd, username);
}

static MockAccount *
find_account_by_email (MockSnapd *snapd, const gchar *email)
{
    GList *link;

    for (link = snapd->accounts; link; link = link->next) {
        MockAccount *user = link->data;
        if (strcmp (user->email, email) == 0)
            return user;
    }

    return NULL;
}

MockAccount *
mock_snapd_find_account_by_email (MockSnapd *snapd, const gchar *email)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    return find_account_by_email (snapd, email);
}

static MockChange *
add_change (MockSnapd *snapd)
{
    MockChange *change;

    change = g_slice_new0 (MockChange);
    snapd->change_index++;
    change->id = g_strdup_printf ("%d", snapd->change_index);
    change->kind = g_strdup ("KIND");
    change->summary = g_strdup ("SUMMARY");
    change->task_index = snapd->change_index * 100;
    snapd->changes = g_list_append (snapd->changes, change);

    return change;
}

MockChange *
mock_snapd_add_change (MockSnapd *snapd)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    return add_change (snapd);
}

MockTask *
mock_change_add_task (MockChange *change, const gchar *kind)
{
    MockTask *task;

    task = g_slice_new0 (MockTask);
    task->id = g_strdup_printf ("%d", change->task_index);
    change->task_index++;
    task->kind = g_strdup (kind);
    task->summary = g_strdup ("SUMMARY");
    task->status = g_strdup ("Do");
    task->progress_label = g_strdup ("LABEL");
    task->progress_done = 0;
    task->progress_total = 1;
    change->tasks = g_list_append (change->tasks, task);

    return task;
}

void
mock_task_set_snap_name (MockTask *task, const gchar *snap_name)
{
    task->snap_name = g_strdup (snap_name);
}

void
mock_task_set_status (MockTask *task, const gchar *status)
{
    g_free (task->status);
    task->status = g_strdup (status);
}

void
mock_task_set_progress (MockTask *task, int done, int total)
{
    task->progress_done = done;
    task->progress_total = total;
}

void
mock_task_set_spawn_time (MockTask *task, const gchar *spawn_time)
{
    g_free (task->spawn_time);
    task->spawn_time = g_strdup (spawn_time);
}

void
mock_task_set_ready_time (MockTask *task, const gchar *ready_time)
{
    g_free (task->ready_time);
    task->ready_time = g_strdup (ready_time);
}

void
mock_change_set_spawn_time (MockChange *change, const gchar *spawn_time)
{
    g_free (change->spawn_time);
    change->spawn_time = g_strdup (spawn_time);
}

void
mock_change_set_ready_time (MockChange *change, const gchar *ready_time)
{
    g_free (change->ready_time);
    change->ready_time = g_strdup (ready_time);
}

static MockSnap *
mock_snap_new (const gchar *name)
{
    MockSnap *snap;

    snap = g_slice_new0 (MockSnap);
    snap->confinement = g_strdup ("strict");
    snap->publisher_display_name = g_strdup ("PUBLISHER-DISPLAY-NAME");
    snap->publisher_id = g_strdup ("PUBLISHER-ID");
    snap->publisher_username = g_strdup ("PUBLISHER-USERNAME");
    snap->icon = g_strdup ("ICON");
    snap->id = g_strdup ("ID");
    snap->name = g_strdup (name);
    snap->revision = g_strdup ("REVISION");
    snap->status = g_strdup ("active");
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
    snap->is_private = TRUE;
    account->private_snaps = g_list_append (account->private_snaps, snap);

    return snap;
}

static gboolean
discharges_match (GStrv discharges0, GStrv discharges1)
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
find_account_by_macaroon (MockSnapd *snapd, const gchar *macaroon, GStrv discharges)
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
    g_free (account->email);
    g_free (account->username);
    g_free (account->password);
    g_free (account->otp);
    g_strfreev (account->ssh_keys);
    g_free (account->macaroon);
    g_strfreev (account->discharges);
    g_list_free_full (account->private_snaps, (GDestroyNotify) mock_snap_free);
    g_slice_free (MockAccount, account);
}

static MockInterface *
mock_interface_new (const gchar *name)
{
    MockInterface *interface;

    interface = g_slice_new0 (MockInterface);
    interface->name = g_strdup (name);
    interface->summary = g_strdup ("SUMMARY");
    interface->doc_url = g_strdup ("URL");

    return interface;
}

MockInterface *
mock_snapd_add_interface (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;
    MockInterface *interface;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    interface = mock_interface_new (name);
    snapd->interfaces = g_list_append (snapd->interfaces, interface);

    return interface;
}

void
mock_interface_set_summary (MockInterface *interface, const gchar *summary)
{
    g_free (interface->summary);
    interface->summary = g_strdup (summary);
}

void
mock_interface_set_doc_url (MockInterface *interface, const gchar *url)
{
    g_free (interface->doc_url);
    interface->doc_url = g_strdup (url);
}

MockSnap *
mock_snapd_add_snap (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;
    MockSnap *snap;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    snap = mock_snap_new (name);
    snapd->snaps = g_list_append (snapd->snaps, snap);

    return snap;
}

static MockSnap *
find_snap (MockSnapd *snapd, const gchar *name)
{
    GList *link;

    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0)
            return snap;
    }

    return NULL;
}

MockSnap *
mock_snapd_find_snap (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);
    return find_snap (snapd, name);
}

void
mock_snapd_add_store_section (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    locker = g_mutex_locker_new (&snapd->mutex);
    snapd->store_sections = g_list_append (snapd->store_sections, g_strdup (name));
}

MockSnap *
mock_snapd_add_store_snap (MockSnapd *snapd, const gchar *name)
{
    g_autoptr(GMutexLocker) locker = NULL;
    MockSnap *snap;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    snap = mock_snap_new (name);
    snap->download_size = 65535;
    snapd->store_snaps = g_list_append (snapd->store_snaps, snap);

    mock_snap_add_track (snap, "latest");

    return snap;
}

static MockSnap *
find_store_snap_by_name (MockSnapd *snapd, const gchar *name, const gchar *channel,  const gchar *revision)
{
    GList *link;

    for (link = snapd->store_snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        if (strcmp (snap->name, name) == 0 &&
            (channel == NULL || g_strcmp0 (snap->channel, channel) == 0) &&
            (revision == NULL || g_strcmp0 (snap->revision, revision) == 0))
            return snap;
    }

    return NULL;
}

static MockSnap *
find_store_snap_by_id (MockSnapd *snapd, const gchar *id)
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

MockApp *
mock_snap_find_app (MockSnap *snap, const gchar *name)
{
    GList *link;

    for (link = snap->apps; link; link = link->next) {
        MockApp *app = link->data;
        if (strcmp (app->name, name) == 0)
            return app;
    }

    return NULL;
}

void
mock_app_set_active (MockApp *app, gboolean active)
{
    app->active = active;
}

void
mock_app_set_enabled (MockApp *app, gboolean enabled)
{
    app->enabled = enabled;
}

void
mock_app_set_common_id (MockApp *app, const gchar *id)
{
    g_free (app->common_id);
    app->common_id = g_strdup (id);
}

void
mock_app_set_daemon (MockApp *app, const gchar *daemon)
{
    g_free (app->daemon);
    app->daemon = g_strdup (daemon);
}

void
mock_app_set_desktop_file (MockApp *app, const gchar *desktop_file)
{
    g_free (app->desktop_file);
    app->desktop_file = g_strdup (desktop_file);
}

static void
add_alias (MockApp *app, const gchar *name, gboolean automatic, gboolean enabled)
{
    MockAlias *a;

    a = g_slice_new (MockAlias);
    a->name = g_strdup (name);
    a->automatic = automatic;
    a->enabled = enabled;
    app->aliases = g_list_append (app->aliases, a);
}

void
mock_app_add_auto_alias (MockApp *app, const gchar *name)
{
    add_alias (app, name, TRUE, TRUE);
}

void
mock_app_add_manual_alias (MockApp *app, const gchar *name, gboolean enabled)
{
    MockAlias *alias = mock_app_find_alias (app, name);
    if (alias != NULL) {
        alias->enabled = enabled;
        return;
    }

    add_alias (app, name, FALSE, enabled);
}

MockAlias *
mock_app_find_alias (MockApp *app, const gchar *name)
{
    GList *link;

    for (link = app->aliases; link; link = link->next) {
        MockAlias *alias = link->data;
        if (strcmp (alias->name, name) == 0)
            return alias;
    }

    return NULL;
}

void
mock_snap_set_base (MockSnap *snap, const gchar *base)
{
    g_free (snap->base);
    snap->base = g_strdup (base);
}

void
mock_snap_set_broken (MockSnap *snap, const gchar *broken)
{
    g_free (snap->broken);
    snap->broken = g_strdup (broken);
}

void
mock_snap_set_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->channel);
    snap->channel = g_strdup (channel);
}

const gchar *
mock_snap_get_channel (MockSnap *snap)
{
    return snap->channel;
}

MockTrack *
mock_snap_add_track (MockSnap *snap, const gchar *name)
{
    GList *link;
    MockTrack *track;

    for (link = snap->tracks; link; link = link->next) {
        track = link->data;
        if (g_strcmp0 (track->name, name) == 0)
            return track;
    }

    track = g_slice_new0 (MockTrack);
    track->name = g_strdup (name);
    snap->tracks = g_list_append (snap->tracks, track);

    return track;
}

MockChannel *
mock_track_add_channel (MockTrack *track, const gchar *risk, const gchar *branch)
{
    MockChannel *channel;

    channel = g_slice_new0 (MockChannel);
    channel->risk = g_strdup (risk);
    channel->branch = g_strdup (branch);
    channel->confinement = g_strdup ("strict");
    channel->epoch = g_strdup ("0");
    channel->revision = g_strdup ("REVISION");
    channel->size = 65535;
    channel->version = g_strdup ("VERSION");
    track->channels = g_list_append (track->channels, channel);

    return channel;
}

void
mock_channel_set_branch (MockChannel *channel, const gchar *branch)
{
    g_free (channel->branch);
    channel->branch = g_strdup (branch);
}

void
mock_channel_set_confinement (MockChannel *channel, const gchar *confinement)
{
    g_free (channel->confinement);
    channel->confinement = g_strdup (confinement);
}

void
mock_channel_set_epoch (MockChannel *channel, const gchar *epoch)
{
    g_free (channel->epoch);
    channel->epoch = g_strdup (epoch);
}

void
mock_channel_set_released_at (MockChannel *channel, const gchar *released_at)
{
    g_free (channel->released_at);
    channel->released_at = g_strdup (released_at);
}

void
mock_channel_set_revision (MockChannel *channel, const gchar *revision)
{
    g_free (channel->revision);
    channel->revision = g_strdup (revision);
}

const gchar *
mock_snap_get_revision (MockSnap *snap)
{
    return snap->revision;
}

void
mock_channel_set_size (MockChannel *channel, int size)
{
    channel->size = size;
}

void
mock_channel_set_version (MockChannel *channel, const gchar *version)
{
    g_free (channel->version);
    channel->version = g_strdup (version);
}

void
mock_snap_set_confinement (MockSnap *snap, const gchar *confinement)
{
    g_free (snap->confinement);
    snap->confinement = g_strdup (confinement);
}

const gchar *
mock_snap_get_confinement (MockSnap *snap)
{
    return snap->confinement;
}

void
mock_snap_set_contact (MockSnap *snap, const gchar *contact)
{
    g_free (snap->contact);
    snap->contact = g_strdup (contact);
}

gboolean
mock_snap_get_dangerous (MockSnap *snap)
{
    return snap->dangerous;
}

const gchar *
mock_snap_get_data (MockSnap *snap)
{
    return snap->snap_data;
}

void
mock_snap_set_description (MockSnap *snap, const gchar *description)
{
    g_free (snap->description);
    snap->description = g_strdup (description);
}

void
mock_snap_set_devmode (MockSnap *snap, gboolean devmode)
{
    snap->devmode = devmode;
}

gboolean
mock_snap_get_devmode (MockSnap *snap)
{
    return snap->devmode;
}

void
mock_snap_set_disabled (MockSnap *snap, gboolean disabled)
{
    snap->disabled = disabled;
}

gboolean
mock_snap_get_disabled (MockSnap *snap)
{
    return snap->disabled;
}

void
mock_snap_set_download_size (MockSnap *snap, int download_size)
{
    snap->download_size = download_size;
}

void
mock_snap_set_error (MockSnap *snap, const gchar *error)
{
    g_free (snap->error);
    snap->error = g_strdup (error);
}

void
mock_snap_set_icon (MockSnap *snap, const gchar *icon)
{
    g_free (snap->icon);
    snap->icon = g_strdup (icon);
}

void
mock_snap_set_icon_data (MockSnap *snap, const gchar *mime_type, GBytes *data)
{
    g_free (snap->icon_mime_type);
    snap->icon_mime_type = g_strdup (mime_type);
    g_bytes_unref (snap->icon_data);
    snap->icon_data = g_bytes_ref (data);
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

void
mock_snap_set_installed_size (MockSnap *snap, int installed_size)
{
    snap->installed_size = installed_size;
}

void
mock_snap_set_jailmode (MockSnap *snap, gboolean jailmode)
{
    snap->jailmode = jailmode;
}

gboolean
mock_snap_get_jailmode (MockSnap *snap)
{
    return snap->jailmode;
}

void
mock_snap_set_license (MockSnap *snap, const gchar *license)
{
    g_free (snap->license);
    snap->license = g_strdup (license);
}

void
mock_snap_set_mounted_from (MockSnap *snap, const gchar *mounted_from)
{
    g_free (snap->mounted_from);
    snap->mounted_from = g_strdup (mounted_from);
}

const gchar *
mock_snap_get_path (MockSnap *snap)
{
    return snap->snap_path;
}

gboolean
mock_snap_get_preferred (MockSnap *snap)
{
    return snap->preferred;
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
mock_snap_set_publisher_display_name (MockSnap *snap, const gchar *display_name)
{
    g_free (snap->publisher_display_name);
    snap->publisher_display_name = g_strdup (display_name);
}

void
mock_snap_set_publisher_id (MockSnap *snap, const gchar *id)
{
    g_free (snap->publisher_id);
    snap->publisher_id = g_strdup (id);
}

void
mock_snap_set_publisher_username (MockSnap *snap, const gchar *username)
{
    g_free (snap->publisher_username);
    snap->publisher_username = g_strdup (username);
}

void
mock_snap_set_publisher_validation (MockSnap *snap, const gchar *validation)
{
    g_free (snap->publisher_validation);
    snap->publisher_validation = g_strdup (validation);
}

void
mock_snap_set_restart_required (MockSnap *snap, gboolean restart_required)
{
    snap->restart_required = restart_required;
}

void
mock_snap_set_revision (MockSnap *snap, const gchar *revision)
{
    g_free (snap->revision);
    snap->revision = g_strdup (revision);
}

void
mock_snap_set_scope_is_wide (MockSnap *snap, gboolean scope_is_wide)
{
    snap->scope_is_wide = scope_is_wide;
}

MockMedia *
mock_snap_add_media (MockSnap *snap, const gchar *type, const gchar *url, int width, int height)
{
    MockMedia *media;

    media = g_slice_new0 (MockMedia);
    media->type = g_strdup (type);
    media->url = g_strdup (url);
    media->width = width;
    media->height = height;
    snap->media = g_list_append (snap->media, media);

    return media;
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
mock_snap_set_title (MockSnap *snap, const gchar *title)
{
    g_free (snap->title);
    snap->title = g_strdup (title);
}

void
mock_snap_set_tracking_channel (MockSnap *snap, const gchar *channel)
{
    g_free (snap->tracking_channel);
    snap->tracking_channel = g_strdup (channel);
}

const gchar *
mock_snap_get_tracking_channel (MockSnap *snap)
{
    return snap->tracking_channel;
}

void
mock_snap_set_trymode (MockSnap *snap, gboolean trymode)
{
    snap->trymode = trymode;
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
mock_snap_add_plug (MockSnap *snap, MockInterface *interface, const gchar *name)
{
    MockPlug *plug;

    plug = g_slice_new0 (MockPlug);
    plug->snap = snap;
    plug->name = g_strdup (name);
    plug->interface = interface;
    plug->label = g_strdup ("LABEL");
    snap->plugs = g_list_append (snap->plugs, plug);

    return plug;
}

MockPlug *
mock_snap_find_plug (MockSnap *snap, const gchar *name)
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
mock_snap_add_slot (MockSnap *snap, MockInterface *interface, const gchar *name)
{
    MockSlot *slot;

    slot = g_slice_new0 (MockSlot);
    slot->snap = snap;
    slot->name = g_strdup (name);
    slot->interface = interface;
    slot->label = g_strdup ("LABEL");
    snap->slots_ = g_list_append (snap->slots_, slot);

    return slot;
}

MockSlot *
mock_snap_find_slot (MockSnap *snap, const gchar *name)
{
    GList *link;

    for (link = snap->slots_; link; link = link->next) {
        MockSlot *slot = link->data;
        if (strcmp (slot->name, name) == 0)
            return slot;
    }

    return NULL;
}

void
mock_snapd_connect (MockSnapd *snapd, MockPlug *plug, MockSlot *slot, gboolean manual, gboolean gadget)
{
    g_return_if_fail (plug != NULL);

    if (slot != NULL) {
        g_autoptr(GList) old_established_connections = NULL;
        g_autoptr(GList) old_undesired_connections = NULL;
        GList *link;
        MockConnection *connection;

        /* Remove existing connections */
        old_established_connections = g_steal_pointer (&snapd->established_connections);
        for (link = old_established_connections; link; link = link->next) {
            connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->established_connections = g_list_append (snapd->established_connections, connection);
        }
        old_undesired_connections = g_steal_pointer (&snapd->undesired_connections);
        for (link = old_undesired_connections; link; link = link->next) {
            connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }

        connection = g_slice_new0 (MockConnection);
        connection->plug = plug;
        connection->slot = slot;
        connection->manual = manual;
        connection->gadget = gadget;
        snapd->established_connections = g_list_append (snapd->established_connections, connection);
    }
    else {
        g_autoptr(GList) old_established_connections = NULL;
        g_autoptr(GList) old_undesired_connections = NULL;
        GList *link;
        MockConnection *connection;
        MockSlot *autoconnected_slot = NULL;

        /* Remove existing connections */
        old_established_connections = g_steal_pointer (&snapd->established_connections);
        for (link = old_established_connections; link; link = link->next) {
            connection = link->data;
            if (connection->plug == plug) {
                if (!connection->manual)
                    autoconnected_slot = connection->slot;
                continue;
            }
            snapd->established_connections = g_list_append (snapd->established_connections, connection);
        }
        old_undesired_connections = g_steal_pointer (&snapd->undesired_connections);
        for (link = old_undesired_connections; link; link = link->next) {
            connection = link->data;
            if (connection->plug == plug)
                continue;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }

        /* Mark as undesired if overriding auto connection */
        if (autoconnected_slot != NULL) {
            connection = g_slice_new0 (MockConnection);
            connection->plug = plug;
            connection->slot = autoconnected_slot;
            connection->manual = manual;
            connection->gadget = gadget;
            snapd->undesired_connections = g_list_append (snapd->undesired_connections, connection);
        }
    }
}

MockSlot *
mock_snapd_find_plug_connection (MockSnapd *snapd, MockPlug *plug)
{
    GList *link;

    for (link = snapd->established_connections; link; link = link->next) {
        MockConnection *connection = link->data;
        if (connection->plug == plug)
            return connection->slot;
    }

    return NULL;
}

static void
add_assertion (MockSnapd *snapd, const gchar *assertion)
{
    snapd->assertions = g_list_append (snapd->assertions, g_strdup (assertion));
}

void
mock_snapd_add_assertion (MockSnapd *snapd, const gchar *assertion)
{
    g_autoptr(GMutexLocker) locker = NULL;

    locker = g_mutex_locker_new (&snapd->mutex);
    return add_assertion (snapd, assertion);
}

GList *
mock_snapd_get_assertions (MockSnapd *snapd)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);
    return snapd->assertions;
}

const gchar *
mock_snapd_get_last_user_agent (MockSnapd *snapd)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "User-Agent");
}

const gchar *
mock_snapd_get_last_accept_language (MockSnapd *snapd)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "Accept-Language");
}

const gchar *
mock_snapd_get_last_allow_interaction (MockSnapd *snapd)
{
    g_autoptr(GMutexLocker) locker = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), NULL);

    locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->last_request_headers == NULL)
        return NULL;

    return soup_message_headers_get_one (snapd->last_request_headers, "X-Allow-Interaction");
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
    if (task->snap != NULL)
        mock_snap_free (task->snap);
    g_free (task->snap_name);
    g_free (task->error);
    g_slice_free (MockTask, task);
}

static void
mock_change_free (MockChange *change)
{
    g_free (change->id);
    g_free (change->kind);
    g_free (change->summary);
    g_free (change->spawn_time);
    g_free (change->ready_time);
    g_list_free_full (change->tasks, (GDestroyNotify) mock_task_free);
    if (change->data != NULL)
        json_node_unref (change->data);
    g_slice_free (MockChange, change);
}

static void
send_response (SoupMessage *message, guint status_code, const gchar *content_type, const guint8 *content, gsize content_length)
{
    soup_message_set_status (message, status_code);
    soup_message_headers_set_content_type (message->response_headers,
                                           content_type, NULL);
    soup_message_headers_set_content_length (message->response_headers,
                                             content_length);

    soup_message_body_append (message->response_body, SOUP_MEMORY_COPY,
                              content, content_length);
}

static void
send_json_response (SoupMessage *message, guint status_code, JsonNode *node)
{
    g_autoptr(JsonGenerator) generator = NULL;
    g_autofree gchar *data;
    gsize data_length;

    generator = json_generator_new ();
    json_generator_set_root (generator, node);
    data = json_generator_to_data (generator, &data_length);

    send_response (message, status_code, "application/json", (guint8*) data, data_length);
}

static JsonNode *
make_response (MockSnapd *snapd, const gchar *type, guint status_code, JsonNode *result, const gchar *change_id, const gchar *suggested_currency) // FIXME: sources
{
    g_autoptr(JsonBuilder) builder = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "type");
    json_builder_add_string_value (builder, type);
    json_builder_set_member_name (builder, "status-code");
    json_builder_add_int_value (builder, status_code);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, soup_status_get_phrase (status_code));
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
    if (snapd->maintenance_kind != NULL) {
        json_builder_set_member_name (builder, "maintenance");
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, snapd->maintenance_kind);
        json_builder_set_member_name (builder, "message");
        json_builder_add_string_value (builder, snapd->maintenance_message);
        json_builder_end_object (builder);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static void
send_sync_response (MockSnapd *snapd, SoupMessage *message, guint status_code, JsonNode *result, const gchar *suggested_currency)
{
    g_autoptr(JsonNode) response = NULL;

    response = make_response (snapd, "sync", status_code, result, NULL, suggested_currency);
    send_json_response (message, status_code, response);
}

static void
send_async_response (MockSnapd *snapd, SoupMessage *message, guint status_code, const gchar *change_id)
{
    g_autoptr(JsonNode) response = NULL;

    response = make_response (snapd, "async", status_code, NULL, change_id, NULL);
    send_json_response (message, status_code, response);
}

static void
send_error_response (MockSnapd *snapd, SoupMessage *message, guint status_code, const gchar *error_message, const gchar *kind)
{
    g_autoptr(JsonBuilder) builder = NULL;
    g_autoptr(JsonNode) response = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "message");
    json_builder_add_string_value (builder, error_message);
    if (kind != NULL) {
        json_builder_set_member_name (builder, "kind");
        json_builder_add_string_value (builder, kind);
    }
    json_builder_end_object (builder);

    response = make_response (snapd, "error", status_code, json_builder_get_root (builder), NULL, NULL);
    send_json_response (message, status_code, response);
}

static void
send_error_bad_request (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 400, error_message, kind);
}

static void
send_error_unauthorized (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 401, error_message, kind);
}

static void
send_error_forbidden (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 403, error_message, kind);
}

static void
send_error_not_found (MockSnapd *snapd, SoupMessage *message, const gchar *error_message, const gchar *kind)
{
    send_error_response (snapd, message, 404, error_message, kind);
}

static void
send_error_method_not_allowed (MockSnapd *snapd, SoupMessage *message, const gchar *error_message)
{
    send_error_response (snapd, message, 405, error_message, NULL);
}

static void
handle_system_info (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonBuilder) builder = NULL;
    GHashTableIter iter;
    gpointer key, value;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snapd->build_id) {
        json_builder_set_member_name (builder, "build-id");
        json_builder_add_string_value (builder, snapd->build_id);
    }
    if (snapd->confinement) {
        json_builder_set_member_name (builder, "confinement");
        json_builder_add_string_value (builder, snapd->confinement);
    }
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
    json_builder_set_member_name (builder, "managed");
    json_builder_add_boolean_value (builder, snapd->managed);
    json_builder_set_member_name (builder, "on-classic");
    json_builder_add_boolean_value (builder, snapd->on_classic);
    json_builder_set_member_name (builder, "kernel-version");
    json_builder_add_string_value (builder, "KERNEL-VERSION");
    json_builder_set_member_name (builder, "locations");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap-mount-dir");
    json_builder_add_string_value (builder, "/snap");
    json_builder_set_member_name (builder, "snap-bin-dir");
    json_builder_add_string_value (builder, "/snap/bin");
    json_builder_end_object (builder);
    if (g_hash_table_size (snapd->sandbox_features) > 0) {
        json_builder_set_member_name (builder, "sandbox-features");
        json_builder_begin_object (builder);
        g_hash_table_iter_init (&iter, snapd->sandbox_features);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
            const gchar *backend = key;
            GPtrArray *backend_features = value;
            guint i;

            json_builder_set_member_name (builder, backend);
            json_builder_begin_array (builder);
            for (i = 0; i < backend_features->len; i++)
                json_builder_add_string_value (builder, g_ptr_array_index (backend_features, i));
            json_builder_end_array (builder);
        }
        json_builder_end_object (builder);
    }
    if (snapd->store) {
        json_builder_set_member_name (builder, "store");
        json_builder_add_string_value (builder, snapd->store);
    }
    json_builder_set_member_name (builder, "refresh");
    json_builder_begin_object (builder);
    if (snapd->refresh_timer) {
        json_builder_set_member_name (builder, "timer");
        json_builder_add_string_value (builder, snapd->refresh_timer);
    }
    else if (snapd->refresh_schedule) {
        json_builder_set_member_name (builder, "schedule");
        json_builder_add_string_value (builder, snapd->refresh_schedule);
    }
    if (snapd->refresh_last) {
        json_builder_set_member_name (builder, "last");
        json_builder_add_string_value (builder, snapd->refresh_last);
    }
    if (snapd->refresh_next) {
        json_builder_set_member_name (builder, "next");
        json_builder_add_string_value (builder, snapd->refresh_next);
    }
    if (snapd->refresh_hold) {
        json_builder_set_member_name (builder, "hold");
        json_builder_add_string_value (builder, snapd->refresh_hold);
    }
    json_builder_end_object (builder);
    json_builder_end_object (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static gboolean
parse_macaroon (const gchar *authorization, GStrv macaroon, GStrv *discharges)
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
    *discharges = (GStrv) d->pdata;
    g_ptr_array_free (d, FALSE);
    d = NULL;
    return TRUE;
}

static MockAccount *
get_account (MockSnapd *snapd, SoupMessage *message)
{
    const gchar *authorization;
    g_autofree gchar *macaroon = NULL;
    g_auto(GStrv) discharges = NULL;

    authorization = soup_message_headers_get_one (message->request_headers, "Authorization");
    if (authorization == NULL)
        return NULL;

    if (!parse_macaroon (authorization, &macaroon, &discharges))
        return NULL;

    return find_account_by_macaroon (snapd, macaroon, discharges);
}

static JsonNode *
get_json (SoupMessage *message)
{
    const gchar *content_type;
    g_autoptr(JsonParser) parser = NULL;
    g_autoptr(GError) error = NULL;

    content_type = soup_message_headers_get_content_type (message->request_headers, NULL);
    if (content_type == NULL)
        return NULL;

    parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, message->request_body->data, message->request_body->length, &error)) {
        g_warning ("Failed to parse request: %s", error->message);
        return NULL;
    }

    return json_node_ref (json_parser_get_root (parser));
}

static void
add_user_response (JsonBuilder *builder, MockAccount *account, gboolean send_macaroon, gboolean send_ssh_keys, gboolean send_email)
{
    int i;

    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_int_value (builder, account->id);
    if (send_email && account->email != NULL) {
        json_builder_set_member_name (builder, "email");
        json_builder_add_string_value (builder, account->email);
    }
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, account->username);
    if (send_ssh_keys && account->ssh_keys != NULL) {
        json_builder_set_member_name (builder, "ssh-keys");
        json_builder_begin_array (builder);
        for (i = 0; account->ssh_keys[i] != NULL; i++)
            json_builder_add_string_value (builder, account->ssh_keys[i]);
        json_builder_end_array (builder);
    }
    if (send_macaroon && account->macaroon != NULL) {
        json_builder_set_member_name (builder, "macaroon");
        json_builder_add_string_value (builder, account->macaroon);
        json_builder_set_member_name (builder, "discharges");
        json_builder_begin_array (builder);
        for (i = 0; account->discharges[i] != NULL; i++)
            json_builder_add_string_value (builder, account->discharges[i]);
        json_builder_end_array (builder);
    }
    json_builder_end_object (builder);
}

static void
handle_login (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonNode) request = NULL;
    JsonObject *o;
    const gchar *email, *password, *otp = NULL;
    MockAccount *account;
    g_autoptr(JsonBuilder) builder = NULL;

    if (strcmp (message->method, "POST") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    request = get_json (message);
    if (request == NULL) {
        send_error_bad_request (snapd, message, "unknown content type", NULL);
        return;
    }

    o = json_node_get_object (request);
    email = json_object_get_string_member (o, "email");
    if (email == NULL)
        email = json_object_get_string_member (o, "username");
    password = json_object_get_string_member (o, "password");
    if (json_object_has_member (o, "otp"))
        otp = json_object_get_string_member (o, "otp");

    if (strstr (email, "@") == NULL) {
        send_error_bad_request (snapd, message, "please use a valid email address.", "invalid-auth-data");
        return;
    }

    account = find_account_by_email (snapd, email);
    if (account == NULL) {
        send_error_unauthorized (snapd, message, "cannot authenticate to snap store: Provided email/password is not correct.", "login-required");
        return;
    }

    if (strcmp (account->password, password) != 0) {
        send_error_unauthorized (snapd, message, "cannot authenticate to snap store: Provided email/password is not correct.", "login-required");
        return;
    }

    if (account->otp != NULL) {
        if (otp == NULL) {
            send_error_unauthorized (snapd, message, "two factor authentication required", "two-factor-required");
            return;
        }
        if (strcmp (account->otp, otp) != 0) {
            send_error_unauthorized (snapd, message, "two factor authentication failed", "two-factor-failed");
            return;
        }
    }

    builder = json_builder_new ();
    add_user_response (builder, account, TRUE, FALSE, TRUE);
    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static JsonNode *
make_app_node (MockApp *app, const gchar *snap_name)
{
    g_autoptr(JsonBuilder) builder = NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snap_name != NULL) {
        json_builder_set_member_name (builder, "snap");
        json_builder_add_string_value (builder, snap_name);
    }
    if (app->name != NULL) {
        json_builder_set_member_name (builder, "name");
        json_builder_add_string_value (builder, app->name);
    }
    if (app->common_id != NULL) {
        json_builder_set_member_name (builder, "common-id");
        json_builder_add_string_value (builder, app->common_id);
    }
    if (app->daemon != NULL) {
        json_builder_set_member_name (builder, "daemon");
        json_builder_add_string_value (builder, app->daemon);
    }
    if (app->desktop_file != NULL) {
        json_builder_set_member_name (builder, "desktop-file");
        json_builder_add_string_value (builder, app->desktop_file);
    }
    if (app->active) {
        json_builder_set_member_name (builder, "active");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if (app->enabled) {
        json_builder_set_member_name (builder, "enabled");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static JsonNode *
make_snap_node (MockSnap *snap)
{
    g_autoptr(JsonBuilder) builder = NULL;
    g_autofree gchar *resource = NULL;
    int screenshot_count = 0;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    if (snap->apps != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "apps");
        json_builder_begin_array (builder);
        for (link = snap->apps; link; link = link->next) {
            MockApp *app = link->data;
            json_builder_add_value (builder, make_app_node (app, NULL));
        }
        json_builder_end_array (builder);
    }
    if (snap->broken) {
        json_builder_set_member_name (builder, "broken");
        json_builder_add_string_value (builder, snap->broken);
    }
    if (snap->base) {
        json_builder_set_member_name (builder, "base");
        json_builder_add_string_value (builder, snap->base);
    }
    if (snap->channel) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, snap->channel);
    }
    if (snap->tracks != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "channels");
        json_builder_begin_object (builder);
        for (link = snap->tracks; link; link = link->next) {
            MockTrack *track = link->data;
            GList *channel_link;

            for (channel_link = track->channels; channel_link; channel_link = channel_link->next) {
                MockChannel *channel = channel_link->data;
                g_autofree gchar *key = NULL;
                g_autofree gchar *name = NULL;

                if (channel->branch != NULL)
                    key = g_strdup_printf ("%s/%s/%s", track->name, channel->risk, channel->branch);
                else
                    key = g_strdup_printf ("%s/%s", track->name, channel->risk);

                if (g_strcmp0 (track->name, "latest") == 0) {
                    if (channel->branch != NULL)
                        name = g_strdup_printf ("%s/%s", channel->risk, channel->branch);
                    else
                        name = g_strdup (channel->risk);
                }
                else
                    name = g_strdup (key);

                json_builder_set_member_name (builder, key);
                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "revision");
                json_builder_add_string_value (builder, channel->revision);
                json_builder_set_member_name (builder, "confinement");
                json_builder_add_string_value (builder, channel->confinement);
                json_builder_set_member_name (builder, "version");
                json_builder_add_string_value (builder, channel->version);
                json_builder_set_member_name (builder, "channel");
                json_builder_add_string_value (builder, name);
                json_builder_set_member_name (builder, "epoch");
                json_builder_add_string_value (builder, channel->epoch);
                json_builder_set_member_name (builder, "size");
                json_builder_add_int_value (builder, channel->size);
                if (channel->released_at != NULL) {
                    json_builder_set_member_name (builder, "released-at");
                    json_builder_add_string_value (builder, channel->released_at);
                }
                json_builder_end_object (builder);
            }
        }
        json_builder_end_object (builder);
    }
    if (snap->apps != NULL) {
        GList *link;
        int id_count = 0;

        for (link = snap->apps; link; link = link->next) {
            MockApp *app = link->data;
            if (app->common_id == NULL)
                continue;
            if (id_count == 0) {
                json_builder_set_member_name (builder, "common-ids");
                json_builder_begin_array (builder);
            }
            id_count++;
            json_builder_add_string_value (builder, app->common_id);
        }
        if (id_count > 0)
            json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "confinement");
    json_builder_add_string_value (builder, snap->confinement);
    if (snap->contact) {
        json_builder_set_member_name (builder, "contact");
        json_builder_add_string_value (builder, snap->contact);
    }
    if (snap->description) {
        json_builder_set_member_name (builder, "description");
        json_builder_add_string_value (builder, snap->description);
    }
    json_builder_set_member_name (builder, "developer");
    json_builder_add_string_value (builder, snap->publisher_username);
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
    json_builder_set_member_name (builder, "jailmode");
    json_builder_add_boolean_value (builder, snap->jailmode);
    if (snap->license) {
        json_builder_set_member_name (builder, "license");
        json_builder_add_string_value (builder, snap->license);
    }
    if (snap->mounted_from) {
        json_builder_set_member_name (builder, "mounted-from");
        json_builder_add_string_value (builder, snap->mounted_from);
    }
    if (snap->media != NULL) {
        GList *link;

        json_builder_set_member_name (builder, "media");
        json_builder_begin_array (builder);
        for (link = snap->media; link; link = link->next) {
            MockMedia *media = link->data;

            if (strcmp (media->type, "screenshot") == 0)
                screenshot_count++;

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "type");
            json_builder_add_string_value (builder, media->type);
            json_builder_set_member_name (builder, "url");
            json_builder_add_string_value (builder, media->url);
            if (media->width > 0 && media->height > 0) {
                json_builder_set_member_name (builder, "width");
                json_builder_add_int_value (builder, media->width);
                json_builder_set_member_name (builder, "height");
                json_builder_add_int_value (builder, media->height);
            }
            json_builder_end_object (builder);
        }
        json_builder_end_array (builder);
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
    if (snap->is_private) {
        json_builder_set_member_name (builder, "private");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_set_member_name (builder, "publisher");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, snap->publisher_id);
    json_builder_set_member_name (builder, "username");
    json_builder_add_string_value (builder, snap->publisher_username);
    json_builder_set_member_name (builder, "display-name");
    json_builder_add_string_value (builder, snap->publisher_display_name);
    if (snap->publisher_validation != NULL) {
        json_builder_set_member_name (builder, "validation");
        json_builder_add_string_value (builder, snap->publisher_validation);
    }
    json_builder_end_object (builder);
    json_builder_set_member_name (builder, "resource");
    resource = g_strdup_printf ("/v2/snaps/%s", snap->name);
    json_builder_add_string_value (builder, resource);
    json_builder_set_member_name (builder, "revision");
    json_builder_add_string_value (builder, snap->revision);
    if (screenshot_count > 0) {
        GList *link;

        json_builder_set_member_name (builder, "screenshots");
        json_builder_begin_array (builder);
        for (link = snap->media; link; link = link->next) {
            MockMedia *media = link->data;

            if (strcmp (media->type, "screenshot") != 0)
                continue;

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "url");
            json_builder_add_string_value (builder, media->url);
            if (media->width > 0 && media->height > 0) {
                json_builder_set_member_name (builder, "width");
                json_builder_add_int_value (builder, media->width);
                json_builder_set_member_name (builder, "height");
                json_builder_add_int_value (builder, media->height);
            }
            json_builder_end_object (builder);
        }
        json_builder_end_array (builder);
    }
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, snap->status);
    if (snap->summary) {
        json_builder_set_member_name (builder, "summary");
        json_builder_add_string_value (builder, snap->summary);
    }
    if (snap->title) {
        json_builder_set_member_name (builder, "title");
        json_builder_add_string_value (builder, snap->title);
    }
    if (snap->tracking_channel) {
        json_builder_set_member_name (builder, "tracking-channel");
        json_builder_add_string_value (builder, snap->tracking_channel);
    }
    if (snap->tracks) {
        GList *link;

        json_builder_set_member_name (builder, "tracks");
        json_builder_begin_array (builder);
        for (link = snap->tracks; link; link = link->next) {
            MockTrack *track = link->data;
            json_builder_add_string_value (builder, track->name);
        }
        json_builder_end_array (builder);
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

        snap = find_snap (snapd, store_snap->name);
        if (snap != NULL && strcmp (store_snap->revision, snap->revision) > 0)
            refreshable_snaps = g_list_append (refreshable_snaps, store_snap);
    }

    return refreshable_snaps;
}

static gboolean
filter_snaps (GStrv selected_snaps, MockSnap *snap)
{
    int i;

    /* If no filter selected, then return all snaps */
    if (selected_snaps == NULL || selected_snaps[0] == NULL)
        return TRUE;

    for (i = 0; selected_snaps[i] != NULL; i++) {
        if (strcmp (selected_snaps[i], snap->name) == 0)
            return TRUE;
    }

    return FALSE;
}

static void
handle_snaps (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    const gchar *content_type;

    content_type = soup_message_headers_get_content_type (message->request_headers, NULL);

    if (strcmp (message->method, "GET") == 0) {
        const gchar *select_param = NULL;
        g_auto(GStrv) selected_snaps = NULL;
        g_autoptr(JsonBuilder) builder = NULL;
        GList *link;

        if (query != NULL) {
            const gchar *snaps_param = NULL;

            select_param = g_hash_table_lookup (query, "select");
            snaps_param = g_hash_table_lookup (query, "snaps");
            if (snaps_param != NULL)
                selected_snaps = g_strsplit (snaps_param, ",", -1);
        }

        builder = json_builder_new ();
        json_builder_begin_array (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;

            if (!filter_snaps (selected_snaps, snap))
                continue;

            if ((select_param == NULL || strcmp (select_param, "enabled") == 0) && strcmp (snap->status, "active") != 0)
                continue;

            json_builder_add_value (builder, make_snap_node (snap));
        }
        json_builder_end_array (builder);

        send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
    }
    else if (strcmp (message->method, "POST") == 0 && g_strcmp0 (content_type, "application/json") == 0) {
        g_autoptr(JsonNode) request = NULL;
        JsonObject *o;
        const gchar *action;

        request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

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

            change = add_change (snapd);
            change->data = json_builder_get_root (builder);
            mock_change_add_task (change, "refresh");
            send_async_response (snapd, message, 202, change->id);
        }
        else {
            send_error_bad_request (snapd, message, "unsupported multi-snap operation", NULL);
            return;
        }
    }
    else if (strcmp (message->method, "POST") == 0 && g_str_has_prefix (content_type, "multipart/")) {
        g_autoptr(SoupMultipart) multipart = NULL;
        int i;
        gboolean classic = FALSE, dangerous = FALSE, devmode = FALSE, jailmode = FALSE;
        g_autofree gchar *action = NULL;
        g_autofree gchar *snap = NULL;
        g_autofree gchar *snap_path = NULL;

        multipart = soup_multipart_new_from_message (message->request_headers, message->request_body);
        if (multipart == NULL) {
            send_error_bad_request (snapd, message, "cannot read POST form", NULL);
            return;
        }

        for (i = 0; i < soup_multipart_get_length (multipart); i++) {
            SoupMessageHeaders *part_headers;
            SoupBuffer *part_body;
            g_autofree gchar *disposition = NULL;
            g_autoptr(GHashTable) params = NULL;

            if (!soup_multipart_get_part (multipart, i, &part_headers, &part_body))
                continue;
            if (!soup_message_headers_get_content_disposition (part_headers, &disposition, &params))
                continue;

            if (strcmp (disposition, "form-data") == 0) {
                const gchar *name = g_hash_table_lookup (params, "name");

                if (g_strcmp0 (name, "action") == 0)
                    action = g_strndup (part_body->data, part_body->length);
                else if (g_strcmp0 (name, "classic") == 0)
                    classic = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "dangerous") == 0)
                    dangerous = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "devmode") == 0)
                    devmode = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "jailmode") == 0)
                    jailmode = strncmp (part_body->data, "true", part_body->length) == 0;
                else if (g_strcmp0 (name, "snap") == 0)
                    snap = g_strndup (part_body->data, part_body->length);
                else if (g_strcmp0 (name, "snap-path") == 0)
                    snap_path = g_strndup (part_body->data, part_body->length);
            }
        }

        if (g_strcmp0 (action, "try") == 0) {
            MockChange *change;
            MockTask *task;

            if (snap_path == NULL) {
                send_error_bad_request (snapd, message, "need 'snap-path' value in form", NULL);
                return;
            }

            if (strcmp (snap_path, "*") == 0) {
                send_error_bad_request (snapd, message, "directory does not contain an unpacked snap", "snap-not-a-snap");
                return;
            }

            change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            task = mock_change_add_task (change, "try");
            task->snap = mock_snap_new ("try");
            task->snap->trymode = TRUE;
            task->snap->snap_path = g_steal_pointer (&snap_path);

            send_async_response (snapd, message, 202, change->id);
        }
        else {
            MockChange *change;
            MockTask *task;

            if (snap == NULL) {
                send_error_bad_request (snapd, message, "cannot find \"snap\" file field in provided multipart/form-data payload", NULL);
                return;
            }

            change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            task = mock_change_add_task (change, "install");
            task->snap = mock_snap_new ("sideload");
            if (classic)
                mock_snap_set_confinement (task->snap, "classic");
            task->snap->dangerous = dangerous;
            task->snap->devmode = devmode; // FIXME: Should set confinement to devmode?
            task->snap->jailmode = jailmode;
            g_free (task->snap->snap_data);
            task->snap->snap_data = g_steal_pointer (&snap);

            send_async_response (snapd, message, 202, change->id);
        }
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static void
handle_snap (MockSnapd *snapd, SoupMessage *message, const gchar *name)
{
    if (strcmp (message->method, "GET") == 0) {
        MockSnap *snap;

        snap = find_snap (snapd, name);
        if (snap != NULL)
            send_sync_response (snapd, message, 200, make_snap_node (snap), NULL);
        else
            send_error_not_found (snapd, message, "cannot find snap", NULL);
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autoptr(JsonNode) request = NULL;
        JsonObject *o;
        const gchar *action, *channel = NULL, *revision = NULL;
        gboolean classic = FALSE, dangerous = FALSE, devmode = FALSE, jailmode = FALSE;

        request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");
        if (json_object_has_member (o, "channel"))
            channel = json_object_get_string_member (o, "channel");
        if (json_object_has_member (o, "revision"))
            revision = json_object_get_string_member (o, "revision");
        if (json_object_has_member (o, "classic"))
            classic = json_object_get_boolean_member (o, "classic");
        if (json_object_has_member (o, "dangerous"))
            dangerous = json_object_get_boolean_member (o, "dangerous");
        if (json_object_has_member (o, "devmode"))
            devmode = json_object_get_boolean_member (o, "devmode");
        if (json_object_has_member (o, "jailmode"))
            jailmode = json_object_get_boolean_member (o, "jailmode");

        if (strcmp (action, "install") == 0) {
            MockSnap *snap;
            MockChange *change;
            MockTask *task;

            if (snapd->decline_auth) {
                send_error_forbidden (snapd, message, "cancelled", "auth-cancelled");
                return;
            }

            snap = find_snap (snapd, name);
            if (snap != NULL) {
                send_error_bad_request (snapd, message, "snap is already installed", "snap-already-installed");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, NULL, NULL);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "cannot install, snap not found", "snap-not-found");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, channel, NULL);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "no snap revision on specified channel", "snap-channel-not-available");
                return;
            }

            snap = find_store_snap_by_name (snapd, name, channel, revision);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "no snap revision available as specified", "snap-revision-not-available");
                return;
            }

            if (strcmp (snap->confinement, "classic") == 0 && !classic) {
                send_error_bad_request (snapd, message, "requires classic confinement", "snap-needs-classic");
                return;
            }
            if (strcmp (snap->confinement, "classic") == 0 && !snapd->on_classic) {
                send_error_bad_request (snapd, message, "requires classic confinement which is only available on classic systems", "snap-needs-classic-system");
                return;
            }
            if (classic && strcmp (snap->confinement, "classic") != 0) {
                send_error_bad_request (snapd, message, "snap not compatible with --classic", "snap-not-classic");
                return;
            }
            if (strcmp (snap->confinement, "devmode") == 0 && !devmode) {
                send_error_bad_request (snapd, message, "requires devmode or confinement override", "snap-needs-devmode");
                return;
            }

            change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            task = mock_change_add_task (change, "install");
            task->snap = mock_snap_new (name);
            mock_snap_set_confinement (task->snap, snap->confinement);
            mock_snap_set_channel (task->snap, snap->channel);
            mock_snap_set_revision (task->snap, snap->revision);
            task->snap->devmode = devmode;
            task->snap->jailmode = jailmode;
            task->snap->dangerous = dangerous;
            if (snap->error != NULL)
                task->error = g_strdup (snap->error);

            send_async_response (snapd, message, 202, change->id);
        }
        else if (strcmp (action, "refresh") == 0) {
            MockSnap *snap;

            snap = find_snap (snapd, name);
            if (snap != NULL)
            {
                MockSnap *store_snap;

                /* Find if we have a store snap with a newer revision */
                store_snap = find_store_snap_by_name (snapd, name, channel, NULL);
                if (store_snap == NULL) {
                    send_error_bad_request (snapd, message, "cannot perform operation on local snap", "snap-local");
                } else if (strcmp (store_snap->revision, snap->revision) > 0) {
                    MockChange *change;

                    mock_snap_set_channel (snap, channel);

                    change = add_change (snapd);
                    mock_change_set_spawn_time (change, snapd->spawn_time);
                    mock_change_set_ready_time (change, snapd->ready_time);
                    mock_change_add_task (change, "refresh");
                    send_async_response (snapd, message, 202, change->id);
                }
                else
                    send_error_bad_request (snapd, message, "snap has no updates available", "snap-no-update-available");
            }
            else
                send_error_bad_request (snapd, message, "cannot refresh: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "remove") == 0) {
            MockSnap *snap;

            if (snapd->decline_auth) {
                send_error_forbidden (snapd, message, "cancelled", "auth-cancelled");
                return;
            }

            snap = find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;
                MockTask *task;

                change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                task = mock_change_add_task (change, "remove");
                mock_task_set_snap_name (task, name);
                if (snap->error != NULL)
                    task->error = g_strdup (snap->error);

                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "snap is not installed", "snap-not-installed");
        }
        else if (strcmp (action, "enable") == 0) {
            MockSnap *snap;

            snap = find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                if (!snap->disabled) {
                    send_error_bad_request (snapd, message, "cannot enable: snap is already enabled", NULL);
                    return;
                }
                snap->disabled = FALSE;

                change = add_change (snapd);
                mock_change_add_task (change, "enable");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot enable: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "disable") == 0) {
            MockSnap *snap;

            snap = find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                if (snap->disabled) {
                    send_error_bad_request (snapd, message, "cannot disable: snap is already disabled", NULL);
                    return;
                }
                snap->disabled = TRUE;

                change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                mock_change_add_task (change, "disable");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot disable: cannot find snap", "snap-not-installed");
        }
        else if (strcmp (action, "switch") == 0) {
            MockSnap *snap;

            snap = find_snap (snapd, name);
            if (snap != NULL)
            {
                MockChange *change;

                mock_snap_set_tracking_channel (snap, channel);

                change = add_change (snapd);
                mock_change_set_spawn_time (change, snapd->spawn_time);
                mock_change_set_ready_time (change, snapd->ready_time);
                mock_change_add_task (change, "switch");
                send_async_response (snapd, message, 202, change->id);
            }
            else
                send_error_bad_request (snapd, message, "cannot switch: cannot find snap", "snap-not-installed");
        }
        else
            send_error_bad_request (snapd, message, "unknown action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, message, "method not allowed");
}

static void
handle_apps (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    const gchar *select_param = NULL;
    g_auto(GStrv) selected_snaps = NULL;
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    if (query != NULL) {
        const gchar *snaps_param = NULL;

        select_param = g_hash_table_lookup (query, "select");
        snaps_param = g_hash_table_lookup (query, "names");
        if (snaps_param != NULL)
            selected_snaps = g_strsplit (snaps_param, ",", -1);
    }

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        GList *app_link;

        if (!filter_snaps (selected_snaps, snap))
            continue;

        for (app_link = snap->apps; app_link; app_link = app_link->next) {
            MockApp *app = app_link->data;

            if (g_strcmp0 (select_param, "service") == 0 && app->daemon == NULL)
                continue;

            json_builder_add_value (builder, make_app_node (app, snap->name));
        }
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static void
handle_icon (MockSnapd *snapd, SoupMessage *message, const gchar *path)
{
    MockSnap *snap;
    g_autofree gchar *name = NULL;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    if (!g_str_has_suffix (path, "/icon")) {
        send_error_not_found (snapd, message, "not found", NULL);
        return;
    }
    name = g_strndup (path, strlen (path) - strlen ("/icon"));

    snap = find_snap (snapd, name);
    if (snap == NULL)
        send_error_not_found (snapd, message, "cannot find snap", NULL);
    else if (snap->icon_data == NULL)
        send_error_not_found (snapd, message, "not found", NULL);
    else
        send_response (message, 200, snap->icon_mime_type,
                       (const guint8 *) g_bytes_get_data (snap->icon_data, NULL),
                       g_bytes_get_size (snap->icon_data));
}

static void
handle_assertions (MockSnapd *snapd, SoupMessage *message, const gchar *type)
{
    if (strcmp (message->method, "GET") == 0) {
        g_autoptr(GString) response_content = NULL;
        g_autofree gchar *type_header = NULL;
        int count = 0;
        GList *link;

        response_content = g_string_new (NULL);
        type_header = g_strdup_printf ("type: %s\n", type);
        for (link = snapd->assertions; link; link = link->next) {
            const gchar *assertion = link->data;

            if (!g_str_has_prefix (assertion, type_header))
                continue;

            count++;
            if (count != 1)
                g_string_append (response_content, "\n\n");
            g_string_append (response_content, assertion);
        }

        if (count == 0) {
            send_error_bad_request (snapd, message, "invalid assert type", NULL);
            return;
        }

        // FIXME: X-Ubuntu-Assertions-Count header
        send_response (message, 200, "application/x.ubuntu.assertion; bundle=y", (guint8*) response_content->str, response_content->len);
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autofree gchar *assertion = NULL;

        assertion = g_strndup (message->request_body->data, message->request_body->length);
        add_assertion (snapd, assertion);
        send_sync_response (snapd, message, 200, NULL, NULL);
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static void
handle_interfaces (MockSnapd *snapd, SoupMessage *message)
{
    if (strcmp (message->method, "GET") == 0) {
        g_autoptr(JsonBuilder) builder = NULL;
        GList *link;

        builder = json_builder_new ();
        json_builder_begin_object (builder);
        json_builder_set_member_name (builder, "plugs");
        json_builder_begin_array (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;
            GList *l;

            for (l = snap->plugs; l; l = l->next) {
                MockPlug *plug = l->data;
                GList *l2;
                g_autoptr(GList) slots = NULL;

                for (l2 = snapd->established_connections; l2; l2 = l2->next) {
                    MockConnection *connection = l2->data;
                    if (connection->plug == plug)
                        slots = g_list_append (slots, connection->slot);
                }

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "snap");
                json_builder_add_string_value (builder, snap->name);
                json_builder_set_member_name (builder, "plug");
                json_builder_add_string_value (builder, plug->name);
                json_builder_set_member_name (builder, "interface");
                json_builder_add_string_value (builder, plug->interface->name);
                json_builder_set_member_name (builder, "label");
                json_builder_add_string_value (builder, plug->label);
                if (slots != NULL) {
                    json_builder_set_member_name (builder, "connections");
                    json_builder_begin_array (builder);
                    for (l2 = slots; l2; l2 = l2->next) {
                        MockSlot *slot = l2->data;
                        json_builder_begin_object (builder);
                        json_builder_set_member_name (builder, "snap");
                        json_builder_add_string_value (builder, slot->snap->name);
                        json_builder_set_member_name (builder, "slot");
                        json_builder_add_string_value (builder, slot->name);
                        json_builder_end_object (builder);
                    }
                    json_builder_end_array (builder);
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

            for (l = snap->slots_; l; l = l->next) {
                MockSlot *slot = l->data;
                GList *l2;
                g_autoptr(GList) plugs = NULL;

                for (l2 = snapd->established_connections; l2; l2 = l2->next) {
                    MockConnection *connection = l2->data;
                    if (connection->slot == slot)
                        plugs = g_list_append (plugs, connection->plug);
                }

                json_builder_begin_object (builder);
                json_builder_set_member_name (builder, "snap");
                json_builder_add_string_value (builder, snap->name);
                json_builder_set_member_name (builder, "slot");
                json_builder_add_string_value (builder, slot->name);
                json_builder_set_member_name (builder, "interface");
                json_builder_add_string_value (builder, slot->interface->name);
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

        send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autoptr(JsonNode) request = NULL;
        JsonObject *o;
        const gchar *action;
        JsonArray *a;
        int i;
        g_autoptr(GList) plugs = NULL;
        g_autoptr(GList) slots = NULL;

        request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");

        a = json_object_get_array_member (o, "plugs");
        for (i = 0; i < json_array_get_length (a); i++) {
            JsonObject *po = json_array_get_object_element (a, i);
            MockSnap *snap;
            MockPlug *plug;

            snap = find_snap (snapd, json_object_get_string_member (po, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, message, "invalid snap", NULL);
                return;
            }
            plug = mock_snap_find_plug (snap, json_object_get_string_member (po, "plug"));
            if (plug == NULL) {
                send_error_bad_request (snapd, message, "invalid plug", NULL);
                return;
            }
            plugs = g_list_append (plugs, plug);
        }

        a = json_object_get_array_member (o, "slots");
        for (i = 0; i < json_array_get_length (a); i++) {
            JsonObject *so = json_array_get_object_element (a, i);
            MockSnap *snap;
            MockSlot *slot;

            snap = find_snap (snapd, json_object_get_string_member (so, "snap"));
            if (snap == NULL) {
                send_error_bad_request (snapd, message, "invalid snap", NULL);
                return;
            }
            slot = mock_snap_find_slot (snap, json_object_get_string_member (so, "slot"));
            if (slot == NULL) {
                send_error_bad_request (snapd, message, "invalid slot", NULL);
                return;
            }
            slots = g_list_append (slots, slot);
        }

        if (strcmp (action, "connect") == 0) {
            MockChange *change;
            GList *link;

            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, message, "at least one plug and slot is required", NULL);
                return;
            }

            for (link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                mock_snapd_connect (snapd, plug, slots->data, TRUE, FALSE);
            }

            change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            mock_change_add_task (change, "connect-snap");
            send_async_response (snapd, message, 202, change->id);
        }
        else if (strcmp (action, "disconnect") == 0) {
            MockChange *change;
            GList *link;
            g_autoptr(GList) old_connections = NULL;

            if (g_list_length (plugs) < 1 || g_list_length (slots) < 1) {
                send_error_bad_request (snapd, message, "at least one plug and slot is required", NULL);
                return;
            }

            for (link = plugs; link; link = link->next) {
                MockPlug *plug = link->data;
                mock_snapd_connect (snapd, plug, NULL, TRUE, FALSE);
            }

            change = add_change (snapd);
            mock_change_set_spawn_time (change, snapd->spawn_time);
            mock_change_set_ready_time (change, snapd->ready_time);
            mock_change_add_task (change, "disconnect");
            send_async_response (snapd, message, 202, change->id);
        }
        else
            send_error_bad_request (snapd, message, "unsupported interface action", NULL);
    }
    else
        send_error_method_not_allowed (snapd, message, "method not allowed");
}

static void
add_connection (JsonBuilder *builder, MockConnection *connection)
{
    MockPlug *plug = connection->plug;
    MockSlot *slot = connection->slot;

    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "slot");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, slot->snap->name);
    json_builder_set_member_name (builder, "slot");
    json_builder_add_string_value (builder, slot->name);
    json_builder_end_object (builder);

    json_builder_set_member_name (builder, "plug");
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, plug->snap->name);
    json_builder_set_member_name (builder, "plug");
    json_builder_add_string_value (builder, plug->name);
    json_builder_end_object (builder);

    json_builder_set_member_name (builder, "interface");
    json_builder_add_string_value (builder, plug->interface->name);

    if (connection->manual) {
        json_builder_set_member_name (builder, "manual");
        json_builder_add_boolean_value (builder, TRUE);
    }

    if (connection->gadget) {
        json_builder_set_member_name (builder, "gadget");
        json_builder_add_boolean_value (builder, TRUE);
    }

    // FIXME: slot-attrs
    // FIXME: plug-attrs

    json_builder_end_object (builder);
}

static void
handle_connections (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_object (builder);

    json_builder_set_member_name (builder, "established");
    json_builder_begin_array (builder);
    for (link = snapd->established_connections; link; link = link->next) {
        MockConnection *connection = link->data;
        add_connection (builder, connection);
    }
    json_builder_end_array (builder);

    if (snapd->undesired_connections != NULL) {
        json_builder_set_member_name (builder, "undesired");
        json_builder_begin_array (builder);
        for (link = snapd->undesired_connections; link; link = link->next) {
            MockConnection *connection = link->data;
            add_connection (builder, connection);
        }
        json_builder_end_array (builder);
    }

    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        GList *l;

        for (l = snap->plugs; l; l = l->next) {
            MockPlug *plug = l->data;
            GList *l2;
            g_autoptr(GList) slots = NULL;

            for (l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->plug == plug)
                    slots = g_list_append (slots, connection->slot);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "plug");
            json_builder_add_string_value (builder, plug->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, plug->interface->name);
            json_builder_set_member_name (builder, "label");
            json_builder_add_string_value (builder, plug->label);
            if (slots != NULL) {
                json_builder_set_member_name (builder, "connections");
                json_builder_begin_array (builder);
                for (l2 = slots; l2; l2 = l2->next) {
                        MockSlot *slot = l2->data;
                        json_builder_begin_object (builder);
                        json_builder_set_member_name (builder, "snap");
                        json_builder_add_string_value (builder, slot->snap->name);
                        json_builder_set_member_name (builder, "slot");
                        json_builder_add_string_value (builder, slot->name);
                        json_builder_end_object (builder);
                }
                json_builder_end_array (builder);
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

        for (l = snap->slots_; l; l = l->next) {
            MockSlot *slot = l->data;
            GList *l2;
            g_autoptr(GList) plugs = NULL;

            for (l2 = snapd->established_connections; l2; l2 = l2->next) {
                MockConnection *connection = l2->data;
                if (connection->slot == slot)
                    plugs = g_list_append (plugs, connection->plug);
            }

            json_builder_begin_object (builder);
            json_builder_set_member_name (builder, "snap");
            json_builder_add_string_value (builder, snap->name);
            json_builder_set_member_name (builder, "slot");
            json_builder_add_string_value (builder, slot->name);
            json_builder_set_member_name (builder, "interface");
            json_builder_add_string_value (builder, slot->interface->name);
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

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static MockTask *
get_current_task (MockChange *change)
{
    GList *link;

    for (link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        if (strcmp (task->status, "Done") != 0)
            return task;
    }

    return NULL;
}

static gboolean
change_get_ready (MockChange *change)
{
    MockTask *task = get_current_task (change);
    return task == NULL || strcmp (task->status, "Error") == 0;
}

static JsonNode *
make_change_node (MockChange *change)
{
    int progress_total, progress_done;
    MockTask *task;
    const gchar *status, *error;
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    for (progress_done = 0, progress_total = 0, link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;
        progress_done += task->progress_done;
        progress_total += task->progress_total;
    }

    task = get_current_task (change);
    status = task != NULL ? task->status : "Done";
    error = task != NULL && strcmp (task->status, "Error") == 0 ? task->error : NULL;

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "id");
    json_builder_add_string_value (builder, change->id);
    json_builder_set_member_name (builder, "kind");
    json_builder_add_string_value (builder, change->kind);
    json_builder_set_member_name (builder, "summary");
    json_builder_add_string_value (builder, change->summary);
    json_builder_set_member_name (builder, "status");
    json_builder_add_string_value (builder, status);
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
        if (task->progress_done >= task->progress_total && task->ready_time != NULL) {
            json_builder_set_member_name (builder, "ready-time");
            json_builder_add_string_value (builder, task->ready_time);
        }
        json_builder_end_object (builder);
    }
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "ready");
    json_builder_add_boolean_value (builder, change_get_ready (change));
    json_builder_set_member_name (builder, "spawn-time");
    json_builder_add_string_value (builder, change->spawn_time);
    if (change_get_ready (change) && change->ready_time != NULL) {
        json_builder_set_member_name (builder, "ready-time");
        json_builder_add_string_value (builder, change->ready_time);
    }
    if (change_get_ready (change) && change->data != NULL) {
        json_builder_set_member_name (builder, "data");
        json_builder_add_value (builder, json_node_ref (change->data));
    }
    if (error != NULL) {
        json_builder_set_member_name (builder, "err");
        json_builder_add_string_value (builder, error);
    }
    json_builder_end_object (builder);

    return json_builder_get_root (builder);
}

static gboolean
change_relates_to_snap (MockChange *change, const gchar *snap_name)
{
    GList *link;

    for (link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;

        if (g_strcmp0 (snap_name, task->snap_name) == 0)
            return TRUE;
        if (task->snap != NULL && g_strcmp0 (snap_name, task->snap->name) == 0)
            return TRUE;
    }

    return FALSE;
}

static void
handle_changes (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    const gchar *select_param;
    const gchar *for_param;
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    select_param = g_hash_table_lookup (query, "select");
    if (select_param == NULL)
        select_param = "in-progress";
    for_param = g_hash_table_lookup (query, "for");

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snapd->changes; link; link = link->next) {
        MockChange *change = link->data;

        if (g_strcmp0 (select_param, "in-progress") == 0 && change_get_ready (change))
            continue;
        if (g_strcmp0 (select_param, "ready") == 0 && !change_get_ready (change))
            continue;
        if (for_param != NULL && !change_relates_to_snap (change, for_param))
            continue;

        json_builder_add_value (builder, make_change_node (change));
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static void
mock_task_complete (MockSnapd *snapd, MockTask *task)
{
    if (strcmp (task->kind, "install") == 0 || strcmp (task->kind, "try") == 0)
        snapd->snaps = g_list_append (snapd->snaps, g_steal_pointer (&task->snap));
    else if (strcmp (task->kind, "remove") == 0) {
        MockSnap *snap;
        snap = find_snap (snapd, task->snap_name);
        snapd->snaps = g_list_remove (snapd->snaps, snap);
        mock_snap_free (snap);
    }
    mock_task_set_status (task, "Done");
}

static void
mock_change_progress (MockSnapd *snapd, MockChange *change)
{
    GList *link;

    for (link = change->tasks; link; link = link->next) {
        MockTask *task = link->data;

        if (task->error != NULL) {
            mock_task_set_status (task, "Error");
            return;
        }

        if (task->progress_done < task->progress_total) {
            mock_task_set_status (task, "Doing");
            task->progress_done++;
            if (task->progress_done == task->progress_total)
                mock_task_complete (snapd, task);
            return;
        }
    }
}

static void
handle_change (MockSnapd *snapd, SoupMessage *message, const gchar *change_id)
{
    if (strcmp (message->method, "GET") == 0) {
        MockChange *change;

        change = get_change (snapd, change_id);
        if (change == NULL) {
            send_error_not_found (snapd, message, "cannot find change", NULL);
            return;
        }
        mock_change_progress (snapd, change);

        send_sync_response (snapd, message, 200, make_change_node (change), NULL);
    }
    else if (strcmp (message->method, "POST") == 0) {
        MockChange *change;
        g_autoptr(JsonNode) request = NULL;
        JsonObject *o;
        const gchar *action;

        change = get_change (snapd, change_id);
        if (change == NULL) {
            send_error_not_found (snapd, message, "cannot find change", NULL);
            return;
        }

        request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        o = json_node_get_object (request);
        action = json_object_get_string_member (o, "action");
        if (strcmp (action, "abort") == 0) {
            MockTask *task;

            task = get_current_task (change);
            if (task == NULL) {
                send_error_bad_request (snapd, message, "task not in progress", NULL);
                return;
            }
            mock_task_set_status (task, "Error");
            task->error = g_strdup ("cancelled");
            send_sync_response (snapd, message, 200, make_change_node (change), NULL);
        }
        else {
            send_error_bad_request (snapd, message, "change action is unsupported", NULL);
            return;
        }
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static gboolean
matches_query (MockSnap *snap, const gchar *query)
{
    return query == NULL || strstr (snap->name, query) != NULL;
}

static gboolean
matches_name (MockSnap *snap, const gchar *name)
{
    return name == NULL || strcmp (snap->name, name) == 0;
}

static gboolean
matches_scope (MockSnap *snap, const gchar *scope)
{
    if (g_strcmp0 (scope, "wide") == 0)
        return TRUE;

    return !snap->scope_is_wide;
}

static gboolean
has_common_id (MockSnap *snap, const gchar *common_id)
{
    GList *link;

    if (common_id == NULL)
        return TRUE;

    for (link = snap->apps; link; link = link->next) {
        MockApp *app = link->data;
        if (g_strcmp0 (app->common_id, common_id) == 0)
            return TRUE;
    }

    return FALSE;
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
handle_find (MockSnapd *snapd, SoupMessage *message, GHashTable *query)
{
    const gchar *common_id_param;
    const gchar *query_param;
    const gchar *name_param;
    const gchar *select_param;
    select_param = g_hash_table_lookup (query, "select");
    const gchar *section_param;
    const gchar *scope_param;
    g_autoptr(JsonBuilder) builder = NULL;
    GList *snaps, *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    common_id_param = g_hash_table_lookup (query, "common-id");
    query_param = g_hash_table_lookup (query, "q");
    name_param = g_hash_table_lookup (query, "name");
    select_param = g_hash_table_lookup (query, "select");
    section_param = g_hash_table_lookup (query, "section");
    scope_param = g_hash_table_lookup (query, "scope");

    if (common_id_param && strcmp (common_id_param, "") == 0)
        common_id_param = NULL;
    if (query_param && strcmp (query_param, "") == 0)
        query_param = NULL;
    if (name_param && strcmp (name_param, "") == 0)
        name_param = NULL;
    if (select_param && strcmp (select_param, "") == 0)
        select_param = NULL;
    if (section_param && strcmp (section_param, "") == 0)
        section_param = NULL;
    if (scope_param && strcmp (scope_param, "") == 0)
        scope_param = NULL;

    if (g_strcmp0 (select_param, "refresh") == 0) {
        g_autoptr(GList) refreshable_snaps = NULL;

        if (query_param != NULL) {
            send_error_bad_request (snapd, message, "cannot use 'q' with 'select=refresh'", NULL);
            return;
        }
        if (name_param != NULL) {
            send_error_bad_request (snapd, message, "cannot use 'name' with 'select=refresh'", NULL);
            return;
        }

        builder = json_builder_new ();
        json_builder_begin_array (builder);
        refreshable_snaps = get_refreshable_snaps (snapd);
        for (link = refreshable_snaps; link; link = link->next)
            json_builder_add_value (builder, make_snap_node (link->data));
        json_builder_end_array (builder);

        send_sync_response (snapd, message, 200, json_builder_get_root (builder), snapd->suggested_currency);
        return;
    }
    else if (g_strcmp0 (select_param, "private") == 0) {
        MockAccount *account;

        account = get_account (snapd, message);
        if (account == NULL) {
            send_error_bad_request (snapd, message, "you need to log in first", "login-required");
            return;
        }
        snaps = account->private_snaps;
    }
    else
        snaps = snapd->store_snaps;

    /* Make a special query that never responds */
    if (g_strcmp0 (query_param, "do-not-respond") == 0)
        return;

    /* Make a special query that simulates a network timeout */
    if (g_strcmp0 (query_param, "network-timeout") == 0)
    {
        send_error_bad_request (snapd, message, "unable to contact snap store", "network-timeout");
        return;
    }

    /* Make a special query that simulates a DNS Failure */
    if (g_strcmp0 (query_param, "dns-failure") == 0)
    {
        send_error_bad_request (snapd, message, "failed to resolve address", "dns-failure");
        return;
    }

    /* Certain characters not allowed in queries */
    if (query_param != NULL) {
        int i;

        for (i = 0; query_param[i] != '\0'; i++) {
            const gchar *invalid_chars = "+=&|><!(){}[]^\"~*?:\\/";
            if (strchr (invalid_chars, query_param[i]) != NULL) {
                send_error_bad_request (snapd, message, "bad query", "bad-query");
                return;
            }
        }
    }

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snaps; link; link = link->next) {
        MockSnap *snap = link->data;

        if (!has_common_id (snap, common_id_param))
            continue;

        if (!in_section (snap, section_param))
            continue;

        if (!matches_query (snap, query_param))
            continue;

        if (!matches_name (snap, name_param))
            continue;

        if (!matches_scope (snap, scope_param))
            continue;

        json_builder_add_value (builder, make_snap_node (snap));
    }
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), snapd->suggested_currency);
}

static void
handle_buy_ready (MockSnapd *snapd, SoupMessage *message)
{
    JsonNode *result;
    MockAccount *account;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    account = get_account (snapd, message);
    if (account == NULL) {
        send_error_bad_request (snapd, message, "you need to log in first", "login-required");
        return;
    }

    if (!account->terms_accepted) {
        send_error_bad_request (snapd, message, "terms of service not accepted", "terms-not-accepted");
        return;
    }

    if (!account->has_payment_methods) {
        send_error_bad_request (snapd, message, "no payment methods", "no-payment-methods");
        return;
    }

    result = json_node_new (JSON_NODE_VALUE);
    json_node_set_boolean (result, TRUE);

    send_sync_response (snapd, message, 200, result, NULL);
}

static void
handle_buy (MockSnapd *snapd, SoupMessage *message)
{
    MockAccount *account;
    g_autoptr(JsonNode) request = NULL;
    JsonObject *o;
    const gchar *snap_id, *currency;
    gdouble price;
    MockSnap *snap;
    gdouble amount;

    if (strcmp (message->method, "POST") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    account = get_account (snapd, message);
    if (account == NULL) {
        send_error_bad_request (snapd, message, "you need to log in first", "login-required");
        return;
    }

    if (!account->terms_accepted) {
        send_error_bad_request (snapd, message, "terms of service not accepted", "terms-not-accepted");
        return;
    }

    if (!account->has_payment_methods) {
        send_error_bad_request (snapd, message, "no payment methods", "no-payment-methods");
        return;
    }

    request = get_json (message);
    if (request == NULL) {
        send_error_bad_request (snapd, message, "unknown content type", NULL);
        return;
    }

    o = json_node_get_object (request);
    snap_id = json_object_get_string_member (o, "snap-id");
    price = json_object_get_double_member (o, "price");
    currency = json_object_get_string_member (o, "currency");

    snap = find_store_snap_by_id (snapd, snap_id);
    if (snap == NULL) {
        send_error_not_found (snapd, message, "not found", NULL); // FIXME: Check is error snapd returns
        return;
    }

    amount = mock_snap_find_price (snap, currency);
    if (amount == 0) {
        send_error_bad_request (snapd, message, "no price found", "payment-declined"); // FIXME: Check is error snapd returns
        return;
    }
    if (amount != price) {
        send_error_bad_request (snapd, message, "invalid price", "payment-declined"); // FIXME: Check is error snapd returns
        return;
    }

    send_sync_response (snapd, message, 200, NULL, NULL);
}

static void
handle_sections (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snapd->store_sections; link; link = link->next)
        json_builder_add_string_value (builder, link->data);
    json_builder_end_array (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static MockSnap *
find_snap_by_alias (MockSnapd *snapd, const gchar *name)
{
    GList *link;

    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        GList *app_link;

        for (app_link = snap->apps; app_link; app_link = app_link->next) {
            MockApp *app = app_link->data;
            MockAlias *alias;

            alias = mock_app_find_alias (app, name);
            if (alias != NULL)
                return snap;
        }
    }

    return NULL;
}

static void
unalias (MockSnapd *snapd, const gchar *name)
{
    GList *link;

    for (link = snapd->snaps; link; link = link->next) {
        MockSnap *snap = link->data;
        GList *app_link;

        for (app_link = snap->apps; app_link; app_link = app_link->next) {
            MockApp *app = app_link->data;
            MockAlias *alias;

            alias = mock_app_find_alias (app, name);
            if (alias != NULL) {
                if (alias->automatic)
                    alias->enabled = FALSE;
                else {
                    app->aliases = g_list_remove (app->aliases, alias);
                    mock_alias_free (alias);
                }
                return;
            }
        }
    }
}

static void
handle_aliases (MockSnapd *snapd, SoupMessage *message)
{
    if (strcmp (message->method, "GET") == 0) {
        g_autoptr(JsonBuilder) builder = NULL;
        GList *link;

        builder = json_builder_new ();
        json_builder_begin_object (builder);
        for (link = snapd->snaps; link; link = link->next) {
            MockSnap *snap = link->data;
            int alias_count = 0;
            GList *link2;

            for (link2 = snap->apps; link2; link2 = link2->next) {
                MockApp *app = link2->data;
                GList *link3;

                for (link3 = app->aliases; link3; link3 = link3->next) {
                    MockAlias *alias = link3->data;
                    g_autofree gchar *command = NULL;

                    if (alias_count == 0) {
                        json_builder_set_member_name (builder, snap->name);
                        json_builder_begin_object (builder);
                    }
                    alias_count++;

                    json_builder_set_member_name (builder, alias->name);
                    json_builder_begin_object (builder);
                    command = g_strdup_printf ("%s.%s", snap->name, app->name);
                    json_builder_set_member_name (builder, "command");
                    json_builder_add_string_value (builder, command);
                    if (alias->automatic) {
                        json_builder_set_member_name (builder, "status");
                        if (alias->enabled)
                            json_builder_add_string_value (builder, "auto");
                        else
                            json_builder_add_string_value (builder, "disabled");
                        json_builder_set_member_name (builder, "auto");
                        json_builder_add_string_value (builder, app->name);
                    }
                    else {
                        json_builder_set_member_name (builder, "status");
                        json_builder_add_string_value (builder, "manual");
                        json_builder_set_member_name (builder, "manual");
                        json_builder_add_string_value (builder, app->name);
                    }
                    json_builder_end_object (builder);
                }
            }

            if (alias_count > 0)
                json_builder_end_object (builder);
        }
        json_builder_end_object (builder);
        send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
    }
    else if (strcmp (message->method, "POST") == 0) {
        g_autoptr(JsonNode) request = NULL;
        JsonObject *o;
        MockSnap *snap;
        const gchar *action;
        const gchar *alias = NULL;
        MockChange *change;

        request = get_json (message);
        if (request == NULL) {
            send_error_bad_request (snapd, message, "unknown content type", NULL);
            return;
        }

        o = json_node_get_object (request);
        if (json_object_has_member (o, "alias"))
            alias = json_object_get_string_member (o, "alias");
        if (json_object_has_member (o, "snap")) {
            snap = find_snap (snapd, json_object_get_string_member (o, "snap"));
            if (snap == NULL) {
                send_error_not_found (snapd, message, "cannot find snap", NULL);
                return;
            }
        }
        else if (alias != NULL) {
            snap = find_snap_by_alias (snapd, alias);
            if (snap == NULL) {
                send_error_not_found (snapd, message, "cannot find snap", NULL);
                return;
            }
        }
        else {
            send_error_not_found (snapd, message, "cannot find snap", NULL);
            return;
        }

        action = json_object_get_string_member (o, "action");
        if (g_strcmp0 (action, "alias") == 0) {
            const gchar *app_name;
            MockApp *app;

            app_name = json_object_get_string_member (o, "app");
            if (app_name == NULL) {
                send_error_not_found (snapd, message, "No app specified", NULL);
                return;
            }
            app = mock_snap_find_app (snap, app_name);
            if (app == NULL) {
                send_error_not_found (snapd, message, "App not found", NULL);
                return;
            }
            mock_app_add_manual_alias (app, alias, TRUE);
        }
        else if (g_strcmp0 (action, "unalias") == 0) {
            unalias (snapd, alias);
        }
        else if (g_strcmp0 (action, "prefer") == 0) {
            snap->preferred = TRUE;
        }
        else {
            send_error_bad_request (snapd, message, "unsupported alias action", NULL);
            return;
        }

        change = add_change (snapd);
        mock_change_set_spawn_time (change, snapd->spawn_time);
        mock_change_set_ready_time (change, snapd->ready_time);
        mock_change_add_task (change, action);
        send_async_response (snapd, message, 202, change->id);
    }
    else {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }
}

static void
handle_snapctl (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonNode) request = NULL;
    JsonObject *o;
    const gchar *context_id;
    JsonArray *args;
    int i;
    g_autoptr(JsonBuilder) builder = NULL;
    g_autoptr(GString) stdout_text = NULL;

    if (strcmp (message->method, "POST") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    request = get_json (message);
    if (request == NULL) {
        send_error_bad_request (snapd, message, "unknown content type", NULL);
        return;
    }

    o = json_node_get_object (request);
    context_id = json_object_get_string_member (o, "context-id");
    args = json_object_get_array_member (o, "args");

    stdout_text = g_string_new ("STDOUT");
    g_string_append_printf (stdout_text, ":%s", context_id);
    for (i = 0; i < json_array_get_length (args); i++) {
        const gchar *arg = json_array_get_string_element (args, i);
        g_string_append_printf (stdout_text, ":%s", arg);
    }

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "stdout");
    json_builder_add_string_value (builder, stdout_text->str);
    json_builder_set_member_name (builder, "stderr");
    json_builder_add_string_value (builder, "STDERR");
    json_builder_end_object (builder);

    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static void
handle_create_user (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonNode) request = NULL;
    JsonObject *o;

    if (strcmp (message->method, "POST") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    request = get_json (message);
    if (request == NULL) {
        send_error_bad_request (snapd, message, "unknown content type", NULL);
        return;
    }

    o = json_node_get_object (request);
    if (json_object_has_member (o, "email")) {
        g_autoptr(JsonBuilder) builder = NULL;
        const gchar *email;
        g_auto(GStrv) email_keys = NULL;
        const gchar *username;
        gboolean sudoer = FALSE, known = FALSE;
        g_auto(GStrv) ssh_keys = NULL;
        MockAccount *a;

        builder = json_builder_new ();

        email = json_object_get_string_member (o, "email");
        if (json_object_has_member (o, "sudoer"))
            sudoer = json_object_get_boolean_member (o, "sudoer");
        if (json_object_has_member (o, "known"))
            known = json_object_get_boolean_member (o, "known");
        email_keys = g_strsplit (email, "@", 2);
        username = email_keys[0];
        ssh_keys = g_strsplit ("KEY1;KEY2", ";", -1);
        a = add_account (snapd, email, username, "password");
        a->sudoer = sudoer;
        a->known = known;
        mock_account_set_ssh_keys (a, ssh_keys);

        add_user_response (builder, a, FALSE, TRUE, FALSE);
        send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
        return;
    }
    else {
        if (json_object_has_member (o, "known") && json_object_get_boolean_member (o, "known")) {
            g_autoptr(JsonBuilder) builder = NULL;
            g_auto(GStrv) ssh_keys = NULL;
            MockAccount *a;

            builder = json_builder_new ();
            json_builder_begin_array (builder);

            ssh_keys = g_strsplit ("KEY1;KEY2", ";", -1);
            a = add_account (snapd, "admin@example.com", "admin", "password");
            a->sudoer = TRUE;
            a->known = TRUE;
            mock_account_set_ssh_keys (a, ssh_keys);
            add_user_response (builder, a, FALSE, TRUE, FALSE);
            a = add_account (snapd, "alice@example.com", "alice", "password");
            a->known = TRUE;
            mock_account_set_ssh_keys (a, ssh_keys);
            add_user_response (builder, a, FALSE, TRUE, FALSE);
            a = add_account (snapd, "bob@example.com", "bob", "password");
            a->known = TRUE;
            mock_account_set_ssh_keys (a, ssh_keys);
            add_user_response (builder, a, FALSE, TRUE, FALSE);

            json_builder_end_array (builder);

            send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
            return;
        }
        else {
            send_error_bad_request (snapd, message, "cannot create user", NULL);
            return;
        }
    }
}

static void
handle_users (MockSnapd *snapd, SoupMessage *message)
{
    g_autoptr(JsonBuilder) builder = NULL;
    GList *link;

    if (strcmp (message->method, "GET") != 0) {
        send_error_method_not_allowed (snapd, message, "method not allowed");
        return;
    }

    builder = json_builder_new ();
    json_builder_begin_array (builder);
    for (link = snapd->accounts; link; link = link->next) {
        MockAccount *a = link->data;
        add_user_response (builder, a, FALSE, FALSE, TRUE);
    }
    json_builder_end_array (builder);
    send_sync_response (snapd, message, 200, json_builder_get_root (builder), NULL);
}

static void
handle_request (SoupServer        *server,
                SoupMessage       *message,
                const char        *path,
                GHashTable        *query,
                SoupClientContext *client,
                gpointer           user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);

    if (snapd->close_on_request) {
        g_autoptr(GIOStream) stream = soup_client_context_steal_connection (client);
        g_autoptr(GError) error = NULL;

        if (!g_io_stream_close (stream, NULL, &error)) {
            g_warning("Failed to close stream: %s", error->message);
        }
        return;
    }

    g_clear_pointer (&snapd->last_request_headers, soup_message_headers_free);
    snapd->last_request_headers = g_boxed_copy (SOUP_TYPE_MESSAGE_HEADERS, message->request_headers);

    if (strcmp (path, "/v2/system-info") == 0)
        handle_system_info (snapd, message);
    else if (strcmp (path, "/v2/login") == 0)
        handle_login (snapd, message);
    else if (strcmp (path, "/v2/snaps") == 0)
        handle_snaps (snapd, message, query);
    else if (g_str_has_prefix (path, "/v2/snaps/"))
        handle_snap (snapd, message, path + strlen ("/v2/snaps/"));
    else if (strcmp (path, "/v2/apps") == 0)
        handle_apps (snapd, message, query);
    else if (g_str_has_prefix (path, "/v2/icons/"))
        handle_icon (snapd, message, path + strlen ("/v2/icons/"));
    else if (strcmp (path, "/v2/assertions") == 0)
        handle_assertions (snapd, message, NULL);
    else if (g_str_has_prefix (path, "/v2/assertions/"))
        handle_assertions (snapd, message, path + strlen ("/v2/assertions/"));
    else if (strcmp (path, "/v2/interfaces") == 0)
        handle_interfaces (snapd, message);
    else if (strcmp (path, "/v2/connections") == 0)
        handle_connections (snapd, message);
    else if (strcmp (path, "/v2/changes") == 0)
        handle_changes (snapd, message, query);
    else if (g_str_has_prefix (path, "/v2/changes/"))
        handle_change (snapd, message, path + strlen ("/v2/changes/"));
    else if (strcmp (path, "/v2/find") == 0)
        handle_find (snapd, message, query);
    else if (strcmp (path, "/v2/buy/ready") == 0)
        handle_buy_ready (snapd, message);
    else if (strcmp (path, "/v2/buy") == 0)
        handle_buy (snapd, message);
    else if (strcmp (path, "/v2/sections") == 0)
        handle_sections (snapd, message);
    else if (strcmp (path, "/v2/aliases") == 0)
        handle_aliases (snapd, message);
    else if (strcmp (path, "/v2/snapctl") == 0)
        handle_snapctl (snapd, message);
    else if (strcmp (path, "/v2/create-user") == 0)
        handle_create_user (snapd, message);
    else if (strcmp (path, "/v2/users") == 0)
        handle_users (snapd, message);
    else
        send_error_not_found (snapd, message, "not found", NULL);
}

static gboolean
mock_snapd_thread_quit (gpointer user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);

    g_main_loop_quit (snapd->loop);

    return G_SOURCE_REMOVE;
}

static void
mock_snapd_finalize (GObject *object)
{
    MockSnapd *snapd = MOCK_SNAPD (object);

    /* shut down the server if it is running */
    mock_snapd_stop (snapd);

    if (g_unlink (snapd->socket_path) < 0)
        g_printerr ("Failed to unlink mock snapd socket\n");
    if (g_rmdir (snapd->dir_path) < 0)
        g_printerr ("Failed to remove temporary directory\n");

    g_clear_pointer (&snapd->dir_path, g_free);
    g_clear_pointer (&snapd->socket_path, g_free);
    g_list_free_full (snapd->accounts, (GDestroyNotify) mock_account_free);
    snapd->accounts = NULL;
    g_list_free_full (snapd->interfaces, (GDestroyNotify) mock_interface_free);
    snapd->interfaces = NULL;
    g_list_free_full (snapd->snaps, (GDestroyNotify) mock_snap_free);
    snapd->snaps = NULL;
    g_free (snapd->build_id);
    g_free (snapd->confinement);
    g_clear_pointer (&snapd->sandbox_features, g_hash_table_unref);
    g_free (snapd->store);
    g_free (snapd->maintenance_kind);
    g_free (snapd->maintenance_message);
    g_free (snapd->refresh_hold);
    g_free (snapd->refresh_last);
    g_free (snapd->refresh_next);
    g_free (snapd->refresh_schedule);
    g_free (snapd->refresh_timer);
    g_list_free_full (snapd->store_sections, g_free);
    snapd->store_sections = NULL;
    g_list_free_full (snapd->store_snaps, (GDestroyNotify) mock_snap_free);
    snapd->store_snaps = NULL;
    g_list_free_full (snapd->established_connections, (GDestroyNotify) mock_connection_free);
    snapd->established_connections = NULL;
    g_list_free_full (snapd->undesired_connections, (GDestroyNotify) mock_connection_free);
    snapd->undesired_connections = NULL;
    g_list_free_full (snapd->assertions, g_free);
    snapd->assertions = NULL;
    g_list_free_full (snapd->changes, (GDestroyNotify) mock_change_free);
    snapd->changes = NULL;
    g_clear_pointer (&snapd->suggested_currency, g_free);
    g_clear_pointer (&snapd->spawn_time, g_free);
    g_clear_pointer (&snapd->ready_time, g_free);
    g_clear_pointer (&snapd->last_request_headers, soup_message_headers_free);
    g_clear_pointer (&snapd->context, g_main_context_unref);
    g_clear_pointer (&snapd->loop, g_main_loop_unref);

    g_cond_clear (&snapd->condition);
    g_mutex_clear (&snapd->mutex);

    G_OBJECT_CLASS (mock_snapd_parent_class)->finalize (object);
}

static GSocket *
open_listening_socket (SoupServer *server, const gchar *socket_path, GError **error)
{
    g_autoptr(GSocket) socket = NULL;
    g_autoptr(GSocketAddress) address = NULL;

    socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           error);
    if (socket == NULL)
        return NULL;

    address = g_unix_socket_address_new (socket_path);
    if (!g_socket_bind (socket, address, TRUE, error))
        return NULL;

    if (!g_socket_listen (socket, error))
        return NULL;

    if (!soup_server_listen_socket (server, socket, 0, error))
        return NULL;

    return g_steal_pointer (&socket);
}

gpointer
mock_snapd_init_thread (gpointer user_data)
{
    MockSnapd *snapd = MOCK_SNAPD (user_data);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&snapd->mutex);
    g_autoptr(SoupServer) server = NULL;
    g_autoptr(GSocket) socket = NULL;
    g_autoptr(GError) error = NULL;

    snapd->context = g_main_context_new ();
    g_main_context_push_thread_default (snapd->context);
    snapd->loop = g_main_loop_new (snapd->context, FALSE);

    server = soup_server_new (SOUP_SERVER_SERVER_HEADER, "MockSnapd/1.0", NULL);
    soup_server_add_handler (server, NULL,
                             handle_request, snapd, NULL);

    socket = open_listening_socket (server, snapd->socket_path, &error);

    g_cond_signal (&snapd->condition);
    if (socket == NULL) {
        g_propagate_error (snapd->thread_init_error, error);
    }
    g_clear_pointer (&locker, g_mutex_locker_free);

    /* run until we're told to stop */
    if (socket != NULL) {
        g_main_loop_run (snapd->loop);
    }

    if (g_unlink (snapd->socket_path) < 0)
        g_printerr ("Failed to unlink mock snapd socket\n");

    g_clear_pointer (&snapd->loop, g_main_loop_unref);
    g_clear_pointer (&snapd->context, g_main_context_unref);

    return NULL;
}

gboolean
mock_snapd_start (MockSnapd *snapd, GError **dest_error)
{
    g_autoptr(GMutexLocker) locker = NULL;
    g_autoptr(GError) error = NULL;

    g_return_val_if_fail (MOCK_IS_SNAPD (snapd), FALSE);

    locker = g_mutex_locker_new (&snapd->mutex);
    /* Has the server already started? */
    if (snapd->thread)
        return TRUE;

    snapd->thread_init_error = &error;
    snapd->thread = g_thread_new ("mock_snapd_thread",
                                  mock_snapd_init_thread,
                                  snapd);
    g_cond_wait (&snapd->condition, &snapd->mutex);
    snapd->thread_init_error = NULL;

    /* If an error occurred during thread startup, clean it up */
    if (error != NULL) {
        g_thread_join (snapd->thread);
        snapd->thread = NULL;

        if (dest_error) {
            g_propagate_error (dest_error, error);
        } else {
            g_warning ("Failed to start server: %s", error->message);
        }
    }

    g_assert_nonnull (snapd->loop);
    g_assert_nonnull (snapd->context);

    return error == NULL;
}

void
mock_snapd_stop (MockSnapd *snapd)
{
    g_return_if_fail (MOCK_IS_SNAPD (snapd));

    if (!snapd->thread)
        return;

    g_main_context_invoke (snapd->context, mock_snapd_thread_quit, snapd);
    g_thread_join (snapd->thread);
    snapd->thread = NULL;
    g_assert_null (snapd->loop);
    g_assert_null (snapd->context);
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
    g_autoptr(GError) error = NULL;

    g_mutex_init (&snapd->mutex);
    g_cond_init (&snapd->condition);

    snapd->sandbox_features = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) g_ptr_array_unref);
    snapd->dir_path = g_dir_make_tmp ("mock-snapd-XXXXXX", &error);
    if (snapd->dir_path == NULL)
        g_warning ("Failed to make temporary directory: %s", error->message);
    g_clear_error (&error);
    snapd->socket_path = g_build_filename (snapd->dir_path, "snapd.socket", NULL);
}
