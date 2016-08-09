#include "config.h"

#include <string.h>

#include "snapd-interfaces.h"

struct _SnapdInterfaces
{
    GObject parent_instance;

    GPtrArray *plugs;
    GPtrArray *slots;  
};

G_DEFINE_TYPE (SnapdInterfaces, snapd_interfaces, G_TYPE_OBJECT)

void
_snapd_interfaces_add_plug (SnapdInterfaces *interfaces, SnapdPlug *plug)
{
    g_return_if_fail (SNAPD_IS_INTERFACES (interfaces));
    g_ptr_array_add (interfaces->plugs, g_object_ref (plug));  
}

void
_snapd_interfaces_add_slot (SnapdInterfaces *interfaces, SnapdSlot *slot)
{
    g_return_if_fail (SNAPD_IS_INTERFACES (interfaces));
    g_ptr_array_add (interfaces->slots, g_object_ref (slot));
}

GPtrArray *
snapd_interfaces_get_plugs (SnapdInterfaces *interfaces)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACES (interfaces), NULL);
    return interfaces->plugs;
}

GPtrArray *
snapd_interfaces_get_slots (SnapdInterfaces *interfaces)
{
    g_return_val_if_fail (SNAPD_IS_INTERFACES (interfaces), NULL);
    return interfaces->slots;
}

static void
snapd_interfaces_finalize (GObject *object)
{
    SnapdInterfaces *interfaces = SNAPD_INTERFACES (object);

    g_clear_pointer (&interfaces->plugs, g_ptr_array_unref);
    g_clear_pointer (&interfaces->slots, g_ptr_array_unref);  
}

static void
snapd_interfaces_class_init (SnapdInterfacesClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = snapd_interfaces_finalize;
}

static void
snapd_interfaces_init (SnapdInterfaces *interfaces)
{
    interfaces->plugs = g_ptr_array_new_with_free_func (g_object_unref);
    interfaces->slots = g_ptr_array_new_with_free_func (g_object_unref);
}
