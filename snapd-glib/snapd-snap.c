#include "config.h"

#include "snapd-snap.h"
#include "snapd-enum-types.h"

struct _SnapdSnap
{
    GObject parent_instance;

    // FIXME: apps
    gchar *channel;
    SnapdConfinement confinement;
    gchar *description;
    gchar *developer;
    gboolean devmode;
    gint64 download_size;
    gchar *icon;
    gchar *id;
    gchar *install_date;
    gint64 installed_size;
    gchar *name;
    // FIXME: prices
    gboolean private;
    gchar *revision;
    SnapdSnapStatus status;
    gchar *summary;
    gboolean trymode;
    SnapdSnapType snap_type;
    gchar *version;
};

enum 
{
    PROP_CHANEL = 1,
    PROP_CONFINEMENT,
    PROP_DESCRIPTION,
    PROP_DEVELOPER,
    PROP_DEVMODE,
    PROP_DOWNLOAD_SIZE,  
    PROP_ICON,
    PROP_ID,
    PROP_INSTALL_DATE,
    PROP_INSTALLED_SIZE,
    PROP_NAME,
    PROP_PRIVATE,
    PROP_REVISION,
    PROP_STATUS,
    PROP_SUMMARY,
    PROP_TRYMODE,
    PROP_SNAP_TYPE,
    PROP_VERSION,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdSnap, snapd_snap, G_TYPE_OBJECT)

const gchar *
snapd_snap_get_channel (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->channel;
}

SnapdConfinement
snapd_snap_get_confinement (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), SNAPD_CONFINEMENT_UNKNOWN);
    return snap->confinement;
}

const gchar *
snapd_snap_get_description (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->description;
}

const gchar *
snapd_snap_get_developer (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->developer;
}

gboolean
snapd_snap_get_devmode (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), FALSE);
    return snap->devmode;
}

gint64
snapd_snap_get_download_size (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), 0);
    return snap->download_size;
}

const gchar *
snapd_snap_get_icon (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->icon;
}

const gchar *
snapd_snap_get_id (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->id;
}

const gchar *
snapd_snap_get_install_date (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->install_date;
}

gint64
snapd_snap_get_installed_size (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), 0);
    return snap->installed_size;
}

const gchar *
snapd_snap_get_name (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->name;
}

gboolean
snapd_snap_get_private (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), FALSE);
    return snap->private;
}

const gchar *
snapd_snap_get_revision (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->revision;
}

SnapdSnapType
snapd_snap_get_snap_type (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), SNAPD_SNAP_TYPE_UNKNOWN);
    return snap->snap_type;
}

SnapdSnapStatus
snapd_snap_get_status (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), SNAPD_SNAP_STATUS_UNKNOWN);
    return snap->status;
}

const gchar *
snapd_snap_get_summary (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->summary;
}

gboolean
snapd_snap_get_trymode (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), FALSE);
    return snap->trymode;
}

const gchar *
snapd_snap_get_version (SnapdSnap *snap)
{
    g_return_val_if_fail (SNAPD_IS_SNAP (snap), NULL);
    return snap->version;
}

