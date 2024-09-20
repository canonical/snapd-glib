if get_option ('docs')

  docgen = find_program('gi-docgen')

  doc_conf = configuration_data ()
  doc_conf.set ('PROJECT_VERSION', meson.project_version ())
  settings_file = configure_file (input: 'settings.toml.in',
                                  output: 'settings.toml',
                                  configuration: doc_conf)
  custom_target('documentation',
                depends: snapd_glib_gir,
                command : [docgen, 'generate', '--config', settings_file, '--output-dir', '@OUTDIR@', join_paths (meson.project_build_root (), 'snapd-glib', 'Snapd-2.gir')],
                output: 'Snapd-2',
                build_by_default: true,
                install: true,
                install_dir: get_option('prefix') / get_option('datadir') / 'doc',)
endif
