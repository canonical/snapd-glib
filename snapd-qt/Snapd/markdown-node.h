/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_MARKDOWN_NODE_H
#define SNAPD_MARKDOWN_NODE_H

#include <QtCore/QObject>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdMarkdownNode : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(NodeType type READ type)
    Q_PROPERTY(QString text READ text)

public:
    enum NodeType
    {
        NodeTypeText,
        NodeTypeParagraph,
        NodeTypeUnorderedList,
        NodeTypeListItem,
        NodeTypeCodeBlock,
        NodeTypeCodeSpan,
        NodeTypeEmphasis,
        NodeTypeStrongEmphasis,
        NodeTypeUrl,
    };
    Q_ENUM(NodeType)
    explicit QSnapdMarkdownNode (void* snapd_object, QObject* parent = 0);
    explicit QSnapdMarkdownNode (const QSnapdMarkdownNode &node);
    QSnapdMarkdownNode& operator = (const QSnapdMarkdownNode &node);

    NodeType type () const;
    QString text () const;
    Q_INVOKABLE int childCount () const;
    Q_INVOKABLE QSnapdMarkdownNode *child (int) const;
};

#endif
