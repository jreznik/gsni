#!/usr/bin/env python3
"""1:1 Python port of examples/custom-icon.c.

Creates a tray icon with a programmatically generated pixmap icon.

Run:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
        python3 bindings/python/examples/custom_icon.py
"""

import gi
gi.require_version("GdkPixbuf", "2.0")
gi.require_version("GLib", "2.0")
from gi.repository import GdkPixbuf, GLib

from gsni import StatusNotifierItem


def generate_tray_icon():
    size = 48
    half = size // 2
    stride = size * 4
    data = bytearray(stride * size)

    for y in range(size):
        for x in range(size):
            inside = (x - half) ** 2 + (y - half) ** 2 < half * half
            off = y * stride + x * 4
            if inside:
                data[off] = (0x50 + x * 180 // size) & 0xff
                data[off + 1] = (0x80 + y * 100 // size) & 0xff
                data[off + 2] = 0xcc
                data[off + 3] = 0xff
            else:
                data[off] = data[off + 1] = data[off + 2] = data[off + 3] = 0

    gb = GLib.Bytes.new(data)
    return GdkPixbuf.Pixbuf.new_from_bytes(
        gb, GdkPixbuf.Colorspace.RGB, True, 8, size, size, stride)


def main():
    icon = generate_tray_icon()
    item = StatusNotifierItem("custom-icon", title="Custom Icon Demo")
    item.status = 1  # ACTIVE
    item.icon_pixbuf = icon
    item.set_tooltip(None, "Custom Icon",
                     "Icon generated from raw pixel data")
    item.register()

    print("Tray icon registered with custom pixmap.")
    print("The icon is a programmatically generated colored circle.")
    print("Press Ctrl+C to exit.")

    loop = GLib.MainLoop.new(None, False)
    loop.run()


if __name__ == "__main__":
    main()
