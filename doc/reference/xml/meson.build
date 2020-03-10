if get_option ('docs')
  ent_conf = configuration_data()
  ent_conf.set('PACKAGE_VERSION', meson.project_version())
  configure_file(input: 'gtkdocentities.ent.in', output: 'gtkdocentities.ent', configuration: ent_conf)
endif
