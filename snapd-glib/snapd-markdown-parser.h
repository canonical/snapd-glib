/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_MARKDOWN_PARSER_H__
#define __SNAPD_MARKDOWN_PARSER_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_MARKDOWN_PARSER (snapd_markdown_parser_get_type())

/**
 * SnapdMarkdownVersion:
 * @SNAPD_MARKDOWN_VERSION_0: the initial version of Snap markdown.
 *
 * Version of markdown to parse. Picking a version will ensure only nodes of the
 * expected type are decoded.
 *
 * Since: 1.48
 */
typedef enum { SNAPD_MARKDOWN_VERSION_0 } SnapdMarkdownVersion;

G_DECLARE_FINAL_TYPE(SnapdMarkdownParser, snapd_markdown_parser, SNAPD,
                     MARKDOWN_PARSER, GObject)

SnapdMarkdownParser *snapd_markdown_parser_new(SnapdMarkdownVersion version);

void snapd_markdown_parser_set_preserve_whitespace(
    SnapdMarkdownParser *parser, gboolean preserve_whitespace);

gboolean
snapd_markdown_parser_get_preserve_whitespace(SnapdMarkdownParser *parser);

GPtrArray *snapd_markdown_parser_parse(SnapdMarkdownParser *parser,
                                       const gchar *text);

G_END_DECLS

#endif /* __SNAPD_MARKDOWN_PARSER_H__ */
