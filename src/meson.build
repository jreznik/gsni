gsni_sources = files(
  'gsni-enums.c',
  'gsni-types.c',
  'gsni-pixbuf.c',
  'gsni-item.c',
  'gsni-item-dbus.c',
  'gsni-watcher.c',
  'gsni-dbusmenu.c',
  'gsni-host.c',
  'gsni-host-item.c',
)

gsni_public_headers = files(
  '../gsni/gsni.h',
  '../gsni/gsni-item.h',
  '../gsni/gsni-host.h',
  '../gsni/gsni-host-item.h',
  '../gsni/gsni-enums.h',
  '../gsni/gsni-types.h',
  '../gsni/gsni-pixbuf.h',
)

gsni_src_inc = include_directories('..')

libgsni = shared_library('gsni',
  gsni_sources,
  dependencies: deps,
  include_directories: [gsni_src_inc, gsni_build_inc],
  install: true,
  version: so_version,
)

install_headers(gsni_public_headers, subdir: 'gsni')

# pkg-config
pkg = import('pkgconfig')
pkg.generate(libgsni,
  name: 'gsni',
  description: 'StatusNotifierItem D-Bus protocol library for GTK4',
  subdirs: 'gsni',
  requires: ['glib-2.0', 'gio-2.0', 'gobject-2.0', 'gdk-pixbuf-2.0'],
)

# Build internal tests as a static helper
if get_option('tests')
  libgsni_dep = declare_dependency(
    link_with: libgsni,
    include_directories: [gsni_src_inc, gsni_build_inc],
    dependencies: deps,
  )
endif

# GObject Introspection
if get_option('introspection')
  gnome = import('gnome')
  gsni_gir = gnome.generate_gir(libgsni,
    sources: gsni_sources + gsni_public_headers,
    namespace: 'Gsni',
    nsversion: gi_version,
    identifier_prefix: 'Gsni',
    symbol_prefix: 'gsni',
    includes: ['GObject-2.0', 'Gio-2.0', 'GdkPixbuf-2.0'],
    header: 'gsni/gsni.h',
    install: true,
    install_dir_gir: get_option('datadir') / 'gir-1.0',
    install_dir_typelib: get_option('libdir') / 'girepository-1.0',
  )
endif
