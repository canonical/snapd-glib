/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_GET_FIND_H__
#define __SNAPD_GET_FIND_H__

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (SnapdGetFind, snapd_get_find, SNAPD, GET_FIND, SnapdRequest)

SnapdGetFind *_snapd_get_find_new                    (GCancellable        *cancellable,
                                                      GAsyncReadyCallback  callback,
                                                      gpointer             user_data);

void          _snapd_get_find_set_query              (SnapdGetFind        *request,
                                                      const gchar         *query);

void          _snapd_get_find_set_name               (SnapdGetFind        *request,
                                                      const gchar         *name);

void          _snapd_get_find_set_select             (SnapdGetFind        *request,
                                                      const gchar         *select);

void          _snapd_get_find_set_section            (SnapdGetFind        *request,
                                                      const gchar         *section);

void          _snapd_get_find_set_scope             (SnapdGetFind        *request,
                                                      const gchar         *scope);

GPtrArray    *_snapd_get_find_get_snaps              (SnapdGetFind        *request);

const gchar  *_snapd_get_find_get_suggested_currency (SnapdGetFind        *request);

G_END_DECLS

#endif /* __SNAPD_GET_FIND_H__ */
