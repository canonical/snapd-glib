/*
 * Copyright (C) 2025 Soumyadeep Ghosh
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-link.h"

/**
  * SECTION: snapd-link
  * @short_description: Links information
  * @include: snapd-glib/snapd-glib.h
  *
  * A #SnapdLink represents a link type (contact, issues etc) that is associated
  * with a snap. Snap links can be queried using snapd_snap_get_links().
  */

/**
  * SnapdLink:
  *
  * #SnapdLink contains link information.
  *
  * Since: 1.69
  */

struct _SnapdLink {
  GObject parent_instance;

  gchar *type;
  GStrv urls;
};

enum { PROP_TYPE = 1, PROP_URLS };

G_DEFINE_TYPE(SnapdLink, snapd_link, G_TYPE_OBJECT)

SnapdLink *snapd_link_new(void) { return g_object_new(SNAPD_TYPE_LINK, NULL); }

/**
  * snapd_link_get_link_type:
  * @link: a #SnapdLink.
  *
  * Get the type for a link, e.g. "contact" or "issues".
  *
  * Returns: (allow-none): a type name
  *
  * Since: 1.69
  */
const gchar *snapd_link_get_url_type(SnapdLink *self) {
  g_return_val_if_fail(SNAPD_IS_LINK(self), NULL);
  return self->type;
}

/**
  * snapd_link_get_urls:
  * @link: a #SnapdLink.
  *
  * Get the list of URL for this link, e.g. [ "mailto:developer@example.com", "discord.gg/example" ]
  *
  * Returns: (transfer none) (array zero-terminated=1): the URLs
  *
  * Since: 1.69
  */
GStrv snapd_link_get_urls(SnapdLink *self) {
  g_return_val_if_fail(SNAPD_IS_LINK(self), NULL);
  return self->urls;
}

static void snapd_link_set_property(GObject *object, guint prop_id,
                                    const GValue *value, GParamSpec *pspec) {
  SnapdLink *self = SNAPD_LINK(object);

  switch (prop_id) {
  case PROP_TYPE:
    g_free(self->type);
    self->type = g_strdup(g_value_get_string(value));
    break;
  case PROP_URLS:
    g_strfreev(self->urls);
    self->urls = g_strdupv(g_value_get_boxed(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_link_get_property(GObject *object, guint prop_id,
                                    GValue *value, GParamSpec *pspec) {
  SnapdLink *self = SNAPD_LINK(object);

  switch (prop_id) {
  case PROP_TYPE:
    g_value_set_string(value, self->type);
    break;
  case PROP_URLS:
    g_value_set_boxed(value, self->urls);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_link_finalize(GObject *object) {
  SnapdLink *self = SNAPD_LINK(object);

  g_clear_pointer(&self->type, g_free);
  g_clear_pointer(&self->urls, g_strfreev);

  G_OBJECT_CLASS(snapd_link_parent_class)->finalize(object);
}

static void snapd_link_class_init(SnapdLinkClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_link_set_property;
  gobject_class->get_property = snapd_link_get_property;
  gobject_class->finalize = snapd_link_finalize;

  g_object_class_install_property(
      gobject_class, PROP_TYPE,
      g_param_spec_string("type", "type", "Type of this link", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_URLS,
      g_param_spec_boxed(
          "urls", "urls", "URLs available for this type of link", G_TYPE_STRV,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void snapd_link_init(SnapdLink *self) {}
