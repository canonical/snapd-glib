#include "config.h"

#include "snapd-client.h"

typedef struct
{
  int dummy;
} SnapdClientPrivate;
 
G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

static void
snapd_client_class_init (SnapdClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
}

static void
snapd_client_init (SnapdClient *client)
{
}
