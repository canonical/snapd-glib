project ('snapd-glib', [ 'c', 'cpp' ],
         version: '1.61',
         meson_version: '>= 0.43.0',
         default_options : [ 'c_std=c11', 'cpp_std=c++11' ])

glib_dep = dependency ('glib-2.0', version: '>= 2.46')
gio_dep = dependency ('gio-2.0', version: '>= 2.46')
gio_unix_dep = dependency ('gio-unix-2.0', version: '>= 2.46')
if get_option('soup2')
  libsoup_dep = dependency ('libsoup-2.4', version: '>= 2.32')
else
  libsoup_dep = dependency ('libsoup-3.0', version: '>= 2.99.2')
endif
json_glib_dep = dependency ('json-glib-1.0', version: '>= 1.1.2')

if get_option ('qt-bindings')
  qt5_core_dep = dependency ('qt5', modules: [ 'Core' ])
  qt5_network_dep = dependency ('qt5', modules: [ 'Network' ])
  qml_dep = dependency ('qt5', modules: [ 'Qml' ])
endif

datadir = join_paths (get_option ('prefix'), get_option ('datadir'))
includedir = join_paths (get_option ('prefix'), get_option ('includedir'))
libdir = join_paths (get_option ('prefix'), get_option ('libdir'))

subdir ('snapd-glib')
subdir ('snapd-qt')
subdir ('tests')
subdir ('examples')
subdir ('doc')
subdir ('po')
