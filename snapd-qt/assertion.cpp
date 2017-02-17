/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/assertion.h"

QSnapdAssertion::QSnapdAssertion (const QString& contents, QObject *parent) :
    QSnapdWrappedObject (NULL, g_object_unref, parent)
{
    wrapped_object = snapd_assertion_new (contents.toStdString ().c_str ());
}

QStringList QSnapdAssertion::headers () const
{
    g_auto(GStrv) headers = NULL;
    QStringList result;

    headers = snapd_assertion_get_headers (SNAPD_ASSERTION (wrapped_object));
    for (int i = 0; headers[i] != NULL; i++)
        result.append (headers[i]);
    return result;
}

QString QSnapdAssertion::header (const QString& name) const
{
    return snapd_assertion_get_header (SNAPD_ASSERTION (wrapped_object), name.toStdString ().c_str ());
}

QString QSnapdAssertion::body () const
{
    return snapd_assertion_get_body (SNAPD_ASSERTION (wrapped_object));
}

QString QSnapdAssertion::signature () const
{
    return snapd_assertion_get_signature (SNAPD_ASSERTION (wrapped_object));
}
