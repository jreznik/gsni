# Python Bindings

libgsni provides Python bindings through GObject Introspection.  Install
the C library first, then `pip install gsni` for the convenience wrapper.

## Install

```sh
# Fedora
sudo dnf install libgsni1 gir1.2-Gsni-1.0

# Or build from source
git clone https://github.com/jreznik/gsni
cd gsni && meson setup build && sudo ninja -C build install

# Then the Python wrapper
pip install gsni
```

Or install everything from source in one step:

```sh
pip install gsni
```

This uses meson-python to compile the C library, generate the typelib,
and install the Python wrapper.

## Raw GI bindings

The GObject Introspection bindings give direct access to the C API:

```python
import gi
gi.require_version("Gsni", "1.0")
from gi.repository import Gsni, Gio

conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
item = Gsni.Item.new("my-tray", conn)
item.set_title("My App")
item.set_icon_name("applications-other")
item.set_status(Gsni.Status.ACTIVE)
item.register()
```

## Pythonic wrapper

The `gsni` package provides a more idiomatic API:

```python
from gsni import StatusNotifierItem

with StatusNotifierItem("my-tray", icon_name="applications-other",
                        title="My App") as item:
    input("Press Enter to exit...")
```

The context manager calls `register()` on enter and `unregister()` on exit.

## Adding a context menu

```python
from gi.repository import Gio

actions = Gio.SimpleActionGroup.new()
act = Gio.SimpleAction.new("say-hello", None)
act.connect("activate", lambda a, p: print("Hello!"))
actions.add_action(act)

menu = Gio.Menu.new()
menu.append("Say Hello", "say-hello")

item.set_menu(menu)
item.set_action_group(actions)
```

The menu uses DBusMenu — it works in KDE Plasma, GNOME, XFCE, MATE,
and wlroots-based bars.

## Flatpak

When running inside Flatpak, request D-Bus access in your manifest:

```json
{
    "finish-args": [
        "--socket=session-bus",
        "--talk-name=org.kde.StatusNotifierWatcher"
    ]
}
```

## Full example

See `bindings/python/examples/simple_tray.py` for a complete working
example with a tray icon and context menu.
