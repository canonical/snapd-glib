/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <gio/gunixsocketaddress.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

#include "snapd-client.h"
#include "snapd-alias.h"
#include "snapd-app.h"
#include "snapd-assertion.h"
#include "snapd-channel.h"
#include "snapd-error.h"
#include "snapd-json.h"
#include "snapd-login.h"
#include "snapd-plug.h"
#include "snapd-screenshot.h"
#include "snapd-slot.h"
#include "snapd-task.h"
#include "requests/snapd-get-aliases.h"
#include "requests/snapd-get-apps.h"
#include "requests/snapd-get-assertions.h"
#include "requests/snapd-get-buy-ready.h"
#include "requests/snapd-get-change.h"
#include "requests/snapd-get-find.h"
#include "requests/snapd-get-icon.h"
#include "requests/snapd-get-interfaces.h"
#include "requests/snapd-get-sections.h"
#include "requests/snapd-get-snap.h"
#include "requests/snapd-get-snaps.h"
#include "requests/snapd-get-system-info.h"
#include "requests/snapd-post-assertions.h"
#include "requests/snapd-post-buy.h"
#include "requests/snapd-post-change.h"
#include "requests/snapd-post-create-user.h"
#include "requests/snapd-post-create-users.h"
#include "requests/snapd-post-login.h"
#include "requests/snapd-post-snapctl.h"

/**
 * SECTION:snapd-client
 * @short_description: Client connection to snapd
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdClient is the means of talking to snapd.
 *
 * To communicate with snapd create a client with snapd_client_new() then
 * send requests.
 *
 * Some requests require authorization which can be set with
 * snapd_client_set_auth_data().
 */

/**
 * SnapdClient:
 *
 * #SnapdClient contains connection state with snapd.
 *
 * Since: 1.0
 */

/**
 * SnapdClientClass:
 *
 * Class structure for #SnapdClient.
 */

/**
 * SECTION:snapd-version
 * @short_description: Library version information
 * @include: snapd-glib/snapd-glib.h
 *
 * Programs can check if snapd-glib feature is enabled by checking for the
 * existance of a define called SNAPD_GLIB_VERSION_<version>, i.e.
 *
 * |[<!-- language="C" -->
 * #ifdef SNAPD_GLIB_VERSION_1_14
 * confinement = snapd_system_information_get_confinement (info);
 * #endif
 * ]|
 */

typedef struct
{
    /* Socket path to connect to */
    gchar *socket_path;

    /* Socket to communicate with snapd */
    GSocket *snapd_socket;

    /* User agent to send to snapd */
    gchar *user_agent;

    /* Authentication data to send with requests to snapd */
    SnapdAuthData *auth_data;

    /* Outstanding requests */
    GMutex requests_mutex;
    GList *requests;

    /* Whether to send the X-Allow-Interaction request header */
    gboolean allow_interaction;

    /* Data received from snapd */
    GMutex buffer_mutex;
    GByteArray *buffer;
    gsize n_read;
} SnapdClientPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

/* snapd API documentation is at https://github.com/snapcore/snapd/wiki/REST-API */

/* Default socket to connect to */
#define SNAPD_SOCKET "/run/snapd.socket"

/* Number of bytes to read at a time */
#define READ_SIZE 1024

/* Number of milliseconds to poll for status in asynchronous operations */
#define ASYNC_POLL_TIME 100

typedef struct
{
    GMainContext *context;

    SnapdClient *client;

    SoupMessage *message;

    GSource *read_source;

    GCancellable *cancellable;
    gulong cancelled_id;

    gboolean responded;
    GAsyncReadyCallback ready_callback;
    gpointer ready_callback_data;

    GError *error;
} SnapdRequestPrivate;

static void snapd_request_async_result_init (GAsyncResultIface *iface);

enum
{
    PROP_CANCELLABLE = 1,
    PROP_READY_CALLBACK,
    PROP_READY_CALLBACK_DATA,
    PROP_LAST
};

G_DEFINE_TYPE_WITH_CODE (SnapdRequest, snapd_request, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_RESULT, snapd_request_async_result_init)
                         G_ADD_PRIVATE (SnapdRequest))

struct _SnapdRequestAsync
{
    SnapdRequest parent_instance;
    gchar *change_id;
    GSource *poll_source;
    SnapdChange *change;
    gboolean sent_cancel;
    SnapdProgressCallback progress_callback;
    gpointer progress_callback_data;
    JsonNode *async_data;
};
G_DECLARE_FINAL_TYPE (SnapdRequestAsync, snapd_request_async, SNAPD, REQUEST_ASYNC, SnapdRequest)
G_DEFINE_TYPE (SnapdRequestAsync, snapd_request_async, snapd_request_get_type ())

struct _SnapdPostSnap
{
    SnapdRequestAsync parent_instance;
    gchar *name;
    gchar *action;
    SnapdInstallFlags flags;
    gchar *channel;
    gchar *revision;
};
G_DECLARE_FINAL_TYPE (SnapdPostSnap, snapd_post_snap, SNAPD, POST_SNAP, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostSnap, snapd_post_snap, snapd_request_async_get_type ())

struct _SnapdPostSnaps
{
    SnapdRequestAsync parent_instance;
    gchar *action;
};
G_DECLARE_FINAL_TYPE (SnapdPostSnaps, snapd_post_snaps, SNAPD, POST_SNAPS, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostSnaps, snapd_post_snaps, snapd_request_async_get_type ())

struct _SnapdPostSnapStream
{
    SnapdRequestAsync parent_instance;
    SnapdInstallFlags install_flags;
    GByteArray *snap_contents;
};
G_DECLARE_FINAL_TYPE (SnapdPostSnapStream, snapd_post_snap_stream, SNAPD, POST_SNAP_STREAM, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostSnapStream, snapd_post_snap_stream, snapd_request_async_get_type ())

struct _SnapdPostSnapTry
{
    SnapdRequestAsync parent_instance;
    gchar *path;
};
G_DECLARE_FINAL_TYPE (SnapdPostSnapTry, snapd_post_snap_try, SNAPD, POST_SNAP_TRY, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostSnapTry, snapd_post_snap_try, snapd_request_async_get_type ())

struct _SnapdPostAliases
{
    SnapdRequestAsync parent_instance;
    gchar *action;
    gchar *snap;
    gchar *app;
    gchar *alias;
};
G_DECLARE_FINAL_TYPE (SnapdPostAliases, snapd_post_aliases, SNAPD, POST_ALIASES, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostAliases, snapd_post_aliases, snapd_request_async_get_type ())

struct _SnapdPostInterfaces
{
    SnapdRequestAsync parent_instance;
    gchar *action;
    gchar *plug_snap;
    gchar *plug_name;
    gchar *slot_snap;
    gchar *slot_name;
};
G_DECLARE_FINAL_TYPE (SnapdPostInterfaces, snapd_post_interfaces, SNAPD, POST_INTERFACES, SnapdRequestAsync)
G_DEFINE_TYPE (SnapdPostInterfaces, snapd_post_interfaces, snapd_request_async_get_type ())

static void send_request (SnapdClient *client, SnapdRequest *request);

static gboolean
respond_cb (gpointer user_data)
{
    SnapdRequest *request = user_data;
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    if (priv->ready_callback != NULL)
        priv->ready_callback (G_OBJECT (priv->client), G_ASYNC_RESULT (request), priv->ready_callback_data);

    return G_SOURCE_REMOVE;
}

static void
snapd_request_respond (SnapdRequest *request, GError *error)
{
    g_autoptr(GSource) source = NULL;
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    if (priv->responded)
        return;
    priv->responded = TRUE;
    priv->error = error;

    source = g_idle_source_new ();
    g_source_set_callback (source, respond_cb, g_object_ref (request), g_object_unref);
    g_source_attach (source, priv->context);
}

static void
snapd_request_complete_unlocked (SnapdRequest *request, GError *error)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);
    SnapdClientPrivate *client_priv = snapd_client_get_instance_private (priv->client);

    if (priv->read_source != NULL)
        g_source_destroy (priv->read_source);
    g_clear_pointer (&priv->read_source, g_source_unref);

    snapd_request_respond (request, error);
    client_priv->requests = g_list_remove (client_priv->requests, request);
    g_object_unref (request);
}

static void
snapd_request_complete (SnapdRequest *request, GError *error)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);
    SnapdClientPrivate *client_priv = snapd_client_get_instance_private (priv->client);
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&client_priv->requests_mutex);
    snapd_request_complete_unlocked (request, error);
}

static gboolean
async_poll_cb (gpointer data)
{
    SnapdRequestAsync *request = data;
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));
    SnapdGetChange *change_request;

    change_request = _snapd_get_change_new (request->change_id, NULL, NULL, NULL);
    send_request (priv->client, SNAPD_REQUEST (change_request));

    request->poll_source = NULL;
    return G_SOURCE_REMOVE;
}

static void
schedule_poll (SnapdRequestAsync *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));
    if (request->poll_source != NULL)
        g_source_destroy (request->poll_source);
    request->poll_source = g_timeout_source_new (ASYNC_POLL_TIME);
    g_source_set_callback (request->poll_source, async_poll_cb, request, NULL);
    g_source_attach (request->poll_source, priv->context);
}

static void
complete_all_requests (SnapdClient *client, GError *error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    g_autoptr(GList) requests = NULL;
    GList *link;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    /* Disconnect socket - we will reconnect on demand */
    if (priv->snapd_socket != NULL)
        g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);

    /* Cancel synchronous requests (we'll never know the result); reschedule async ones (can reconnect to check result) */
    requests = g_list_copy (priv->requests);
    for (link = requests; link; link = link->next) {
        SnapdRequest *request = link->data;

        if (SNAPD_IS_REQUEST_ASYNC (request))
            schedule_poll (SNAPD_REQUEST_ASYNC (request));
        else
            snapd_request_complete_unlocked (request, g_error_copy (error));
    }
}

static gboolean
snapd_request_return_error (SnapdRequest *request, GError **error)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    if (priv->error != NULL) {
        g_propagate_error (error, priv->error);
        priv->error = NULL;
        return FALSE;
    }

    /* If no error provided from snapd, use a generic cancelled error */
    if (g_cancellable_set_error_if_cancelled (priv->cancellable, error))
        return FALSE;

    return TRUE;
}

