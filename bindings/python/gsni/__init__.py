"""Pythonic wrapper for libgsni StatusNotifierItem bindings.

Exposes the raw GObject Introspection types through a slightly
more idiomatic Python API while staying thin.
"""

import gi
gi.require_version("Gsni", "1.0")
gi.require_version("Gio", "2.0")

from gi.repository import Gsni, Gio  # noqa: F401, E402


class StatusNotifierItem:
    """A system tray icon with optional context menu."""

    def __init__(self, id, icon_name=None, title=None, category=None,
                 connection=None):
        if connection is None:
            connection = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        self._item = Gsni.Item.new(id, connection)
        self._item.set_item_is_menu(True)

        if title:
            self._item.set_title(title)
        if icon_name:
            self._item.set_icon_name(icon_name)
        if category:
            self._item.set_category(category)

    @property
    def id(self):
        return self._item.get_id()

    @property
    def title(self):
        return self._item.get_title()

    @title.setter
    def title(self, value):
        self._item.set_title(value)

    @property
    def icon_name(self):
        return self._item.get_icon_name()

    @icon_name.setter
    def icon_name(self, value):
        self._item.set_icon_name(value)

    @property
    def status(self):
        return self._item.get_status()

    @status.setter
    def status(self, value):
        self._item.set_status(value)

    @property
    def menu(self):
        return self._item.get_menu()

    @menu.setter
    def menu(self, model):
        self._item.set_menu(model)

    @property
    def action_group(self):
        return self._item.get_action_group()

    @action_group.setter
    def action_group(self, actions):
        self._item.set_action_group(actions)

    def set_tooltip(self, icon_name, title, subtitle):
        self._item.set_tooltip(icon_name or "", title or "", subtitle or "")

    def set_attention_icon(self, name):
        self._item.set_attention_icon_name(name)
        self._item.set_status(Gsni.Status.NEEDSATTENTION)

    def clear_attention(self):
        self._item.set_status(Gsni.Status.ACTIVE)

    def register(self):
        return self._item.register()

    def unregister(self):
        self._item.unregister()

    def show_notification(self, title, body, icon_name=None, timeout=10000):
        self._item.show_notification(title, body, icon_name or "", timeout)

    def connect(self, signal, callback):
        return self._item.connect(signal, callback)

    def __enter__(self):
        self.register()
        return self

    def __exit__(self, *args):
        self.unregister()
