# Documentation

The `docs` directory contains the gi-docgen configuration for building
the API reference and user manual.

## Building the docs

Install prerequisites:

```sh
sudo dnf install gobject-introspection-devel
pip3 install gi-docgen
```

Build with docs enabled:

```sh
meson setup build -Ddocs=enabled
ninja -C build
```

The generated HTML documentation will be in `build/docs/gsni-1.0/`.

## Without GObject Introspection

If `gobject-introspection-devel` is not available, gi-docgen can still
generate documentation from source annotations, but the API reference
will be limited.  Install the full stack for best results.
