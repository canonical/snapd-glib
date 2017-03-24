/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef STREAM_WRAPPER_H
#define STREAM_WRAPPER_H

#include <glib-object.h>
#include <gio/gio.h>

#include <QPointer>
#include <QIODevice>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (StreamWrapper, stream_wrapper, SNAPD, STREAM_WRAPPER, GInputStream)

struct _StreamWrapper
{
    GInputStream parent_instance;
    QPointer<QIODevice> ioDevice;
};

struct _StreamWrapperClass
{
    GInputStreamClass parent_class;
};

G_END_DECLS

#endif
