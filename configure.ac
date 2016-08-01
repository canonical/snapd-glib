m4_define([glib_req_version], [2.37.6])

AC_PREREQ([2.63])

AC_INIT([snapd-glib], [0.0.1])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([snapd-glib/snapd-glib.h])

AM_INIT_AUTOMAKE([1.11 no-define foreign -Wno-portability dist-xz no-dist-gzip tar-ustar subdir-objects])

AM_SILENT_RULES([yes])
AM_PATH_GLIB_2_0
AM_PROG_CC_C_O

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

dnl === Dependencies ==========================================================

PKG_CHECK_MODULES(SNAPD, [gobject-2.0 libsoup-2.4 json-glib-1.0])

dnl === Introspection =========================================================

GOBJECT_INTROSPECTION_CHECK([0.9.5])

dnl === Output ================================================================

AC_SUBST(SNAPD_CFLAGS)
AC_SUBST(SNAPD_LIBS)

AC_CONFIG_FILES([
        Makefile

        snapd-glib/Makefile
        snapd-glib/snapd-glib-1.0.pc
])

AC_OUTPUT
