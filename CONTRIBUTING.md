# Contributing

## Building

```sh
meson setup build
ninja -C build
sudo ninja -C build install
```

To build with docs enabled:

```sh
meson setup build -Ddocs=true
ninja -C build
```

## Running tests

```sh
meson test -C build --print-errorlogs
```

## Python tests

```sh
GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
  python3 -m pytest bindings/python/tests/ -v
```

## Code style

- C17 with GLib/GObject conventions
- Function names: `gsni_type_action` (namespace, type, action)
- Use `g_autoptr` and `g_autofree` for automatic cleanup
- Validate all public inputs with `g_return_if_fail`
- Doc comments use `/** ... */` for GObject Introspection

## Submitting changes

1. Fork the repository
2. Create a branch
3. Make your changes
4. Run the tests (`meson test -C build`)
5. Submit a pull request
