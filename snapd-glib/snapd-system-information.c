#include "config.h"

#include "snapd-system-information.h"

struct _SnapdSystemInformation
{
    GObject parent_instance;
};

G_DEFINE_TYPE (SnapdSystemInformation, snapd_system_information, G_TYPE_OBJECT)

static void
snapd_system_information_class_init (SnapdSystemInformationClass *klass)
{
}

static void
snapd_system_information_init (SnapdSystemInformation *system_information)
{
}
