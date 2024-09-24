/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#pragma once

#include "snapd-request.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(SnapdGetSections, snapd_get_sections, SNAPD, GET_SECTIONS,
                     SnapdRequest)

SnapdGetSections *_snapd_get_sections_new(GCancellable *cancellable,
                                          GAsyncReadyCallback callback,
                                          gpointer user_data);

GStrv _snapd_get_sections_get_sections(SnapdGetSections *request);

G_END_DECLS
