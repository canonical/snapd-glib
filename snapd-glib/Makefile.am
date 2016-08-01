# main library
source_h = \
	snapd-client.h

source_c = \
	snapd-client.c

lib_LTLIBRARIES = libsnapd-glib-1.0.la

libsnapd_glib_1_0_la_CPPFLAGS = \
	-DPREFIX=\""$(prefix)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DSNAPD_LOCALEDIR=\""$(localedir)"\" \
	-DSNAPD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"Snapd\" \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(SNAPD_DEBUG_CFLAGS)
libsnapd_glib_1_0_la_CFLAGS = $(SNAPD_CFLAGS)
libsnapd_glib_1_0_la_LIBADD = $(SNAPD_LIBS)
libsnapd_glib_1_0_la_SOURCES = $(source_c) $(source_h)

snapdincludedir = $(includedir)/snapd-glib-1.0/snapd-glib
snapdinclude_DATA = \
	$(source_h) \
	snapd-glib.h

pkgconfig_DATA = snapd-glib-1.0.pc
pkgconfigdir = $(libdir)/pkgconfig

# intospection
-include $(INTROSPECTION_MAKEFILE)

if HAVE_INTROSPECTION
INTROSPECTION_GIRS = Snapd-1.0.gir

Snapd-1.0.gir: libsnapd-glib-1.0.la Makefile

Snapd_1_0_gir_NAMESPACE = Snapd
Snapd_1_0_gir_VERSION = 1.0
Snapd_1_0_gir_LIBS = libsnapd-glib-1.0.la
Snapd_1_0_gir_FILES = $(source_c) $(source_h)
Snapd_1_0_gir_CFLAGS = -DSNAPD_COMPILATION=1 -I$(top_srcdir) -I$(top_builddir) $(SNAPD_CFLAGS)
Snapd_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Snapd_1_0_gir_SCANNERFLAGS = --warn-all --pkg-export snapd-glib-1.0 --c-include "snapd-glib/snapd-glib.h"

girdir = $(datadir)/gir-1.0
dist_gir_DATA = Snapd-1.0.gir

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = Snapd-1.0.typelib

CLEANFILES = $(dist_gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION
