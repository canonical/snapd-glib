/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-system-information.h"

/**
 * SECTION:snapd-system-information
 * @short_description: System information class
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdSystemInformation object contains the system information returned
 * from snapd. It is requested using snapd_client_get_system_information_sync().
 */

/**
 * SnapdSystemInformation:
 *
 * #SnapdSystemInformation is an opaque data structure and can only be accessed
 * using the provided functions.
 */

struct _SnapdSystemInformation
{
    GObject parent_instance;

    gchar *binaries_directory;
    gchar *kernel_version;
    gboolean on_classic;
    gboolean managed;
    gchar *mount_directory;
    gchar *os_id;
    gchar *os_version;
    gchar *series;
    gchar *store;
    gchar *version;
};

enum 
{
    PROP_ON_CLASSIC = 1,
    PROP_OS_ID,
    PROP_OS_VERSION,
    PROP_SERIES,
    PROP_STORE,  
    PROP_VERSION,
    PROP_MANAGED,
    PROP_KERNEL_VERSION,
    PROP_BINARIES_DIRECTORY,
    PROP_MOUNT_DIRECTORY,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdSystemInformation, snapd_system_information, G_TYPE_OBJECT)

/**
 * snapd_system_information_get_binaries_directory:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the directory snap binaries are stored, e.g. "/snap/bin".
 *
 * Returns: a directory.
 */
const gchar *
snapd_system_information_get_binaries_directory (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->binaries_directory;
}

/**
 * snapd_system_information_get_kernel_version:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the version of the kernel snapd is running on, e.g. "4.10.0-15-generic".
 *
 * Returns: a version string.
 */
const gchar *
snapd_system_information_get_kernel_version (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->kernel_version;
}

/**
 * snapd_system_information_get_managed:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get if snapd is running on a managed system.
 *
 * Returns: %TRUE if running on a managed system.
 */
gboolean
snapd_system_information_get_managed (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), FALSE);
    return system_information->managed;
}

/**
 * snapd_system_information_get_mount_directory:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the directory snaps are mounted, e.g. "/snap".
 *
 * Returns: a directory.
 */
const gchar *
snapd_system_information_get_mount_directory (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->mount_directory;
}

/**
 * snapd_system_information_get_on_classic:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get if this system is a classic system.
 *
 * Returns: %TRUE if running on a classic system.
 */
gboolean
snapd_system_information_get_on_classic (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), FALSE);
    return system_information->on_classic;
}

/**
 * snapd_system_information_get_os_id:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the operating system ID, e.g. "ubuntu".
 *
 * Returns: an operating system ID.
 */
const gchar *
snapd_system_information_get_os_id (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->os_id;
}

/**
 * snapd_system_information_get_os_version:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the operating system version, e.g. "16.04".
 *
 * Returns: a version string.
 */
const gchar *
snapd_system_information_get_os_version (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->os_version;
}

/**
 * snapd_system_information_get_series:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the series of snapd running, e.g. "16".
 *
 * Returns: a series string.
 */
const gchar *
snapd_system_information_get_series (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->series;
}

/**
 * snapd_system_information_get_store:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the store being used by snapd, e.g. "Ubuntu"
 *
 * Returns: (allow-none): a store id or %NULL.
 */
const gchar *
snapd_system_information_get_store (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->store;
}

/**
 * snapd_system_information_get_version:
 * @system_information: a #SnapdSystemInformation.
 *
 * Get the version of snapd running, e.g. "2.11+ppa174-1".
 *
 * Returns: a version string.
 */
const gchar *
snapd_system_information_get_version (SnapdSystemInformation *system_information)
{
    g_return_val_if_fail (SNAPD_IS_SYSTEM_INFORMATION (system_information), NULL);
    return system_information->version;
}

