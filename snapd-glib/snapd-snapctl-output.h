/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef __SNAPD_SNAPCTL_OUTPUT_H__
#define __SNAPD_SNAPCTL_OUTPUT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_SNAPCTL_OUTPUT  (snapd_snapctl_output_get_type ())

G_DECLARE_FINAL_TYPE (SnapdSnapCtlOutput, snapd_snapctl_output, SNAPD, SNAPCTL_OUTPUT, GObject)

struct _SnapdSnapCtlOutputClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_snapctl_output_get_stdout (SnapdSnapCtlOutput *output);

const gchar *snapd_snapctl_output_get_stderr (SnapdSnapCtlOutput *output);

G_END_DECLS

#endif /* __SNAPD_SNAPCTL_OUTPUT_H__ */
