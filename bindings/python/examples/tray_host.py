#!/usr/bin/env python3
"""1:1 Python port of examples/tray-host.c.

GTK4 system tray host — shows all registered SNI items, left-click
activates, right-click shows the DBusMenu context menu.

Run:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
        python3 bindings/python/examples/tray_host.py
"""

import gi
gi.require_version("Gtk", "4.0")
gi.require_version("Gdk", "4.0")
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")
gi.require_version("Gsni", "1.0")
from gi.repository import Gtk, Gdk, Gio, GLib, Gsni


# ── DBusMenu helpers ────────────────────────────────────────

def fetch_menu_layout(bus_name, menu_path):
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    if not conn:
        return None

    props = ["type", "label", "enabled", "children-display",
             "toggle-type", "toggle-state"]
    filter_builder = GLib.VariantBuilder.new(
        GLib.VariantType.new("as"))
    for p in props:
        filter_builder.add_value(GLib.Variant.new_string(p))

    try:
        result = conn.call_sync(
            bus_name, menu_path,
            "com.canonical.dbusmenu", "GetLayout",
            GLib.Variant("(iias)", (0, -1, filter_builder.end())),
            None, Gio.DBusCallFlags.NONE, -1, None)
        return result
    except GLib.Error:
        return None


def trigger_menu_event(bus_name, menu_path, item_id):
    if not bus_name or not menu_path:
        return
    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    if not conn:
        return
    try:
        conn.call_sync(
            bus_name, menu_path,
            "com.canonical.dbusmenu", "Event",
            GLib.Variant("(isvu)", (item_id, "clicked",
                                     GLib.Variant("s", ""), 0)),
            None, Gio.DBusCallFlags.NONE, -1, None)
    except GLib.Error:
        pass


# ── Popover builder ─────────────────────────────────────────

def populate_popover(box, layout_node, bus_name, menu_path, popover,
                     is_root):
    item_id = layout_node.get_child_value(0).get_int32()
    props = layout_node.get_child_value(1)
    children = layout_node.get_child_value(2)

    type_str = "standard"
    label_str = None
    enabled = True

    type_val = props.lookup_value("type", None)
    if type_val:
        type_str = type_val.get_string()
    label_val = props.lookup_value("label", None)
    if label_val:
        label_str = label_val.get_string()
    enabled_val = props.lookup_value("enabled", None)
    if enabled_val:
        enabled = enabled_val.get_boolean()

    n_children = children.n_children()

    if is_root:
        for i in range(n_children):
            child = children.get_child_value(i)
            unwrapped = child.get_variant()
            populate_popover(box, unwrapped, bus_name, menu_path,
                             popover, False)
        return

    if type_str == "separator":
        sep = Gtk.Separator.new(Gtk.Orientation.HORIZONTAL)
        box.append(sep)
        return

    if n_children > 0:
        sub_label = Gtk.Label.new(label_str or "")
        sub_label.set_halign(Gtk.Align.START)
        sub_label.set_margin_start(8)
        box.append(sub_label)

        for i in range(n_children):
            child = children.get_child_value(i)
            unwrapped = child.get_variant()
            populate_popover(box, unwrapped, bus_name, menu_path,
                             popover, False)

        sep2 = Gtk.Separator.new(Gtk.Orientation.HORIZONTAL)
        box.append(sep2)
    else:
        btn = Gtk.Button.new_with_label(label_str or "")
        btn.set_sensitive(enabled)
        btn.set_halign(Gtk.Align.FILL)
        btn.set_has_frame(False)

        click = Gtk.GestureClick.new()
        click.connect("pressed", _on_menu_item_clicked,
                      bus_name, menu_path, item_id, popover)
        btn.add_controller(click)
        box.append(btn)


def _on_menu_item_clicked(gesture, n_press, x, y,
                          bus_name, menu_path, item_id, popover):
    trigger_menu_event(bus_name, menu_path, item_id)
    popover.popdown()


def show_context_menu(host_item, row, x, y):
    service = host_item.get_service()
    if not service:
        return
    bus_name = service.split("/")[0]
    menu_path = host_item.get_menu_path()
    if not menu_path:
        return

    reply = fetch_menu_layout(bus_name, menu_path)
    if not reply:
        return

    layout = reply.get_child_value(1)

    popover = Gtk.Popover.new()
    rect = Gdk.Rectangle()
    rect.x = int(x); rect.y = int(y); rect.width = 1; rect.height = 1
    popover.set_pointing_to(rect)
    popover.set_has_arrow(False)

    box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 0)
    popover.set_child(box)

    populate_popover(box, layout, bus_name, menu_path, popover, True)

    popover.set_parent(row)
    popover.popup()


