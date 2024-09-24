/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "stream-wrapper.h"

#include <QIODevice>

G_DEFINE_TYPE(StreamWrapper, stream_wrapper, G_TYPE_INPUT_STREAM)

static gssize stream_wrapper_read_fn(GInputStream *stream, void *buffer,
                                     gsize count, GCancellable *cancellable,
                                     GError **error) {
  StreamWrapper *wrapper = SNAPD_STREAM_WRAPPER(stream);
  qint64 nRead;

  if (wrapper->ioDevice == NULL)
    return 0;

  nRead = wrapper->ioDevice->read((char *)buffer, count);
  if (nRead >= 0)
    return nRead;

  g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                      wrapper->ioDevice->errorString().toStdString().c_str());
  return -1;
}

static gboolean stream_wrapper_close_fn(GInputStream *stream,
                                        GCancellable *cancellable,
                                        GError **error) {
  StreamWrapper *wrapper = SNAPD_STREAM_WRAPPER(stream);

  if (wrapper->ioDevice == NULL)
    return TRUE;

  wrapper->ioDevice->close();
  return TRUE;
}

static void stream_wrapper_init(StreamWrapper *wrapper) {}

static void stream_wrapper_class_init(StreamWrapperClass *klass) {
  GInputStreamClass *input_stream_class = G_INPUT_STREAM_CLASS(klass);

  input_stream_class->read_fn = stream_wrapper_read_fn;
  input_stream_class->close_fn = stream_wrapper_close_fn;
}
