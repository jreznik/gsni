# gsni — Python bindings for libgsni

StatusNotifierItem D-Bus system tray icons from Python.

## Install

First install the C library:

```sh
# Fedora
sudo dnf install libgsni1 gir1.2-Gsni-1.0

# Or build from source
git clone https://github.com/jreznik/gsni
cd gsni && meson setup build && sudo ninja -C build install
```

Then install the Python package:

```sh
pip install gsni
```

## Usage

```python
import gi
gi.require_version("Gsni", "1.0")
from gi.repository import Gsni, Gio

conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
item = Gsni.Item.new("my-tray-icon", conn)
item.set_title("My App")
item.set_icon_name("applications-other")
item.set_status(Gsni.Status.ACTIVE)
item.register()
```

Or use the convenience wrapper:

```python
from gsni import StatusNotifierItem

with StatusNotifierItem("my-tray", icon_name="applications-other",
                        title="My App") as item:
    input("Press Enter to exit...")
```

## License

LGPL-2.1-or-later
