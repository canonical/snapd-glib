/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-auth-data.h"

/**
 * SECTION: snapd-auth-data
 * @short_description: Authorization data
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdAuthData contains authorization data to communicate with snapd.
 * Authenticating with snapd_login_sync() or snapd_client_login_sync() returns
 * authorization data that can be used for requests by calling
 * snapd_client_set_auth_data().
 *
 * It is recommended that the data is securely stored between sessions so
 * authentication is not required to be repeated. The authorization data is
 * made up of printable strings that can be easily written to a file/database.
 */

/**
 * SnapdAuthData:
 *
 * #SnapdAuthData contains authorization data used to communicate with snapd.
 *
 * The authorization data is in the form of a
 * [Macaroon](https://research.google.com/pubs/pub41892.html).
 *
 * Since: 1.0
 */

struct _SnapdAuthData {
  GObject parent_instance;

  gchar *macaroon;
  GStrv discharges;
};

enum { PROP_MACAROON = 1, PROP_DISCHARGES, PROP_LAST };

G_DEFINE_TYPE(SnapdAuthData, snapd_auth_data, G_TYPE_OBJECT)

/**
 * snapd_auth_data_new:
 * @macaroon: serialized macaroon used to authorize access to snapd.
 * @discharges: (array zero-terminated=1): serialized discharges.
 *
 * Create some authorization data.
 *
 * Returns: a new #SnapdAuthData
 *
 * Since: 1.0
 **/
SnapdAuthData *snapd_auth_data_new(const gchar *macaroon, GStrv discharges) {
  g_return_val_if_fail(macaroon != NULL, NULL);
  return g_object_new(SNAPD_TYPE_AUTH_DATA, "macaroon", macaroon, "discharges",
                      discharges, NULL);
}

/**
 * snapd_auth_data_get_macaroon:
 * @auth_data: a #SnapdAuthData.
 *
 * Get the Macaroon that this authorization uses.
 *
 * Returns: the serialized Macaroon used to authorize access to snapd.
 *
 * Since: 1.0
 */
const gchar *snapd_auth_data_get_macaroon(SnapdAuthData *self) {
  g_return_val_if_fail(SNAPD_IS_AUTH_DATA(self), NULL);
  return self->macaroon;
}

/**
 * snapd_auth_data_get_discharges:
 * @auth_data: a #SnapdAuthData.
 *
 * Get the discharges that this authorization uses.
 *
 * Returns: (transfer none) (array zero-terminated=1): the discharges as
 * serialized strings.
 *
 * Since: 1.0
 */
GStrv snapd_auth_data_get_discharges(SnapdAuthData *self) {
  g_return_val_if_fail(SNAPD_IS_AUTH_DATA(self), NULL);
  return self->discharges;
}

static void snapd_auth_data_set_property(GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec) {
  SnapdAuthData *self = SNAPD_AUTH_DATA(object);

  switch (prop_id) {
  case PROP_MACAROON:
    g_free(self->macaroon);
    self->macaroon = g_strdup(g_value_get_string(value));
    break;
  case PROP_DISCHARGES:
    g_strfreev(self->discharges);
    self->discharges = g_strdupv(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_auth_data_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec) {
  SnapdAuthData *self = SNAPD_AUTH_DATA(object);

  switch (prop_id) {
  case PROP_MACAROON:
    g_value_set_string(value, self->macaroon);
    break;
  case PROP_DISCHARGES:
    g_value_set_boxed(value, self->discharges);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_auth_data_finalize(GObject *object) {
  SnapdAuthData *self = SNAPD_AUTH_DATA(object);

  g_clear_pointer(&self->macaroon, g_free);
  g_clear_pointer(&self->discharges, g_strfreev);

  G_OBJECT_CLASS(snapd_auth_data_parent_class)->finalize(object);
}

static void snapd_auth_data_class_init(SnapdAuthDataClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_auth_data_set_property;
  gobject_class->get_property = snapd_auth_data_get_property;
  gobject_class->finalize = snapd_auth_data_finalize;

  g_object_class_install_property(
      gobject_class, PROP_MACAROON,
      g_param_spec_string("macaroon", "macaroon", "Serialized macaroon", NULL,
                          G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_DISCHARGES,
      g_param_spec_boxed("discharges", "discharges", "Serialized discharges",
                         G_TYPE_STRV,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME |
                             G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void snapd_auth_data_init(SnapdAuthData *self) {}
