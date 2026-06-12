# Getting Started with libgsni

libgsni gives your GTK4 application a system tray icon using the
StatusNotifierItem (SNI) D-Bus protocol.  The icon appears in KDE Plasma,
GNOME (with the AppIndicator extension), XFCE, MATE, Budgie, and
wlroots-based bars.

## Your first tray icon

```c
#include <gio/gio.h>
#include <gsni/gsni.h>

int main(int argc, char *argv[])
{
    GApplication *app = g_application_new("org.example.MyApp",
                                           G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(app, NULL, NULL);

    GDBusConnection *bus = g_application_get_dbus_connection(app);

    /* Create a tray item */
    GsniItem *item = gsni_item_new("my-tray-icon", bus, NULL);
    gsni_item_set_title(item, "My Application");
    gsni_item_set_icon_name(item, "applications-other");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    gsni_item_register(item, NULL);

    g_application_run(app, argc, argv);

    g_object_unref(item);
    g_object_unref(app);
    return 0;
}
```

Build and run:

```sh
gcc -o myapp myapp.c $(pkg-config --cflags --libs gsni gio-2.0)
./myapp
```

Your application icon appears in the system tray.

## Adding a context menu

Set a `GMenu` and `GActionGroup` on the item:

```c
GSimpleActionGroup *actions = g_simple_action_group_new();

GSimpleAction *open = g_simple_action_new("open", NULL);
g_signal_connect(open, "activate", G_CALLBACK(on_open), NULL);
g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(open));

GSimpleAction *quit = g_simple_action_new("quit", NULL);
g_signal_connect(quit, "activate", G_CALLBACK(on_quit), NULL);
g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(quit));

GMenu *menu = g_menu_new();
g_menu_append(menu, "Open", "open");
g_menu_append(menu, "Quit", "quit");

gsni_item_set_action_group(item, G_ACTION_GROUP(actions));
gsni_item_set_menu(item, G_MENU_MODEL(menu));
```

Click the tray icon to see the menu.  Click a menu item to trigger its
action.

## Tooltips

```c
gsni_item_set_tooltip(item, "dialog-information",
    "My Application", "Ready");
```

The tooltip appears when the user hovers over the icon.

## Scrolling

Connect to the `scroll` signal to handle mouse wheel events on the icon:

```c
static void
on_scroll(GsniItem *item, gint delta, GsniScrollOrientation ori,
          gpointer data)
{
    g_print("Scrolled %d\n", delta);
}

g_signal_connect(item, "scroll", G_CALLBACK(on_scroll), NULL);
```

## Attention state

Flash the tray icon to get the user's attention:

```c
gsni_item_set_attention_icon_name(item, "dialog-warning");
gsni_item_set_status(item, GSNI_STATUS_NEEDS_ATTENTION);
```

The system tray will blink between your normal icon and the attention icon
until you set the status back.

## Flatpak

When running inside Flatpak, request explicit D-Bus access in your manifest:

```json
{
    "finish-args": [
        "--socket=session-bus",
        "--talk-name=org.kde.StatusNotifierWatcher"
    ]
}
```

## Next steps

- Read the [API reference](api/) for `GsniItem`, `GsniHost`, and `GsniHostItem`
- See `examples/` in the source tree for complete working examples
- The [concepts page](concepts.html) explains the D-Bus protocol behind the scenes
