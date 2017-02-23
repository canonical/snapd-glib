SUBDIRS = tests

CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

source_h = \
	snapd-alias.h \
	snapd-app.h \
	snapd-assertion.h \
	snapd-auth-data.h \
	snapd-change.h \
	snapd-client.h \
	snapd-connection.h \
	snapd-error.h \
	snapd-icon.h \
	snapd-login.h \
	snapd-plug.h \
	snapd-price.h \
	snapd-screenshot.h \
	snapd-slot.h \
	snapd-snap.h \
	snapd-system-information.h \
	snapd-task.h \
	snapd-user-information.h

source_c = \
	snapd-alias.c \
	snapd-app.c \
	snapd-assertion.c \
	snapd-auth-data.c \
	snapd-change.c \
	snapd-client.c \
	snapd-connection.c \
	snapd-error.c \
	snapd-icon.c \
	snapd-login.c \
	snapd-plug.c \
	snapd-price.c \
	snapd-screenshot.c \
	snapd-slot.c \
	snapd-snap.c \
	snapd-system-information.c \
	snapd-task.c \
	snapd-user-information.c

# glib-mkenums rules
glib_enum_h = snapd-enum-types.h
glib_enum_c = snapd-enum-types.c
glib_enum_headers = $(source_h)
include Makefile.am.enums

snapd_la = libsnapd-glib.la
lib_LTLIBRARIES = $(snapd_la)

libsnapd_glib_la_CPPFLAGS = \
	-DSNAPD_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"Snapd\" \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(SNAPD_DEBUG_CFLAGS)
libsnapd_glib_la_CFLAGS = $(SNAPD_CFLAGS) $(WARN_CFLAGS)
libsnapd_glib_la_LDFLAGS = -version-info 1:0:0
libsnapd_glib_la_LIBADD = $(SNAPD_LIBS)
libsnapd_glib_la_SOURCES = $(source_c) $(source_h) $(BUILT_SOURCES)

snapdincludedir = $(includedir)/snapd-glib/snapd-glib
snapdinclude_DATA = \
	$(source_h) \
	snapd-enum-types.h \
	snapd-glib.h

EXTRA_DIST += snapd-glib.h

pkgconfig_DATA = snapd-glib.pc
pkgconfigdir = $(libdir)/pkgconfig

# intospection
-include $(INTROSPECTION_MAKEFILE)

if HAVE_INTROSPECTION
INTROSPECTION_GIRS = Snapd-1.gir

Snapd-1.gir: $(snapd_la) Makefile

Snapd_1_gir_NAMESPACE = Snapd
Snapd_1_gir_VERSION = 1
Snapd_1_gir_LIBS = $(snapd_la)
Snapd_1_gir_FILES = $(source_c) $(source_h) snapd-enum-types.h snapd-enum-types.c
Snapd_1_gir_CFLAGS = -DSNAPD_COMPILATION=1 -I$(top_srcdir) -I$(top_builddir) $(SNAPD_CFLAGS)
Snapd_1_gir_INCLUDES = GObject-2.0 Gio-2.0
Snapd_1_gir_SCANNERFLAGS = --warn-all --pkg-export snapd-glib --c-include "snapd-glib/snapd-glib.h"

girdir = $(datadir)/gir-1.0
dist_gir_DATA = Snapd-1.gir

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = Snapd-1.typelib

CLEANFILES += $(dist_gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION

if ENABLE_VAPIGEN
include $(VAPIGEN_MAKEFILE)

snapd-glib.vapi: $(dist_gir_DATA)

VAPIGEN_VAPIS = snapd-glib.vapi

snapd_glib_vapi_DEPS = gio-2.0
snapd_glib_vapi_METADATADIRS = $(srcdir)
snapd_glib_vapi_FILES = $(dist_gir_DATA)

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(srcdir)/snapd-glib.deps

EXTRA_DIST += snapd-glib.deps
CLEANFILES += snapd-glib.vapi
endif #ENABLE_VAPIGEN
