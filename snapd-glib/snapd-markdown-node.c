/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <ctype.h>

#include "config.h"

#include "snapd-markdown-node.h"
#include "snapd-enum-types.h"

/**
 * SECTION:snapd-markdown-node
 * @short_description: Snap markdown node
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdMarkdownNode represents a markdown node extracted from text. See #SnapdMarkdownParser for more information.
 */

/**
 * SnapdMarkdownNode:
 *
 * #SnapdMarkdownNode is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.48
 */
struct _SnapdMarkdownNode
{
    GObject parent_instance;

    SnapdMarkdownNodeType node_type;
    gchar *text;
    GPtrArray *children;
};

enum
{
    PROP_NODE_TYPE = 1,
    PROP_TEXT,
    PROP_CHILDREN,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdMarkdownNode, snapd_markdown_node, G_TYPE_OBJECT)

static void
snapd_markdown_node_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdMarkdownNode *node = SNAPD_MARKDOWN_NODE (object);

    switch (prop_id) {
    case PROP_NODE_TYPE:
        node->node_type = g_value_get_enum (value);
        break;
    case PROP_TEXT:
        g_free (node->text);
        node->text = g_strdup (g_value_get_string (value));
        break;
    case PROP_CHILDREN:
        g_clear_pointer (&node->children, g_ptr_array_unref);
        if (g_value_get_boxed (value) != NULL)
            node->children = g_ptr_array_ref (g_value_get_boxed (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_markdown_node_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdMarkdownNode *node = SNAPD_MARKDOWN_NODE (object);

    switch (prop_id) {
    case PROP_NODE_TYPE:
        g_value_set_enum (value, node->node_type);
        break;
    case PROP_TEXT:
        g_value_set_string (value, node->text);
        break;
    case PROP_CHILDREN:
        g_value_set_boxed (value, node->children);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_markdown_node_finalize (GObject *object)
{
    SnapdMarkdownNode *node = SNAPD_MARKDOWN_NODE (object);

    g_clear_pointer (&node->text, g_free);
    g_clear_pointer (&node->children, g_ptr_array_unref);

    G_OBJECT_CLASS (snapd_markdown_node_parent_class)->finalize (object);
}

static void
snapd_markdown_node_class_init (SnapdMarkdownNodeClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_markdown_node_set_property;
    gobject_class->get_property = snapd_markdown_node_get_property;
    gobject_class->finalize = snapd_markdown_node_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_NODE_TYPE,
                                     g_param_spec_enum ("node-type",
                                                        "node-type",
                                                        "Type of node",
                                                        SNAPD_TYPE_MARKDOWN_NODE_TYPE, 0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_TEXT,
                                     g_param_spec_string ("text",
                                                          "text",
                                                          "Text this node contains",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CHILDREN,
                                     g_param_spec_boxed ("children",
                                                         "children",
                                                         "Child nodes",
                                                         G_TYPE_PTR_ARRAY,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_markdown_node_init (SnapdMarkdownNode *node)
{
}

/**
 * snapd_markdown_node_get_node_type:
 * @node: a #SnapdMarkdownNode.
 *
 * Get the type of node this is.
 *
 * Returns: a #SnapdMarkdownNodeType
 *
 * Since: 1.48
 */
SnapdMarkdownNodeType
snapd_markdown_node_get_node_type (SnapdMarkdownNode *node)
{
    g_return_val_if_fail (SNAPD_IS_MARKDOWN_NODE (node), 0);
    return node->node_type;
}

/**
 * snapd_markdown_node_get_text:
 * @node: a #SnapdMarkdownNode.
 *
 * Gets the text associated with this node. This is only present for nodes of type
 * %SNAPD_MARKDOWN_NODE_TYPE_TEXT.
 *
 * Returns: a UTF-8 string or %NULL if none in this node.
 *
 * Since: 1.48
 */
const gchar *
snapd_markdown_node_get_text (SnapdMarkdownNode *node)
{
    g_return_val_if_fail (SNAPD_IS_MARKDOWN_NODE (node), NULL);
    return node->text;
}

/**
 * snapd_markdown_node_get_children:
 * @node: a #SnapdMarkdownNode.
 *
 * Get the child nodes of this node.
 *
 * Returns: (transfer none) (element-type SnapdMarkdownNode): child nodes or %NULL if none.
 * Since: 1.48
 */
GPtrArray *
snapd_markdown_node_get_children (SnapdMarkdownNode *node)
{
    g_return_val_if_fail (SNAPD_IS_MARKDOWN_NODE (node), NULL);
    return node->children;
}
