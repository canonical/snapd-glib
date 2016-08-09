#include "config.h"

#include <string.h>

#include "snapd-plug.h"

struct _SnapdPlug
{
    GObject parent_instance;

    gchar *name;
    gchar *snap;
    gchar *interface;
    gchar *label;
    GPtrArray *connections;
};

enum 
{
    PROP_NAME = 1,
    PROP_SNAP,
    PROP_INTERFACE,
    PROP_LABEL,  
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdPlug, snapd_plug, G_TYPE_OBJECT)

const gchar *
snapd_plug_get_name (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->name;
}

const gchar *
snapd_plug_get_snap (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->snap;
}

const gchar *
snapd_plug_get_interface (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->interface;
}

const gchar *
snapd_plug_get_label (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->label;
}

GPtrArray *
snapd_plug_get_connections (SnapdPlug *plug)
{
    g_return_val_if_fail (SNAPD_IS_PLUG (plug), NULL);
    return plug->connections;
}

static void
snapd_plug_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (plug->name);
        plug->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_SNAP:
        g_free (plug->snap);
        plug->snap = g_strdup (g_value_get_string (value));
        break;
    case PROP_INTERFACE:
        g_free (plug->interface);
        plug->interface = g_strdup (g_value_get_string (value));
        break;
    case PROP_LABEL:
        g_free (plug->label);
        plug->label = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, plug->name);
        break;
    case PROP_SNAP:
        g_value_set_string (value, plug->snap);
        break;
    case PROP_INTERFACE:
        g_value_set_string (value, plug->interface);
        break;
    case PROP_LABEL:
        g_value_set_string (value, plug->label);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_plug_finalize (GObject *object)
{
    SnapdPlug *plug = SNAPD_PLUG (object);

    g_clear_pointer (&plug->name, g_free);
    g_clear_pointer (&plug->snap, g_free);
    g_clear_pointer (&plug->interface, g_free);
    g_clear_pointer (&plug->label, g_free);
    g_clear_pointer (&plug->connections, g_ptr_array_unref);
}

static void
snapd_plug_class_init (SnapdPlugClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_plug_set_property;
    gobject_class->get_property = snapd_plug_get_property; 
    gobject_class->finalize = snapd_plug_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "Plug name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SNAP,
                                     g_param_spec_string ("snap",
                                                          "snap",
                                                          "Snap this plug is on",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_INTERFACE,
                                     g_param_spec_string ("interface",
                                                          "interface",
                                                          "Interface this plug provides",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_LABEL,
                                     g_param_spec_string ("label",
                                                          "label",
                                                          "Short description of this plug",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_plug_init (SnapdPlug *plug)
{
    plug->connections = g_ptr_array_new_with_free_func (g_object_unref);
}
