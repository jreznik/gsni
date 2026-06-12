# gsni — Python bindings for libgsni

StatusNotifierItem D-Bus system tray icons from Python.

## Install

First install the C library:
```sh
sudo dnf install libgsni1 gir1.2-Gsni-1.0
```
Or build from source: `git clone https://github.com/jreznik/gsni && cd gsni && meson setup build && sudo ninja -C build install`

Then:
```sh
pip install gsni
```

## Usage

```python
from gsni import StatusNotifierItem

with StatusNotifierItem("my-tray", icon_name="applications-other",
                        title="My App") as item:
    input("Press Enter to exit...")
```

## License

LGPL-2.1-or-later
