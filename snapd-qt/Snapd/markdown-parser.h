/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_MARKDOWN_PARSER_H
#define SNAPD_MARKDOWN_PARSER_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <Snapd/MarkdownNode>

class QSnapdMarkdownParserPrivate;
#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdMarkdownParser : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool preserveWhitespace READ preserveWhitespace WRITE
                 setPreserveWhitespace)

public:
  enum MarkdownVersion {
    MarkdownVersion0,
  };
  Q_ENUM(MarkdownVersion)
  explicit QSnapdMarkdownParser(MarkdownVersion version, QObject *parent = 0);
  ~QSnapdMarkdownParser();

  void setPreserveWhitespace(bool preserveWhitespace) const;
  bool preserveWhitespace() const;
  QList<QSnapdMarkdownNode> parse(const QString &text) const;

private:
  QScopedPointer<QSnapdMarkdownParserPrivate> d_ptr;
  Q_DECLARE_PRIVATE(QSnapdMarkdownParser)
};

#endif
