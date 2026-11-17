if get_option ('docs')
  docgen = find_program('gi-docgen')

  doc_conf = configuration_data ()
  doc_conf.set ('PROJECT_VERSION', meson.project_version ())
  settings_file = configure_file (input: 'settings.toml.in',
                                  output: 'settings.toml',
                                  configuration: doc_conf)
  custom_target('documentation',
                depends: snapd_glib_gir,
                depend_files: files('tutorials.md',
                                    'how-tos.md',
                                    'explanations.md',
                ),
                command : [docgen,
                           'generate',
                           '--config', settings_file,
                           '--no-namespace-dir',
                           '--content-dir', meson.current_source_dir(),
                           '--output-dir', '@OUTDIR@' / 'html',
                           meson.project_build_root () / 'snapd-glib' / 'Snapd-2.gir'],
                output: 'html',
                build_by_default: true,
                install: true,
                install_dir: get_option('prefix') / get_option('datadir') / 'doc' / meson.project_name()) # installs to PREFIX/share/doc/snapd-glib/html
endif
