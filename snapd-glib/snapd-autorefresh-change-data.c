/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-autorefresh-change-data.h"

struct _SnapdAutorefreshChangeData {
  SnapdChangeData parent_instance;
  GStrv snap_names;
  GStrv refresh_forced;
};

G_DEFINE_TYPE(SnapdAutorefreshChangeData, snapd_autorefresh_change_data,
              SNAPD_TYPE_CHANGE_DATA)

enum { PROP_SNAP_NAMES = 1, PROP_REFRESH_FORCED, N_PROPERTIES };

/**
 * snapd_autorefresh_change_data_get_snap_names:
 * @change_data: a #SnapdAutorefreshChangeData
 *
 * return: (transfer none): a GStrv with the snap names, or NULL if the property
 * wasn't defined
 */
GStrv snapd_autorefresh_change_data_get_snap_names(
    SnapdAutorefreshChangeData *self) {
  g_return_val_if_fail(SNAPD_IS_AUTOREFRESH_CHANGE_DATA(self), NULL);
  return self->snap_names;
}

/**
 * snapd_autorefresh_change_data_get_refresh_forced:
 * @change_data: a #SnapdAutorefreshChangeData
 *
 * return: (transfer none): a GStrv with the snap names, or NULL if the property
 * wasn't defined
 */
GStrv snapd_autorefresh_change_data_get_refresh_forced(
    SnapdAutorefreshChangeData *self) {
  g_return_val_if_fail(SNAPD_IS_AUTOREFRESH_CHANGE_DATA(self), NULL);
  return self->refresh_forced;
}

static void snapd_autorefresh_change_data_set_property(GObject *object,
                                                       guint prop_id,
                                                       const GValue *value,
                                                       GParamSpec *pspec) {
  SnapdAutorefreshChangeData *self = SNAPD_AUTOREFRESH_CHANGE_DATA(object);

  switch (prop_id) {
  case PROP_SNAP_NAMES:
    g_strfreev(self->snap_names);
    self->snap_names = g_strdupv(g_value_get_boxed(value));
    break;
  case PROP_REFRESH_FORCED:
    g_strfreev(self->refresh_forced);
    self->refresh_forced = g_strdupv(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_autorefresh_change_data_get_property(GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec) {
  SnapdAutorefreshChangeData *self = SNAPD_AUTOREFRESH_CHANGE_DATA(object);

  switch (prop_id) {
  case PROP_SNAP_NAMES:
    g_value_set_boxed(value, self->snap_names);
    break;
  case PROP_REFRESH_FORCED:
    g_value_set_boxed(value, self->refresh_forced);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_autorefresh_change_data_finalize(GObject *object) {
  SnapdAutorefreshChangeData *self = SNAPD_AUTOREFRESH_CHANGE_DATA(object);

  g_clear_pointer(&self->snap_names, g_strfreev);
  g_clear_pointer(&self->refresh_forced, g_strfreev);

  G_OBJECT_CLASS(snapd_autorefresh_change_data_parent_class)->finalize(object);
}

static void snapd_autorefresh_change_data_class_init(
    SnapdAutorefreshChangeDataClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_autorefresh_change_data_set_property;
  gobject_class->get_property = snapd_autorefresh_change_data_get_property;
  gobject_class->finalize = snapd_autorefresh_change_data_finalize;

  g_object_class_install_property(
      gobject_class, PROP_SNAP_NAMES,
      g_param_spec_boxed("snap-names", "Snap Names",
                         "Names of the snaps that have been autorefreshed.",
                         G_TYPE_STRV,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property(
      gobject_class, PROP_REFRESH_FORCED,
      g_param_spec_boxed(
          "refresh-forced", "Refresh forced",
          "Names of the snaps that have been forced to autorefresh.",
          G_TYPE_STRV, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void
snapd_autorefresh_change_data_init(SnapdAutorefreshChangeData *self) {}
