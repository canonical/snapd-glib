/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __MOCK_SNAPD_H__
#define __MOCK_SNAPD_H__

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define MOCK_TYPE_SNAPD  (mock_snapd_get_type ())

G_DECLARE_FINAL_TYPE (MockSnapd, mock_snapd, MOCK, SNAPD, GObject)

struct _MockSnapdClass
{
    /*< private >*/
    GObjectClass parent_class;
};

typedef struct _MockAccount MockAccount;
typedef struct _MockAlias MockAlias;
typedef struct _MockApp MockApp;
typedef struct _MockChange MockChange;
typedef struct _MockChannel MockChannel;
typedef struct _MockMedia MockMedia;
typedef struct _MockPlug MockPlug;
typedef struct _MockPrice MockPrice;
typedef struct _MockSlot MockSlot;
typedef struct _MockSnap MockSnap;
typedef struct _MockTask MockTask;
typedef struct _MockTrack MockTrack;

MockSnapd      *mock_snapd_new                    (void);

const gchar    *mock_snapd_get_socket_path        (MockSnapd     *snapd);

void            mock_snapd_set_close_on_request   (MockSnapd     *snapd,
                                                   gboolean       close_on_request);

void            mock_snapd_set_decline_auth       (MockSnapd     *snapd,
                                                   gboolean       decline_auth);

gboolean        mock_snapd_start                  (MockSnapd     *snapd,
                                                   GError       **error);

void            mock_snapd_stop                   (MockSnapd     *snapd);

void            mock_snapd_set_build_id           (MockSnapd     *snapd,
                                                   const gchar   *build_id);

void            mock_snapd_set_confinement        (MockSnapd     *snapd,
                                                   const gchar   *confinement);

void            mock_snapd_add_sandbox_feature    (MockSnapd     *snapd,
                                                   const gchar   *backend,
                                                   const gchar   *feature);

void            mock_snapd_set_store              (MockSnapd     *snapd,
                                                   const gchar   *name);

void            mock_snapd_set_managed            (MockSnapd     *snapd,
                                                   gboolean       managed);

void            mock_snapd_set_on_classic         (MockSnapd     *snapd,
                                                   gboolean       on_classic);

void            mock_snapd_set_refresh_hold       (MockSnapd     *snapd,
                                                   const gchar   *refresh_hold);

void            mock_snapd_set_refresh_last       (MockSnapd     *snapd,
                                                   const gchar   *refresh_last);

void            mock_snapd_set_refresh_next       (MockSnapd     *snapd,
                                                   const gchar   *refresh_next);

void            mock_snapd_set_refresh_schedule   (MockSnapd     *snapd,
                                                   const gchar   *schedule);

void            mock_snapd_set_refresh_timer      (MockSnapd     *snapd,
                                                   const gchar   *timer);

void            mock_snapd_set_suggested_currency (MockSnapd     *snapd,
                                                   const gchar   *currency);

void            mock_snapd_set_progress_total     (MockSnapd     *snapd,
                                                   int            total);

void            mock_snapd_set_spawn_time         (MockSnapd     *snapd,
                                                   const gchar   *spawn_time);

void            mock_snapd_set_ready_time         (MockSnapd     *snapd,
                                                   const gchar   *ready_time);

MockAccount    *mock_snapd_add_account            (MockSnapd     *snapd,
                                                   const gchar   *email,
                                                   const gchar   *username,
                                                   const gchar   *password);

void            mock_account_set_terms_accepted   (MockAccount   *account,
                                                   gboolean       terms_accepted);

void            mock_account_set_has_payment_methods (MockAccount   *account,
                                                      gboolean       has_payment_methods);

const gchar    *mock_account_get_macaroon         (MockAccount   *account);

gchar         **mock_account_get_discharges       (MockAccount   *account);

gboolean        mock_account_get_sudoer           (MockAccount   *account);

gboolean        mock_account_get_known            (MockAccount   *account);

void            mock_account_set_otp              (MockAccount   *account,
                                                   const gchar   *otp);

void            mock_account_set_ssh_keys         (MockAccount   *account,
                                                   gchar        **ssh_keys);

MockAccount    *mock_snapd_find_account_by_username (MockSnapd     *snapd,
                                                     const gchar   *username);

MockAccount    *mock_snapd_find_account_by_email  (MockSnapd     *snapd,
                                                   const gchar   *email);

MockChange     *mock_snapd_add_change             (MockSnapd     *snapd);

