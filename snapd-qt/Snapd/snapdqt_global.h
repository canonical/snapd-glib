/*
 * Copyright (C) 2025 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <QtCore/QtGlobal>

#if defined(LIBSNAPDQT)
#define LIBSNAPDQT_EXPORT __attribute__((visibility("default")))
#else
#define LIBSNAPDQT_EXPORT Q_DECL_IMPORT
#endif
