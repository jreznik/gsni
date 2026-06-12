#!/usr/bin/env python3
"""Python example using libgsni via GObject Introspection.

Registers a tray icon with a context menu. Click 'Exit' to quit.

Run:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
        python3 bindings/python/examples/simple_tray.py
"""

import gi
gi.require_version("Gsni", "1.0")
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")

from gi.repository import Gsni, Gio, GLib


def on_quit(action, param, loop):
    print("Quitting...")
    loop.quit()


def on_say_hello(action, param):
    print("Hello from the system tray!")


def main():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    loop = GLib.MainLoop.new(None, False)

    # Build actions
    actions = Gio.SimpleActionGroup.new()
    act_quit = Gio.SimpleAction.new("quit", None)
    act_quit.connect("activate", on_quit, loop)
    actions.add_action(act_quit)

    act_hello = Gio.SimpleAction.new("hello", None)
    act_hello.connect("activate", on_say_hello)
    actions.add_action(act_hello)

    # Build menu
    menu = Gio.Menu.new()
    menu.append("Say Hello", "hello")
    menu.append("Exit", "quit")

    # Create tray icon
    item = Gsni.Item.new("py-example", conn)
    item.set_title("Python Tray Demo")
    item.set_icon_name("applications-other")
    item.set_status(Gsni.Status.ACTIVE)
    item.set_menu(menu)
    item.set_action_group(actions)
    item.register()

    print("Tray icon registered. Click it for the menu.")
    print("Press Ctrl+C to quit.")
    loop.run()


if __name__ == "__main__":
    main()
