DOC_MODULE = snapd-glib
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/snapd-glib $(top_builddir)/snapd-glib

SCANGOBJ_OPTIONS =
SCAN_OPTIONS = --deprecated-guards="SNAPD_DISABLE_DEPRECATED" --rebuild-types --ignore-headers="$(top_srcdir)/snapd-glib/tests/mock-snapd.h $(top_srcdir)/snapd-glib/snapd-json.h"
MKDB_OPTIONS = --output-format=xml --name-space=snapd
MKTMPL_OPTIONS =
FIXXREF_OPTIONS = \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gobject \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gio

HFILE_GLOB = $(top_srcdir)/snapd-glib/*.h $(top_builddir)/snapd-glib/*.h
CFILE_GLOB = $(top_srcdir)/snapd-glib/*.c

IGNORE_HFILES = \
	snapd-glib.h 		\
	snapd-enum-types.h

EXTRA_HFILES =

HTML_IMAGES =

content_files = \
	version.xml

expand_content_files =

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(SNAPD_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/snapd-glib/libsnapd-glib.la $(SNAPD_LIBS)

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += version.xml.in

if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = \
	DOC_MODULE=$(DOC_MODULE) \
	DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
	SRCDIR=$(abs_srcdir) \
	BUILDDIR=$(abs_builddir)

TESTS = $(GTKDOC_CHECK)
endif
