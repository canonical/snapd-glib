/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_ASSERTION_H__
#define __SNAPD_ASSERTION_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_ASSERTION  (snapd_assertion_get_type ())

G_DECLARE_FINAL_TYPE (SnapdAssertion, snapd_assertion, SNAPD, ASSERTION, GObject)

SnapdAssertion *snapd_assertion_new                   (const gchar    *content);

gchar         **snapd_assertion_get_headers           (SnapdAssertion *assertion);

gchar          *snapd_assertion_get_header            (SnapdAssertion *assertion,
                                                       const gchar    *name);

gchar          *snapd_assertion_get_body              (SnapdAssertion *assertion);

gchar          *snapd_assertion_get_signature         (SnapdAssertion *assertion);

G_END_DECLS

#endif /* __SNAPD_ASSERTION_H__ */
