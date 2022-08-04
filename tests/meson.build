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
                              dependencies: [ glib_dep, snapd_glib_dep ],
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

if get_option ('qt-bindings')
  moc_files = qt5.preprocess (moc_headers: [ 'test-qt.h' ])

  test_executable = executable ('test-qt',
                                'test-qt.cpp', moc_files,
                                dependencies: [ glib_dep, snapd_qt_dep ],
                                link_with: [ mock_snapd_lib ],
                                cpp_args: [ '-DVERSION="@0@"'.format (meson.project_version ()) ],
                                install_dir: installed_tests_exec_dir,
                                install: true)
  test ('Tests (Qt)', test_executable, timeout: 600)
  test_file = configure_file (input: 'test-qt.test.in',
                              output: 'test-qt.test',
                              configuration: test_data_conf)
  install_data (test_file, install_dir: installed_tests_data_dir)

  test_executable = executable ('test-markdown-qt',
                                'test-markdown-qt.cpp',
                                dependencies: [ glib_dep, snapd_qt_dep ],
                                link_with: [ mock_snapd_lib ],
                                install_dir: installed_tests_exec_dir,
                                install: true)
  test ('Markdown tests (Qt)', test_executable, timeout: 600)
  test_file = configure_file (input: 'test-markdown-qt.test.in',
                              output: 'test-markdown-qt.test',
                              configuration: test_data_conf)
  install_data (test_file, install_dir: installed_tests_data_dir)
endif
