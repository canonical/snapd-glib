/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_TASK_H
#define SNAPD_TASK_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <Snapd/WrappedObject>

class Q_DECL_EXPORT QSnapdTask : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY (QString id READ id)
    Q_PROPERTY (QString kind READ kind)
    Q_PROPERTY (QString summary READ summary)
    Q_PROPERTY (QString status READ status)
    Q_PROPERTY (bool ready READ ready)
    Q_PROPERTY (qint64 progressDone READ progressDone)
    Q_PROPERTY (qint64 progressTotal READ progressTotal)
    Q_PROPERTY (QDateTime spawnTime READ spawnTime)
    Q_PROPERTY (QDateTime readyTime READ readyTime)

public:
    explicit QSnapdTask (void* snapd_object, QObject* parent = 0);

    Q_INVOKABLE QString id () const;
    Q_INVOKABLE QString kind () const;
    Q_INVOKABLE QString summary () const;
    Q_INVOKABLE QString status () const;
    Q_INVOKABLE bool ready () const;
    Q_INVOKABLE qint64 progressDone () const;
    Q_INVOKABLE qint64 progressTotal () const;
    Q_INVOKABLE QDateTime spawnTime () const;
    Q_INVOKABLE QDateTime readyTime () const;
};

#endif