static GObject *
snapd_get_source_object (GAsyncResult *result)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (result));
    return g_object_ref (priv->client);
}

static void
snapd_request_async_result_init (GAsyncResultIface *iface)
{
    iface->get_source_object = snapd_get_source_object;
}

static void
snapd_request_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdRequest *request = SNAPD_REQUEST (object);
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    switch (prop_id)
    {
    case PROP_CANCELLABLE:
        g_set_object (&priv->cancellable, g_value_get_object (value));
        break;
    case PROP_READY_CALLBACK:
        priv->ready_callback = g_value_get_pointer (value);
        break;
    case PROP_READY_CALLBACK_DATA:
        priv->ready_callback_data = g_value_get_pointer (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_request_finalize (GObject *object)
{
    SnapdRequest *request = SNAPD_REQUEST (object);
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    g_clear_object (&priv->message);
    if (priv->read_source != NULL)
        g_source_destroy (priv->read_source);
    g_clear_pointer (&priv->read_source, g_source_unref);
    g_cancellable_disconnect (priv->cancellable, priv->cancelled_id);
    g_clear_object (&priv->cancellable);
    g_clear_pointer (&priv->error, g_error_free);
    g_clear_pointer (&priv->context, g_main_context_unref);

    G_OBJECT_CLASS (snapd_request_parent_class)->finalize (object);
}

static void
snapd_request_class_init (SnapdRequestClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->set_property = snapd_request_set_property;
   gobject_class->finalize = snapd_request_finalize;

   g_object_class_install_property (gobject_class,
                                    PROP_CANCELLABLE,
                                    g_param_spec_object ("cancellable",
                                                         "cancellable",
                                                         "",
                                                         G_TYPE_CANCELLABLE,
                                                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_READY_CALLBACK,
                                    g_param_spec_pointer ("ready-callback",
                                                          "ready-callback",
                                                          "",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_READY_CALLBACK_DATA,
                                    g_param_spec_pointer ("ready-callback-data",
                                                          "ready-callback-data",
                                                          "",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_request_init (SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    priv->context = g_main_context_ref_thread_default ();
}

static void
append_string (GByteArray *array, const gchar *value)
{
    g_byte_array_append (array, (const guint8 *) value, strlen (value));
}

/* Converts a language in POSIX format and to be RFC2616 compliant */
static gchar *
posix_lang_to_rfc2616 (const gchar *language)
{
    /* Don't include charset variants, etc */
    if (strchr (language, '.') || strchr (language, '@'))
        return NULL;

    /* Ignore "C" locale, which g_get_language_names() always includes as a fallback. */
    if (strcmp (language, "C") == 0)
        return NULL;

    return g_strdelimit (g_ascii_strdown (language, -1), "_", '-');
}

/* Converts @quality from 0-100 to 0.0-1.0 and appends to @str */
static gchar *
add_quality_value (const gchar *str, int quality)
{
    g_return_val_if_fail (str != NULL, NULL);

    if (quality >= 0 && quality < 100) {
        /* We don't use %.02g because of "." vs "," locale issues */
        if (quality % 10)
            return g_strdup_printf ("%s;q=0.%02d", str, quality);
        else
            return g_strdup_printf ("%s;q=0.%d", str, quality / 10);
    } else
        return g_strdup (str);
}

/* Returns a RFC2616 compliant languages list from system locales */
/* Copied from libsoup */
static gchar *
get_accept_languages (void)
{
    const char * const * lang_names;
    g_autoptr(GPtrArray) langs = NULL;
    int delta;
    guint i;

    lang_names = g_get_language_names ();
    g_return_val_if_fail (lang_names != NULL, NULL);

    /* Build the array of languages */
    langs = g_ptr_array_new_with_free_func (g_free);
    for (i = 0; lang_names[i] != NULL; i++) {
        gchar *lang = posix_lang_to_rfc2616 (lang_names[i]);
        if (lang != NULL)
            g_ptr_array_add (langs, lang);
    }

    /* Add quality values */
    if (langs->len < 10)
        delta = 10;
    else if (langs->len < 20)
        delta = 5;
    else
        delta = 1;
    for (i = 0; i < langs->len; i++) {
        gchar *lang = langs->pdata[i];
        langs->pdata[i] = add_quality_value (lang, 100 - i * delta);
        g_free (lang);
    }

    /* Fallback to "en" if list is empty */
    if (langs->len == 0)
        return g_strdup ("en");

    g_ptr_array_add (langs, NULL);
    return g_strjoinv (", ", (char **)langs->pdata);
}

static SoupMessage *
generate_post_aliases_request (SnapdRequest *request)
{
    SnapdPostAliases *r = SNAPD_POST_ALIASES (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/aliases");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    if (r->snap != NULL) {
        json_builder_set_member_name (builder, "snap");
        json_builder_add_string_value (builder, r->snap);
    }
    if (r->app != NULL) {
        json_builder_set_member_name (builder, "app");
        json_builder_add_string_value (builder, r->app);
    }
    if (r->alias != NULL) {
        json_builder_set_member_name (builder, "alias");
        json_builder_add_string_value (builder, r->alias);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static SoupMessage *
generate_post_interfaces_request (SnapdRequest *request)
{
    SnapdPostInterfaces *r = SNAPD_POST_INTERFACES (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/interfaces");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    json_builder_set_member_name (builder, "plugs");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, r->plug_snap);
    json_builder_set_member_name (builder, "plug");
    json_builder_add_string_value (builder, r->plug_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_set_member_name (builder, "slots");
    json_builder_begin_array (builder);
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "snap");
    json_builder_add_string_value (builder, r->slot_snap);
    json_builder_set_member_name (builder, "slot");
    json_builder_add_string_value (builder, r->slot_name);
    json_builder_end_object (builder);
    json_builder_end_array (builder);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static gboolean
times_equal (GDateTime *time1, GDateTime *time2)
{
    if (time1 == NULL || time2 == NULL)
        return time1 == time2;
    return g_date_time_equal (time1, time2);
}

static gboolean
tasks_equal (SnapdTask *task1, SnapdTask *task2)
{
    return g_strcmp0 (snapd_task_get_id (task1), snapd_task_get_id (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_kind (task1), snapd_task_get_kind (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_summary (task1), snapd_task_get_summary (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_status (task1), snapd_task_get_status (task2)) == 0 &&
           g_strcmp0 (snapd_task_get_progress_label (task1), snapd_task_get_progress_label (task2)) == 0 &&
           snapd_task_get_progress_done (task1) == snapd_task_get_progress_done (task2) &&
           snapd_task_get_progress_total (task1) == snapd_task_get_progress_total (task2) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2)) &&
           times_equal (snapd_task_get_spawn_time (task1), snapd_task_get_spawn_time (task2));
}

static gboolean
changes_equal (SnapdChange *change1, SnapdChange *change2)
{
    GPtrArray *tasks1, *tasks2;

    if (change1 == NULL || change2 == NULL)
        return change1 == change2;

    tasks1 = snapd_change_get_tasks (change1);
    tasks2 = snapd_change_get_tasks (change2);
    if (tasks1 == NULL || tasks2 == NULL) {
        if (tasks1 != tasks2)
            return FALSE;
    }
    else {
        int i;

        if (tasks1->len != tasks2->len)
            return FALSE;
        for (i = 0; i < tasks1->len; i++) {
            SnapdTask *t1 = tasks1->pdata[i], *t2 = tasks2->pdata[i];
            if (!tasks_equal (t1, t2))
                return FALSE;
        }
    }

    return g_strcmp0 (snapd_change_get_id (change1), snapd_change_get_id (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_kind (change1), snapd_change_get_kind (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_summary (change1), snapd_change_get_summary (change2)) == 0 &&
           g_strcmp0 (snapd_change_get_status (change1), snapd_change_get_status (change2)) == 0 &&
           !!snapd_change_get_ready (change1) == !!snapd_change_get_ready (change2) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2)) &&
           times_equal (snapd_change_get_spawn_time (change1), snapd_change_get_spawn_time (change2));

    return TRUE;
}

static void
send_cancel (SnapdRequestAsync *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));
    g_autofree gchar *path = NULL;
    SnapdPostChange *change_request;

    if (request->sent_cancel)
        return;
    request->sent_cancel = TRUE;

    change_request = _snapd_post_change_new (request->change_id, "abort", NULL, NULL, NULL);

    send_request (priv->client, SNAPD_REQUEST (change_request));
}

static gboolean
parse_async_response (SnapdRequest *request, SoupMessage *message, GError **error)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);
    SnapdRequestAsync *r = SNAPD_REQUEST_ASYNC (request);
    g_autoptr(JsonObject) response = NULL;
    g_autofree gchar *change_id = NULL;

    response = _snapd_json_parse_response (message, error);
    if (response == NULL)
        return FALSE;
    change_id = _snapd_json_get_async_result (response, error);
    if (change_id == NULL)
        return FALSE;

    r->change_id = g_strdup (change_id);

    /* Immediately cancel if requested */
    if (g_cancellable_is_cancelled (priv->cancellable)) {
        send_cancel (r);
        return TRUE;
    }

    /* Poll for updates */
    schedule_poll (r);

    return TRUE;
}

static SnapdRequestAsync *
find_change_request (SnapdClient *client, const gchar *change_id)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GList *link;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (link = priv->requests; link; link = link->next) {
        SnapdRequest *request = link->data;

        if (SNAPD_IS_REQUEST_ASYNC (request) &&
            strcmp (SNAPD_REQUEST_ASYNC (request)->change_id, change_id) == 0)
            return SNAPD_REQUEST_ASYNC (request);
    }

    return NULL;
}

static SoupMessage *
generate_post_snap_request (SnapdRequest *request)
{
    SnapdPostSnap *r = SNAPD_POST_SNAP (request);
    g_autofree gchar *escaped = NULL, *path = NULL;
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    escaped = soup_uri_encode (r->name, NULL);
    path = g_strdup_printf ("http://snapd/v2/snaps/%s", escaped);
    message = soup_message_new ("POST", path);

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    if (r->channel != NULL) {
        json_builder_set_member_name (builder, "channel");
        json_builder_add_string_value (builder, r->channel);
    }
    if (r->revision != NULL) {
        json_builder_set_member_name (builder, "revision");
        json_builder_add_string_value (builder, r->revision);
    }
    if ((r->flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0) {
        json_builder_set_member_name (builder, "classic");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((r->flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0) {
        json_builder_set_member_name (builder, "dangerous");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((r->flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0) {
        json_builder_set_member_name (builder, "devmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    if ((r->flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0) {
        json_builder_set_member_name (builder, "jailmode");
        json_builder_add_boolean_value (builder, TRUE);
    }
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static SoupMessage *
generate_post_snaps_request (SnapdRequest *request)
{
    SnapdPostSnaps *r = SNAPD_POST_SNAPS (request);
    SoupMessage *message;
    g_autoptr(JsonBuilder) builder = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/snaps");

    builder = json_builder_new ();
    json_builder_begin_object (builder);
    json_builder_set_member_name (builder, "action");
    json_builder_add_string_value (builder, r->action);
    json_builder_end_object (builder);
    _snapd_json_set_body (message, builder);

    return message;
}

static void
append_multipart_value (SoupMultipart *multipart, const gchar *name, const gchar *value)
{
    g_autoptr(SoupMessageHeaders) headers = NULL;
    g_autoptr(GHashTable) params = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;

    headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_MULTIPART);
    params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup (name));
    soup_message_headers_set_content_disposition (headers, "form-data", params);
    buffer = soup_buffer_new_take ((guchar *) g_strdup (value), strlen (value));
    soup_multipart_append_part (multipart, headers, buffer);
}

static SoupMessage *
generate_post_snap_stream_request (SnapdRequest *request)
{
    SnapdPostSnapStream *r = SNAPD_POST_SNAP_STREAM (request);
    SoupMessage *message;
    g_autoptr(GHashTable) params = NULL;
    g_autoptr(SoupBuffer) buffer = NULL;
    g_autoptr(SoupMultipart) multipart = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/snaps");

    multipart = soup_multipart_new ("multipart/form-data");
    if ((r->install_flags & SNAPD_INSTALL_FLAGS_CLASSIC) != 0)
        append_multipart_value (multipart, "classic", "true");
    if ((r->install_flags & SNAPD_INSTALL_FLAGS_DANGEROUS) != 0)
        append_multipart_value (multipart, "dangerous", "true");
    if ((r->install_flags & SNAPD_INSTALL_FLAGS_DEVMODE) != 0)
        append_multipart_value (multipart, "devmode", "true");
    if ((r->install_flags & SNAPD_INSTALL_FLAGS_JAILMODE) != 0)
        append_multipart_value (multipart, "jailmode", "true");

    params = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_insert (params, g_strdup ("name"), g_strdup ("snap"));
    g_hash_table_insert (params, g_strdup ("filename"), g_strdup ("x"));
    soup_message_headers_set_content_disposition (message->request_headers, "form-data", params);
    soup_message_headers_set_content_type (message->request_headers, "application/vnd.snap", NULL);
    buffer = soup_buffer_new (SOUP_MEMORY_TEMPORARY, r->snap_contents->data, r->snap_contents->len);
    soup_multipart_append_part (multipart, message->request_headers, buffer);
    soup_multipart_to_message (multipart, message->request_headers, message->request_body);
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);

    return message;
}

static SoupMessage *
generate_post_snap_try_request (SnapdRequest *request)
{
    SnapdPostSnapTry *r = SNAPD_POST_SNAP_TRY (request);
    SoupMessage *message;
    g_autoptr(SoupMultipart) multipart = NULL;

    message = soup_message_new ("POST", "http://snapd/v2/snaps");

    multipart = soup_multipart_new ("multipart/form-data");
    append_multipart_value (multipart, "action", "try");
    append_multipart_value (multipart, "snap-path", r->path);
    soup_multipart_to_message (multipart, message->request_headers, message->request_body);
    soup_message_headers_set_content_length (message->request_headers, message->request_body->length);

    return message;
}

static SnapdRequest *
get_first_request (SnapdClient *client)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    GList *link;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&priv->requests_mutex);

    for (link = priv->requests; link; link = link->next) {
        SnapdRequest *request = link->data;

        /* Return first non-async request or async request without change id */
        if (SNAPD_IS_REQUEST_ASYNC (request)) {
            if (SNAPD_REQUEST_ASYNC (request)->change_id == NULL)
                return request;
        }
        else
            return request;
    }

    return NULL;
}

/* Check if we have all HTTP chunks */
static gboolean
have_chunked_body (const gchar *body, gsize body_length)
{
    while (TRUE) {
        const gchar *chunk_start;
        gsize chunk_header_length, chunk_length;

        /* Read chunk header, stopping on zero length chunk */
        chunk_start = g_strstr_len (body, body_length, "\r\n");
        if (chunk_start == NULL)
            return FALSE;
        chunk_header_length = chunk_start - body + 2;
        chunk_length = strtoul (body, NULL, 16);
        if (chunk_length == 0)
            return TRUE;

        /* Check enough space for chunk body */
        if (chunk_header_length + chunk_length + strlen ("\r\n") > body_length)
            return FALSE;
        // FIXME: Validate that \r\n is on the end of a chunk?
        body += chunk_header_length + chunk_length;
        body_length -= chunk_header_length + chunk_length;
    }
}

/* If more than one HTTP chunk, re-order buffer to contain one chunk.
 * Assumes body is a valid chunked data block (as checked with have_chunked_body()) */
static void
compress_chunks (gchar *body, gsize body_length, gchar **combined_start, gsize *combined_length, gsize *total_length)
{
    gchar *chunk_start;

    /* Use first chunk as output */
    *combined_length = strtoul (body, NULL, 16);
    *combined_start = strstr (body, "\r\n") + 2;

    /* Copy any remaining chunks beside the first one */
    chunk_start = *combined_start + *combined_length + 2;
    while (TRUE) {
        gsize chunk_length;

        chunk_length = strtoul (chunk_start, NULL, 16);
        chunk_start = strstr (chunk_start, "\r\n") + 2;
        if (chunk_length == 0)
            break;

        /* Move this chunk on the end of the last one */
        memmove (*combined_start + *combined_length, chunk_start, chunk_length);
        *combined_length += chunk_length;

        chunk_start += chunk_length + 2;
    }

    *total_length = chunk_start - body;
}

static void
complete_change (SnapdClient *client, const gchar *change_id, GError *error)
{
    SnapdRequestAsync *request;

    request = find_change_request (client, change_id);
    if (request != NULL)
        snapd_request_complete (SNAPD_REQUEST (request), error);
}

static void
update_changes (SnapdClient *client, SnapdChange *change, JsonNode *data, const gchar *err)
{
    SnapdRequestAsync *request;
    SnapdRequestPrivate *priv;

    request = find_change_request (client, snapd_change_get_id (change));
    if (request == NULL)
        return;
    priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));

    /* Update caller with progress */
    if (!changes_equal (request->change, change)) {
        g_set_object (&request->change, change);
        // NOTE: tasks is passed for ABI compatibility - this field is
        // deprecated and can be accessed with snapd_change_get_tasks ()
        if (request->progress_callback != NULL)
            request->progress_callback (client, change, snapd_change_get_tasks (change), request->progress_callback_data);
    }

    /* Complete parent */
    if (snapd_change_get_ready (change)) {
        GError *error = NULL;

        if (data != NULL)
            request->async_data = json_node_ref (data); // FIXME: Use vfunc

        if (!g_cancellable_set_error_if_cancelled (priv->cancellable, &error) &&
            err != NULL)
            g_set_error_literal (&error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_FAILED,
                                 err);

        snapd_request_complete (SNAPD_REQUEST (request), error);
        return;
    }

    /* Poll for updates */
    schedule_poll (request);
}

static void
parse_response (SnapdClient *client, SnapdRequest *request, SoupMessage *message)
{
    GError *error = NULL;

    if (!SNAPD_REQUEST_GET_CLASS (request)->parse_response (request, message, &error)) {
        if (SNAPD_IS_GET_CHANGE (request)) {
            complete_change (client, _snapd_get_change_get_change_id (SNAPD_GET_CHANGE (request)), error);
            snapd_request_complete (request, NULL);
        }
        else if (SNAPD_IS_POST_CHANGE (request)) {
            complete_change (client, _snapd_post_change_get_change_id (SNAPD_POST_CHANGE (request)), error);
            snapd_request_complete (request, NULL);
        }
        else
            snapd_request_complete (request, error);
        return;
    }

    if (SNAPD_IS_GET_CHANGE (request))
        update_changes (client,
                        _snapd_get_change_get_change (SNAPD_GET_CHANGE (request)),
                        _snapd_get_change_get_data (SNAPD_GET_CHANGE (request)),
                        _snapd_get_change_get_err (SNAPD_GET_CHANGE (request)));
    else if (SNAPD_IS_POST_CHANGE (request))
        update_changes (client,
                        _snapd_post_change_get_change (SNAPD_POST_CHANGE (request)),
                        _snapd_post_change_get_data (SNAPD_POST_CHANGE (request)),
                        _snapd_post_change_get_err (SNAPD_POST_CHANGE (request)));

    if (!SNAPD_IS_REQUEST_ASYNC (request))
        snapd_request_complete (request, NULL);
}

static gboolean
read_cb (GSocket *socket, GIOCondition condition, SnapdRequest *r)
{
    SnapdRequestPrivate *r_priv = snapd_request_get_instance_private (r);
    SnapdClientPrivate *client_priv = snapd_client_get_instance_private (r_priv->client);
    gssize n_read;
    gchar *body;
    gsize header_length;
    g_autoptr(SoupMessageHeaders) headers = NULL;
    gchar *combined_start;
    gsize content_length, combined_length;
    g_autoptr(GError) error = NULL;
    g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&client_priv->buffer_mutex);

    if (client_priv->n_read + READ_SIZE > client_priv->buffer->len)
        g_byte_array_set_size (client_priv->buffer, client_priv->n_read + READ_SIZE);
    n_read = g_socket_receive (socket,
                               (gchar *) (client_priv->buffer->data + client_priv->n_read),
                               READ_SIZE,
                               NULL,
                               &error);

    if (n_read == 0) {
        g_autoptr(GError) e = NULL;

        e = g_error_new (SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "snapd connection closed");
        complete_all_requests (r_priv->client, e);

        r_priv->read_source = NULL;
        return G_SOURCE_REMOVE;
    }

    if (n_read < 0) {
        g_autoptr(GError) e = NULL;

        if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
            return TRUE;

        e = g_error_new (SNAPD_ERROR,
                         SNAPD_ERROR_READ_FAILED,
                         "Failed to read from snapd: %s",
                         error->message);
        complete_all_requests (r_priv->client, e);

        r_priv->read_source = NULL;
        return G_SOURCE_REMOVE;
    }

    client_priv->n_read += n_read;

    while (TRUE) {
        SnapdRequest *request;
        SnapdRequestPrivate *priv;
        g_autoptr(GError) e = NULL;

        /* Look for header divider */
        body = g_strstr_len ((gchar *) client_priv->buffer->data, client_priv->n_read, "\r\n\r\n");
        if (body == NULL)
            return G_SOURCE_CONTINUE;
        body += 4;
        header_length = body - (gchar *) client_priv->buffer->data;

        /* Match this response to the next uncompleted request */
        request = get_first_request (r_priv->client);
        if (request == NULL) {
            g_warning ("Ignoring unexpected response");
            return G_SOURCE_REMOVE;
        }
        priv = snapd_request_get_instance_private (request);

        /* Parse headers */
        if (!soup_headers_parse_response ((gchar *) client_priv->buffer->data, header_length, priv->message->response_headers,
                                          NULL, &priv->message->status_code, &priv->message->reason_phrase)) {
            e = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Failed to parse headers from snapd");
            complete_all_requests (priv->client, e);
            priv->read_source = NULL;
            return G_SOURCE_REMOVE;
        }

        /* Read content and process content */
        switch (soup_message_headers_get_encoding (priv->message->response_headers)) {
        case SOUP_ENCODING_EOF:
            if (!g_socket_is_closed (client_priv->snapd_socket))
                return G_SOURCE_CONTINUE;

            content_length = client_priv->n_read - header_length;
            soup_message_body_append (priv->message->response_body, SOUP_MEMORY_COPY, body, content_length);
            parse_response (priv->client, request, priv->message);
            break;

        case SOUP_ENCODING_CHUNKED:
            // FIXME: Find a way to abort on error
            if (!have_chunked_body (body, client_priv->n_read - header_length))
                return G_SOURCE_CONTINUE;

            compress_chunks (body, client_priv->n_read - header_length, &combined_start, &combined_length, &content_length);
            soup_message_body_append (priv->message->response_body, SOUP_MEMORY_COPY, combined_start, combined_length);
            parse_response (priv->client, request, priv->message);
            break;

        case SOUP_ENCODING_CONTENT_LENGTH:
            content_length = soup_message_headers_get_content_length (priv->message->response_headers);
            if (client_priv->n_read < header_length + content_length)
                return G_SOURCE_CONTINUE;

            soup_message_body_append (priv->message->response_body, SOUP_MEMORY_COPY, body, content_length);
            parse_response (priv->client, request, priv->message);
            break;

        default:
            e = g_error_new (SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unable to determine header encoding");
            complete_all_requests (priv->client, e);
            priv->read_source = NULL;
            return G_SOURCE_REMOVE;
        }

        /* Move remaining data to the start of the buffer */
        g_byte_array_remove_range (client_priv->buffer, 0, header_length + content_length);
        client_priv->n_read -= header_length + content_length;
    }
}

static void
request_cancelled_cb (GCancellable *cancellable, SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    /* Asynchronous requests require asking snapd to stop them */
    if (SNAPD_IS_REQUEST_ASYNC (request)) {
        SnapdRequestAsync *r = SNAPD_REQUEST_ASYNC (request);

        /* Cancel if we have got a response from snapd */
        if (r->change_id != NULL)
            send_cancel (r);
    }
    else {
        GError *error = NULL;
        g_cancellable_set_error_if_cancelled (priv->cancellable, &error);
        snapd_request_respond (request, error);
    }
}

static void
send_request (SnapdClient *client, SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);
    SnapdClientPrivate *client_priv = snapd_client_get_instance_private (client);
    g_autofree gchar *accept_languages = NULL;
    g_autoptr(GByteArray) request_data = NULL;
    SoupURI *uri;
    SoupMessageHeadersIter iter;
    const char *name, *value;
    g_autoptr(SoupBuffer) buffer = NULL;
    gssize n_written;
    g_autoptr(GError) local_error = NULL;

    // NOTE: Would love to use libsoup but it doesn't support unix sockets
    // https://bugzilla.gnome.org/show_bug.cgi?id=727563

    priv->client = client;

    {
        g_autoptr(GMutexLocker) locker = g_mutex_locker_new (&client_priv->requests_mutex);
        client_priv->requests = g_list_append (client_priv->requests, request);
    }

    if (priv->cancellable != NULL)
        priv->cancelled_id = g_cancellable_connect (priv->cancellable, G_CALLBACK (request_cancelled_cb), request, NULL);

    priv->message = SNAPD_REQUEST_GET_CLASS (request)->generate_request (request);
    soup_message_headers_append (priv->message->request_headers, "Host", "");
    soup_message_headers_append (priv->message->request_headers, "Connection", "keep-alive");
    if (client_priv->user_agent != NULL)
        soup_message_headers_append (priv->message->request_headers, "User-Agent", client_priv->user_agent);
    if (client_priv->allow_interaction)
        soup_message_headers_append (priv->message->request_headers, "X-Allow-Interaction", "true");

    accept_languages = get_accept_languages ();
    soup_message_headers_append (priv->message->request_headers, "Accept-Language", accept_languages);

    if (client_priv->auth_data != NULL) {
        g_autoptr(GString) authorization = NULL;
        gchar **discharges;
        gsize i;

        authorization = g_string_new ("");
        g_string_append_printf (authorization, "Macaroon root=\"%s\"", snapd_auth_data_get_macaroon (client_priv->auth_data));
        discharges = snapd_auth_data_get_discharges (client_priv->auth_data);
        if (discharges != NULL)
            for (i = 0; discharges[i] != NULL; i++)
                g_string_append_printf (authorization, ",discharge=\"%s\"", discharges[i]);
        soup_message_headers_append (priv->message->request_headers, "Authorization", authorization->str);
    }

    request_data = g_byte_array_new ();
    append_string (request_data, priv->message->method);
    append_string (request_data, " ");
    uri = soup_message_get_uri (priv->message);
    append_string (request_data, uri->path);
    if (uri->query != NULL) {
        append_string (request_data, "?");
        append_string (request_data, uri->query);
    }
    append_string (request_data, " HTTP/1.1\r\n");
    soup_message_headers_iter_init (&iter, priv->message->request_headers);
    while (soup_message_headers_iter_next (&iter, &name, &value)) {
        append_string (request_data, name);
        append_string (request_data, ": ");
        append_string (request_data, value);
        append_string (request_data, "\r\n");
    }
    append_string (request_data, "\r\n");

    buffer = soup_message_body_flatten (priv->message->request_body);
    g_byte_array_append (request_data, (const guint8 *) buffer->data, buffer->length);

    if (client_priv->snapd_socket == NULL) {
        g_autoptr(GSocketAddress) address = NULL;
        g_autoptr(GError) error_local = NULL;

        client_priv->snapd_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                                  G_SOCKET_TYPE_STREAM,
                                                  G_SOCKET_PROTOCOL_DEFAULT,
                                                  &error_local);
        if (client_priv->snapd_socket == NULL) {
            GError *error = g_error_new (SNAPD_ERROR,
                                         SNAPD_ERROR_CONNECTION_FAILED,
                                         "Unable to create snapd socket: %s",
                                         error_local->message);
            snapd_request_complete (request, error);
            return;
        }
        g_socket_set_blocking (client_priv->snapd_socket, FALSE);
        address = g_unix_socket_address_new (client_priv->socket_path);
        if (!g_socket_connect (client_priv->snapd_socket, address, priv->cancellable, &error_local)) {
            g_clear_object (&client_priv->snapd_socket);
            GError *error = g_error_new (SNAPD_ERROR,
                                         SNAPD_ERROR_CONNECTION_FAILED,
                                         "Unable to connect snapd socket: %s",
                                         error_local->message);
            snapd_request_complete (request, error);
            return;
        }
    }

    priv->read_source = g_socket_create_source (client_priv->snapd_socket, G_IO_IN, NULL);
    g_source_set_name (priv->read_source, "snapd-glib-read-source");
    g_source_set_callback (priv->read_source, (GSourceFunc) read_cb, request, NULL);
    g_source_attach (priv->read_source, priv->context);

    /* send HTTP request */
    // FIXME: Check for short writes
    n_written = g_socket_send (client_priv->snapd_socket, (const gchar *) request_data->data, request_data->len, priv->cancellable, &local_error);
    if (n_written < 0) {
        GError *error = g_error_new (SNAPD_ERROR,
                                     SNAPD_ERROR_WRITE_FAILED,
                                     "Failed to write to snapd: %s",
                                     local_error->message);
        snapd_request_complete (request, error);
    }
}

/**
 * snapd_client_connect_async:
 * @client: a #SnapdClient
 * @cancellable: (allow-none): a #GCancellable or %NULL
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * This method is no longer required and does nothing, snapd-glib now connects on demand.
 *
 * Since: 1.3
 * Deprecated: 1.24
 */
void
snapd_client_connect_async (SnapdClient *client,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(GTask) task = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_return_boolean (task, TRUE);
}

/**
 * snapd_client_connect_finish:
 * @client: a #SnapdClient
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_connect_async().
 * See snapd_client_connect_sync() for more information.
 *
 * Returns: %TRUE if successfully connected to snapd.
 *
 * Since: 1.3
 */
gboolean
snapd_client_connect_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, client), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_set_socket_path:
 * @client: a #SnapdClient
 * @socket_path: (allow-none): a socket path or %NULL to reset to the default.
 *
 * Set the Unix socket path to connect to snapd with.
 * Defaults to the system socket.
 *
 * Since: 1.24
 */
void
snapd_client_set_socket_path (SnapdClient *client, const gchar *socket_path)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_free (priv->socket_path);
    if (priv->socket_path != NULL)
        priv->socket_path = g_strdup (socket_path);
    else
        priv->socket_path = g_strdup (SNAPD_SOCKET);
}

/**
 * snapd_client_get_socket_path:
 * @client: a #SnapdClient
 *
 * Get the unix socket path to connect to snapd with.
 *
 * Returns: socket path.
 *
 * Since: 1.24
 */
const gchar *
snapd_client_get_socket_path (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);
    return priv->socket_path;
}

/**
 * snapd_client_set_user_agent:
 * @client: a #SnapdClient
 * @user_agent: (allow-none): a user agent or %NULL.
 *
 * Set the HTTP user-agent that is sent with each request to snapd.
 * Defaults to "snapd-glib/VERSION".
 *
 * Since: 1.16
 */
void
snapd_client_set_user_agent (SnapdClient *client, const gchar *user_agent)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_free (priv->user_agent);
    priv->user_agent = g_strdup (user_agent);
}

/**
 * snapd_client_get_user_agent:
 * @client: a #SnapdClient
 *
 * Get the HTTP user-agent that is sent with each request to snapd.
 *
 * Returns: user agent or %NULL if none set.
 *
 * Since: 1.16
 */
const gchar *
snapd_client_get_user_agent (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);
    return priv->user_agent;
}

