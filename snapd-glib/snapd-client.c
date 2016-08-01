#include "config.h"

#include <gio/gunixsocketaddress.h>

#include "snapd-client.h"

typedef struct
{
    GSocket *snapd_socket;
} SnapdClientPrivate;
 
G_DEFINE_TYPE_WITH_PRIVATE (SnapdClient, snapd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (snapd-client-error-quark, snapd_client_error)

// snapd API documentation is at https://github.com/snapcore/snapd/blob/master/docs/rest.md

#define SNAPD_SOCKET "/run/snapd.socket"

gboolean
snapd_client_connect_sync (SnapdClient *client, GCancellable *cancellable, GError **error)
{
    SnapdClientPrivate *priv = snapd_client_get_instance_private (client);
    g_autoptr(GSocketAddress) address = NULL;
    g_autoptr(GError) error_local = NULL;

    g_return_val_if_fail (SNAPD_IS_CLIENT (client), FALSE);  
    g_return_val_if_fail (priv->snapd_socket == NULL, FALSE);

    priv->snapd_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                       G_SOCKET_TYPE_STREAM,
                                       G_SOCKET_PROTOCOL_DEFAULT,
                                       &error_local);
    if (priv->snapd_socket == NULL) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
                     "Unable to open snapd socket: %s",
                     error_local->message);
        return FALSE;
    }
    address = g_unix_socket_address_new (SNAPD_SOCKET);
    if (!g_socket_connect (priv->snapd_socket, address, cancellable, &error_local)) {
        g_set_error (error,
                     SNAPD_CLIENT_ERROR,
                     SNAPD_CLIENT_ERROR_CONNECTION_FAILED,
                     "Unable to connect snapd socket: %s",
                     error_local->message);
        g_clear_object (&priv->snapd_socket);
        return FALSE;
    }
}

static void
snapd_client_class_init (SnapdClientClass *klass)
{
   GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
}

static void
snapd_client_init (SnapdClient *client)
{
}
