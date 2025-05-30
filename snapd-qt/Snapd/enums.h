/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef SNAPD_ENUMS_H
#define SNAPD_ENUMS_H

#include <QtCore/QObject>

#include "snapdqt_global.h"

class LIBSNAPDQT_EXPORT QSnapdEnums : public QObject {
  Q_OBJECT

public:
  enum AliasStatus {
    AliasStatusUnknown,
    AliasStatusDefault,
    AliasStatusEnabled,
    AliasStatusDisabled,
    AliasStatusAuto,
    AliasStatusManual
  };
  Q_ENUM(AliasStatus)

  enum DaemonType {
    DaemonTypeNone,
    DaemonTypeUnknown,
    DaemonTypeSimple,
    DaemonTypeForking,
    DaemonTypeOneshot,
    DaemonTypeDbus,
    DaemonTypeNotify
  };
  Q_ENUM(DaemonType)

  enum SnapConfinement {
    SnapConfinementUnknown,
    SnapConfinementStrict,
    SnapConfinementDevmode,
    SnapConfinementClassic
  };
  Q_ENUM(SnapConfinement)

  enum SnapType {
    SnapTypeUnknown,
    SnapTypeApp,
    SnapTypeKernel,
    SnapTypeGadget,
    SnapTypeOperatingSystem,
    SnapTypeCore,
    SnapTypeBase,
    SnapTypeSnapd
  };
  Q_ENUM(SnapType)

  enum SnapStatus {
    SnapStatusUnknown,
    SnapStatusAvailable,
    SnapStatusPriced,
    SnapStatusInstalled,
    SnapStatusActive
  };
  Q_ENUM(SnapStatus)

  enum SystemConfinement {
    SystemConfinementUnknown,
    SystemConfinementStrict,
    SystemConfinementPartial
  };
  Q_ENUM(SystemConfinement)

  enum PublisherValidation {
    PublisherValidationUnknown,
    PublisherValidationUnproven,
    PublisherValidationVerified,
    PublisherValidationStarred
  };
  Q_ENUM(PublisherValidation)

  enum MaintenanceKind {
    MaintenanceKindUnknown,
    MaintenanceKindDaemonRestart,
    MaintenanceKindSystemRestart
  };
  Q_ENUM(MaintenanceKind)

  enum SnapNoticeType {
    SnapNoticeTypeUnknown,
    SnapNoticeTypeChangeUpdate,
    SnapNoticeTypeRefreshInhibit,
    SnapNoticeTypeSnapRunInhibit
  };
  Q_ENUM(SnapNoticeType)
};

#endif