static void
snapd_system_information_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSystemInformation *system_information = SNAPD_SYSTEM_INFORMATION (object);

    switch (prop_id) {
    case PROP_BINARIES_DIRECTORY:
        g_free (system_information->binaries_directory);
        system_information->binaries_directory = g_strdup (g_value_get_string (value));
        break;
    case PROP_KERNEL_VERSION:
        g_free (system_information->kernel_version);
        system_information->kernel_version = g_strdup (g_value_get_string (value));
        break;
    case PROP_MANAGED:
        system_information->managed = g_value_get_boolean (value);
        break;
    case PROP_MOUNT_DIRECTORY:
        g_free (system_information->mount_directory);
        system_information->mount_directory = g_strdup (g_value_get_string (value));
        break;
    case PROP_ON_CLASSIC:
        system_information->on_classic = g_value_get_boolean (value);
        break;
    case PROP_OS_ID:
        g_free (system_information->os_id);
        system_information->os_id = g_strdup (g_value_get_string (value));
        break;
    case PROP_OS_VERSION:
        g_free (system_information->os_version);
        system_information->os_version = g_strdup (g_value_get_string (value));
        break;
    case PROP_SERIES:
        g_free (system_information->series);      
        system_information->series = g_strdup (g_value_get_string (value));
        break;
    case PROP_STORE:
        g_free (system_information->store);
        system_information->store = g_strdup (g_value_get_string (value));
        break;
    case PROP_VERSION:
        g_free (system_information->version);
        system_information->version = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_system_information_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSystemInformation *system_information = SNAPD_SYSTEM_INFORMATION (object);

    switch (prop_id) {
    case PROP_BINARIES_DIRECTORY:
        g_value_set_string (value, system_information->binaries_directory);
        break;
    case PROP_KERNEL_VERSION:
        g_value_set_string (value, system_information->kernel_version);
        break;
    case PROP_MANAGED:
        g_value_set_boolean (value, system_information->managed);
        break;
    case PROP_MOUNT_DIRECTORY:
        g_value_set_string (value, system_information->mount_directory);
        break;
    case PROP_ON_CLASSIC:
        g_value_set_boolean (value, system_information->on_classic);
        break;
    case PROP_OS_ID:
        g_value_set_string (value, system_information->os_id);
        break;
    case PROP_OS_VERSION:
        g_value_set_string (value, system_information->os_version);
        break;
    case PROP_SERIES:
        g_value_set_string (value, system_information->series);
        break;
    case PROP_STORE:
        g_value_set_string (value, system_information->store);
        break;
    case PROP_VERSION:
        g_value_set_string (value, system_information->version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_system_information_finalize (GObject *object)
{
    SnapdSystemInformation *system_information = SNAPD_SYSTEM_INFORMATION (object);

    g_clear_pointer (&system_information->binaries_directory, g_free);
    g_clear_pointer (&system_information->kernel_version, g_free);
    g_clear_pointer (&system_information->mount_directory, g_free);
    g_clear_pointer (&system_information->os_id, g_free);
    g_clear_pointer (&system_information->os_version, g_free);
    g_clear_pointer (&system_information->series, g_free);
    g_clear_pointer (&system_information->store, g_free);  
    g_clear_pointer (&system_information->version, g_free);
}

static void
snapd_system_information_class_init (SnapdSystemInformationClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_system_information_set_property;
    gobject_class->get_property = snapd_system_information_get_property; 
    gobject_class->finalize = snapd_system_information_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_BINARIES_DIRECTORY,
                                     g_param_spec_string ("binaries-directory",
                                                          "binaries-directory",
                                                          "Directory with snap binaries",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_KERNEL_VERSION,
                                     g_param_spec_string ("kernel-version",
                                                          "kernel-version",
                                                          "Kernel version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_MANAGED,
                                     g_param_spec_boolean ("managed",
                                                           "managed",
                                                           "TRUE if snapd managing the system",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_MOUNT_DIRECTORY,
                                     g_param_spec_string ("mount-directory",
                                                          "mount-directory",
                                                          "Directory snaps are mounted in",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ON_CLASSIC,
                                     g_param_spec_boolean ("on-classic",
                                                           "on-classic",
                                                           "TRUE if running in a classic system",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_OS_ID,
                                     g_param_spec_string ("os-id",
                                                          "os-id",
                                                          "Operating system ID",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_OS_VERSION,
                                     g_param_spec_string ("os-version",
                                                          "os-version",
                                                          "Operating system version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SERIES,
                                     g_param_spec_string ("series",
                                                          "series",
                                                          "Snappy release series",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STORE,
                                     g_param_spec_string ("store",
                                                          "store",
                                                          "Snap store",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_VERSION,
                                     g_param_spec_string ("version",
                                                          "version",
                                                          "Snappy version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_system_information_init (SnapdSystemInformation *system_information)
{
}