static void
snapd_snap_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdSnap *snap = SNAPD_SNAP (object);

    switch (prop_id) {
    case PROP_CHANEL:
        g_free (snap->channel);
        snap->channel = g_strdup (g_value_get_string (value));
        break;
    case PROP_CONFINEMENT:
        snap->confinement = g_value_get_enum (value);
        break;
    case PROP_DESCRIPTION:
        g_free (snap->description);
        snap->description = g_strdup (g_value_get_string (value));
        break;
    case PROP_DEVELOPER:
        g_free (snap->developer);
        snap->developer = g_strdup (g_value_get_string (value));
        break;
    case PROP_DEVMODE:
        snap->devmode = g_value_get_boolean (value);
        break;
    case PROP_DOWNLOAD_SIZE:
        snap->download_size = g_value_get_int64 (value);
        break;
    case PROP_ICON:
        g_free (snap->icon);
        snap->icon = g_strdup (g_value_get_string (value));
        break;
    case PROP_ID:
        g_free (snap->id);
        snap->id = g_strdup (g_value_get_string (value));
        break;
    case PROP_INSTALL_DATE:
        g_free (snap->install_date);
        snap->install_date = g_strdup (g_value_get_string (value));
        break;
    case PROP_INSTALLED_SIZE:
        snap->installed_size = g_value_get_int64 (value);
        break;
    case PROP_NAME:
        g_free (snap->name);
        snap->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_PRIVATE:
        snap->private = g_value_get_boolean (value);
        break;
    case PROP_REVISION:
        g_free (snap->revision);
        snap->revision = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP_TYPE:
        snap->snap_type = g_value_get_enum (value);
        break;
    case PROP_STATUS:
        snap->status = g_value_get_enum (value);
        break;
    case PROP_SUMMARY:
        g_free (snap->summary);
        snap->summary = g_strdup (g_value_get_string (value));
        break;
    case PROP_TRYMODE:
        snap->trymode = g_value_get_boolean (value);
        break;
    case PROP_VERSION:
        g_free (snap->version);
        snap->version = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snap_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdSnap *snap = SNAPD_SNAP (object);

    switch (prop_id) {
    case PROP_CHANEL:
        g_value_set_string (value, snap->channel);
        break;
    case PROP_CONFINEMENT:
        g_value_set_enum (value, snap->confinement);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, snap->description);
        break;
    case PROP_DEVELOPER:
        g_value_set_string (value, snap->developer);
        break;
    case PROP_DEVMODE:
        g_value_set_boolean (value, snap->devmode);
        break;
    case PROP_DOWNLOAD_SIZE:
        g_value_set_int64 (value, snap->download_size);
        break;
    case PROP_ICON:
        g_value_set_string (value, snap->icon);
        break;
    case PROP_ID:
        g_value_set_string (value, snap->id);
        break;
    case PROP_INSTALL_DATE:
        g_value_set_string (value, snap->install_date);
        break;
    case PROP_INSTALLED_SIZE:
        g_value_set_int64 (value, snap->installed_size);
        break;
    case PROP_NAME:
        g_value_set_string (value, snap->name);
        break;
    case PROP_PRIVATE:
        g_value_set_boolean (value, snap->private);
        break;
    case PROP_REVISION:
        g_value_set_string (value, snap->revision);
        break;
    case PROP_SNAP_TYPE:
        g_value_set_enum (value, snap->snap_type);
        break;
    case PROP_STATUS:
        g_value_set_enum (value, snap->status);
        break;
    case PROP_SUMMARY:
        g_value_set_string (value, snap->summary);
        break;
    case PROP_TRYMODE:
        g_value_set_boolean (value, snap->trymode);
        break;
    case PROP_VERSION:
        g_value_set_string (value, snap->version);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_snap_finalize (GObject *object)
{
    SnapdSnap *snap = SNAPD_SNAP (object);

    g_clear_pointer (&snap->channel, g_free);
    g_clear_pointer (&snap->description, g_free);
    g_clear_pointer (&snap->developer, g_free);
    g_clear_pointer (&snap->icon, g_free);
    g_clear_pointer (&snap->id, g_free);
    g_clear_pointer (&snap->install_date, g_free);
    g_clear_pointer (&snap->name, g_free);
    g_clear_pointer (&snap->summary, g_free);
    g_clear_pointer (&snap->version, g_free);
}

static void
snapd_snap_class_init (SnapdSnapClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_snap_set_property;
    gobject_class->get_property = snapd_snap_get_property; 
    gobject_class->finalize = snapd_snap_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_CHANEL,
                                     g_param_spec_string ("channel",
                                                          "channel",
                                                          "Channel the snap is currently tracking",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CONFINEMENT,
                                     g_param_spec_enum ("confinement",
                                                        "confinement",
                                                        "Confinement requested by the snap",
                                                        SNAPD_TYPE_CONFINEMENT, SNAPD_CONFINEMENT_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DESCRIPTION,
                                     g_param_spec_string ("description",
                                                          "description",
                                                          "Description of the snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DEVELOPER,
                                     g_param_spec_string ("developer",
                                                          "developer",
                                                          "Developer who created the snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DEVMODE,
                                     g_param_spec_boolean ("devmode",
                                                           "devmode",
                                                           "TRUE if the snap is currently installed in devmode",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DOWNLOAD_SIZE,
                                     g_param_spec_int64 ("download-size",
                                                         "download-size",
                                                         "Download size in bytes",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ICON,
                                     g_param_spec_string ("icon",
                                                          "icon",
                                                          "URL to the snap icon",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_string ("id",
                                                          "id",
                                                          "Unique ID for this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INSTALL_DATE,
                                     g_param_spec_string ("install-date",
                                                          "install-date",
                                                          "Date this snap was installed",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INSTALLED_SIZE,
                                     g_param_spec_int64 ("installed-size",
                                                         "installed-size",
                                                         "Installed size in bytes",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "The snap name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PRIVATE,
                                     g_param_spec_boolean ("private",
                                                           "private",
                                                           "TRUE if this snap is only available to its author",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_REVISION,
                                     g_param_spec_string ("revision",
                                                          "revision",
                                                          "Revision of this snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_STATUS,
                                     g_param_spec_enum ("status",
                                                        "status",
                                                        "State of this snap",
                                                        SNAPD_TYPE_SNAP_STATUS, SNAPD_SNAP_STATUS_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SUMMARY,
                                     g_param_spec_string ("summary",
                                                          "summary",
                                                          "One line description",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TRYMODE,
                                     g_param_spec_boolean ("trymode",
                                                           "trymode",
                                                           "TRUE if this snap is installed in try mode",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP_TYPE,
                                     g_param_spec_enum ("snap-type",
                                                        "snap-type",
                                                        "Snap type",
                                                        SNAPD_TYPE_SNAP_TYPE, SNAPD_SNAP_TYPE_UNKNOWN,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_VERSION,
                                     g_param_spec_string ("version",
                                                          "version",
                                                          "Snap version",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_snap_init (SnapdSnap *snap)
{
}
