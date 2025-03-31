project ('snapd-glib', [ 'c', 'cpp' ],
         version: '1.68',
         meson_version: '>= 0.58.0',
         default_options : [ 'c_std=c11' ])


glib_dep = dependency ('glib-2.0', version: '>= 2.46')
gio_dep = dependency ('gio-2.0', version: '>= 2.46')
gio_unix_dep = dependency ('gio-unix-2.0', version: '>= 2.46')

build_introspection = get_option ('introspection')
if get_option('docs')
  build_introspection = true
endif

if get_option('soup2')
  libsoup_dep = dependency ('libsoup-2.4', version: '>= 2.32')
else
  libsoup_dep = dependency ('libsoup-3.0', version: '>= 2.99.2')
endif
json_glib_dep = dependency ('json-glib-1.0', version: '>= 1.1.2')

datadir = join_paths (get_option ('prefix'), get_option ('datadir'))
includedir = join_paths (get_option ('prefix'), get_option ('includedir'))
libdir = join_paths (get_option ('prefix'), get_option ('libdir'))

subdir ('snapd-glib')
if get_option('qt5') or get_option('qt6')
    subdir ('snapd-qt')
endif
if get_option ('tests')
  subdir ('tests')
endif
if get_option ('examples')
  subdir ('examples')
endif
subdir ('po')

if get_option('docs')
  subdir ('doc')
endif
