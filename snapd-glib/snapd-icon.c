#include "config.h"

#include <string.h>

#include "snapd-icon.h"

struct _SnapdIcon
{
    GObject parent_instance;

    gchar *mime_type;
    guint8 *data;
    gsize data_length;
};

enum 
{
    PROP_MIME_TYPE = 1,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdIcon, snapd_icon, G_TYPE_OBJECT)

SnapdIcon *
snapd_icon_new (void)
{
    return g_object_new (SNAPD_TYPE_ICON, NULL);
}

const gchar *
snapd_icon_get_mime_type (SnapdIcon *icon)
{
    g_return_val_if_fail (SNAPD_IS_ICON (icon), NULL);
    return icon->mime_type;
}

void
_snapd_icon_set_data (SnapdIcon *icon, const guint8 *data, gsize data_length)
{
    g_return_if_fail (SNAPD_IS_ICON (icon));
    g_free (icon->data);
    icon->data = g_malloc (data_length);
    memcpy (icon->data, data, data_length);
    icon->data_length = data_length;
}

const guint8 *
snapd_icon_get_data (SnapdIcon *icon)
{
    g_return_val_if_fail (SNAPD_IS_ICON (icon), 0);
    return icon->data;
}

gsize
snapd_icon_get_data_length (SnapdIcon *icon)
{
    g_return_val_if_fail (SNAPD_IS_ICON (icon), 0);  
    return icon->data_length;
}

static void
snapd_icon_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdIcon *icon = SNAPD_ICON (object);

    switch (prop_id) {
    case PROP_MIME_TYPE:
        g_free (icon->mime_type);
        icon->mime_type = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_icon_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdIcon *icon = SNAPD_ICON (object);

    switch (prop_id) {
    case PROP_MIME_TYPE:
        g_value_set_string (value, icon->mime_type);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_icon_finalize (GObject *object)
{
    SnapdIcon *icon = SNAPD_ICON (object);

    g_clear_pointer (&icon->mime_type, g_free);
    g_clear_pointer (&icon->data, g_free);  
}

static void
snapd_icon_class_init (SnapdIconClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_icon_set_property;
    gobject_class->get_property = snapd_icon_get_property; 
    gobject_class->finalize = snapd_icon_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_MIME_TYPE,
                                     g_param_spec_string ("mime-type",
                                                          "mime-type",
                                                          "Icon mime type",
                                                          NULL,
                                                          G_PARAM_READWRITE));
}

static void
snapd_icon_init (SnapdIcon *icon)
{
}
