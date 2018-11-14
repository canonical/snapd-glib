/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_REQUEST_H__
#define __SNAPD_REQUEST_H__

#include <glib-object.h>
#include <libsoup/soup.h>

#include "snapd-maintenance.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE (SnapdRequest, snapd_request, SNAPD, REQUEST, GObject)

struct _SnapdRequestClass
{
    GObjectClass parent_class;

    SoupMessage *(*generate_request)(SnapdRequest *request);
    gboolean (*parse_response)(SnapdRequest *request, SoupMessage *message, SnapdMaintenance **maintenance, GError **error);
};

void          _snapd_request_set_source_object (SnapdRequest *request,
                                                GObject      *object);

GMainContext *_snapd_request_get_context       (SnapdRequest *request);

GCancellable *_snapd_request_get_cancellable   (SnapdRequest *request);

void          _snapd_request_generate          (SnapdRequest *request);

SoupMessage  *_snapd_request_get_message       (SnapdRequest *request);

void          _snapd_request_return            (SnapdRequest *request,
                                                GError       *error);

gboolean      _snapd_request_propagate_error   (SnapdRequest *request,
                                                GError      **error);

G_END_DECLS

#endif /* __SNAPD_REQUEST_H__ */
