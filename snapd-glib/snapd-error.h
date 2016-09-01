/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_ERROR_H__
#define __SNAPD_ERROR_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SnapdError:
 * @SNAPD_ERROR_CONNECTION_FAILED: not able to connect to snapd.
 * @SNAPD_ERROR_WRITE_FAILED: an error occurred while writing to snapd.
 * @SNAPD_ERROR_READ_FAILED: an error occured while reading from snapd.
 * @SNAPD_ERROR_BAD_REQUEST: snapd did not understand the request that was sent.
 * @SNAPD_ERROR_BAD_RESPONSE: the response received from snapd was not
 *     understood.
 * @SNAPD_ERROR_AUTH_DATA_REQUIRED: the requested operation requires
 *     authorization data.
 * @SNAPD_ERROR_AUTH_DATA_INVALID: the provided authorization data is invalid.
 * @SNAPD_ERROR_TWO_FACTOR_REQUIRED: login requires a two factor code.
 * @SNAPD_ERROR_TWO_FACTOR_INVALID: the two factor code provided at login is
 *     invalid.
 * @SNAPD_ERROR_PERMISSION_DENIED: this user account is not permitted to perform
 *     the requested operation.
 * @SNAPD_ERROR_FAILED: an unspecified error occurred while communicating
 *     with snapd.
 *
 * Error codes returned by snapd operations.
 */
typedef enum
{  
    SNAPD_ERROR_CONNECTION_FAILED,
    SNAPD_ERROR_WRITE_FAILED,
    SNAPD_ERROR_READ_FAILED,
    SNAPD_ERROR_BAD_REQUEST,
    SNAPD_ERROR_BAD_RESPONSE,
    SNAPD_ERROR_AUTH_DATA_REQUIRED,
    SNAPD_ERROR_AUTH_DATA_INVALID,
    SNAPD_ERROR_TWO_FACTOR_REQUIRED,
    SNAPD_ERROR_TWO_FACTOR_INVALID,
    SNAPD_ERROR_PERMISSION_DENIED,
    SNAPD_ERROR_FAILED
} SnapdError;

/**
 * SNAPD_ERROR:
 *
 * Error domain for errors returned by snapd. Errors in this domain will
 * be from the #SnapdError enumeration. See #GError for information
 * on error domains.
 */
#define SNAPD_ERROR snapd_error_quark ()

GQuark                  snapd_error_quark                          (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __SNAPD_ERROR_H__ */
