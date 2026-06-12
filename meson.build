project('gsni', 'c',
  version: '1.0.1',
  license: 'LGPL-2.1-or-later',
  meson_version: '>= 1.0',
  default_options: [
    'warning_level=2',
    'c_std=c17',
    'default_library=shared',
  ]
)

version_array = meson.project_version().split('.')
api_version = version_array[0]
gi_version = '@0@.0'.format(version_array[0])
so_version = '1.0.1'

gsni_build_inc = include_directories('.')

cc = meson.get_compiler('c')

add_project_arguments(
  cc.get_supported_arguments([
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-cast-function-type',
    '-fstack-protector-strong',
  ]),
  language: 'c',
)

if get_option('optimization') != '0'
  add_project_arguments('-D_FORTIFY_SOURCE=2', language: 'c')
endif

add_project_link_arguments(
  cc.get_supported_link_arguments([
    '-Wl,-z,relro',
    '-Wl,-z,now',
  ]),
  language: 'c',
)

deps = [
  dependency('glib-2.0', version: '>= 2.80'),
  dependency('gio-2.0'),
  dependency('gobject-2.0'),
  dependency('gdk-pixbuf-2.0'),
]

gsni_config = configuration_data()
gsni_config.set_quoted('GSNI_VERSION', meson.project_version())
gsni_config.set('GSNI_MAJOR_VERSION', version_array[0])
gsni_config.set('GSNI_MINOR_VERSION', version_array[1])
gsni_config.set('GSNI_MICRO_VERSION', version_array[2])

configure_file(
  input: 'gsni-version.h.in',
  output: 'gsni-version.h',
  configuration: gsni_config,
  install_dir: get_option('includedir') / 'gsni',
)

subdir('src')
subdir('tests')
subdir('examples')
subdir('docs')
subdir('bindings/python')
