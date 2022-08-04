library_name = 'snapd-qt'
if not get_option('soup2')
  library_name += '-2'
endif

qt_name= 'Snapd'
if not get_option('soup2')
  qt_name += '2'
endif

install_header_dir = join_paths (includedir, library_name, 'Snapd')
qml_dir = join_paths (libdir, 'qt5', 'qml', qt_name)
cmake_dir = join_paths (libdir, 'cmake', qt_name)

source_cpp = [
  'alias.cpp',
  'app.cpp',
  'assertion.cpp',
  'auth-data.cpp',
  'change.cpp',
  'channel.cpp',
  'client.cpp',
  'connection.cpp',
  'icon.cpp',
  'interface.cpp',
  'markdown-node.cpp',
  'markdown-parser.cpp',
  'maintenance.cpp',
  'media.cpp',
  'plug.cpp',
  'plug-ref.cpp',
  'price.cpp',
  'request.cpp',
  'screenshot.cpp',
  'slot.cpp',
  'slot-ref.cpp',
  'snap.cpp',
  'stream-wrapper.cpp',
  'system-information.cpp',
  'task.cpp',
  'user-information.cpp',
]

source_h = [
  'Snapd/alias.h',
  'Snapd/app.h',
  'Snapd/assertion.h',
  'Snapd/auth-data.h',
  'Snapd/change.h',
  'Snapd/channel.h',
  'Snapd/client.h',
  'Snapd/connection.h',
  'Snapd/enums.h',
  'Snapd/icon.h',
  'Snapd/interface.h',
  'Snapd/maintenance.h',
  'Snapd/markdown-node.h',
  'Snapd/markdown-parser.h',
  'Snapd/media.h',
  'Snapd/plug.h',
  'Snapd/plug-ref.h',
  'Snapd/price.h',
  'Snapd/request.h',
  'Snapd/screenshot.h',
  'Snapd/slot.h',
  'Snapd/slot-ref.h',
  'Snapd/snap.h',
  'Snapd/system-information.h',
  'Snapd/task.h',
  'Snapd/user-information.h',
  'Snapd/wrapped-object.h',
]

source_alias_h = [
  'Snapd/Alias',
  'Snapd/App',
  'Snapd/Assertion',
  'Snapd/AuthData',
  'Snapd/Change',
  'Snapd/Channel',
  'Snapd/Client',
  'Snapd/Connection',
  'Snapd/Enums',
  'Snapd/Icon',
  'Snapd/Interface',
  'Snapd/Maintenance',
  'Snapd/MarkdownNode',
  'Snapd/MarkdownParser',
  'Snapd/Media',
  'Snapd/Plug',
  'Snapd/PlugRef',
  'Snapd/Price',
  'Snapd/Request',
  'Snapd/Screenshot',
  'Snapd/Slot',
  'Snapd/SlotRef',
  'Snapd/Snap',
  'Snapd/SystemInformation',
  'Snapd/Task',
  'Snapd/UserInformation',
  'Snapd/WrappedObject',
]

source_private_h = [
  'client-private.h',
  'stream-wrapper.h',
  'variant.h',
]

if get_option ('qt-bindings')
  qt5 = import ('qt5')
  moc_files = qt5.preprocess (moc_headers: [ source_h, source_private_h ],
                              dependencies: [ qt5_core_dep, qt5_network_dep ])

  snapd_qt_lib = library (library_name,
                          source_cpp, moc_files,
                          version: '1.0.0',
                          dependencies: [ qt5_core_dep, qt5_network_dep, glib_dep, gio_dep, snapd_glib_dep ],
                          cpp_args: [ '-DQT_NO_SIGNALS_SLOTS_KEYWORDS' ],
                          install: true)

  snapd_qt_dep = declare_dependency (link_with: snapd_qt_lib,
                                     dependencies: [ qt5_network_dep ],
                                     include_directories: include_directories ('.'))

  install_headers (source_h + source_alias_h,
                   install_dir: install_header_dir)

  pc = import ('pkgconfig')
  pc.generate (libraries: snapd_qt_lib,
               filebase: library_name,
               name: 'Snapd Qt',
               description: 'Libary for accessing snapd',
               version: meson.project_version ())

  install_data ('qmldir',
                install_dir: qml_dir)

  cmake_conf = configuration_data ()
  cmake_conf.set ('libdir', libdir)
  cmake_conf.set ('includedir', includedir)
  cmake_conf.set ('version', meson.project_version ())
  cmake_conf.set ('library_name', library_name)
  cmake_file = configure_file (input: 'SnapdConfig.cmake.in',
                               output: 'SnapdConfig.cmake',
                               configuration: cmake_conf)
  cmake_version_file = configure_file (input: 'SnapdConfigVersion.cmake.in',
                                       output: 'SnapdConfigVersion.cmake',
                                       configuration: cmake_conf)
  install_data (cmake_file, cmake_version_file,
                install_dir: cmake_dir)

  if get_option ('qml-bindings')
    install_data ('qmldir',
                  install_dir: qml_dir)

    qml_moc_files = qt5.preprocess (moc_headers: 'qml-plugin.h',
                                    dependencies: qml_dep)
    library (qt_name,
             'qml-plugin.cpp', qml_moc_files,
             dependencies: [ qml_dep, snapd_qt_dep ],
             build_rpath: qml_dir,
             install: true,
             install_dir: qml_dir)
  endif
endif
