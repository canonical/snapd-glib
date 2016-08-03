#include "config.h"

#include "snapd-auth-data.h"

struct _SnapdAuthData
{
    GObject parent_instance;

    gchar *macaroon;
    // FIXME: discharges
};

enum 
{
    PROP_MACAROON = 1,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdAuthData, snapd_auth_data, G_TYPE_OBJECT)

const gchar *
snapd_auth_data_get_macaroon (SnapdAuthData *auth_data)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), NULL);
    return auth_data->macaroon;
}

gsize
snapd_auth_data_get_discharge_count (SnapdAuthData *auth_data)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), 0);
    return 0; // FIXME
}

const gchar *
snapd_auth_data_get_discharge (SnapdAuthData *auth_data, int index)
{
    g_return_val_if_fail (SNAPD_IS_AUTH_DATA (auth_data), 0);  
    return NULL; // FIXME
}

static void
snapd_auth_data_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdAuthData *auth_data = SNAPD_AUTH_DATA (object);

    switch (prop_id) {
    case PROP_MACAROON:
        g_free (auth_data->macaroon);
        auth_data->macaroon = g_strdup (g_value_get_string (value));
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
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_auth_data_init (SnapdAuthData *auth_data)
{
}
