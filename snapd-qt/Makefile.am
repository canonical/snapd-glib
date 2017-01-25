pkgconfig_DATA = snapd-qt.pc
pkgconfigdir = $(libdir)/pkgconfig

source_cpp = app.cpp auth-data.cpp change.cpp client.cpp connection.cpp icon.cpp price.cpp request.cpp snap.cpp system-information.cpp task.cpp
source_h = Snapd/App Snapd/app.h \
           Snapd/AuthData Snapd/auth-data.h \
           Snapd/Change Snapd/change.h \
           Snapd/Client Snapd/client.h \
           Snapd/Connection Snapd/connection.h \
           Snapd/Icon Snapd/icon.h \
           Snapd/Price Snapd/price.h \
           Snapd/Request Snapd/request.h \
           Snapd/Snap Snapd/snap.h \
           Snapd/SystemInformation Snapd/system-information.h \
           Snapd/Task Snapd/task.h \
           Snapd/WrappedObject Snapd/wrapped-object.h
source_moc = $(source_cpp:.cpp=_moc.cpp) wrapped-object_moc.cpp

%_moc.cpp: Snapd/%.h
	$(AM_V_GEN) moc --qt=qt5 $< -o $@

libsnapd_qt_la_CPPFLAGS = $(SNAPD_QT_CFLAGS) $(SNAPD_CFLAGS) $(WARN_CFLAGS) -I$(top_srcdir)
libsnapd_qt_la_LDFLAGS = -version-info 1:0:0
libsnapd_qt_la_LIBADD = $(top_builddir)/snapd-glib/libsnapd-glib.la $(SNAPD_QT_LIBS)
libsnapd_qt_la_SOURCES = $(source_cpp) $(source_moc) $(source_h)

snapdincludedir = $(includedir)/snapd-qt/Snapd

cmakedir = $(libdir)/cmake/Snapd
cmake_in_files = SnapdConfig.cmake.in SnapdConfigVersion.cmake.in
SnapdConfig.cmake: SnapdConfig.cmake.in Makefile
	@sed -e "s|\@libdir\@|$(libdir)|" -e "s|\@includedir\@|$(includedir)|" $< > $@

if COMPILE_QML_MODULE
qmlmodule_DATA = qmldir
qmlmoduledir = $(libdir)/qt5/qml/Snapd
qmlmodule_LTLIBRARIES = libSnapd.la
libSnapd_la_SOURCES = qml-plugin.cpp qml-plugin.h qml-plugin_moc.cpp
libSnapd_la_CXXFLAGS = $(WARN_CXXFLAGS) -fPIC $(QML_MODULE_CFLAGS)
libSnapd_la_LIBADD = $(QML_MODULE_LIBS) -lsnapd-qt
libSnapd_la_LDFLAGS = -rpath $(qmlmoduledir) -module -avoid-version -no-undefined
qml-plugin_moc.cpp: qml-plugin.h
	$(AM_V_GEN) moc --qt=qt5 $< -o $@
endif

EXTRA_DIST = $(cmake_in_files) qmldir
CLEANFILES = $(source_moc)
DISTCLEANFILES = SnapdConfig.cmake

#if COMPILE_SNAPD_QT
lib_LTLIBRARIES = libsnapd-qt.la
snapdinclude_DATA = $(source_h)
cmake_DATA = $(cmake_in_files:.cmake.in=.cmake)
#endif