/**
 * snapd_client_set_allow_interaction:
 * @client: a #SnapdClient
 * @allow_interaction: whether to allow interaction.
 *
 * Set whether snapd operations are allowed to interact with the user.
 * This affects operations that use polkit authorisation.
 * Defaults to TRUE.
 *
 * Since: 1.19
 */
void
snapd_client_set_allow_interaction (SnapdClient *client, gboolean allow_interaction)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    priv->allow_interaction = allow_interaction;
}

/**
 * snapd_client_get_allow_interaction:
 * @client: a #SnapdClient
 *
 * Get whether snapd operations are allowed to interact with the user.
 *
 * Returns: %TRUE if interaction is allowed.
 *
 * Since: 1.19
 */
gboolean
snapd_client_get_allow_interaction (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);

    priv = snapd_client_get_instance_private (client);
    return priv->allow_interaction;
}

/**
 * snapd_client_login_async:
 * @client: a #SnapdClient.
 * @username: usename to log in with.
 * @password: password to log in with.
 * @otp: (allow-none): response to one-time password challenge.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get authorization to install/remove snaps.
 * See snapd_client_login_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_login_async (SnapdClient *client,
                          const gchar *username, const gchar *password, const gchar *otp,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostLogin *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_post_login_new (username, password, otp, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_login_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_login_async().
 * See snapd_client_login_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdAuthData or %NULL on error.
 *
 * Since: 1.0
 */
