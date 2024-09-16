if get_option ('docs')
  doc_conf = configuration_data ()
  doc_conf.set ('PROJECT_VERSION', meson.project_version ())
  settings_file = configure_file (input: 'settings.toml.in',
                              output: 'settings.toml',
                              configuration: doc_conf)
  custom_target('documentation',
          depends: snapd_glib_gir,
          command : ['gi-docgen', 'generate', '--config', settings_file, '--output-dir='+join_paths (meson.project_build_root (), 'doc', 'html'), join_paths (meson.project_build_root (), 'snapd-glib', 'Snapd-2.gir')],
          output: 'fake',
          build_by_default : true)
endif
