#!/usr/bin/env python3
"""1:1 Python port of examples/menu-item.c.

Registers a tray icon with a context menu and activation/scroll callbacks.

Run:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
        python3 bindings/python/examples/menu_item.py
"""

import gi
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")
from gi.repository import Gio, GLib

from gsni import StatusNotifierItem


def on_activate(item, x, y):
    print(f"Activated at ({x}, {y})")


def on_scroll(item, delta, orientation):
    dirs = {0: "up", 1: "down", 2: "left", 3: "right"}
    print(f"Scrolled {delta} {dirs.get(int(orientation), '?')}")


def on_open(action, param):
    print("Menu: Open")


def on_quit(action, param, loop):
    print("Menu: Quit")
    loop.quit()


def main():
    loop = GLib.MainLoop.new(None, False)

    actions = Gio.SimpleActionGroup.new()
    act_open = Gio.SimpleAction.new("open", None)
    act_open.connect("activate", on_open)
    actions.add_action(act_open)
    act_quit = Gio.SimpleAction.new("quit", None)
    act_quit.connect("activate", on_quit, loop)
    actions.add_action(act_quit)

    menu = Gio.Menu.new()
    menu.append("Open", "open")
    menu.append("Quit", "quit")

    item = StatusNotifierItem("simple-tray",
                              icon_name="applications-other",
                              title="Simple Tray Demo")
    item.status = 1  # ACTIVE
    item.menu = menu
    item.action_group = actions
    item.set_tooltip(None, "Simple Tray Demo", "Click for menu")
    item.on_activate(on_activate)
    item.on_scroll(on_scroll)
    item.register()

    print("Tray icon with menu registered.")
    print("Click the icon to see the context menu.")
    print("Scroll to test scroll events.")
    print("Press Ctrl+C to exit.")
    loop.run()


if __name__ == "__main__":
    main()
