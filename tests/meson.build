if not get_option('tests')
  subdir_done()
endif

gtest_dep = dependency('gio-2.0')

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('MALLOC_CHECK_', '2')
test_env.set('G_DEBUG', 'fatal-warnings')

test_deps = [deps, declare_dependency(link_with: libgsni,
               include_directories: [include_directories('..'), gsni_build_inc])]

test_item = executable('test-item', 'test-item.c',
  dependencies: test_deps,
)
test('item', test_item, env: test_env)

test_pixbuf = executable('test-pixbuf', 'test-pixbuf.c',
  dependencies: test_deps,
)
test('pixbuf', test_pixbuf, env: test_env)

test_watcher = executable('test-watcher', 'test-watcher.c',
  dependencies: test_deps,
)
test('watcher', test_watcher, env: test_env)

test_conformance = executable('test-conformance', 'test-conformance.c',
  dependencies: test_deps,
)
test('conformance', test_conformance, env: test_env, timeout: 30)
