include $(top_srcdir)/glib-tap.mk

AM_CPPFLAGS = -g \
	$(WARN_CFLAGS) \
	$(SNAPD_CFLAGS) \
	-I$(top_srcdir)
LDADD = $(top_builddir)/snapd-glib/libsnapd-glib.la $(SNAPD_LIBS)

test_programs = \
	test

test_SOURCES = test.c mock-snapd.c mock-snapd.h