SnapdAuthData *
snapd_client_login_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdPostLogin *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_LOGIN (result), NULL);

    request = SNAPD_POST_LOGIN (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_post_login_get_auth_data (request));
}

/**
 * snapd_client_set_auth_data:
 * @client: a #SnapdClient.
 * @auth_data: (allow-none): a #SnapdAuthData or %NULL.
 *
 * Set the authorization data to use for requests. Authorization data can be
 * obtained by:
 *
 * - Logging into snapd using snapd_login_sync() or snapd_client_login_sync()
 *   (requires root access)
 *
 * - Using an existing authorization with snapd_auth_data_new().
 *
 * Since: 1.0
 */
void
snapd_client_set_auth_data (SnapdClient *client, SnapdAuthData *auth_data)
{
    SnapdClientPrivate *priv;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    priv = snapd_client_get_instance_private (client);
    g_clear_object (&priv->auth_data);
    if (auth_data != NULL)
        priv->auth_data = g_object_ref (auth_data);
}

/**
 * snapd_client_get_auth_data:
 * @client: a #SnapdClient.
 *
 * Get the authorization data that is used for requests.
 *
 * Returns: (transfer none) (allow-none): a #SnapdAuthData or %NULL.
 *
 * Since: 1.0
 */
SnapdAuthData *
snapd_client_get_auth_data (SnapdClient *client)
{
    SnapdClientPrivate *priv;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);

    priv = snapd_client_get_instance_private (client);

    return priv->auth_data;
}

