"""Tests for the Python bindings — both raw GI and Pythonic wrapper.

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
    assert int(Gsni.Status.NEEDSATTENTION) == 2
    assert int(Gsni.Category.APPLICATIONSTATUS) == 0
    assert int(Gsni.Category.HARDWARE) == 3


def test_item_create():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-item", conn)
    assert item is not None
    assert item.get_id() == "test-item"
    assert item.get_item_is_menu() is True


def test_item_properties():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-props", conn)
    item.set_title("Test Title")
    assert item.get_title() == "Test Title"
    item.set_icon_name("test-icon")
    assert item.get_icon_name() == "test-icon"
    item.set_status(Gsni.Status.ACTIVE)
    assert item.get_status() == Gsni.Status.ACTIVE
    item.set_category(Gsni.Category.HARDWARE)
    assert item.get_category() == Gsni.Category.HARDWARE
    item.set_overlay_icon_name("overlay")
    assert item.get_overlay_icon_name() == "overlay"
    item.set_attention_icon_name("attention")
    assert item.get_attention_icon_name() == "attention"
    item.set_item_is_menu(False)
    assert item.get_item_is_menu() is False
    item.set_window_id(42)
    assert item.get_window_id() == 42


def test_item_tooltip():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-tip", conn)
    # Set tooltip — just verify no crash. get_tooltip returns internal
    # pointer (transfer none), GI may double-free if wrapped.
    item.set_tooltip("tip-icon", "Title", "Sub")


def test_item_register_unregister():
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    item = Gsni.Item.new("test-reg", conn)
    item.set_title("Test")
    item.register()
    assert item.get_bus_name() is not None
    assert item.get_bus_name().startswith("org.kde.")
    item.unregister()


# ── Pythonic wrapper tests ──────────────────────────────────

from gsni import StatusNotifierItem


def test_wrapper_create():
    item = StatusNotifierItem("py-test", icon_name="python", title="Python")
    assert item.id == "py-test"
    assert item.title == "Python"
    assert item.icon_name == "python"
    assert item.item_is_menu is True


def test_wrapper_property_setters():
    item = StatusNotifierItem("py-test")
    item.title = "New Title"
    assert item.title == "New Title"
    item.icon_name = "new-icon"
    assert item.icon_name == "new-icon"
    item.status = Gsni.Status.ACTIVE
    assert item.status == Gsni.Status.ACTIVE
    item.category = Gsni.Category.HARDWARE
    assert item.category == Gsni.Category.HARDWARE
    item.overlay_icon_name = "overlay"
    assert item.overlay_icon_name == "overlay"
    item.attention_icon_name = "attention"
    assert item.attention_icon_name == "attention"
    item.item_is_menu = False
    assert item.item_is_menu is False
    item.window_id = 7
    assert item.window_id == 7


def test_wrapper_register_unregister():
    item = StatusNotifierItem("py-reg-test")
    item.register()
    assert item.bus_name.startswith("org.kde.")
    item.unregister()


def test_wrapper_context_manager():
    with StatusNotifierItem("py-ctx-test") as item:
        assert item.bus_name.startswith("org.kde.")


def test_wrapper_tooltip():
    item = StatusNotifierItem("py-tip")
    item.set_tooltip("icon", "Title", "Sub")
    # just check no exception raised


def test_wrapper_attention():
    item = StatusNotifierItem("py-attn")
    item.set_attention_icon("dialog-warning")
    assert item.status == Gsni.Status.NEEDSATTENTION
    item.clear_attention()
    assert item.status == Gsni.Status.ACTIVE
