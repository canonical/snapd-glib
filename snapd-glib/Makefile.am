CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

# main library
source_h = \
	snapd-auth-data.h \
	snapd-client.h \
	snapd-payment-method.h \
	snapd-payment-method-list.h \
	snapd-price.h \
	snapd-snap.h \
	snapd-snap-list.h \
	snapd-system-information.h

source_c = \
	snapd-auth-data.c \
	snapd-client.c \
	snapd-payment-method.c \
	snapd-payment-method-list.c \
	snapd-price.c \
	snapd-snap.c \
	snapd-snap-list.c \
	snapd-system-information.c

# glib-mkenums rules
glib_enum_h = snapd-enum-types.h
glib_enum_c = snapd-enum-types.c
glib_enum_headers = $(source_h)
include Makefile.am.enums

lib_LTLIBRARIES = libsnapd-glib-1.0.la

libsnapd_glib_1_0_la_CPPFLAGS = \
	-DSNAPD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"Snapd\" \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(SNAPD_DEBUG_CFLAGS)
libsnapd_glib_1_0_la_CFLAGS = $(SNAPD_CFLAGS)
libsnapd_glib_1_0_la_LIBADD = $(SNAPD_LIBS)
libsnapd_glib_1_0_la_SOURCES = $(source_c) $(source_h) $(BUILT_SOURCES)

snapdincludedir = $(includedir)/snapd-glib-1.0/snapd-glib
snapdinclude_DATA = \
	$(source_h) \
	snapd-enum-types.h \
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
Snapd_1_0_gir_FILES = $(source_c) $(source_h) snapd-enum-types.h snapd-enum-types.c
Snapd_1_0_gir_CFLAGS = -DSNAPD_COMPILATION=1 -I$(top_srcdir) -I$(top_builddir) $(SNAPD_CFLAGS)
Snapd_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
Snapd_1_0_gir_SCANNERFLAGS = --warn-all --pkg-export snapd-glib-1.0 --c-include "snapd-glib/snapd-glib.h"

girdir = $(datadir)/gir-1.0
dist_gir_DATA = Snapd-1.0.gir

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = Snapd-1.0.typelib

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION
