/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <glib/gi18n-lib.h>

#include "snapd-interface.h"

/**
 * SECTION: snapd-interface
 * @short_description: Snap interface info
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdInterface represents information about a particular
 * interface type and the related plugs and slots provided by snaps on
 * the system.
 *
 * Available interfaces can be queried using
 * snapd_client_get_interfaces2_sync().
 */

/**
 * SnapdInterface:
 *
 * #SnapdInterface contains information about a Snap interface.
 *
 * Since: 1.48
 */

struct _SnapdInterface {
  GObject parent_instance;

  gchar *name;
  gchar *summary;
  gchar *doc_url;
  GPtrArray *plugs;
  GPtrArray *slots;
};

enum {
  PROP_NAME = 1,
  PROP_SUMMARY,
  PROP_DOC_URL,
  PROP_PLUGS,
  PROP_SLOTS,
};

G_DEFINE_TYPE(SnapdInterface, snapd_interface, G_TYPE_OBJECT);

/**
 * snapd_interface_get_name:
 * @interface: a #SnapdInterface
 *
 * Get the name of this interface.
 *
 * Returns: a name.
 *
 * Since: 1.48
 */
const gchar *snapd_interface_get_name(SnapdInterface *self) {
  g_return_val_if_fail(SNAPD_IS_INTERFACE(self), NULL);
  return self->name;
}

/**
 * snapd_interface_get_summary:
 * @interface: a #SnapdInterface
 *
 * Get the summary of this interface.
 *
 * Returns: a summary.
 *
 * Since: 1.48
 */
const gchar *snapd_interface_get_summary(SnapdInterface *self) {
  g_return_val_if_fail(SNAPD_IS_INTERFACE(self), NULL);
  return self->summary;
}

/**
 * snapd_interface_get_doc_url:
 * @interface: a #SnapdInterface
 *
 * Get the documentation URL of this interface.
 *
 * Returns: a URL.
 *
 * Since: 1.48
 */
const gchar *snapd_interface_get_doc_url(SnapdInterface *self) {
  g_return_val_if_fail(SNAPD_IS_INTERFACE(self), NULL);
  return self->doc_url;
}

/**
 * snapd_interface_get_plugs:
 * @interface: a #SnapdInterface
 *
 * Get the plugs matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdPlug): an array of #SnapdPlug.
 *
 * Since: 1.48
 */
GPtrArray *snapd_interface_get_plugs(SnapdInterface *self) {
  g_return_val_if_fail(SNAPD_IS_INTERFACE(self), NULL);
  return self->plugs;
}

/**
 * snapd_interface_get_slots:
 * @interface: a #SnapdInterface
 *
 * Get the slots matching this interface type.
 *
 * Returns: (transfer none) (element-type SnapdSlot): an array of #SnapdSlot.
 *
 * Since: 1.48
 */
GPtrArray *snapd_interface_get_slots(SnapdInterface *self) {
  g_return_val_if_fail(SNAPD_IS_INTERFACE(self), NULL);
  return self->slots;
}

/**
 * snapd_interface_make_label:
 * @interface: a #SnapdInterface
 *
 * Make a label for this interface suitable for a user interface.
 *
 * Returns: a newly allocated label string.
 *
 * Since: 1.57
 */
gchar *snapd_interface_make_label(SnapdInterface *self) {
  if (strcmp(self->name, "account-control") == 0)
    return g_strdup(_("Add user accounts and change passwords"));
  else if (strcmp(self->name, "alsa") == 0)
    return g_strdup(_("Play and record sound"));
  else if (strcmp(self->name, "audio-playback") == 0)
    return g_strdup(_("Play audio"));
  else if (strcmp(self->name, "audio-record") == 0)
    return g_strdup(_("Record audio"));
  else if (strcmp(self->name, "avahi-observe") == 0)
    return g_strdup(
        _("Detect network devices using mDNS/DNS-SD (Bonjour/zeroconf)"));
  else if (strcmp(self->name, "bluetooth-control") == 0)
    return g_strdup(_("Access bluetooth hardware directly"));
  else if (strcmp(self->name, "bluez") == 0)
    return g_strdup(_("Use bluetooth devices"));
  else if (strcmp(self->name, "camera") == 0)
    return g_strdup(_("Use your camera"));
  else if (strcmp(self->name, "cups-control") == 0)
    return g_strdup(_("Print documents"));
  else if (strcmp(self->name, "joystick") == 0)
    return g_strdup(_("Use any connected joystick"));
  else if (strcmp(self->name, "docker") == 0)
    return g_strdup(_("Allow connecting to the Docker service"));
  else if (strcmp(self->name, "firewall-control") == 0)
    return g_strdup(_("Configure network firewall"));
  else if (strcmp(self->name, "fuse-support") == 0)
    return g_strdup(_("Setup and use privileged FUSE filesystems"));
  else if (strcmp(self->name, "fwupd") == 0)
    return g_strdup(_("Update firmware on this device"));
  else if (strcmp(self->name, "hardware-observe") == 0)
    return g_strdup(_("Access hardware information"));
  else if (strcmp(self->name, "hardware-random-control") == 0)
    return g_strdup(_("Provide entropy to hardware random number generator"));
  else if (strcmp(self->name, "hardware-random-observe") == 0)
    return g_strdup(_("Use hardware-generated random numbers"));
  else if (strcmp(self->name, "home") == 0)
    return g_strdup(_("Access files in your home folder"));
  else if (strcmp(self->name, "libvirt") == 0)
    return g_strdup(_("Access libvirt service"));
  else if (strcmp(self->name, "locale-control") == 0)
    return g_strdup(_("Change system language and region settings"));
  else if (strcmp(self->name, "location-control") == 0)
    return g_strdup(_("Change location settings and providers"));
  else if (strcmp(self->name, "location-observe") == 0)
    return g_strdup(_("Access your location"));
  else if (strcmp(self->name, "log-observe") == 0)
    return g_strdup(_("Read system and application logs"));
  else if (strcmp(self->name, "lxd") == 0)
    return g_strdup(_("Access LXD service"));
  else if (strcmp(self->name, "modem-manager") == 0)
    return g_strdup(_("Use and configure modems"));
  else if (strcmp(self->name, "mount-observe") == 0)
    return g_strdup(_("Read system mount information and disk quotas"));
  else if (strcmp(self->name, "mpris") == 0)
    return g_strdup(_("Control music and video players"));
  else if (strcmp(self->name, "network-control") == 0)
    return g_strdup(_("Change low-level network settings"));
  else if (strcmp(self->name, "network-manager") == 0)
    return g_strdup(_("Access the NetworkManager service to read and change "
                      "network settings"));
  else if (strcmp(self->name, "network-observe") == 0)
    return g_strdup(_("Read access to network settings"));
  else if (strcmp(self->name, "network-setup-control") == 0)
    return g_strdup(_("Change network settings"));
  else if (strcmp(self->name, "network-setup-observe") == 0)
    return g_strdup(_("Read network settings"));
  else if (strcmp(self->name, "ofono") == 0)
    return g_strdup(_("Access the ofono service to read and change network "
                      "settings for mobile telephony"));
  else if (strcmp(self->name, "openvswitch") == 0)
    return g_strdup(_("Control Open vSwitch hardware"));
  else if (strcmp(self->name, "optical-drive") == 0)
    return g_strdup(_("Read from CD/DVD"));
  else if (strcmp(self->name, "password-manager-service") == 0)
    return g_strdup(_("Read, add, change, or remove saved passwords"));
  else if (strcmp(self->name, "ppp") == 0)
    return g_strdup(_("Access pppd and ppp devices for configuring "
                      "Point-to-Point Protocol connections"));
  else if (strcmp(self->name, "process-control") == 0)
    return g_strdup(_("Pause or end any process on the system"));
  else if (strcmp(self->name, "pulseaudio") == 0)
    return g_strdup(_("Play and record sound"));
  else if (strcmp(self->name, "raw-usb") == 0)
    return g_strdup(_("Access USB hardware directly"));
  else if (strcmp(self->name, "removable-media") == 0)
    return g_strdup(_("Read/write files on removable storage devices"));
  else if (strcmp(self->name, "screen-inhibit-control") == 0)
    return g_strdup(_("Prevent screen sleep/lock"));
  else if (strcmp(self->name, "serial-port") == 0)
    return g_strdup(_("Access serial port hardware"));
  else if (strcmp(self->name, "shutdown") == 0)
    return g_strdup(_("Restart or power off the device"));
  else if (strcmp(self->name, "snapd-control") == 0)
    return g_strdup(_("Install, remove and configure software"));
  else if (strcmp(self->name, "storage-framework-service") == 0)
    return g_strdup(_("Access Storage Framework service"));
  else if (strcmp(self->name, "system-observe") == 0)
    return g_strdup(_("Read process and system information"));
  else if (strcmp(self->name, "system-trace") == 0)
    return g_strdup(_("Monitor and control any running program"));
  else if (strcmp(self->name, "time-control") == 0)
    return g_strdup(_("Change the date and time"));
  else if (strcmp(self->name, "timeserver-control") == 0)
    return g_strdup(_("Change time server settings"));
  else if (strcmp(self->name, "timezone-control") == 0)
    return g_strdup(_("Change the time zone"));
  else if (strcmp(self->name, "udisks2") == 0)
    return g_strdup(_("Access the UDisks2 service for configuring disks and "
                      "removable media"));
  else if (strcmp(self->name, "unity8-calendar") == 0)
    return g_strdup(_("Read/change shared calendar events in Ubuntu Unity 8"));
  else if (strcmp(self->name, "unity8-contacts") == 0)
    return g_strdup(_("Read/change shared contacts in Ubuntu Unity 8"));
  else if (strcmp(self->name, "upower-observe") == 0)
    return g_strdup(_("Access energy usage data"));
  else if (strcmp(self->name, "u2f-devices") == 0)
    return g_strdup(_("Read/write access to U2F devices exposed"));
  if (self->summary != NULL)
    return g_strdup_printf("%s: %s", self->name, self->summary);
  else
    return g_strdup(self->name);
}

