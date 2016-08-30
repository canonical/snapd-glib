libexec_PROGRAMS = snapd-login-service

snapd_login_service_CFLAGS = $(SNAPD_LOGIN_SERVICE_CFLAGS) $(WARN_CFLAGS) -I$(top_srcdir)
snapd_login_service_LDADD = $(SNAPD_LOGIN_SERVICE_LIBS) $(top_builddir)/snapd-glib/libsnapd-glib.la
snapd_login_service_SOURCES = snapd-login-service.c login-service.c

login-service.c login-service.h: io.snapcraft.SnapdLoginService.xml Makefile
	$(AM_V_GEN) gdbus-codegen --generate-c-code=login-service $<

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = io.snapcraft.SnapdLoginService.conf

interfacesdir = $(datadir)/dbus-1/interfaces
interfaces_DATA = io.snapcraft.SnapdLoginService.xml

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_in_files = io.snapcraft.SnapdLoginService.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@servicedir\@|$(libexecdir)|" $< > $@

polkitdir = $(datadir)/polkit-1/actions
polkit_DATA = io.snapcraft.SnapdLoginService.policy

EXTRA_DIST = $(dbusconf_DATA) $(interfaces_DATA) $(dbusservice_in_files)
DISTCLEANFILES = $(dbusservice_DATA)
