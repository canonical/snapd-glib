/*
 * Copyright (C) 2025 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

/*
 * This function is declared in this private header because it must be private,
 * but also is needed in the tests.
 */

static GSocket *open_snapd_socket(const gchar *socket_path,
                                  GCancellable *cancellable, GError **error) {
  g_autoptr(GError) error_local = NULL;
  g_autoptr(GSocket) socket =
      g_socket_new(G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM,
                   G_SOCKET_PROTOCOL_DEFAULT, &error_local);
  if (socket == NULL) {
    g_set_error(error, SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED,
                "Unable to create snapd socket: %s", error_local->message);
    return NULL;
  }
  g_socket_set_blocking(socket, FALSE);
  g_autoptr(GSocketAddress) address = NULL;
  if (socket_path[0] == '@') {
    address = g_unix_socket_address_new_with_type(
        socket_path + 1, -1, G_UNIX_SOCKET_ADDRESS_ABSTRACT);
  } else {
    address = g_unix_socket_address_new(socket_path);
  }
  if (!g_socket_connect(socket, address, cancellable, &error_local)) {
    g_set_error(error, SNAPD_ERROR, SNAPD_ERROR_CONNECTION_FAILED,
                "Unable to connect snapd socket: %s", error_local->message);
    return NULL;
  }

  return g_steal_pointer(&socket);
}
