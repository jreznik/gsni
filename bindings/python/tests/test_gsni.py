"""Basic smoke tests for the Python bindings.

Run with:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src python3 -m pytest
"""

import gi
gi.require_version("Gsni", "1.0")
gi.require_version("Gio", "2.0")

from gi.repository import Gsni, Gio


def test_gi_import():
    assert Gsni.Item is not None
    assert Gsni.Host is not None
    assert Gsni.HostItem is not None


def test_enums():
    assert Gsni.Status.PASSIVE is not None
    assert Gsni.Status.ACTIVE is not None
    # GI strips underscores: NEEDS_ATTENTION → NEEDSATTENTION
    assert int(Gsni.Status.NEEDSATTENTION) == 2
    assert int(Gsni.Category.APPLICATIONSTATUS) == 0
    assert int(Gsni.Category.HARDWARE) == 3


def test_item_create():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-item", conn)
    assert item is not None
    assert item.get_id() == "test-item"
    assert item.get_item_is_menu() is True
    assert item.get_title() is None or isinstance(item.get_title(), str)


def test_item_properties():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-props", conn)
    item.set_title("Test Title")
    assert item.get_title() == "Test Title"
    item.set_icon_name("test-icon")
    assert item.get_icon_name() == "test-icon"
    item.set_status(Gsni.Status.ACTIVE)
    assert item.get_status() == Gsni.Status.ACTIVE


def test_item_register_unregister():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-reg", conn)
    item.set_title("Test")
    item.register()
    assert item.get_bus_name() is not None
    assert item.get_bus_name().startswith("org.kde.")
    item.unregister()
