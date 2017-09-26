/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/alias.h"

QSnapdAlias::QSnapdAlias (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

QString QSnapdAlias::app () const
{
    return snapd_alias_get_app (SNAPD_ALIAS (wrapped_object));
}

QString QSnapdAlias::name () const
{
    return snapd_alias_get_name (SNAPD_ALIAS (wrapped_object));
}

QString QSnapdAlias::snap () const
{
    return snapd_alias_get_snap (SNAPD_ALIAS (wrapped_object));
}

QSnapdEnums::AliasStatus QSnapdAlias::status () const
{
    switch (snapd_alias_get_status (SNAPD_ALIAS (wrapped_object)))
    {
    default:
    case SNAPD_ALIAS_STATUS_UNKNOWN:
        return QSnapdEnums::AliasStatusUnknown;
    case SNAPD_ALIAS_STATUS_DEFAULT:
        return QSnapdEnums::AliasStatusDefault;
    case SNAPD_ALIAS_STATUS_ENABLED:
        return QSnapdEnums::AliasStatusEnabled;
    case SNAPD_ALIAS_STATUS_DISABLED:
        return QSnapdEnums::AliasStatusDisabled;
    case SNAPD_ALIAS_STATUS_AUTO:
      return QSnapdEnums::AliasStatusAuto;
    }
}
