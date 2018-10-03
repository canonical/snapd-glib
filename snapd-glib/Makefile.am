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
	snapd-channel.h \
	snapd-client.h \
	snapd-connection.h \
	snapd-error.h \
	snapd-icon.h \
	snapd-login.h \
	snapd-media.h \
	snapd-plug.h \
	snapd-price.h \
	snapd-screenshot.h \
	snapd-slot.h \
	snapd-snap.h \
	snapd-system-information.h \
	snapd-task.h \
	snapd-user-information.h \
	snapd-version.h

source_private_h = \
	snapd-json.h \
	requests/snapd-get-aliases.h \
	requests/snapd-get-apps.h \
	requests/snapd-get-assertions.h \
	requests/snapd-get-buy-ready.h \
	requests/snapd-get-change.h \
	requests/snapd-get-changes.h \
	requests/snapd-get-find.h \
	requests/snapd-get-icon.h \
	requests/snapd-get-interfaces.h \
	requests/snapd-get-sections.h \
	requests/snapd-get-snap.h \
	requests/snapd-get-snaps.h \
	requests/snapd-get-system-info.h \
	requests/snapd-get-users.h \
	requests/snapd-post-aliases.h \
	requests/snapd-post-assertions.h \
	requests/snapd-post-buy.h \
	requests/snapd-post-change.h \
	requests/snapd-post-create-user.h \
	requests/snapd-post-create-users.h \
	requests/snapd-post-interfaces.h \
	requests/snapd-post-login.h \
	requests/snapd-post-snap.h \
	requests/snapd-post-snap-stream.h \
	requests/snapd-post-snap-try.h \
	requests/snapd-post-snaps.h \
	requests/snapd-post-snapctl.h \
	requests/snapd-request.h \
	requests/snapd-request-async.h

source_c = \
	snapd-alias.c \
	snapd-app.c \
	snapd-assertion.c \
	snapd-auth-data.c \
	snapd-change.c \
	snapd-channel.c \
	snapd-client.c \
	snapd-client-sync.c \
	snapd-connection.c \
	snapd-error.c \
	snapd-icon.c \
	snapd-login.c \
	snapd-media.c \
	snapd-plug.c \
	snapd-price.c \
	snapd-screenshot.c \
	snapd-slot.c \
	snapd-snap.c \
	snapd-system-information.c \
	snapd-task.c \
	snapd-user-information.c

source_private_c = \
	snapd-json.c \
	requests/snapd-get-aliases.c \
	requests/snapd-get-apps.c \
	requests/snapd-get-assertions.c \
	requests/snapd-get-buy-ready.c \
	requests/snapd-get-change.c \
	requests/snapd-get-changes.c \
	requests/snapd-get-find.c \
	requests/snapd-get-icon.c \
	requests/snapd-get-interfaces.c \
	requests/snapd-get-sections.c \
	requests/snapd-get-snap.c \
	requests/snapd-get-snaps.c \
	requests/snapd-get-system-info.c \
	requests/snapd-get-users.c \
	requests/snapd-post-aliases.c \
	requests/snapd-post-assertions.c \
	requests/snapd-post-buy.c \
	requests/snapd-post-change.c \
	requests/snapd-post-create-user.c \
	requests/snapd-post-create-users.c \
	requests/snapd-post-interfaces.c \
	requests/snapd-post-login.c \
	requests/snapd-post-snap.c \
	requests/snapd-post-snap-stream.c \
	requests/snapd-post-snap-try.c \
	requests/snapd-post-snaps.c \
	requests/snapd-post-snapctl.c \
	requests/snapd-request.c \
	requests/snapd-request-async.c

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
libsnapd_glib_la_LDFLAGS = -version-info 1:0:0 -export-symbols-regex \^snapd_.*
libsnapd_glib_la_LIBADD = $(SNAPD_LIBS)
libsnapd_glib_la_SOURCES = $(source_private_c) $(source_c) $(source_private_h) $(source_h)
nodist_libsnapd_glib_la_SOURCES = $(BUILT_SOURCES)

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
gir_DATA = Snapd-1.gir

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = Snapd-1.typelib

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif # HAVE_INTROSPECTION

if ENABLE_VAPIGEN
include $(VAPIGEN_MAKEFILE)

snapd-glib.vapi: $(gir_DATA)

VAPIGEN_VAPIS = snapd-glib.vapi

snapd_glib_vapi_DEPS = gio-2.0
snapd_glib_vapi_METADATADIRS = $(srcdir)
snapd_glib_vapi_FILES = $(gir_DATA)

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(srcdir)/snapd-glib.deps

EXTRA_DIST += snapd-glib.deps
CLEANFILES += snapd-glib.vapi
endif #ENABLE_VAPIGEN