/**
 * snapd_client_get_system_information_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Request system information asynchronously from snapd.
 * See snapd_client_get_system_information_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_system_information_async (SnapdClient *client,
                                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSystemInfo *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_system_info_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_system_information_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_system_information_async().
 * See snapd_client_get_system_information_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdSystemInformation or %NULL on error.
 *
 * Since: 1.0
 */
SnapdSystemInformation *
snapd_client_get_system_information_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetSystemInfo *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SYSTEM_INFO (result), NULL);

    request = SNAPD_GET_SYSTEM_INFO (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_system_info_get_system_information (request));
}

/**
 * snapd_client_list_one_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information of a single installed snap.
 * See snapd_client_list_one_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_list_one_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_snap_new (name, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_list_one_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_list_one_async().
 * See snapd_client_list_one_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
SnapdSnap *
snapd_client_list_one_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetSnap *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SNAP (result), NULL);

    request = SNAPD_GET_SNAP (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_snap_get_snap (request));
}

/**
 * snapd_client_get_apps_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdGetAppsFlags to control what results are returned.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on installed apps.
 * See snapd_client_get_apps_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_get_apps_async (SnapdClient *client,
                             SnapdGetAppsFlags flags,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetApps *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_apps_new (cancellable, callback, user_data);
    if ((flags & SNAPD_GET_APPS_FLAGS_SELECT_SERVICES) != 0)
        _snapd_get_apps_set_select (request, "service");
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_apps_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_apps_async().
 * See snapd_client_get_apps_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdApp): an array of #SnapdApp or %NULL on error.
 *
 * Since: 1.25
 */
GPtrArray *
snapd_client_get_apps_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetApps *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_APPS (result), NULL);

    request = SNAPD_GET_APPS (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_apps_get_apps (request));
}

/**
 * snapd_client_get_icon_async:
 * @client: a #SnapdClient.
 * @name: name of snap to get icon for.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the icon for an installed snap.
 * See snapd_client_get_icon_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_icon_async (SnapdClient *client,
                             const gchar *name,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetIcon *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_icon_new (name, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_icon_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_icon_async().
 * See snapd_client_get_icon_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdIcon or %NULL on error.
 *
 * Since: 1.0
 */
SnapdIcon *
snapd_client_get_icon_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetIcon *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ICON (result), NULL);

    request = SNAPD_GET_ICON (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_get_icon_get_icon (request));
}

/**
 * snapd_client_list_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get information on all installed snaps.
 * See snapd_client_list_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_list_async (SnapdClient *client,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSnaps *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_snaps_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_list_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_list_async().
 * See snapd_client_list_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_client_list_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetSnaps *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SNAPS (result), NULL);

    request = SNAPD_GET_SNAPS (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_snaps_get_snaps (request));
}

/**
 * snapd_client_get_assertions_async:
 * @client: a #SnapdClient.
 * @type: assertion type to get.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get assertions.
 * See snapd_client_get_assertions_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_get_assertions_async (SnapdClient *client,
                                   const gchar *type,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetAssertions *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (type != NULL);

    request = _snapd_get_assertions_new (type, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_assertions_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_assertions_async().
 * See snapd_client_get_assertions_sync() for more information.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of assertions or %NULL on error.
 *
 * Since: 1.8
 */
gchar **
snapd_client_get_assertions_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetAssertions *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ASSERTIONS (result), NULL);

    request = SNAPD_GET_ASSERTIONS (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_strdupv (_snapd_get_assertions_get_assertions (request));
}

/**
 * snapd_client_add_assertions_async:
 * @client: a #SnapdClient.
 * @assertions: assertions to add.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously add an assertion.
 * See snapd_client_add_assertions_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_add_assertions_async (SnapdClient *client,
                                   gchar **assertions,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostAssertions *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (assertions != NULL);

    request = _snapd_post_assertions_new (assertions, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_add_assertions_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_add_assertions_async().
 * See snapd_client_add_assertions_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_add_assertions_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ASSERTIONS (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_get_interfaces_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the installed snap interfaces.
 * See snapd_client_get_interfaces_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_get_interfaces_async (SnapdClient *client,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetInterfaces *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_interfaces_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_interfaces_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @plugs: (out) (allow-none) (transfer container) (element-type SnapdPlug): the location to store the array of #SnapdPlug or %NULL.
 * @slots: (out) (allow-none) (transfer container) (element-type SnapdSlot): the location to store the array of #SnapdSlot or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_interfaces_async().
 * See snapd_client_get_interfaces_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_get_interfaces_finish (SnapdClient *client, GAsyncResult *result,
                                    GPtrArray **plugs, GPtrArray **slots,
                                    GError **error)
{
    SnapdGetInterfaces *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_INTERFACES (result), FALSE);

    request = SNAPD_GET_INTERFACES (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return FALSE;
    if (plugs)
       *plugs = g_ptr_array_ref (_snapd_get_interfaces_get_plugs (request));
    if (slots)
       *slots = g_ptr_array_ref (_snapd_get_interfaces_get_slots (request));
    return TRUE;
}

static void
send_interface_request (SnapdClient *client,
                        const gchar *action,
                        const gchar *plug_snap, const gchar *plug_name,
                        const gchar *slot_snap, const gchar *slot_name,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostInterfaces *request;

    request = SNAPD_POST_INTERFACES (g_object_new (snapd_post_interfaces_get_type (),
                                                   "cancellable", cancellable,
                                                   "ready-callback", callback,
                                                   "ready-callback-data", user_data,
                                                   NULL));
    request->action = g_strdup (action);
    request->plug_snap = g_strdup (plug_snap);
    request->plug_name = g_strdup (plug_name);
    request->slot_snap = g_strdup (slot_snap);
    request->slot_name = g_strdup (slot_name);
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_connect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to connect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to connect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously connect two interfaces together.
 * See snapd_client_connect_interface_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_connect_interface_async (SnapdClient *client,
                                      const gchar *plug_snap, const gchar *plug_name,
                                      const gchar *slot_snap, const gchar *slot_name,
                                      SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                      GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));

    send_interface_request (client,
                            "connect",
                            plug_snap, plug_name,
                            slot_snap, slot_name,
                            progress_callback, progress_callback_data,
                            cancellable, callback, user_data);
}

/**
 * snapd_client_connect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_connect_interface_async().
 * See snapd_client_connect_interface_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_connect_interface_finish (SnapdClient *client,
                                       GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_INTERFACES (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_disconnect_interface_async:
 * @client: a #SnapdClient.
 * @plug_snap: name of snap containing plug.
 * @plug_name: name of plug to disconnect.
 * @slot_snap: name of snap containing socket.
 * @slot_name: name of slot to disconnect.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously disconnect two interfaces.
 * See snapd_client_disconnect_interface_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_disconnect_interface_async (SnapdClient *client,
                                         const gchar *plug_snap, const gchar *plug_name,
                                         const gchar *slot_snap, const gchar *slot_name,
                                         SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));

    send_interface_request (client,
                            "disconnect",
                            plug_snap, plug_name,
                            slot_snap, slot_name,
                            progress_callback, progress_callback_data,
                            cancellable, callback, user_data);
}

/**
 * snapd_client_disconnect_interface_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disconnect_interface_async().
 * See snapd_client_disconnect_interface_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disconnect_interface_finish (SnapdClient *client,
                                          GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_INTERFACES (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_find_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @query: query string to send.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_find_async (SnapdClient *client,
                         SnapdFindFlags flags, const gchar *query,
                         GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (query != NULL);
    snapd_client_find_section_async (client, flags, NULL, query, cancellable, callback, user_data);
}

/**
 * snapd_client_find_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.0
 */
