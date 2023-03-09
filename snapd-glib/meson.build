library_name = 'snapd-glib'
if not get_option('soup2')
  library_name += '-2'
endif

install_header_dir = join_paths (includedir, library_name, 'snapd-glib')

source_h = [
  'snapd-alias.h',
  'snapd-app.h',
  'snapd-assertion.h',
  'snapd-auth-data.h',
  'snapd-category.h',
  'snapd-category-details.h',
  'snapd-change.h',
  'snapd-channel.h',
  'snapd-client.h',
  'snapd-connection.h',
  'snapd-error.h',
  'snapd-icon.h',
  'snapd-interface.h',
  'snapd-login.h',
  'snapd-maintenance.h',
  'snapd-markdown-node.h',
  'snapd-markdown-parser.h',
  'snapd-media.h',
  'snapd-plug.h',
  'snapd-plug-ref.h',
  'snapd-price.h',
  'snapd-screenshot.h',
  'snapd-slot.h',
  'snapd-slot-ref.h',
  'snapd-snap.h',
  'snapd-system-information.h',
  'snapd-task.h',
  'snapd-user-information.h',
  'snapd-version.h',
]

source_private_h = [
  'requests/snapd-json.h',
  'requests/snapd-get-aliases.h',
  'requests/snapd-get-apps.h',
  'requests/snapd-get-assertions.h',
  'requests/snapd-get-buy-ready.h',
  'requests/snapd-get-categories.h',
  'requests/snapd-get-change.h',
  'requests/snapd-get-changes.h',
  'requests/snapd-get-connections.h',
  'requests/snapd-get-find.h',
  'requests/snapd-get-icon.h',
  'requests/snapd-get-interfaces.h',
  'requests/snapd-get-interfaces-legacy.h',
  'requests/snapd-get-sections.h',
  'requests/snapd-get-snap.h',
  'requests/snapd-get-snap-conf.h',
  'requests/snapd-get-snaps.h',
  'requests/snapd-get-system-info.h',
  'requests/snapd-get-themes.h',
  'requests/snapd-get-users.h',
  'requests/snapd-post-aliases.h',
  'requests/snapd-post-assertions.h',
  'requests/snapd-post-buy.h',
  'requests/snapd-post-change.h',
  'requests/snapd-post-create-user.h',
  'requests/snapd-post-create-users.h',
  'requests/snapd-post-download.h',
  'requests/snapd-post-interfaces.h',
  'requests/snapd-post-login.h',
  'requests/snapd-post-logout.h',
  'requests/snapd-post-snap.h',
  'requests/snapd-post-snap-stream.h',
  'requests/snapd-post-snap-try.h',
  'requests/snapd-post-snaps.h',
  'requests/snapd-post-snapctl.h',
  'requests/snapd-post-themes.h',
  'requests/snapd-put-snap-conf.h',
  'requests/snapd-request.h',
  'requests/snapd-request-async.h',
]

source_c = [
  'snapd-alias.c',
  'snapd-app.c',
  'snapd-assertion.c',
  'snapd-auth-data.c',
  'snapd-category.c',
  'snapd-category-details.c',
  'snapd-change.c',
  'snapd-channel.c',
  'snapd-client.c',
  'snapd-client-sync.c',
  'snapd-connection.c',
  'snapd-error.c',
  'snapd-icon.c',
  'snapd-interface.c',
  'snapd-login.c',
  'snapd-maintenance.c',
  'snapd-markdown-node.c',
  'snapd-markdown-parser.c',
  'snapd-media.c',
  'snapd-plug.c',
  'snapd-plug-ref.c',
  'snapd-price.c',
  'snapd-screenshot.c',
  'snapd-slot.c',
  'snapd-slot-ref.c',
  'snapd-snap.c',
  'snapd-system-information.c',
  'snapd-task.c',
  'snapd-user-information.c',
]

