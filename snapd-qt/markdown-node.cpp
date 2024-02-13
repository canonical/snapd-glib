/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <snapd-glib/snapd-glib.h>

#include "Snapd/markdown-node.h"

QSnapdMarkdownNode::QSnapdMarkdownNode (void *snapd_object, QObject *parent) : QSnapdWrappedObject (g_object_ref (snapd_object), g_object_unref, parent) {}
QSnapdMarkdownNode::QSnapdMarkdownNode (const QSnapdMarkdownNode &node) : QSnapdMarkdownNode (node.wrapped_object, node.parent ()) {}

QSnapdMarkdownNode::NodeType QSnapdMarkdownNode::type () const
{
    switch (snapd_markdown_node_get_node_type (SNAPD_MARKDOWN_NODE (wrapped_object)))
    {
    default:
    case SNAPD_MARKDOWN_NODE_TYPE_TEXT:
        return QSnapdMarkdownNode::NodeTypeText;
    case SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH:
        return QSnapdMarkdownNode::NodeTypeParagraph;
    case SNAPD_MARKDOWN_NODE_TYPE_UNORDERED_LIST:
        return QSnapdMarkdownNode::NodeTypeUnorderedList;
    case SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM:
        return QSnapdMarkdownNode::NodeTypeListItem;
    case SNAPD_MARKDOWN_NODE_TYPE_CODE_BLOCK:
        return QSnapdMarkdownNode::NodeTypeCodeBlock;
    case SNAPD_MARKDOWN_NODE_TYPE_CODE_SPAN:
        return QSnapdMarkdownNode::NodeTypeCodeSpan;
    case SNAPD_MARKDOWN_NODE_TYPE_EMPHASIS:
        return QSnapdMarkdownNode::NodeTypeEmphasis;
    case SNAPD_MARKDOWN_NODE_TYPE_STRONG_EMPHASIS:
        return QSnapdMarkdownNode::NodeTypeStrongEmphasis;
    case SNAPD_MARKDOWN_NODE_TYPE_URL:
        return QSnapdMarkdownNode::NodeTypeUrl;
    }
}

QString QSnapdMarkdownNode::text () const
{
    return snapd_markdown_node_get_text (SNAPD_MARKDOWN_NODE (wrapped_object));
}

int QSnapdMarkdownNode::childCount () const
{
    GPtrArray *children;

    children = snapd_markdown_node_get_children (SNAPD_MARKDOWN_NODE (wrapped_object));
    return children != NULL ? children->len : 0;
}

QSnapdMarkdownNode *QSnapdMarkdownNode::child (int n) const
{
    GPtrArray *children;

    children = snapd_markdown_node_get_children (SNAPD_MARKDOWN_NODE (wrapped_object));
    if (children == NULL || n < 0 || (guint) n >= children->len)
        return NULL;
    return new QSnapdMarkdownNode (children->pdata[n]);
}

QSnapdMarkdownNode & QSnapdMarkdownNode::operator=(const QSnapdMarkdownNode& node)
{
    if (&node == this) {
        return *this;
    }
    g_object_unref(wrapped_object);
    wrapped_object = node.wrapped_object;
    g_object_ref(wrapped_object);
    return *this;
}