GPtrArray *
snapd_client_find_finish (SnapdClient *client, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    return snapd_client_find_section_finish (client, result, suggested_currency, error);
}

/**
 * snapd_client_find_section_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdFindFlags to control how the find is performed.
 * @section: (allow-none): store section to search in or %NULL to search in all sections.
 * @query: (allow-none): query string to send or %NULL to get all snaps from the given section.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in the store.
 * See snapd_client_find_section_sync() for more information.
 *
 * Since: 1.7
 */
void
snapd_client_find_section_async (SnapdClient *client,
                                 SnapdFindFlags flags, const gchar *section, const gchar *query,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetFind *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (section != NULL || query != NULL);

    request = _snapd_get_find_new (cancellable, callback, user_data);
    if ((flags & SNAPD_FIND_FLAGS_MATCH_NAME) != 0)
        _snapd_get_find_set_name (request, query);
    else
        _snapd_get_find_set_query (request, query);
    if ((flags & SNAPD_FIND_FLAGS_SELECT_PRIVATE) != 0)
        _snapd_get_find_set_select (request, "private");
    else if ((flags & SNAPD_FIND_FLAGS_SELECT_REFRESH) != 0)
        _snapd_get_find_set_select (request, "refresh");
    _snapd_get_find_set_section (request, section);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_find_section_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @suggested_currency: (allow-none): location to store the ISO 4217 currency that is suggested to purchase with.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_async().
 * See snapd_client_find_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.7
 */
GPtrArray *
snapd_client_find_section_finish (SnapdClient *client, GAsyncResult *result, gchar **suggested_currency, GError **error)
{
    SnapdGetFind *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_FIND (result), NULL);

    request = SNAPD_GET_FIND (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;

    if (suggested_currency != NULL)
        *suggested_currency = g_strdup (_snapd_get_find_get_suggested_currency (request));
    return g_ptr_array_ref (_snapd_get_find_get_snaps (request));
}

/**
 * snapd_client_find_refreshable_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously find snaps in store that are newer revisions than locally installed versions.
 * See snapd_client_find_refreshable_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_find_refreshable_async (SnapdClient *client,
                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetFind *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_find_new (cancellable, callback, user_data);
    _snapd_get_find_set_select (request, "refresh");
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_find_refreshable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_find_refreshable_async().
 * See snapd_client_find_refreshable_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdSnap): an array of #SnapdSnap or %NULL on error.
 *
 * Since: 1.5
 */
GPtrArray *
snapd_client_find_refreshable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetFind *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_FIND (result), NULL);

    request = SNAPD_GET_FIND (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;

    return g_ptr_array_ref (_snapd_get_find_get_snaps (request));
}

/**
 * snapd_client_install_async:
 * @client: a #SnapdClient.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap from the store.
 * See snapd_client_install_sync() for more information.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use snapd_client_install2_async()
 */
void
snapd_client_install_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    snapd_client_install2_async (client, SNAPD_INSTALL_FLAGS_NONE, name, channel, NULL, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_install_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install_async().
 * See snapd_client_install_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 * Deprecated: 1.12: Use snapd_client_install2_finish()
 */
gboolean
snapd_client_install_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    return snapd_client_install2_finish (client, result, error);
}

/**
 * snapd_client_install2_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @name: name of snap to install.
 * @channel: (allow-none): channel to install from or %NULL for default.
 * @revision: (allow-none): revision to install or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap from the store.
 * See snapd_client_install2_sync() for more information.
 *
 * Since: 1.12
 */
