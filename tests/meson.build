mock_snapd_lib = static_library ('mock-snapd',
                                 [ 'mock-snapd.c', 'mock-snapd.h' ],
                                 dependencies: [ glib_dep, gio_unix_dep, libsoup_dep, json_glib_dep ])

test_executable = executable ('test-glib',
                              'test-glib.c',
                              dependencies: [ glib_dep, snapd_glib_dep ],
                              link_with: [ mock_snapd_lib ],
                              c_args: [ '-DVERSION="@0@"'.format (meson.project_version ()) ])
test ('Tests', test_executable)

test_executable = executable ('test-markdown-glib',
                              'test-markdown-glib.c',
                              dependencies: [ glib_dep, snapd_glib_dep ],
                              link_with: [ mock_snapd_lib ])
test ('Markdown tests', test_executable)

if get_option ('qt-bindings')
  moc_files = qt5.preprocess (moc_headers: [ 'test-qt.h' ])

  test_executable = executable ('test-qt',
                                'test-qt.cpp', moc_files,
                                dependencies: [ glib_dep, snapd_qt_dep ],
                                link_with: [ mock_snapd_lib ],
                                cpp_args: [ '-DVERSION="@0@"'.format (meson.project_version ()) ])
  test ('Tests (Qt)', test_executable)

  test_executable = executable ('test-markdown-qt',
                                'test-markdown-qt.cpp',
                                dependencies: [ glib_dep, snapd_qt_dep ],
                                link_with: [ mock_snapd_lib ])
  test ('Markdown tests (Qt)', test_executable)
endif
