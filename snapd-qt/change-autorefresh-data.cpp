/*
 * Copyright (C) 2024 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/change-autorefresh-data.h"

QSnapdAutorefreshChangeData::QSnapdAutorefreshChangeData(void *snapd_object,
                                                         QObject *parent)
    : QSnapdChangeData(snapd_object, parent) {}

static QStringList gstrv_to_qstringlist(GStrv data) {
  QStringList retval;

  if (data != NULL)
    for (gchar **element = data; *element != NULL; element++)
      retval << *element;
  return retval;
}

QStringList QSnapdAutorefreshChangeData::snapNames() const {
  QStringList list;

  GStrv data = snapd_autorefresh_change_data_get_snap_names(
      SNAPD_AUTOREFRESH_CHANGE_DATA(wrapped_object));
  return gstrv_to_qstringlist(data);
}

QStringList QSnapdAutorefreshChangeData::refreshForced() const {
  QStringList list;

  GStrv data = snapd_autorefresh_change_data_get_refresh_forced(
      SNAPD_AUTOREFRESH_CHANGE_DATA(wrapped_object));
  return gstrv_to_qstringlist(data);
}
