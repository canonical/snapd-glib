m4_define([glib_req_version], [2.37.6])

AC_PREREQ([2.63])

AC_INIT([snapd-glib], [0.14])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([snapd-glib/snapd-glib.h])

AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip])

AM_SILENT_RULES([yes])
AM_PATH_GLIB_2_0
AM_PROG_CC_C_O

WARN_CFLAGS="-Wall"
AC_SUBST(WARN_CFLAGS)

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

dnl === Dependencies ==========================================================

PKG_CHECK_MODULES(SNAPD, [gobject-2.0 gio-2.0 gio-unix-2.0 libsoup-2.4 json-glib-1.0])
PKG_CHECK_MODULES(SNAPD_LOGIN_SERVICE, [gio-2.0 gio-unix-2.0 polkit-gobject-1])

dnl === Translations ==========================================================

AC_PROG_INTLTOOL()
AC_SUBST([GETTEXT_PACKAGE], [snapd-glib])

dnl === Documentation =========================================================

GTK_DOC_CHECK([1.20], [--flavour no-tmpl])

dnl === Introspection =========================================================

GOBJECT_INTROSPECTION_CHECK([0.9.5])

VAPIGEN_CHECK([0.16])

dnl === Output ================================================================

AC_SUBST(SNAPD_CFLAGS)
AC_SUBST(SNAPD_LIBS)

AC_CONFIG_FILES([
        Makefile
        doc/Makefile
        doc/reference/Makefile
        doc/reference/version.xml
        po/Makefile.in
        snapd-glib/Makefile
        snapd-glib/snapd-glib.pc
        snapd-login-service/Makefile
])

AC_OUTPUT

dnl === Summary ===============================================================

echo ""
echo " Snapd-GLib - $VERSION"
echo ""
echo " • Prefix: ${prefix}"
echo ""
echo " • Compiler: ${CC}"
echo " • Compiler flags: ${WARN_CFLAGS} ${CFLAGS} ${MAINTAINER_CFLAGS}"
echo ""
echo " • API reference: ${enable_gtk_doc}"
echo " • Build introspection data: ${enable_introspection}"
echo " • Build Vala bindings: ${enable_vala}"
echo ""
