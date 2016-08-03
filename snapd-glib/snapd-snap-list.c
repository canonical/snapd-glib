#include "config.h"

#include "snapd-snap-list.h"

struct _SnapdSnapList
{
    GObject parent_instance;

    GPtrArray *snaps;
};

G_DEFINE_TYPE (SnapdSnapList, snapd_snap_list, G_TYPE_OBJECT)

void
_snapd_snap_list_add (SnapdSnapList *snap_list, SnapdSnap *snap)
{
    g_ptr_array_add (snap_list->snaps, g_object_ref (snap));
}

gsize
snapd_snap_list_length (SnapdSnapList *snap_list)
{
    g_return_val_if_fail (SNAPD_IS_SNAP_LIST (snap_list), 0);
    return snap_list->snaps->len;
}

SnapdSnap *
snapd_snap_list_index (SnapdSnapList *snap_list, gsize index)
{
    g_return_val_if_fail (SNAPD_IS_SNAP_LIST (snap_list), NULL);  
    return snap_list->snaps->pdata[index];
}

static void
snapd_snap_list_finalize (GObject *object)
{
    SnapdSnapList *snap_list = SNAPD_SNAP_LIST (object);

    g_ptr_array_foreach (snap_list->snaps, (GFunc) g_object_unref, FALSE);
    g_ptr_array_free (snap_list->snaps, TRUE);
    snap_list->snaps = NULL;
}

static void
snapd_snap_list_class_init (SnapdSnapListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = snapd_snap_list_finalize;
}

static void
snapd_snap_list_init (SnapdSnapList *snap_list)
{
    snap_list->snaps = g_ptr_array_new ();
}
