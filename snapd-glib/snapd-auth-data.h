/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_AUTH_DATA_H__
#define __SNAPD_AUTH_DATA_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_AUTH_DATA  (snapd_auth_data_get_type ())

G_DECLARE_FINAL_TYPE (SnapdAuthData, snapd_auth_data, SNAPD, AUTH_DATA, GObject)

struct _SnapdAuthDataClass
{
    /*< private >*/
    GObjectClass parent_class;
};

SnapdAuthData  *snapd_auth_data_new                 (const gchar    *macaroon,
                                                     gchar         **discharges);

const gchar    *snapd_auth_data_get_macaroon        (SnapdAuthData  *auth_data);

gchar         **snapd_auth_data_get_discharges      (SnapdAuthData  *auth_data);

G_END_DECLS

#endif /* __SNAPD_AUTH_DATA_H__ */