MockTask       *mock_change_add_task              (MockChange    *change,
                                                   const gchar   *kind);

void            mock_task_set_snap_name           (MockTask      *task,
                                                   const gchar   *snap_name);

void            mock_task_set_status              (MockTask      *task,
                                                   const gchar   *status);

void            mock_task_set_progress            (MockTask      *task,
                                                   int            done,
                                                   int            total);

void            mock_task_set_spawn_time          (MockTask      *task,
                                                   const gchar   *spawn_time);

void            mock_task_set_ready_time          (MockTask      *task,
                                                   const gchar   *ready_time);

void            mock_change_set_spawn_time        (MockChange    *change,
                                                   const gchar   *spawn_time);

void            mock_change_set_ready_time        (MockChange    *change,
                                                   const gchar   *ready_time);

MockSnap       *mock_account_add_private_snap     (MockAccount   *account,
                                                   const gchar   *name);

MockSnap       *mock_snapd_add_snap               (MockSnapd     *snapd,
                                                   const gchar   *name);

MockSnap       *mock_snapd_find_snap              (MockSnapd     *snapd,
                                                   const gchar   *name);

void            mock_snapd_add_store_section      (MockSnapd     *snapd,
                                                   const gchar   *name);

MockSnap       *mock_snapd_add_store_snap         (MockSnapd     *snapd,
                                                   const gchar   *name);

MockApp        *mock_snap_add_app                 (MockSnap      *snap,
                                                   const gchar   *name);

MockApp        *mock_snap_find_app                (MockSnap      *snap,
                                                   const gchar   *name);

void            mock_app_set_active               (MockApp       *app,
                                                   gboolean       active);

void            mock_app_set_enabled              (MockApp       *app,
                                                   gboolean       enabled);

void            mock_app_set_common_id            (MockApp       *app,
                                                   const gchar   *id);

void            mock_app_set_daemon               (MockApp       *app,
                                                   const gchar   *daemon);

void            mock_app_set_desktop_file         (MockApp       *app,
                                                   const gchar   *desktop_file);

void            mock_app_add_auto_alias           (MockApp       *app,
                                                   const gchar   *alias);

void            mock_app_add_manual_alias         (MockApp       *app,
                                                   const gchar   *alias,
                                                   gboolean       enabled);

MockAlias      *mock_app_find_alias               (MockApp       *app,
                                                   const gchar   *name);

void            mock_snap_set_base                (MockSnap      *snap,
                                                   const gchar   *base);

void            mock_snap_set_broken              (MockSnap      *snap,
                                                   const gchar   *broken);

void            mock_snap_set_channel             (MockSnap      *snap,
                                                   const gchar   *channel);

const gchar    *mock_snap_get_channel             (MockSnap      *snap);

void            mock_snap_add_common_id           (MockSnap      *snap,
                                                   const gchar   *id);

MockTrack      *mock_snap_add_track               (MockSnap      *snap,
                                                   const gchar   *name);

MockChannel    *mock_track_add_channel            (MockTrack     *track,
                                                   const gchar   *risk,
                                                   const gchar   *branch);

void            mock_channel_set_confinement      (MockChannel  *channel,
                                                   const gchar   *confinement);

void            mock_channel_set_branch           (MockChannel  *channel,
                                                   const gchar   *branch);

void            mock_channel_set_epoch            (MockChannel  *channel,
                                                   const gchar   *epoch);

void            mock_channel_set_revision         (MockChannel  *channel,
                                                   const gchar   *revision);

void            mock_channel_set_size             (MockChannel  *channel,
                                                   int           size);

void            mock_channel_set_version          (MockChannel  *channel,
                                                   const gchar   *version);

void            mock_snap_set_confinement         (MockSnap      *snap,
                                                   const gchar   *confinement);

const gchar    *mock_snap_get_confinement         (MockSnap      *snap);

void            mock_snap_set_contact             (MockSnap      *snap,
                                                   const gchar   *contact);

gboolean        mock_snap_get_dangerous           (MockSnap      *snap);

const gchar    *mock_snap_get_data                (MockSnap      *snap);

void            mock_snap_set_description         (MockSnap      *snap,
                                                   const gchar   *description);

void            mock_snap_set_devmode             (MockSnap      *snap,
                                                   gboolean       devmode);

gboolean        mock_snap_get_devmode             (MockSnap      *snap);

void            mock_snap_set_disabled            (MockSnap      *snap,
                                                   gboolean       disabled);

