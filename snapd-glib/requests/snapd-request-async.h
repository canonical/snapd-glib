/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_REQUEST_ASYNC_H__
#define __SNAPD_REQUEST_ASYNC_H__

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include "snapd-request.h"
#include "snapd-change.h"
#include "snapd-client.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE (SnapdRequestAsync, snapd_request_async, SNAPD, REQUEST_ASYNC, SnapdRequest)

struct _SnapdRequestAsyncClass
{
    SnapdRequestClass parent_class;

    gboolean (*parse_result)(SnapdRequestAsync *request, JsonNode *result, GError **error);
};

const gchar *_snapd_request_async_get_change_id   (SnapdRequestAsync *request);

gboolean     _snapd_request_async_parse_result    (SnapdRequestAsync *request,
                                                   JsonNode          *result,
                                                   GError           **error);

void         _snapd_request_async_report_progress (SnapdRequestAsync *request,
                                                   SnapdClient       *client,
                                                   SnapdChange       *change);

G_END_DECLS

#endif /* __SNAPD_REQUEST_ASYNC_H__ */