source_private_c = [
  'requests/snapd-json.c',
  'requests/snapd-get-aliases.c',
  'requests/snapd-get-apps.c',
  'requests/snapd-get-assertions.c',
  'requests/snapd-get-buy-ready.c',
  'requests/snapd-get-categories.c',
  'requests/snapd-get-change.c',
  'requests/snapd-get-changes.c',
  'requests/snapd-get-connections.c',
  'requests/snapd-get-find.c',
  'requests/snapd-get-icon.c',
  'requests/snapd-get-interfaces.c',
  'requests/snapd-get-interfaces-legacy.c',
  'requests/snapd-get-sections.c',
  'requests/snapd-get-snap.c',
  'requests/snapd-get-snap-conf.c',
  'requests/snapd-get-snaps.c',
  'requests/snapd-get-system-info.c',
  'requests/snapd-get-themes.c',
  'requests/snapd-get-users.c',
  'requests/snapd-post-aliases.c',
  'requests/snapd-post-assertions.c',
  'requests/snapd-post-buy.c',
  'requests/snapd-post-change.c',
  'requests/snapd-post-create-user.c',
  'requests/snapd-post-create-users.c',
  'requests/snapd-post-download.c',
  'requests/snapd-post-interfaces.c',
  'requests/snapd-post-login.c',
  'requests/snapd-post-logout.c',
  'requests/snapd-post-snap.c',
  'requests/snapd-post-snap-stream.c',
  'requests/snapd-post-snap-try.c',
  'requests/snapd-post-snaps.c',
  'requests/snapd-post-snapctl.c',
  'requests/snapd-post-themes.c',
  'requests/snapd-put-snap-conf.c',
  'requests/snapd-request.c',
  'requests/snapd-request-async.c',
]

common_cflags = [ '-DSNAPD_COMPILATION=1', '-DVERSION="@0@"'.format (meson.project_version ()), '-DG_LOG_DOMAIN="Snapd"', '-DGETTEXT_PACKAGE="snapd-glib"' ]

gnome = import ('gnome')
snapd_glib_enums = gnome.mkenums ('snapd-enum-types',
                                  sources: source_h,
                                  c_template: 'snapd-enum-types.c.in',
                                  h_template: 'snapd-enum-types.h.in',
                                  install_header: true,
                                  install_dir: install_header_dir)
snapd_glib_lib = library (library_name,
                          source_private_c + source_c + source_private_h + source_h, snapd_glib_enums,
                          version: '1.0.0',
                          include_directories: include_directories ('..'),
                          dependencies: [ glib_dep, gio_dep, gio_unix_dep, libsoup_dep, json_glib_dep ],
                          c_args: common_cflags,
                          link_depends: 'snapd-glib.map',
                          link_args: '-Wl,--version-script,@0@/@1@'.format (meson.current_source_dir(), 'snapd-glib.map'),
                          install: true)

snapd_glib_dep = declare_dependency (link_with: snapd_glib_lib,
                                     sources: snapd_glib_enums[1],
                                     include_directories: include_directories ('..'))

install_headers (source_h + [ 'snapd-glib.h' ],
                 install_dir: install_header_dir)

pc = import ('pkgconfig')
pc.generate (libraries: [ snapd_glib_lib ],
             filebase: library_name,
             name: 'Snapd GLib',
             description: 'Libary for accessing snapd',
             version: meson.project_version (),
             subdirs: library_name)

if get_option ('introspection')
  nsversion = '2'
  if get_option('soup2')
    nsversion = '1'
  endif
  snapd_glib_gir = gnome.generate_gir (snapd_glib_lib,
                                       sources: source_c + source_h + snapd_glib_enums,
                                       namespace: 'Snapd',
                                       nsversion: nsversion,
                                       export_packages: 'snapd-glib',
                                       header: 'snapd-glib/snapd-glib.h',
                                       includes: [ 'GObject-2.0', 'Gio-2.0' ],
                                       install: true,
                                       extra_args: [ '-DSNAPD_COMPILATION=1' ])

  if get_option ('vala-bindings')
    gnome.generate_vapi (library_name,
                         sources: snapd_glib_gir[0],
                         packages: 'gio-2.0',
                         install: true)
  endif
endif
