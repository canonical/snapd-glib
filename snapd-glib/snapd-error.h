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
 * @SNAPD_ERROR_WRITE_ERROR: an error occurred while writing to snapd.
 * @SNAPD_ERROR_READ_ERROR: an error occured while reading from snapd.
 * @SNAPD_ERROR_PARSE_ERROR: the data received from snapd was not understood.
 * @SNAPD_ERROR_GENERAL_ERROR: an unspecified error occurred while communicating
 *     with snapd.
 * @SNAPD_ERROR_LOGIN_REQUIRED: the requested operation requires a login to be
 *     performed.
 * @SNAPD_ERROR_INVALID_AUTH_DATA: the provided authorization data is invalid.
 * @SNAPD_ERROR_TWO_FACTOR_REQUIRED: login requires a two factor code.
 * @SNAPD_ERROR_TWO_FACTOR_FAILED: the two factor code provided at login is
 *     invalid.
 * @SNAPD_ERROR_BAD_REQUEST: snapd did not understand the request that was sent.
 * @SNAPD_ERROR_PERMISSION_DENIED: this user account is not permitted to perform
 *     the requested operation.
 *
 * Error codes returned by snapd operations.
 */
typedef enum
{  
    SNAPD_ERROR_CONNECTION_FAILED,
    SNAPD_ERROR_WRITE_ERROR,
    SNAPD_ERROR_READ_ERROR,
    SNAPD_ERROR_PARSE_ERROR,
    SNAPD_ERROR_GENERAL_ERROR,
    SNAPD_ERROR_LOGIN_REQUIRED,
    SNAPD_ERROR_INVALID_AUTH_DATA,  
    SNAPD_ERROR_TWO_FACTOR_REQUIRED,
    SNAPD_ERROR_TWO_FACTOR_FAILED,
    SNAPD_ERROR_BAD_REQUEST,
    SNAPD_ERROR_PERMISSION_DENIED,
    SNAPD_ERROR_LAST
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
