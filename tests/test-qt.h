/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <QtCore/QObject>

class ProgressCounter: public QObject
{
    Q_OBJECT

public:
    int progress_done = 0;

public slots:
    void progress ()
    {
        progress_done++;
    }
};
