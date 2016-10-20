pkgconfig_DATA = snapd-qt.pc
pkgconfigdir = $(libdir)/pkgconfig

source_cpp = auth-data.cpp client.cpp icon.cpp login.cpp reply.cpp snap.cpp system-information.cpp
source_moc = $(source_cpp:.cpp=_moc.cpp)
source_h = Snapd/AuthData Snapd/auth-data.h \
           Snapd/Client Snapd/client.h \
           Snapd/Icon Snapd/icon.h \
           Snapd/Login Snapd/login.h \
           Snapd/Reply Snapd/reply.h \
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

EXTRA_DIST = $(cmake_in_files)
CLEANFILES = $(source_moc)
DISTCLEANFILES = SnapdConfig.cmake

#if COMPILE_SNAPD_QT
lib_LTLIBRARIES = libsnapd-qt.la
snapdinclude_DATA = $(source_h)
cmake_DATA = $(cmake_in_files:.cmake.in=.cmake)
#endif