void
snapd_client_install2_async (SnapdClient *client,
                             SnapdInstallFlags flags,
                             const gchar *name, const gchar *channel, const gchar *revision,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
    request->flags = flags;
    request->name = g_strdup (name);
    request->action = g_strdup ("install");
    request->channel = g_strdup (channel);
    request->revision = g_strdup (revision);
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_install2_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install2_async().
 * See snapd_client_install2_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.12
 */
gboolean
snapd_client_install2_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

typedef struct
{
    SnapdClient *client;
    SnapdPostSnapStream *request;
    GCancellable *cancellable;
    GInputStream *stream;
} InstallStreamData;

static InstallStreamData *
install_stream_data_new (SnapdClient *client, SnapdPostSnapStream *request, GCancellable *cancellable, GInputStream *stream)
{
    InstallStreamData *data;

    data = g_slice_new (InstallStreamData);
    data->client = g_object_ref (client);
    data->request = g_object_ref (request);
    data->cancellable = cancellable != NULL ? g_object_ref (cancellable) : NULL;
    data->stream = g_object_ref (stream);

    return data;
}

static void
install_stream_data_free (InstallStreamData *data)
{
    g_object_unref (data->client);
    g_object_unref (data->request);
    if (data->cancellable != NULL)
        g_object_unref (data->cancellable);
    g_object_unref (data->stream);
    g_slice_free (InstallStreamData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC(InstallStreamData, install_stream_data_free)

static void
stream_read_cb (GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    g_autoptr(InstallStreamData) data = user_data;
    g_autoptr(GBytes) read_data = NULL;
    g_autoptr(GError) error = NULL;

    read_data = g_input_stream_read_bytes_finish (data->stream, result, &error);
    if (!snapd_request_return_error (SNAPD_REQUEST (data->request), &error))
        return;

    if (g_bytes_get_size (read_data) == 0)
        send_request (data->client, SNAPD_REQUEST (data->request));
    else {
        InstallStreamData *d;
        g_byte_array_append (data->request->snap_contents, g_bytes_get_data (read_data, NULL), g_bytes_get_size (read_data));
        d = g_steal_pointer (&data);
        g_input_stream_read_bytes_async (d->stream, 65535, G_PRIORITY_DEFAULT, d->cancellable, stream_read_cb, d);
    }
}

/**
 * snapd_client_install_stream_async:
 * @client: a #SnapdClient.
 * @flags: a set of #SnapdInstallFlags to control install options.
 * @stream: a #GInputStream containing the snap file contents to install.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously install a snap.
 * See snapd_client_install_stream_sync() for more information.
 *
 * Since: 1.9
 */
void
snapd_client_install_stream_async (SnapdClient *client,
                                   SnapdInstallFlags flags,
                                   GInputStream *stream,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapStream *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (G_IS_INPUT_STREAM (stream));

    request = SNAPD_POST_SNAP_STREAM (g_object_new (snapd_post_snap_stream_get_type (),
                                                    "cancellable", cancellable,
                                                    "ready-callback", callback,
                                                    "ready-callback-data", user_data,
                                                    NULL));
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;
    request->install_flags = flags;
    request->snap_contents = g_byte_array_new ();
    g_input_stream_read_bytes_async (stream, 65535, G_PRIORITY_DEFAULT, cancellable, stream_read_cb, install_stream_data_new (client, request, cancellable, stream));
}

/**
 * snapd_client_install_stream_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_install_stream_async().
 * See snapd_client_install_stream_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_install_stream_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP_STREAM (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_try_async:
 * @client: a #SnapdClient.
 * @path: path to snap directory to try.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously try a snap.
 * See snapd_client_try_sync() for more information.
 *
 * Since: 1.9
 */
void
snapd_client_try_async (SnapdClient *client,
                        const gchar *path,
                        SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapTry *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (path != NULL);

    request = SNAPD_POST_SNAP_TRY (g_object_new (snapd_post_snap_try_get_type (),
                                                 "cancellable", cancellable,
                                                 "ready-callback", callback,
                                                 "ready-callback-data", user_data,
                                                 NULL));
    request->path = g_strdup (path);
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_try_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_try_async().
 * See snapd_client_try_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.9
 */
gboolean
snapd_client_try_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP_TRY (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_refresh_async:
 * @client: a #SnapdClient.
 * @name: name of snap to refresh.
 * @channel: (allow-none): channel to refresh from or %NULL for default.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ensure an installed snap is at the latest version.
 * See snapd_client_refresh_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_refresh_async (SnapdClient *client,
                            const gchar *name, const gchar *channel,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
    request->name = g_strdup (name);
    request->action = g_strdup ("refresh");
    request->channel = g_strdup (channel);
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_refresh_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_refresh_async().
 * See snapd_client_refresh_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_refresh_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_refresh_all_async:
 * @client: a #SnapdClient.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ensure all snaps are updated to their latest versions.
 * See snapd_client_refresh_all_sync() for more information.
 *
 * Since: 1.5
 */
void
snapd_client_refresh_all_async (SnapdClient *client,
                                SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnaps *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = SNAPD_POST_SNAPS (g_object_new (snapd_post_snaps_get_type (),
                                              "cancellable", cancellable,
                                              "ready-callback", callback,
                                              "ready-callback-data", user_data,
                                              NULL));
    request->action = g_strdup ("refresh");
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_refresh_all_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_refresh_all_async().
 * See snapd_client_refresh_all_sync() for more information.
 *
 * Returns: (transfer full): a %NULL-terminated array of the snap names refreshed or %NULL on error.
 *
 * Since: 1.5
 */
gchar **
snapd_client_refresh_all_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdRequest *request;
    g_autoptr(GPtrArray) snap_names = NULL;
    JsonObject *o;
    g_autoptr(JsonArray) a = NULL;
    guint i;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAPS (result), FALSE);

    request = SNAPD_REQUEST (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;

    if (SNAPD_REQUEST_ASYNC (request)->async_data == NULL || json_node_get_value_type (SNAPD_REQUEST_ASYNC (request)->async_data) != JSON_TYPE_OBJECT) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "Unexpected result type");
        return NULL;
    }
    o = json_node_get_object (SNAPD_REQUEST_ASYNC (request)->async_data);
    if (o == NULL) {
        g_set_error_literal (error,
                             SNAPD_ERROR,
                             SNAPD_ERROR_READ_FAILED,
                             "No result returned");
        return NULL;
    }
    a = _snapd_json_get_array (o, "snap-names");
    snap_names = g_ptr_array_new ();
    for (i = 0; i < json_array_get_length (a); i++) {
        JsonNode *node = json_array_get_element (a, i);
        if (json_node_get_value_type (node) != G_TYPE_STRING) {
            g_set_error_literal (error,
                                 SNAPD_ERROR,
                                 SNAPD_ERROR_READ_FAILED,
                                 "Unexpected snap name type");
            return NULL;
        }

        g_ptr_array_add (snap_names, g_strdup (json_node_get_string (node)));
    }
    g_ptr_array_add (snap_names, NULL);

    return (gchar **) g_steal_pointer (&snap_names->pdata);
}

/**
 * snapd_client_remove_async:
 * @client: a #SnapdClient.
 * @name: name of snap to remove.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously uninstall a snap.
 * See snapd_client_remove_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_remove_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
    request->name = g_strdup (name);
    request->action = g_strdup ("remove");
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_remove_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_remove_async().
 * See snapd_client_remove_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_remove_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_enable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to enable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously enable an installed snap.
 * See snapd_client_enable_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_enable_async (SnapdClient *client,
                           const gchar *name,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
    request->name = g_strdup (name);
    request->action = g_strdup ("enable");
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_enable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_enable_async().
 * See snapd_client_enable_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_enable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_disable_async:
 * @client: a #SnapdClient.
 * @name: name of snap to disable.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously disable an installed snap.
 * See snapd_client_disable_sync() for more information.
 *
 * Since: 1.0
 */
void
snapd_client_disable_async (SnapdClient *client,
                            const gchar *name,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnap *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (name != NULL);

    request = SNAPD_POST_SNAP (g_object_new (snapd_post_snap_get_type (),
                                             "cancellable", cancellable,
                                             "ready-callback", callback,
                                             "ready-callback-data", user_data,
                                             NULL));
    request->name = g_strdup (name);
    request->action = g_strdup ("disable");
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_disable_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disable_async().
 * See snapd_client_disable_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.0
 */
gboolean
snapd_client_disable_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAP (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_check_buy_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously check if able to buy snaps.
 * See snapd_client_check_buy_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_check_buy_async (SnapdClient *client,
                              GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetBuyReady *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_buy_ready_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_check_buy_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_check_buy_async().
 * See snapd_client_check_buy_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_check_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_GET_BUY_READY (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_buy_async:
 * @client: a #SnapdClient.
 * @id: id of snap to buy.
 * @amount: amount of currency to spend, e.g. 0.99.
 * @currency: the currency to buy with as an ISO 4217 currency code, e.g. "NZD".
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously buy a snap from the store.
 * See snapd_client_buy_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_buy_async (SnapdClient *client,
                        const gchar *id, gdouble amount, const gchar *currency,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostBuy *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (id != NULL);
    g_return_if_fail (currency != NULL);

    request = _snapd_post_buy_new (id, amount, currency, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_buy_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_buy_async().
 * See snapd_client_buy_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.3
 */
gboolean
snapd_client_buy_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_BUY (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_create_user_async:
 * @client: a #SnapdClient.
 * @email: the email of the user to create.
 * @flags: a set of #SnapdCreateUserFlags to control how the user account is created.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_create_user_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_create_user_async (SnapdClient *client,
                                const gchar *email, SnapdCreateUserFlags flags,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostCreateUser *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (email != NULL);

    request = _snapd_post_create_user_new (email, cancellable, callback, user_data);
    if ((flags & SNAPD_CREATE_USER_FLAGS_SUDO) != 0)
        _snapd_post_create_user_set_sudoer (request, TRUE);
    if ((flags & SNAPD_CREATE_USER_FLAGS_KNOWN) != 0)
        _snapd_post_create_user_set_known (request, TRUE);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_create_user_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_create_user_async().
 * See snapd_client_create_user_sync() for more information.
 *
 * Returns: (transfer full): a #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
SnapdUserInformation *
snapd_client_create_user_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdPostCreateUser *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_CREATE_USER (result), NULL);

    request = SNAPD_POST_CREATE_USER (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_object_ref (_snapd_post_create_user_get_user_information (request));
}

/**
 * snapd_client_create_users_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create local user accounts using the system-user assertions that are valid for this device.
 * See snapd_client_create_users_sync() for more information.
 *
 * Since: 1.3
 */
void
snapd_client_create_users_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostCreateUsers *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_post_create_users_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_create_users_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_create_users_async().
 * See snapd_client_create_users_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdUserInformation): an array of #SnapdUserInformation or %NULL on error.
 *
 * Since: 1.3
 */
GPtrArray *
snapd_client_create_users_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdPostCreateUsers *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_POST_CREATE_USERS (result), NULL);

    request = SNAPD_POST_CREATE_USERS (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_post_create_users_get_users_information (request));
}

/**
 * snapd_client_get_sections_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create a local user account.
 * See snapd_client_get_sections_sync() for more information.
 *
 * Since: 1.7
 */
void
snapd_client_get_sections_async (SnapdClient *client,
                                 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetSections *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_sections_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_sections_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_sections_async().
 * See snapd_client_get_sections_sync() for more information.
 *
 * Returns: (transfer full) (array zero-terminated=1): an array of section names or %NULL on error.
 *
 * Since: 1.7
 */
gchar **
snapd_client_get_sections_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetSections *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_SECTIONS (result), NULL);

    request = SNAPD_GET_SECTIONS (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_strdupv (_snapd_get_sections_get_sections (request));
}

/**
 * snapd_client_get_aliases_async:
 * @client: a #SnapdClient.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously get the available aliases.
 * See snapd_client_get_aliases_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_get_aliases_async (SnapdClient *client,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdGetAliases *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    request = _snapd_get_aliases_new (cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_get_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_get_aliases_async().
 * See snapd_client_get_aliases_sync() for more information.
 *
 * Returns: (transfer container) (element-type SnapdAlias): an array of #SnapdAlias or %NULL on error.
 *
 * Since: 1.8
 */
GPtrArray *
snapd_client_get_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    SnapdGetAliases *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), NULL);
    g_return_val_if_fail (SNAPD_IS_GET_ALIASES (result), NULL);

    request = SNAPD_GET_ALIASES (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return NULL;
    return g_ptr_array_ref (_snapd_get_aliases_get_aliases (request));
}

static void
send_change_aliases_request (SnapdClient *client,
                             const gchar *action,
                             const gchar *snap, const gchar *app, const gchar *alias,
                             SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostAliases *request;

    request = SNAPD_POST_ALIASES (g_object_new (snapd_post_aliases_get_type (),
                                                "cancellable", cancellable,
                                                "ready-callback", callback,
                                                "ready-callback-data", user_data,
                                                NULL));
    request->action = g_strdup (action);
    request->snap = g_strdup (snap);
    request->app = g_strdup (app);
    request->alias = g_strdup (alias);
    SNAPD_REQUEST_ASYNC (request)->progress_callback = progress_callback;
    SNAPD_REQUEST_ASYNC (request)->progress_callback_data = progress_callback_data;

    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_alias_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @app: an app in the snap to make the alias to.
 * @alias: the name of the alias (i.e. the command that will run this app).
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously create an alias to an app.
 * See snapd_client_alias_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_alias_async (SnapdClient *client,
                          const gchar *snap, const gchar *app, const gchar *alias,
                          SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    g_return_if_fail (app != NULL);
    g_return_if_fail (alias != NULL);
    send_change_aliases_request (client, "alias", snap, app, alias, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_alias_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_alias_async().
 * See snapd_client_alias_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_alias_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_unalias_async:
 * @client: a #SnapdClient.
 * @snap: (allow-none): the name of the snap to modify or %NULL.
 * @alias: (allow-none): the name of the alias to remove or %NULL to remove all aliases for the given snap.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously remove an alias from an app.
 * See snapd_client_unalias_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_unalias_async (SnapdClient *client,
                            const gchar *snap, const gchar *alias,
                            SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (alias != NULL);
    send_change_aliases_request (client, "unalias", snap, NULL, alias, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_unalias_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_unalias_async().
 * See snapd_client_unalias_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_unalias_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_prefer_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously ???.
 * See snapd_client_prefer_sync() for more information.
 *
 * Since: 1.25
 */
void
snapd_client_prefer_async (SnapdClient *client,
                           const gchar *snap,
                           SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                           GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (snap != NULL);
    send_change_aliases_request (client, "prefer", snap, NULL, NULL, progress_callback, progress_callback_data, cancellable, callback, user_data);
}

/**
 * snapd_client_prefer_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_prefer_async().
 * See snapd_client_prefer_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.25
 */
gboolean
snapd_client_prefer_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_ALIASES (result), FALSE);

    return snapd_request_return_error (SNAPD_REQUEST (result), error);
}

/**
 * snapd_client_enable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_enable_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_alias_async()
 */
void
snapd_client_enable_aliases_async (SnapdClient *client,
                                   const gchar *snap, gchar **aliases,
                                   SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(GTask) task = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_enable_aliases_async is deprecated");
}

/**
 * snapd_client_enable_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_enable_aliases_async().
 * See snapd_client_enable_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_unalias_finish()
 */
gboolean
snapd_client_enable_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, client), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_disable_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_disable_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_unalias_async()
 */
void
snapd_client_disable_aliases_async (SnapdClient *client,
                                    const gchar *snap, gchar **aliases,
                                    SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                    GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(GTask) task = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_disable_aliases_async is deprecated");
}

/**
 * snapd_client_disable_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_disable_aliases_async().
 * See snapd_client_disable_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_unalias_finish()
 */
gboolean
snapd_client_disable_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, client), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_reset_aliases_async:
 * @client: a #SnapdClient.
 * @snap: the name of the snap to modify.
 * @aliases: the aliases to modify.
 * @progress_callback: (allow-none) (scope async): function to callback with progress.
 * @progress_callback_data: (closure): user data to pass to @progress_callback.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously change the state of aliases.
 * See snapd_client_reset_aliases_sync() for more information.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_disable_aliases_async()
 */
void
snapd_client_reset_aliases_async (SnapdClient *client,
                                  const gchar *snap, gchar **aliases,
                                  SnapdProgressCallback progress_callback, gpointer progress_callback_data,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    g_autoptr(GTask) task = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_if_fail (SNAPD_IS_CLIENT (client));

    task = g_task_new (client, cancellable, callback, user_data);
    g_task_return_new_error (task, SNAPD_ERROR, SNAPD_ERROR_FAILED, "snapd_client_reset_aliases_async is deprecated");
}

/**
 * snapd_client_reset_aliases_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_reset_aliases_async().
 * See snapd_client_reset_aliases_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 * Deprecated: 1.25: Use snapd_client_disable_aliases_finish()
 */
gboolean
snapd_client_reset_aliases_finish (SnapdClient *client, GAsyncResult *result, GError **error)
{
    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (g_task_is_valid (result, client), FALSE);

    return g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * snapd_client_run_snapctl_async:
 * @client: a #SnapdClient.
 * @context_id: context for this call.
 * @args: the arguments to pass to snapctl.
 * @cancellable: (allow-none): a #GCancellable or %NULL.
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied.
 * @user_data: (closure): the data to pass to callback function.
 *
 * Asynchronously run a snapctl command.
 * See snapd_client_run_snapctl_sync() for more information.
 *
 * Since: 1.8
 */
void
snapd_client_run_snapctl_async (SnapdClient *client,
                                const gchar *context_id, gchar **args,
                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
    SnapdPostSnapctl *request;

    g_return_if_fail (SNAPD_IS_CLIENT (client));
    g_return_if_fail (context_id != NULL);
    g_return_if_fail (args != NULL);

    request = _snapd_post_snapctl_new (context_id, args, cancellable, callback, user_data);
    send_request (client, SNAPD_REQUEST (request));
}

/**
 * snapd_client_run_snapctl_finish:
 * @client: a #SnapdClient.
 * @result: a #GAsyncResult.
 * @stdout_output: (out) (allow-none): the location to write the stdout from the command or %NULL.
 * @stderr_output: (out) (allow-none): the location to write the stderr from the command or %NULL.
 * @error: (allow-none): #GError location to store the error occurring, or %NULL to ignore.
 *
 * Complete request started with snapd_client_run_snapctl_async().
 * See snapd_client_run_snapctl_sync() for more information.
 *
 * Returns: %TRUE on success or %FALSE on error.
 *
 * Since: 1.8
 */
gboolean
snapd_client_run_snapctl_finish (SnapdClient *client, GAsyncResult *result,
                                 gchar **stdout_output, gchar **stderr_output,
                                 GError **error)
{
    SnapdPostSnapctl *request;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);
    g_return_val_if_fail (SNAPD_IS_POST_SNAPCTL (result), FALSE);

    request = SNAPD_POST_SNAPCTL (result);

    if (!snapd_request_return_error (SNAPD_REQUEST (request), error))
        return FALSE;

    if (stdout_output)
        *stdout_output = g_strdup (_snapd_post_snapctl_get_stdout_output (request));
    if (stderr_output)
        *stderr_output = g_strdup (_snapd_post_snapctl_get_stderr_output (request));

    return TRUE;
}

/**
 * snapd_client_new:
 *
 * Create a new client to talk to snapd.
 *
 * Returns: a new #SnapdClient
 *
 * Since: 1.0
 **/
SnapdClient *
snapd_client_new (void)
{
    return g_object_new (SNAPD_TYPE_CLIENT, NULL);
}

/**
 * snapd_client_new_from_socket:
 * @socket: A #GSocket that is connected to snapd.
 *
 * Create a new client to talk on an existing socket.
 *
 * Returns: a new #SnapdClient
 *
 * Since: 1.5
 **/
SnapdClient *
snapd_client_new_from_socket (GSocket *socket)
{
    SnapdClient *client;
    SnapdClientPrivate *priv;

    client = snapd_client_new ();
    priv = snapd_client_get_instance_private (SNAPD_CLIENT (client));
    priv->snapd_socket = g_object_ref (socket);
    g_socket_set_blocking (priv->snapd_socket, FALSE);

    return client;
}

static void
snapd_client_finalize (GObject *object)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (SNAPD_CLIENT (object));

    g_mutex_clear (&priv->requests_mutex);
    g_mutex_clear (&priv->buffer_mutex);
    g_clear_pointer (&priv->socket_path, g_free);
    g_clear_pointer (&priv->user_agent, g_free);
    g_clear_object (&priv->auth_data);
    g_list_free_full (priv->requests, g_object_unref);
    priv->requests = NULL;
    if (priv->snapd_socket != NULL)
        g_socket_close (priv->snapd_socket, NULL);
    g_clear_object (&priv->snapd_socket);
    g_clear_pointer (&priv->buffer, g_byte_array_unref);

    G_OBJECT_CLASS (snapd_client_parent_class)->finalize (object);
}

static void
snapd_client_class_init (SnapdClientClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   gobject_class->finalize = snapd_client_finalize;
}

static void
snapd_client_init (SnapdClient *client)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);

    priv->socket_path = g_strdup (SNAPD_SOCKET);
    priv->user_agent = g_strdup ("snapd-glib/" VERSION);
    priv->allow_interaction = TRUE;
    priv->buffer = g_byte_array_new ();
    g_mutex_init (&priv->requests_mutex);
    g_mutex_init (&priv->buffer_mutex);
}

static void
snapd_request_async_finalize (GObject *object)
{
    SnapdRequestAsync *request = SNAPD_REQUEST_ASYNC (object);

    g_clear_pointer (&request->change_id, g_free);
    g_clear_pointer (&request->poll_source, g_source_destroy);
    g_clear_object (&request->change);
    g_clear_pointer (&request->async_data, json_node_unref);

    G_OBJECT_CLASS (snapd_request_async_parent_class)->finalize (object);
}

static void
snapd_request_async_class_init (SnapdRequestAsyncClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->parse_response = parse_async_response;
   gobject_class->finalize = snapd_request_async_finalize;
}

static void
snapd_request_async_init (SnapdRequestAsync *request)
{
}

static void
snapd_post_snap_finalize (GObject *object)
{
    SnapdPostSnap *request = SNAPD_POST_SNAP (object);

    g_clear_pointer (&request->name, g_free);
    g_clear_pointer (&request->action, g_free);
    g_clear_pointer (&request->channel, g_free);
    g_clear_pointer (&request->revision, g_free);

    G_OBJECT_CLASS (snapd_post_snap_parent_class)->finalize (object);
}

static void
snapd_post_snap_class_init (SnapdPostSnapClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_request;
   gobject_class->finalize = snapd_post_snap_finalize;
}

static void
snapd_post_snap_init (SnapdPostSnap *request)
{
}

static void
snapd_post_snaps_finalize (GObject *object)
{
    SnapdPostSnaps *request = SNAPD_POST_SNAPS (object);

    g_clear_pointer (&request->action, g_free);

    G_OBJECT_CLASS (snapd_post_snaps_parent_class)->finalize (object);
}

static void
snapd_post_snaps_class_init (SnapdPostSnapsClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snaps_request;
   gobject_class->finalize = snapd_post_snaps_finalize;
}

static void
snapd_post_snaps_init (SnapdPostSnaps *request)
{
}

static void
snapd_post_snap_stream_finalize (GObject *object)
{
    SnapdPostSnapStream *request = SNAPD_POST_SNAP_STREAM (object);

    g_clear_pointer (&request->snap_contents, g_byte_array_unref);

    G_OBJECT_CLASS (snapd_post_snap_stream_parent_class)->finalize (object);
}

static void
snapd_post_snap_stream_class_init (SnapdPostSnapStreamClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_stream_request;
   gobject_class->finalize = snapd_post_snap_stream_finalize;
}

static void
snapd_post_snap_stream_init (SnapdPostSnapStream *request)
{
}

static void
snapd_post_snap_try_finalize (GObject *object)
{
    SnapdPostSnapTry *request = SNAPD_POST_SNAP_TRY (object);

    g_clear_pointer (&request->path, g_free);

    G_OBJECT_CLASS (snapd_post_snap_try_parent_class)->finalize (object);
}

static void
snapd_post_snap_try_class_init (SnapdPostSnapTryClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_snap_try_request;
   gobject_class->finalize = snapd_post_snap_try_finalize;
}

static void
snapd_post_snap_try_init (SnapdPostSnapTry *request)
{
}

static void
snapd_post_aliases_finalize (GObject *object)
{
    SnapdPostAliases *request = SNAPD_POST_ALIASES (object);

    g_clear_pointer (&request->action, g_free);
    g_clear_pointer (&request->snap, g_free);
    g_clear_pointer (&request->app, g_free);
    g_clear_pointer (&request->alias, g_free);

    G_OBJECT_CLASS (snapd_post_aliases_parent_class)->finalize (object);
}

static void
snapd_post_aliases_class_init (SnapdPostAliasesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_aliases_request;
   gobject_class->finalize = snapd_post_aliases_finalize;
}

static void
snapd_post_aliases_init (SnapdPostAliases *request)
{
}

static void
snapd_post_interfaces_finalize (GObject *object)
{
    SnapdPostInterfaces *request = SNAPD_POST_INTERFACES (object);

    g_clear_pointer (&request->action, g_free);
    g_clear_pointer (&request->plug_snap, g_free);
    g_clear_pointer (&request->plug_name, g_free);
    g_clear_pointer (&request->slot_snap, g_free);
    g_clear_pointer (&request->slot_name, g_free);

    G_OBJECT_CLASS (snapd_post_interfaces_parent_class)->finalize (object);
}

static void
snapd_post_interfaces_class_init (SnapdPostInterfacesClass *klass)
{
   SnapdRequestClass *request_class = SNAPD_REQUEST_CLASS (klass);
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

   request_class->generate_request = generate_post_interfaces_request;
   gobject_class->finalize = snapd_post_interfaces_finalize;
}

static void
snapd_post_interfaces_init (SnapdPostInterfaces *request)
{
}
