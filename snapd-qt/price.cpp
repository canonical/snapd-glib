/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/price.h"

QSnapdPrice::QSnapdPrice (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}

double QSnapdPrice::amount () const
{
    return snapd_price_get_amount (SNAPD_PRICE (wrapped_object));
}

QString QSnapdPrice::currency () const
{
    return snapd_price_get_currency (SNAPD_PRICE (wrapped_object));
}
