#include "config.h"

#include "snapd-auth-data.h"

struct _SnapdAuthData
{
    GObject parent_instance;
};
 
G_DEFINE_TYPE (SnapdAuthData, snapd_auth_data, G_TYPE_OBJECT)

static void
snapd_auth_data_class_init (SnapdAuthDataClass *klass)
{
}

static void
snapd_auth_data_init (SnapdAuthData *auth_data)
{
}
