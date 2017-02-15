/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-snapctl-output.h"

/**
 * SECTION: snapd-snapctl-output
 * @short_description: snapctl output.
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdSnapCtlOutput represents output from a snapctl command as returned from 
 * snapd_client_run_snapctl_sync().
 */

/**
 * SnapdSnapCtlOutput:
 *
 * #SnapdSnapCtlOutput is an opaque data structure and can only be accessed
 * using the provided functions.
 */

struct _SnapdSnapCtlOutput
{
    GObject parent_instance;

    gchar *stdout;
    gchar *stderr;
};

enum 
{
    PROP_STDOUT = 1,
    PROP_STDERR,  
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdSnapCtlOutput, snapd_snapctl_output, G_TYPE_OBJECT)

/**
 * snapd_snapctl_output_get_stdout:
 * @output: a #SnapdSnapCtlOutput.
 *
 * Get the stdout output from this command.
 *
 * Returns: command output.
 */
const gchar *
snapd_snapctl_output_get_stdout (SnapdSnapCtlOutput *output)
{
    g_return_val_if_fail (SNAPD_IS_SNAPCTL_OUTPUT (output), NULL);
    return output->stdout;
}

/**
 * snapd_snapctl_output_get_stderr:
 * @output: a #SnapdSnapCtlOutput.
 *
 * Get the stderr output from this command.
 *
 * Returns: command output.
 */
const gchar *
snapd_snapctl_output_get_stderr (SnapdSnapCtlOutput *output)
{
    g_return_val_if_fail (SNAPD_IS_SNAPCTL_OUTPUT (output), NULL);
    return output->stderr;
}

static void
snapd_snapctl_output_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSnapCtlOutput *output = SNAPD_SNAPCTL_OUTPUT (object);

    switch (prop_id) {
    case PROP_STDOUT:
        g_free (output->stdout);
        output->stdout = g_strdup (g_value_get_string (value));
        break;
    case PROP_STDERR:
        g_free (output->stderr);
        output->stderr = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snapctl_output_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSnapCtlOutput *output = SNAPD_SNAPCTL_OUTPUT (object);

    switch (prop_id) {
    case PROP_STDOUT:
        g_value_set_string (value, output->stdout);
        break;
    case PROP_STDERR:
        g_value_set_string (value, output->stderr);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snapctl_output_finalize (GObject *object)
{
    SnapdSnapCtlOutput *output = SNAPD_SNAPCTL_OUTPUT (object);

    g_clear_pointer (&output->stdout, g_free);
    g_clear_pointer (&output->stderr, g_free);
}

static void
snapd_snapctl_output_class_init (SnapdSnapCtlOutputClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_snapctl_output_set_property;
    gobject_class->get_property = snapd_snapctl_output_get_property; 
    gobject_class->finalize = snapd_snapctl_output_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_STDOUT,
                                     g_param_spec_string ("stdout",
                                                          "stdout",
                                                          "stdout from snapctl command",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (gobject_class,
                                     PROP_STDERR,
                                     g_param_spec_string ("stderr",
                                                          "stderr",
                                                          "stderr from snapctl command",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_snapctl_output_init (SnapdSnapCtlOutput *output)
{
}
