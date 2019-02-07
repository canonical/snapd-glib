/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_MARKDOWN_NODE_H__
#define __SNAPD_MARKDOWN_NODE_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_MARKDOWN_NODE  (snapd_markdown_node_get_type ())

G_DECLARE_FINAL_TYPE (SnapdMarkdownNode, snapd_markdown_node, SNAPD, MARKDOWN_NODE, GObject)

/**
 * SnapdMarkdownNodeType:
 * @SNAPD_MARKDOWN_NODE_TYPE_TEXT: a piece of text.
 * @SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH: a paragraph.
 * @SNAPD_MARKDOWN_NODE_TYPE_UNORDERED_LIST: an unordered list.
 * @SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM: a list item.
 * @SNAPD_MARKDOWN_NODE_TYPE_CODE_BLOCK: a code block.
 * @SNAPD_MARKDOWN_NODE_TYPE_CODE_SPAN: a code span.
 * @SNAPD_MARKDOWN_NODE_TYPE_EMPHASIS: emphasised text.
 * @SNAPD_MARKDOWN_NODE_TYPE_STRONG_EMPHASIS: strongly emphasised text.
 * @SNAPD_MARKDOWN_NODE_TYPE_URL: a URL.
 *
 * Type of markdown node.
 *
 * Since: 1.48
 */
typedef enum
{
     SNAPD_MARKDOWN_NODE_TYPE_TEXT,
     SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH,
     SNAPD_MARKDOWN_NODE_TYPE_UNORDERED_LIST,
     SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM,
     SNAPD_MARKDOWN_NODE_TYPE_CODE_BLOCK,
     SNAPD_MARKDOWN_NODE_TYPE_CODE_SPAN,
     SNAPD_MARKDOWN_NODE_TYPE_EMPHASIS,
     SNAPD_MARKDOWN_NODE_TYPE_STRONG_EMPHASIS,
     SNAPD_MARKDOWN_NODE_TYPE_URL
} SnapdMarkdownNodeType;

SnapdMarkdownNodeType  snapd_markdown_node_get_node_type (SnapdMarkdownNode *node);

const gchar           *snapd_markdown_node_get_text      (SnapdMarkdownNode *node);

GPtrArray             *snapd_markdown_node_get_children  (SnapdMarkdownNode *node);

G_END_DECLS

#endif /* __SNAPD_MARKDOWN_NODE_H__ */