def activate_item(host_item, x, y):
    service = host_item.get_service()
    if not service:
        return
    bus_name = service.split("/")[0]

    conn = Gio.bus_get_sync(Gio.BusType.SESSION, None)
    if not conn:
        return
    try:
        conn.call_sync(
            bus_name, "/StatusNotifierItem",
            "org.kde.StatusNotifierItem", "Activate",
            GLib.Variant("(ii)", (int(x), int(y))),
            None, Gio.DBusCallFlags.NONE, -1, None)
    except GLib.Error:
        pass


# ── Item lifecycle ──────────────────────────────────────────

class TrayHostApp:
    def __init__(self):
        self.item_widgets = {}

    def on_activate(self, gapp):
        app = gapp
        window = Gtk.ApplicationWindow.new(app)
        window.set_title("libgsni — System Tray Host")
        window.set_default_size(400, 300)

        scrolled = Gtk.ScrolledWindow.new()
        window.set_child(scrolled)

        self.tray_box = Gtk.Box.new(Gtk.Orientation.VERTICAL, 2)
        self.tray_box.set_spacing(4)
        self.tray_box.set_margin_start(8)
        self.tray_box.set_margin_top(8)
        scrolled.set_child(self.tray_box)

        self.host = Gsni.Host.new(
            Gio.bus_get_sync(Gio.BusType.SESSION, None))
        self.host.connect("item-added", self._on_item_added)
        self.host.connect("item-removed", self._on_item_removed)

        try:
            self.host.register()
        except GLib.Error as e:
            msg = Gtk.Label.new(
                "No StatusNotifierWatcher available.\n"
                "Run this on a session with system tray support.")
            self.tray_box.append(msg)
            print(f"Host registration failed: {e.message}")

        for _ in range(50):
            GLib.main_context_default().iteration(False)
        self._load_existing()
        window.present()

    def _on_item_added(self, host, item):
        print(f"Item added: {item.get_id()}")
        self._add_tray_icon(item)

    def _on_item_removed(self, host, item):
        print(f"Item removed: {item.get_id()}")
        self._remove_tray_icon(item)

    def _load_existing(self):
        model = self.host.get_items()
        for i in range(model.get_n_items()):
            self._add_tray_icon(model.get_item(i))

    def _add_tray_icon(self, item):
        row = Gtk.Box.new(Gtk.Orientation.HORIZONTAL, 6)

        image = Gtk.Image.new()
        icon_name = item.get_icon_name()
        if icon_name:
            image.set_from_icon_name(icon_name)
        else:
            image.set_from_icon_name("image-missing")
        image.set_pixel_size(24)
        row.append(image)

        title = item.get_title() or "(untitled)"
        item_id = item.get_id() or ""
        label = Gtk.Label.new(f"<b>{title}</b>\n<small>{item_id}</small>")
        label.set_use_markup(True)
        label.set_xalign(0.0)
        row.append(label)

        status = item.get_status()
        sl = Gtk.Label.new("")
        if status == Gsni.Status.ACTIVE:
            sl.set_text("Active")
        elif status == Gsni.Status.NEEDSATTENTION:
            sl.set_text("Attention")
        else:
            sl.set_text("Passive")
        sl.set_margin_start(8)
        row.append(sl)

        click = Gtk.GestureClick.new()
        click.set_button(0)
        click.connect("pressed", self._on_icon_pressed, item)
        row.add_controller(click)

        self.tray_box.append(row)
        self.item_widgets[item] = row

    def _remove_tray_icon(self, item):
        row = self.item_widgets.pop(item, None)
        if row:
            self.tray_box.remove(row)

    def _on_icon_pressed(self, gesture, n_press, x, y, item):
        button = gesture.get_current_button()
        row = gesture.get_widget()
        if button == Gdk.BUTTON_SECONDARY:
            show_context_menu(item, row, x, y)
        else:
            activate_item(item, x, y)


def main():
    app = Gtk.Application.new("org.libgsni.TrayHost",
                              Gio.ApplicationFlags.DEFAULT_FLAGS)
    host_app = TrayHostApp()
    app.connect("activate", host_app.on_activate)
    app.run()


if __name__ == "__main__":
    main()
