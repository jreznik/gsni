if not get_option('examples')
  subdir_done()
endif

example_deps = [deps, declare_dependency(link_with: libgsni,
                  include_directories: [include_directories('..'), gsni_build_inc])]

example_simple = executable('simple-item', 'simple-item.c',
  dependencies: example_deps,
)

example_menu = executable('menu-item', 'menu-item.c',
  dependencies: example_deps,
)

example_music = executable('music-controller', 'music-controller.c',
  dependencies: example_deps,
)

gtk4_dep = dependency('gtk4', required: false)
if gtk4_dep.found()
  tray_host_deps = example_deps + [gtk4_dep]
  example_tray = executable('tray-host', 'tray-host.c',
    dependencies: tray_host_deps,
  )
endif