gboolean        mock_snap_get_disabled            (MockSnap      *snap);

void            mock_snap_set_download_size       (MockSnap      *snap,
                                                   int           download_size);

void            mock_snap_set_error               (MockSnap      *snap,
                                                   const gchar   *error);

void            mock_snap_set_icon                (MockSnap      *snap,
                                                   const gchar   *icon);

void            mock_snap_set_icon_data           (MockSnap      *snap,
                                                   const gchar   *mime_type,
                                                   GBytes        *data);

void            mock_snap_set_id                  (MockSnap      *snap,
                                                   const gchar   *id);

void            mock_snap_set_install_date        (MockSnap      *snap,
                                                   const gchar   *install_date);

void            mock_snap_set_installed_size      (MockSnap      *snap,
                                                   int           installed_size);

void            mock_snap_set_jailmode            (MockSnap      *snap,
                                                   gboolean       jailmode);

gboolean        mock_snap_get_jailmode            (MockSnap      *snap);

void            mock_snap_set_license             (MockSnap      *snap,
                                                   const gchar   *license);

void            mock_snap_set_mounted_from        (MockSnap      *snap,
                                                   const gchar   *mounted_from);

const gchar    *mock_snap_get_path                (MockSnap      *snap);

gboolean        mock_snap_get_preferred           (MockSnap      *snap);

MockPrice      *mock_snap_add_price               (MockSnap      *snap,
                                                   gdouble        amount,
                                                   const gchar   *currency);

void            mock_snap_set_publisher_display_name (MockSnap *snap,
                                                      const gchar *display_name);

void            mock_snap_set_publisher_id        (MockSnap *snap,
                                                   const gchar *id);

void            mock_snap_set_publisher_username  (MockSnap *snap,
                                                   const gchar *username);

void            mock_snap_set_publisher_validation (MockSnap *snap,
                                                    const gchar *validation);

void            mock_snap_set_restart_required    (MockSnap      *snap,
                                                   gboolean       restart_required);

void            mock_snap_set_revision            (MockSnap      *snap,
                                                   const gchar   *revision);

const gchar    *mock_snap_get_revision            (MockSnap      *snap);

void            mock_snap_set_scope_is_wide       (MockSnap      *snap,
                                                   gboolean       scope_is_wide);

MockMedia      *mock_snap_add_media               (MockSnap      *snap,
                                                   const gchar   *type,
                                                   const gchar   *url,
                                                   int            width,
                                                   int            height);

void            mock_snap_set_status              (MockSnap      *snap,
                                                   const gchar   *status);

void            mock_snap_set_summary             (MockSnap      *snap,
                                                   const gchar   *summary);

void            mock_snap_set_title               (MockSnap      *snap,
                                                   const gchar   *title);

void            mock_snap_set_tracking_channel    (MockSnap      *snap,
                                                   const gchar   *channel);

const gchar    *mock_snap_get_tracking_channel    (MockSnap      *snap);

void            mock_snap_set_trymode             (MockSnap      *snap,
                                                   gboolean       trymode);

void            mock_snap_set_type                (MockSnap      *snap,
                                                   const gchar   *type);

void            mock_snap_set_version             (MockSnap      *snap,
                                                   const gchar   *version);

void            mock_snap_add_store_section       (MockSnap      *snap,
                                                   const gchar   *section);

MockPlug       *mock_snap_add_plug                (MockSnap      *snap,
                                                   const gchar   *name);

MockPlug       *mock_snap_find_plug               (MockSnap      *snap,
                                                   const gchar   *name);

MockSlot       *mock_snap_add_slot                (MockSnap      *snap,
                                                   const gchar   *name);

MockSlot       *mock_snap_find_slot               (MockSnap      *snap,
                                                   const gchar   *name);

void            mock_plug_set_connection          (MockPlug      *plug,
                                                   MockSlot      *slot);

MockSlot       *mock_plug_get_connection          (MockPlug      *plug);

void            mock_snapd_add_assertion          (MockSnapd     *snapd,
                                                   const gchar   *assertion);

GList          *mock_snapd_get_assertions         (MockSnapd     *snapd);

const gchar    *mock_snapd_get_last_user_agent    (MockSnapd     *snapd);

const gchar    *mock_snapd_get_last_accept_language (MockSnapd     *snapd);

const gchar    *mock_snapd_get_last_allow_interaction (MockSnapd *snapd);

G_END_DECLS

#endif /* __MOCK_SNAPD_H__ */
