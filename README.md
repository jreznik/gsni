# libgsni

**libgsni** is a C library for adding system tray icons to GTK4 applications
using the StatusNotifierItem (SNI) D-Bus protocol. It also provides a host
side for building system trays that consume SNI items.

If you have a GTK4 application and want an icon in the system tray — an icon
the user can click, scroll, or right-click for a context menu — libgsni is
what you need.

## Quick Example

```c
#include <gsni/gsni.h>

int main(int argc, char *argv[])
{
    GApplication *app = g_application_new("org.example.MyApp",
                                           G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(app, NULL, NULL);

    GDBusConnection *bus = g_application_get_dbus_connection(app);

    GsniItem *item = gsni_item_new("my-tray-icon", bus, NULL);
    gsni_item_set_title(item, "My Application");
    gsni_item_set_icon_name(item, "my-app-icon");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    gsni_item_register(item, NULL);

    g_application_run(app, argc, argv);

    g_object_unref(item);
    g_object_unref(app);
    return 0;
}
```

## Features

* Full implementation of the StatusNotifierItem D-Bus specification
* Dual menu export: `com.canonical.dbusmenu` and `org.gtk.Menus`
* Works with KDE Plasma, GNOME, XFCE, MATE, Budgie, and wlroots-based trays
* Pixbuf-based icon support with automatic multi-size export
* Tooltip support with icon, title, and subtitle
* Mouse wheel scroll events forwarded to your application
* System tray host side for writing custom panels
* Automatic watcher reconnection

## Dependencies

| Dependency    | Minimum Version |
|---------------|-----------------|
| GLib          | 2.80            |
| GIO           | 2.80            |
| GObject       | 2.80            |
| GdkPixbuf     | 2.42            |

## Building

```sh
meson setup build
ninja -C build
sudo ninja -C build install
```

## Documentation

API reference and examples at: https://github.com/jreznik/gsni

## License

LGPL-2.1-or-later
Copyright (C) 2024 Jaroslav Reznik &lt;jreznik@redhat.com&gt;
