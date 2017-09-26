if COMPILE_QML_MODULE
pkgconfig_DATA = snapd-qt.pc
pkgconfigdir = $(libdir)/pkgconfig
endif

source_cpp = alias.cpp app.cpp assertion.cpp auth-data.cpp change.cpp client.cpp connection.cpp icon.cpp plug.cpp price.cpp request.cpp screenshot.cpp slot.cpp snap.cpp system-information.cpp task.cpp
source_moc = $(source_cpp:.cpp=_moc.cpp) client-private_moc.cpp wrapped-object_moc.cpp enums_moc.cpp
source_h = Snapd/Alias Snapd/alias.h \
           Snapd/App Snapd/app.h \
           Snapd/Assertion Snapd/assertion.h \
           Snapd/AuthData Snapd/auth-data.h \
           Snapd/Change Snapd/change.h \
           Snapd/Client Snapd/client.h \
           Snapd/Connection Snapd/connection.h \
           Snapd/Enums Snapd/enums.h \
           Snapd/Icon Snapd/icon.h \
           Snapd/Plug Snapd/plug.h \
           Snapd/Price Snapd/price.h \
           Snapd/Request Snapd/request.h \
           Snapd/Screenshot Snapd/screenshot.h \
           Snapd/Slot Snapd/slot.h \
           Snapd/Snap Snapd/snap.h \
           Snapd/SystemInformation Snapd/system-information.h \
           Snapd/Task Snapd/task.h \
           Snapd/WrappedObject Snapd/wrapped-object.h
private_h = client-private.h stream-wrapper.h

%_moc.cpp: Snapd/%.h
	$(AM_V_GEN) $(MOC5) $< -o $@
client-private_moc.cpp: client-private.h
	$(AM_V_GEN) $(MOC5) $< -o $@

libsnapd_qt_la_CPPFLAGS = -DQT_NO_SIGNALS_SLOTS_KEYWORDS --std=c++11 $(SNAPD_QT_CFLAGS) $(SNAPD_CFLAGS) $(WARN_CFLAGS) -I$(top_srcdir)
libsnapd_qt_la_LDFLAGS = -version-info 1:0:0
libsnapd_qt_la_LIBADD = $(top_builddir)/snapd-glib/libsnapd-glib.la $(SNAPD_QT_LIBS)
libsnapd_qt_la_SOURCES = $(source_cpp) stream-wrapper.cpp $(source_h) $(private_h)
nodist_libsnapd_qt_la_SOURCES = $(source_moc)

snapdincludedir = $(includedir)/snapd-qt/Snapd

cmakedir = $(libdir)/cmake/Snapd
cmake_in_files = SnapdConfig.cmake.in SnapdConfigVersion.cmake.in
SnapdConfig.cmake: SnapdConfig.cmake.in Makefile
	@sed -e "s|\@libdir\@|$(libdir)|" -e "s|\@includedir\@|$(includedir)|" $< > $@

if COMPILE_QML_MODULE
qmlmodule_DATA = qmldir
qmlmoduledir = $(libdir)/qt5/qml/Snapd
qmlmodule_LTLIBRARIES = libSnapd.la
libSnapd_la_SOURCES = qml-plugin.cpp qml-plugin.h
nodist_libSnapd_la_SOURCES = qml-plugin_moc.cpp
libSnapd_la_CXXFLAGS = --std=c++11 $(WARN_CXXFLAGS) -fPIC $(QML_MODULE_CFLAGS)
libSnapd_la_LIBADD = $(QML_MODULE_LIBS) libsnapd-qt.la
libSnapd_la_LDFLAGS = -rpath $(qmlmoduledir) -module -avoid-version -no-undefined
qml-plugin_moc.cpp: qml-plugin.h
	$(AM_V_GEN) $(MOC5) $< -o $@
endif

EXTRA_DIST = $(cmake_in_files) qmldir
CLEANFILES = $(source_moc) qml-plugin_moc.cpp
DISTCLEANFILES = SnapdConfig.cmake

if COMPILE_SNAPD_QT
lib_LTLIBRARIES = libsnapd-qt.la
snapdinclude_DATA = $(source_h)
cmake_DATA = $(cmake_in_files:.cmake.in=.cmake)
endif
