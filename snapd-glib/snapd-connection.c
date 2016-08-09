#include "config.h"

#include <string.h>

#include "snapd-connection.h"

struct _SnapdConnection
{
    GObject parent_instance;

    gchar *name;
    gchar *snap;
};

enum 
{
    PROP_NAME = 1,
    PROP_SNAP,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdConnection, snapd_connection, G_TYPE_OBJECT)

const gchar *
snapd_connection_get_name (SnapdConnection *icon)
{
    g_return_val_if_fail (SNAPD_IS_CONNECTION (icon), NULL);
    return icon->name;
}

const gchar *
snapd_connection_get_snap (SnapdConnection *icon)
{
    g_return_val_if_fail (SNAPD_IS_CONNECTION (icon), NULL);
    return icon->snap;
}

static void
snapd_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdConnection *icon = SNAPD_CONNECTION (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (icon->name);
        icon->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (icon->snap);
        icon->snap = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_connection_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdConnection *icon = SNAPD_CONNECTION (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, icon->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, icon->snap);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_connection_finalize (GObject *object)
{
    SnapdConnection *icon = SNAPD_CONNECTION (object);

    g_clear_pointer (&icon->name, g_free);
    g_clear_pointer (&icon->snap, g_free);
}

static void
snapd_connection_class_init (SnapdConnectionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_connection_set_property;
    gobject_class->get_property = snapd_connection_get_property; 
    gobject_class->finalize = snapd_connection_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Name of connection/plug on snap",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this connection is made to",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_connection_init (SnapdConnection *icon)
{
}
