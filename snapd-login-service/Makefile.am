libexec_PROGRAMS = snapd-login-service

login_sources = login-service.c login-service.h

snapd_login_service_CFLAGS = $(SNAPD_LOGIN_SERVICE_CFLAGS) $(WARN_CFLAGS) -I$(top_srcdir)
snapd_login_service_LDADD = $(SNAPD_LOGIN_SERVICE_LIBS) $(top_builddir)/snapd-glib/libsnapd-glib.la
snapd_login_service_SOURCES = snapd-login-service.c $(login_sources)

$(login_sources): io.snapcraft.SnapdLoginService.xml Makefile
	$(AM_V_GEN) gdbus-codegen --generate-c-code=login-service $<

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = io.snapcraft.SnapdLoginService.conf

interfacesdir = $(datadir)/dbus-1/interfaces
interfaces_DATA = io.snapcraft.SnapdLoginService.xml

dbusservicedir = $(datadir)/dbus-1/system-services
dbusservice_in_files = io.snapcraft.SnapdLoginService.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@servicedir\@|$(libexecdir)|" $< > $@

polkitdir = $(datadir)/polkit-1/actions
polkit_in_files = io.snapcraft.SnapdLoginService.policy.in
polkit_DATA = $(polkit_in_files:.policy.in=.policy)
@INTLTOOL_POLICY_RULE@

EXTRA_DIST = $(dbusconf_DATA) $(interfaces_DATA) $(dbusservice_in_files) $(polkit_in_files)
CLEANFILES = $(login_sources) $(polkit_DATA)
DISTCLEANFILES = $(dbusservice_DATA)
