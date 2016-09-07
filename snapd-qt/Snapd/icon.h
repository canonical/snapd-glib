/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_ICON_H
#define SNAPD_ICON_H

#include <QtCore/QObject>

namespace Snapd
{
struct IconPrivate;

class Q_DECL_EXPORT Icon : public QObject
{
    Q_OBJECT

public:
    explicit Icon (QObject* parent, void* snapd_object);
    Icon (const Icon&);

    QString mime_type ();
    QByteArray data ();

private:
    IconPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Icon)  
};

}

#endif
