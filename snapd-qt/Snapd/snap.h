/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_SNAP_H
#define SNAPD_SNAP_H

#include <QtCore/QObject>
#include <QtCore/QDate>
#include <Snapd/WrappedObject>
#include <Snapd/App>
#include <Snapd/Channel>
#include <Snapd/Price>
#include <Snapd/Screenshot>

class Q_DECL_EXPORT QSnapdSnap : public QSnapdWrappedObject
{
    Q_OBJECT

    Q_PROPERTY(int appCount READ appCount)
    Q_PROPERTY(QString channel READ channel)
    Q_PROPERTY(int channelCount READ channelCount)
    Q_PROPERTY(QSnapdChannel::QSnapdConfinement confinement READ confinement)
    Q_PROPERTY(QString contact READ contact)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QString developer READ developer)
    Q_PROPERTY(bool devmode READ devmode)
    Q_PROPERTY(qint64 downloadSize READ downloadSize)
    Q_PROPERTY(QString icon READ icon)
    Q_PROPERTY(QString id READ id)
    Q_PROPERTY(QDateTime installDate READ installDate)
    Q_PROPERTY(qint64 installedSize READ installedSize)
    Q_PROPERTY(bool jailmode READ jailmode)
    Q_PROPERTY(QString license READ license)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(int priceCount READ priceCount)
    Q_PROPERTY(bool isPrivate READ isPrivate)
    Q_PROPERTY(QString revision READ revision)
    Q_PROPERTY(QSnapdSnapType snapType READ snapType)
    Q_PROPERTY(QSnapdSnapStatus status READ status)
    Q_PROPERTY(QString summary READ summary)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString trackingChannel READ trackingChannel)
    Q_PROPERTY(QStringList tracks READ tracks)
    Q_PROPERTY(bool trymode READ trymode)
    Q_PROPERTY(QString version READ version)

public:
    enum QSnapdSnapType
    {
        TypeUnknown,
        App,
        Kernel,
        Gadget,
        OperatingSystem
    };
    Q_ENUM(QSnapdSnapType)

    enum QSnapdSnapStatus
    {
        StatusUnknown,
        Available,
        Priced,
        Installed,
        Active
    };
    Q_ENUM(QSnapdSnapStatus)

    explicit QSnapdSnap (void* snapd_object, QObject* parent = 0);

    int appCount () const;
    Q_INVOKABLE QSnapdApp *app (int) const;
    QString channel () const;
    int channelCount () const;
    Q_INVOKABLE QSnapdChannel *channel (int) const;
    Q_INVOKABLE QSnapdChannel *matchChannel (QString) const;
    QSnapdChannel::QSnapdConfinement confinement () const;
    QString contact () const;
    QString description () const;
    QString developer () const;
    bool devmode () const;
    qint64 downloadSize () const;
    QString icon () const;
    QString id () const;
    QDateTime installDate () const;
    qint64 installedSize () const;
    bool jailmode () const;
    QString license () const;
    QString name () const;
    int priceCount () const;
    Q_INVOKABLE QSnapdPrice *price (int) const;
    bool isPrivate () const;
    QString revision () const;
    int screenshotCount () const;
    Q_INVOKABLE QSnapdScreenshot *screenshot (int) const;
    QSnapdSnapType snapType () const;
    QSnapdSnapStatus status () const;
    QString summary () const;
    QString title () const;
    QString trackingChannel () const;
    QStringList tracks () const;
    bool trymode () const;
    QString version () const;
};

#endif
