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

QSnapdAssertion::QSnapdAssertion (void *snapd_object, QObject *parent) : QSnapdWrappedObject (snapd_object, g_object_unref, parent) {}

QString QSnapdAssertion::type () const
{
    return snapd_assertion_get_assertion_type (SNAPD_ASSERTION (wrapped_object));
}

QString QSnapdAssertion::authorityId () const
{
    return snapd_assertion_get_authority_id (SNAPD_ASSERTION (wrapped_object));
}

QString QSnapdAssertion::revision () const
{
    return snapd_assertion_get_revision (SNAPD_ASSERTION (wrapped_object));
}

QString QSnapdAssertion::signKeySHA3_384 () const
{
    return snapd_assertion_get_sign_key_sha3_384 (SNAPD_ASSERTION (wrapped_object));
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
