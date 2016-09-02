/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-error.h"

/**
 * SECTION:snapd-error
 * @short_description: Snap error codes
 * @include: snapd-glib/snapd-glib.h
 *
 * Error code definitions for various snapd operations.
 */

/**
 * snapd_error_quark:
 *
 * Gets the Snapd Error Quark.
 *
 * Returns: a #GQuark.
 **/
G_DEFINE_QUARK (snapd-error-quark, snapd_error)
