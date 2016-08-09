#include "config.h"

#include "snapd-auth-data.h"

struct _SnapdAuthData
{
    GObject parent_instance;

    gchar *macaroon;
    GPtrArray *discharges;
};

enum 
{
    PROP_MACAROON = 1,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdAuthData, snapd_auth_data, G_TYPE_OBJECT)

SnapdAuthData *
snapd_auth_data_new (void)
{
    return g_object_new (SNAPD_TYPE_AUTH_DATA, NULL);
}

void
snapd_auth_data_set_macaroon (SnapdAuthData *auth_data, const gchar *macaroon)
{
    g_return_if_fail (SNAPD_IS_AUTH_DATA (auth_data));
    g_free (auth_data->macaroon);
    auth_data->macaroon = g_strdup (macaroon);
}

const gchar *
snapd_auth_data_get_macaroon (SnapdAuthData *auth_data)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), NULL);
    return auth_data->macaroon;
}

void
snapd_auth_data_add_discharge (SnapdAuthData *auth_data, const gchar *discharge)
{
    g_return_if_fail (SNAPD_IS_AUTH_DATA (auth_data));
    g_ptr_array_add (auth_data->discharges, g_strdup (discharge));
}

gsize
snapd_auth_data_get_discharge_count (SnapdAuthData *auth_data)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), 0);
    return auth_data->discharges->len;
}

const gchar *
snapd_auth_data_get_discharge (SnapdAuthData *auth_data, int index)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), 0);  
    return g_ptr_array_index (auth_data->discharges, index);
}

static void
snapd_auth_data_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdAuthData *auth_data = SNAPD_AUTH_DATA (object);

    switch (prop_id) {
    case PROP_MACAROON:
        snapd_auth_data_set_macaroon (auth_data, g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_auth_data_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdAuthData *auth_data = SNAPD_AUTH_DATA (object);

    switch (prop_id) {
    case PROP_MACAROON:
        g_value_set_string (value, auth_data->macaroon);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_auth_data_finalize (GObject *object)
{
    SnapdAuthData *auth_data = SNAPD_AUTH_DATA (object);

    g_clear_pointer (&auth_data->macaroon, g_free);
    g_clear_pointer (&auth_data->discharges, g_ptr_array_unref);
}

static void
snapd_auth_data_class_init (SnapdAuthDataClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_auth_data_set_property;
    gobject_class->get_property = snapd_auth_data_get_property; 
    gobject_class->finalize = snapd_auth_data_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_MACAROON,
                                     g_param_spec_string ("macaroon",
                                                          "macaroon",
                                                          "Serialized macaroon",
                                                          NULL,
                                                          G_PARAM_READWRITE));
}

static void
snapd_auth_data_init (SnapdAuthData *auth_data)
{
    auth_data->discharges = g_ptr_array_new_with_free_func (g_free);
}