static void snapd_interface_set_property(GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec) {
  SnapdInterface *self = SNAPD_INTERFACE(object);

  switch (prop_id) {
  case PROP_NAME:
    g_free(self->name);
    self->name = g_strdup(g_value_get_string(value));
    break;
  case PROP_SUMMARY:
    g_free(self->summary);
    self->summary = g_strdup(g_value_get_string(value));
    break;
  case PROP_DOC_URL:
    g_free(self->doc_url);
    self->doc_url = g_strdup(g_value_get_string(value));
    break;
  case PROP_PLUGS:
    g_clear_pointer(&self->plugs, g_ptr_array_unref);
    self->plugs = g_value_dup_boxed(value);
    break;
  case PROP_SLOTS:
    g_clear_pointer(&self->slots, g_ptr_array_unref);
    self->slots = g_value_dup_boxed(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_interface_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec) {
  SnapdInterface *self = SNAPD_INTERFACE(object);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string(value, self->name);
    break;
  case PROP_SUMMARY:
    g_value_set_string(value, self->summary);
    break;
  case PROP_DOC_URL:
    g_value_set_string(value, self->doc_url);
    break;
  case PROP_PLUGS:
    g_value_set_boxed(value, self->plugs);
    break;
  case PROP_SLOTS:
    g_value_set_boxed(value, self->slots);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void snapd_interface_finalize(GObject *object) {
  SnapdInterface *self = SNAPD_INTERFACE(object);

  g_clear_pointer(&self->name, g_free);
  g_clear_pointer(&self->summary, g_free);
  g_clear_pointer(&self->doc_url, g_free);
  g_clear_pointer(&self->plugs, g_ptr_array_unref);
  g_clear_pointer(&self->slots, g_ptr_array_unref);

  G_OBJECT_CLASS(snapd_interface_parent_class)->finalize(object);
}

static void snapd_interface_class_init(SnapdInterfaceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = snapd_interface_set_property;
  gobject_class->get_property = snapd_interface_get_property;
  gobject_class->finalize = snapd_interface_finalize;

  g_object_class_install_property(
      gobject_class, PROP_NAME,
      g_param_spec_string("name", "name", "Interface name", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SUMMARY,
      g_param_spec_string("summary", "summary", "Interface summary", NULL,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK |
                              G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_DOC_URL,
      g_param_spec_string(
          "doc-url", "doc-url", "Interface documentation URL", NULL,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_PLUGS,
      g_param_spec_boxed(
          "plugs", "plugs", "Plugs of this interface type", G_TYPE_PTR_ARRAY,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
  g_object_class_install_property(
      gobject_class, PROP_SLOTS,
      g_param_spec_boxed(
          "slots", "slots", "Slots of this interface type", G_TYPE_PTR_ARRAY,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
              G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void snapd_interface_init(SnapdInterface *self) {}
