m4_define([glib_req_version], [2.37.6])

AC_PREREQ([2.63])

AC_INIT([snapd-glib], [1.11])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([snapd-glib/snapd-glib.h])

AM_INIT_AUTOMAKE([1.11 foreign dist-xz no-dist-gzip])

AM_SILENT_RULES([yes])
AM_PATH_GLIB_2_0
AM_PROG_CC_C_O
AC_PROG_CXX

WARN_CFLAGS="-Wall"
AC_SUBST(WARN_CFLAGS)

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

dnl === Dependencies ==========================================================

PKG_CHECK_MODULES(SNAPD, [gobject-2.0 gio-2.0 gio-unix-2.0 libsoup-2.4 json-glib-1.0])
PKG_CHECK_MODULES(SNAPD_LOGIN_SERVICE, [gio-2.0 gio-unix-2.0 polkit-gobject-1])

dnl === Test suite ============================================================

GLIB_TESTS

dnl === snapd-qt ==============================================================

AC_ARG_ENABLE(snapd-qt,
	AS_HELP_STRING([--enable-snapd-qt],[Enable snapd-qt [[default=auto]]]),
	[enable_snapd_qt=$enableval],
	[enable_snapd_qt="auto"])
compile_snapd_qt=no
if test x"$enable_snapd_qt" != "xno"; then
    PKG_CHECK_MODULES(SNAPD_QT, [Qt5Core Qt5Network],
    [compile_snapd_qt=yes
     AC_CHECK_TOOLS(MOC5, [moc])
     if test x"$(readlink $(which $MOC5))" = xqtchooser; then
         MOC5="$MOC5 --qt=qt5"
     fi
    ],
    [if test "x$enable_snapd_qt" != xauto; then
        AC_MSG_FAILURE(
          [--enable-snapd-qt was given, but test for Qt5 failed])
      fi
    ])
fi
AM_CONDITIONAL(COMPILE_SNAPD_QT, test x"$compile_snapd_qt" != "xno")

AC_ARG_ENABLE(qml-module,
	AS_HELP_STRING([--enable-qml-module],[Enable QML module [[default=auto]]]),
	[enable_qml_module=$enableval],
	[enable_qml_module="auto"])
compile_qml_module=no
if test x"$enable_qml_module" != "xno"; then
    PKG_CHECK_MODULES(QML_MODULE, [
        Qt5Qml
    ],
    [compile_qml_module=yes],
    [if test "x$enable_qml_module" != xauto; then
        AC_MSG_FAILURE(
          [--enable-qml-module was given, but test for QML failed])
      fi
    ])
fi
AM_CONDITIONAL(COMPILE_QML_MODULE, test x"$compile_qml_module" != "xno")

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
        snapd-qt/Makefile
        snapd-qt/SnapdConfigVersion.cmake
        snapd-qt/snapd-qt.pc
        snapd-login-service/Makefile
        tests/Makefile
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
echo " • API reference:            ${enable_gtk_doc}"
echo " • GObject introspection:    ${enable_introspection}"
echo " • Vala bindings:            ${enable_vala}"
echo " • snapd-qt:                 ${compile_snapd_qt}"
echo " • QML module:               ${compile_qml_module}"
echo ""
