/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-request.h"

enum
{
    PROP_SOURCE_OBJECT = 1,  
    PROP_CANCELLABLE,
    PROP_READY_CALLBACK,
    PROP_READY_CALLBACK_DATA,
    PROP_LAST
};

typedef struct
{
    GObject *source_object;

    /* Context request was made from */
    GMainContext *context;

    SoupMessage *message;

    GCancellable *cancellable;
    gulong cancelled_id;

    gboolean responded;
    GAsyncReadyCallback ready_callback;
    gpointer ready_callback_data;

    GError *error;
} SnapdRequestPrivate;

static void snapd_request_async_result_init (GAsyncResultIface *iface);

G_DEFINE_TYPE_WITH_CODE (SnapdRequest, snapd_request, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_RESULT, snapd_request_async_result_init)
                         G_ADD_PRIVATE (SnapdRequest))

GMainContext *
_snapd_request_get_context (SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);
    return priv->context;
}

GCancellable *
_snapd_request_get_cancellable (SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));
    return priv->cancellable;
}

void
_snapd_request_set_source_object (SnapdRequest *request, GObject *object)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));  
    priv->source_object = g_object_ref (object);
}

SoupMessage *
_snapd_request_get_message (SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (SNAPD_REQUEST (request));

    if (priv->message == NULL)
        priv->message = SNAPD_REQUEST_GET_CLASS (request)->generate_request (request);

    return priv->message;  
}

static gboolean
respond_cb (gpointer user_data)
{
    SnapdRequest *request = SNAPD_REQUEST (user_data);
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    if (priv->ready_callback != NULL)
        priv->ready_callback (priv->source_object, G_ASYNC_RESULT (request), priv->ready_callback_data);

    return G_SOURCE_REMOVE;
}

void
_snapd_request_return (SnapdRequest *request, GError *error)
{
    g_autoptr(GSource) source = NULL;
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    if (priv->responded)
        return;
    priv->responded = TRUE;
    if (error != NULL)
        priv->error = g_error_copy (error);

    source = g_idle_source_new ();
    g_source_set_callback (source, respond_cb, g_object_ref (request), g_object_unref);
    g_source_attach (source, _snapd_request_get_context (request));
}

gboolean
_snapd_request_propagate_error (SnapdRequest *request, GError **error)
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
    return g_object_ref (priv->source_object);
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
    case PROP_SOURCE_OBJECT:
        g_set_object (&priv->source_object, g_value_get_object (value));
        break;      
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

    g_clear_object (&priv->source_object);
    g_clear_object (&priv->message);
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
                                    PROP_SOURCE_OBJECT,
                                    g_param_spec_object ("source-object",
                                                         "source-object",
                                                         "Source object",
                                                         G_TYPE_OBJECT,
                                                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_CANCELLABLE,
                                    g_param_spec_object ("cancellable",
                                                         "cancellable",
                                                         "Cancellable",
                                                         G_TYPE_CANCELLABLE,
                                                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_READY_CALLBACK,
                                    g_param_spec_pointer ("ready-callback",
                                                          "ready-callback",
                                                          "Ready callback",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
   g_object_class_install_property (gobject_class,
                                    PROP_READY_CALLBACK_DATA,
                                    g_param_spec_pointer ("ready-callback-data",
                                                          "ready-callback-data",
                                                          "Ready callback data",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_request_init (SnapdRequest *request)
{
    SnapdRequestPrivate *priv = snapd_request_get_instance_private (request);

    priv->context = g_main_context_ref_thread_default ();
}
