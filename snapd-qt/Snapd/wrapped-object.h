/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_WRAPPED_OBJECT_H
#define SNAPD_WRAPPED_OBJECT_H

#include <QtCore/QObject>

namespace Snapd
{
class Q_DECL_EXPORT WrappedObject : public QObject
{
    Q_OBJECT

public:
    explicit WrappedObject (void* object, void (*unref_func)(void *), QObject *parent = 0) : QObject (parent), wrapped_object (object), unref_func (unref_func) {}
    ~WrappedObject () 
    {
        unref_func (wrapped_object);
    }

protected:
    void *wrapped_object;

private:
    void (*unref_func)(void *);
};

}

#endif
