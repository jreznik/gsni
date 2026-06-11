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
