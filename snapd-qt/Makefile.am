pkgconfig_DATA = snapd-qt.pc
pkgconfigdir = $(libdir)/pkgconfig

source_cpp = auth-data.cpp client.cpp icon.cpp request.cpp snap.cpp system-information.cpp
source_moc = $(source_cpp:.cpp=_moc.cpp)
source_h = Snapd/AuthData Snapd/auth-data.h \
           Snapd/Client Snapd/client.h \
           Snapd/Icon Snapd/icon.h \
           Snapd/Request Snapd/request.h \
           Snapd/Snap Snapd/snap.h \
           Snapd/SystemInformation Snapd/system-information.h

%_moc.cpp: Snapd/%.h
	$(AM_V_GEN) moc --qt=qt5 $< -o $@

libsnapd_qt_la_CPPFLAGS = $(SNAPD_QT_CFLAGS) $(SNAPD_CFLAGS) $(WARN_CFLAGS)
libsnapd_qt_la_LIBADD = $(top_srcdir)/snapd-glib/libsnapd-glib.la $(SNAPD_QT_LIBS)
libsnapd_qt_la_SOURCES = $(source_cpp) $(source_moc) $(source_h)

snapdincludedir = $(includedir)/snapd-qt/Snapd

cmakedir = $(libdir)/cmake/Snapd
cmake_in_files = SnapdConfig.cmake.in SnapdConfigVersion.cmake.in
SnapdConfig.cmake: SnapdConfig.cmake.in Makefile
	@sed -e "s|\@libdir\@|$(libdir)|" -e "s|\@includedir\@|$(includedir)|" $< > $@

qmlmodule_DATA = qmldir
qmlmoduledir = $(libdir)/qt5/qml/Snapd
qmlmodule_LTLIBRARIES = libSnapd.la
libSnapd_la_SOURCES = qml-plugin.cpp qml-plugin.h qml-plugin_moc.cpp
libSnapd_la_CXXFLAGS = $(WARN_CXXFLAGS) -fPIC $(QML_MODULE_CFLAGS)
libSnapd_la_LIBADD = $(QML_MODULE_LIBS) -lsnapd-qt
libSnapd_la_LDFLAGS = -rpath $(qmlmoduledir) -module -avoid-version -no-undefined
qml-plugin_moc.cpp: qml-plugin.h
	$(AM_V_GEN) moc --qt=qt5 $< -o $@

EXTRA_DIST = $(cmake_in_files)
CLEANFILES = $(source_moc)
DISTCLEANFILES = SnapdConfig.cmake

#if COMPILE_SNAPD_QT
lib_LTLIBRARIES = libsnapd-qt.la
snapdinclude_DATA = $(source_h)
cmake_DATA = $(cmake_in_files:.cmake.in=.cmake)
#endif
