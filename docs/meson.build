gidocgen = find_program('gi-docgen', required: get_option('docs'))

if not gidocgen.found()
  subdir_done()
endif

gsni_toml = configure_file(
  input: 'gsni.toml.in',
  output: 'gsni.toml',
  configuration: gsni_config,
)

content_files = files(
  'getting-started.md',
)

gir_target = []
if get_option('introspection')
  gir_target = gsni_gir
endif

if gir_target.length() > 0
  custom_target('gsni-docs',
    input: [gsni_toml, gir_target[0]],
    output: 'gsni-1.0',
    command: [
      gidocgen, 'generate', '--quiet',
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    depends: gir_target,
    build_by_default: true,
  )
else
  message('GObject Introspection not built — docs will not include API reference.')
endif
