if get_option ('tests')
  installed_tests_exec_dir = join_paths (get_option ('libexecdir'), 'installed-tests', meson.project_name())
  installed_tests_data_dir = join_paths (get_option ('datadir'), 'installed-tests', meson.project_name())

  if not get_option('soup2')
    installed_tests_exec_dir += '-2'
    installed_tests_data_dir += '-2'
  endif

  test_data_conf = configuration_data ()
  test_data_conf.set ('installed_tests_exec_dir', join_paths (get_option ('prefix'), installed_tests_exec_dir))

  mock_snapd_lib = static_library ('mock-snapd',
                                  [ 'mock-snapd.c', 'mock-snapd.h' ],
                                  dependencies: [ glib_dep, gio_unix_dep, libsoup_dep, json_glib_dep ])

  test_executable = executable ('test-glib',
                                'test-glib.c',
                                dependencies: [ glib_dep, snapd_glib_dep, json_glib_dep ],
                                link_with: [ mock_snapd_lib ],
                                c_args: [ '-DVERSION="@0@"'.format (meson.project_version ()) ],
                                install_dir: installed_tests_exec_dir,
                                install: true)
  test ('Tests', test_executable, timeout: 600)
  test_file = configure_file (input: 'test-glib.test.in',
                              output: 'test-glib.test',
                              configuration: test_data_conf)
  install_data (test_file, install_dir: installed_tests_data_dir)

  test_executable = executable ('test-markdown-glib',
                                'test-markdown-glib.c',
                                dependencies: [ glib_dep, snapd_glib_dep ],
                                link_with: [ mock_snapd_lib ],
                                install_dir: installed_tests_exec_dir,
                                install: true)
  test ('Markdown tests', test_executable, timeout: 600)
  test_file = configure_file (input: 'test-markdown-glib.test.in',
                              output: 'test-markdown-glib.test',
                              configuration: test_data_conf)
  install_data (test_file, install_dir: installed_tests_data_dir)

  if get_option('qt5') or get_option('qt6')
    moc_files = qt.preprocess (moc_headers: [ 'test-qt.h' ])

    test_executable = executable (f'test-@qt_version@',
                                  'test-qt.cpp', moc_files,
                                  dependencies: [ glib_dep, snapd_qt_dep, json_glib_dep ],
                                  link_with: [ mock_snapd_lib ],
                                  cpp_args: [ '-DVERSION="@0@"'.format (meson.project_version ()) ],
                                  install_dir: installed_tests_exec_dir,
                                  install: true)
    test (f'Tests (@qt_version@', test_executable, timeout: 600)
    test_file = configure_file (input: f'test-@qt_version@.test.in',
                                output: f'test-@qt_version@.test',
                                configuration: test_data_conf)
    install_data (test_file, install_dir: installed_tests_data_dir)

    test_executable = executable (f'test-markdown-@qt_version@',
                                  'test-markdown-qt.cpp',
                                  dependencies: [ glib_dep, snapd_qt_dep ],
                                  link_with: [ mock_snapd_lib ],
                                  install_dir: installed_tests_exec_dir,
                                  install: true)
    test (f'Markdown tests (@qt_version@)', test_executable, timeout: 600)
    test_file = configure_file (input: f'test-markdown-@qt_version@.test.in',
                                output: f'test-markdown-@qt_version@.test',
                                configuration: test_data_conf)
    install_data (test_file, install_dir: installed_tests_data_dir)
  endif
endif
