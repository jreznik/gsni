"""Python bindings for libgsni — StatusNotifierItem tray icons.

Thin wrapper over GObject Introspection. All GI types are available
directly via ``from gi.repository import Gsni``. This module adds
Pythonic property access and context manager support.

Usage:
    from gsni import StatusNotifierItem
    with StatusNotifierItem("my-id", icon_name="my-icon",
                            title="My App") as item:
        input("Press Enter to exit...")
"""

import gi
gi.require_version("Gsni", "1.0")
gi.require_version("Gio", "2.0")

from gi.repository import Gsni, Gio  # noqa: F401, E402


class StatusNotifierItem:
    """A system tray icon with menus, tooltips, and scroll events.

    Wraps :class:`Gsni.Item` with Pythonic property access and a
    context manager that calls ``register()`` on enter and
    ``unregister()`` on exit.
    """

    def __init__(self, id, icon_name=None, title=None, category=None,
                 connection=None, item_is_menu=True):
        if connection is None:
            connection = Gio.bus_get_sync(Gio.BusType.SESSION, None)
        self._item = Gsni.Item.new(id, connection)
        self._item.set_item_is_menu(item_is_menu)

        if title:
            self._item.set_title(title)
        if icon_name:
            self._item.set_icon_name(icon_name)
        if category is not None:
            self._item.set_category(category)

    # ── identity ────────────────────────────────────────────

    @property
    def id(self):
        return self._item.get_id()

    @property
    def bus_name(self):
        return self._item.get_bus_name()

    @property
    def connected(self):
        return self._item.get_connected()

    # ── title ───────────────────────────────────────────────

    @property
    def title(self):
        return self._item.get_title()

    @title.setter
    def title(self, value):
        self._item.set_title(value)

    # ── status ──────────────────────────────────────────────

    @property
    def status(self):
        return self._item.get_status()

    @status.setter
    def status(self, value):
        self._item.set_status(value)

    def set_attention_icon(self, name):
        self._item.set_attention_icon_name(name)
        self._item.set_status(Gsni.Status.NEEDSATTENTION)

    def clear_attention(self):
        self._item.set_status(Gsni.Status.ACTIVE)

    # ── category ────────────────────────────────────────────

    @property
    def category(self):
        return self._item.get_category()

    @category.setter
    def category(self, value):
        self._item.set_category(value)

    # ── icons ───────────────────────────────────────────────

    @property
    def icon_name(self):
        return self._item.get_icon_name()

    @icon_name.setter
    def icon_name(self, value):
        self._item.set_icon_name(value)

    @property
    def icon_pixbuf(self):
        return self._item.get_icon_pixbuf()

    @icon_pixbuf.setter
    def icon_pixbuf(self, value):
        self._item.set_icon_pixbuf(value)

    @property
    def overlay_icon_name(self):
        return self._item.get_overlay_icon_name()

    @overlay_icon_name.setter
    def overlay_icon_name(self, value):
        self._item.set_overlay_icon_name(value)

    @property
    def attention_icon_name(self):
        return self._item.get_attention_icon_name()

    @attention_icon_name.setter
    def attention_icon_name(self, value):
        self._item.set_attention_icon_name(value)

    @property
    def attention_movie_name(self):
        return self._item.get_attention_movie_name()

    @attention_movie_name.setter
    def attention_movie_name(self, value):
        self._item.set_attention_movie_name(value)

    @property
    def icon_theme_path(self):
        return self._item.get_icon_theme_path()

    @icon_theme_path.setter
    def icon_theme_path(self, value):
        self._item.set_icon_theme_path(value)

    # ── tooltip ─────────────────────────────────────────────

    def set_tooltip(self, icon_name, title, subtitle):
        self._item.set_tooltip(icon_name or "", title or "", subtitle or "")

    # ── menu + actions ──────────────────────────────────────

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

    @property
    def item_is_menu(self):
        return self._item.get_item_is_menu()

    @item_is_menu.setter
    def item_is_menu(self, value):
        self._item.set_item_is_menu(value)

    def set_menu(self, menu_model, action_group=None):
        """Set both the menu model and action group at once."""
        self._item.set_menu(menu_model)
        if action_group:
            self._item.set_action_group(action_group)

    # ── window ──────────────────────────────────────────────

    @property
    def window_id(self):
        return self._item.get_window_id()

    @window_id.setter
    def window_id(self, value):
        self._item.set_window_id(value)

    # ── activation token ────────────────────────────────────

    @property
    def activation_token(self):
        return self._item.get_activation_token()

    # ── lifecycle ───────────────────────────────────────────

    def register(self):
        return self._item.register()

    def unregister(self):
        self._item.unregister()

    def show_notification(self, title, body, icon_name=None, timeout=10000):
        self._item.show_notification(title, body, icon_name or "", timeout)

    # ── signals ─────────────────────────────────────────────

    def connect(self, signal, callback):
        """Connect to a GObject signal (e.g. 'activate', 'scroll')."""
        return self._item.connect(signal, callback)

    def on_activate(self, callback):
        """Shorthand for ``connect('activate', callback)``."""
        return self._item.connect("activate", callback)

    def on_scroll(self, callback):
        """Shorthand for ``connect('scroll', callback)``."""
        return self._item.connect("scroll", callback)

    # ── context manager ─────────────────────────────────────

    def __enter__(self):
        self.register()
        return self

    def __exit__(self, *args):
        self.unregister()
