#include "config.h"

#include <string.h>

#include "snapd-app.h"

struct _SnapdApp
{
    GObject parent_instance;

    gchar *name;
};

enum 
{
    PROP_NAME = 1,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdApp, snapd_app, G_TYPE_OBJECT)

/**
 * snapd_app_get_name:
 * @app: a #SnapdApp.
 *
 * Returns: the name of this app.
 */
const gchar *
snapd_app_get_name (SnapdApp *app)
{
    g_return_val_if_fail (SNAPD_IS_APP (app), NULL);
    return app->name;
}

static void
snapd_app_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdApp *app = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_free (app->name);
        app->name = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdApp *app = SNAPD_APP (object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, app->name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_app_finalize (GObject *object)
{
    SnapdApp *app = SNAPD_APP (object);

    g_clear_pointer (&app->name, g_free);
}

static void
snapd_app_class_init (SnapdAppClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_app_set_property;
    gobject_class->get_property = snapd_app_get_property; 
    gobject_class->finalize = snapd_app_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NAME,
                                     g_param_spec_string ("name",
                                                          "name",
                                                          "App name",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_app_init (SnapdApp *app)
{
}
