/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/markdown-parser.h"

class QSnapdMarkdownParserPrivate {
public:
  QSnapdMarkdownParserPrivate(QSnapdMarkdownParser::MarkdownVersion version) {
    SnapdMarkdownVersion v;
    switch (version) {
    default:
    case QSnapdMarkdownParser::MarkdownVersion0:
      v = SNAPD_MARKDOWN_VERSION_0;
      break;
    }
    parser = snapd_markdown_parser_new(v);
  }

  ~QSnapdMarkdownParserPrivate() { g_object_unref(parser); }

  SnapdMarkdownParser *parser;
};

QSnapdMarkdownParser::QSnapdMarkdownParser(
    QSnapdMarkdownParser::MarkdownVersion version, QObject *parent)
    : QObject(parent), d_ptr(new QSnapdMarkdownParserPrivate(version)) {}

QSnapdMarkdownParser::~QSnapdMarkdownParser() {}

void QSnapdMarkdownParser::setPreserveWhitespace(
    bool preserveWhitespace) const {
  Q_D(const QSnapdMarkdownParser);
  snapd_markdown_parser_set_preserve_whitespace(d->parser, preserveWhitespace);
}

bool QSnapdMarkdownParser::preserveWhitespace() const {
  Q_D(const QSnapdMarkdownParser);
  return snapd_markdown_parser_get_preserve_whitespace(d->parser);
}

QList<QSnapdMarkdownNode>
QSnapdMarkdownParser::parse(const QString &text) const {
  Q_D(const QSnapdMarkdownParser);
  g_autoptr(GPtrArray) nodes =
      snapd_markdown_parser_parse(d->parser, text.toUtf8().constData());
  QList<QSnapdMarkdownNode> nodes_list;
  for (uint i = 0; i < nodes->len; i++) {
    SnapdMarkdownNode *node = (SnapdMarkdownNode *)g_ptr_array_index(nodes, i);
    nodes_list.append(QSnapdMarkdownNode(node));
  }
  return nodes_list;
}
