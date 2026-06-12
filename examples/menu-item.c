/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Example with a context menu and activation callback.
 */

#include <gio/gio.h>
#include "gsni/gsni.h"

static void
on_activate(GsniItem *item, gint x, gint y, gpointer data)
{
    g_print("Activated at (%d, %d)\n", x, y);
}

static void
on_scroll(GsniItem *item, gint delta, GsniScrollOrientation ori,
          gpointer data)
{
    const gchar *dir = "?";
    switch (ori) {
    case GSNI_SCROLL_UP:    dir = "up"; break;
    case GSNI_SCROLL_DOWN:  dir = "down"; break;
    case GSNI_SCROLL_LEFT:  dir = "left"; break;
    case GSNI_SCROLL_RIGHT: dir = "right"; break;
    }
    g_print("Scrolled %d %s\n", delta, dir);
}

static void
on_open_cb(GSimpleAction *action, GVariant *param, gpointer data)
{
    g_print("Menu: Open\n");
}

static void
on_quit_cb(GSimpleAction *action, GVariant *param, gpointer data)
{
    g_print("Menu: Quit\n");
    GMainLoop *loop = data;
    g_main_loop_quit(loop);
}

int
main(int argc, char *argv[])
{
    g_autoptr(GApplication) app = NULL;
    g_autoptr(GDBusConnection) bus = NULL;
    g_autoptr(GsniItem) item = NULL;
    g_autoptr(GMenu) menu = NULL;
    g_autoptr(GSimpleActionGroup) actions = NULL;
    g_autoptr(GSimpleAction) open_action = NULL;
    g_autoptr(GSimpleAction) quit_action = NULL;
    g_autoptr(GMainLoop) loop = NULL;

    app = g_application_new("org.example.SimpleTray",
                            G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(app, NULL, NULL);
    bus = g_application_get_dbus_connection(app);

    loop = g_main_loop_new(NULL, FALSE);

    /* Create tray icon */
    item = gsni_item_new("simple-tray", bus, NULL);
    gsni_item_set_title(item, "Simple Tray Demo");
    gsni_item_set_icon_name(item, "applications-other");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    gsni_item_set_item_is_menu(item, TRUE);
    gsni_item_set_tooltip(item, NULL, "Simple Tray Demo",
                          "Click for menu");

    /* Build a context menu */
    actions = g_simple_action_group_new();

    open_action = g_simple_action_new("open", NULL);
    g_signal_connect(open_action, "activate", G_CALLBACK(on_open_cb), NULL);
    g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(open_action));

    quit_action = g_simple_action_new("quit", NULL);
    g_signal_connect(quit_action, "activate", G_CALLBACK(on_quit_cb), loop);
    g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(quit_action));

    menu = g_menu_new();
    g_menu_append(menu, "Open", "open");
    g_menu_append(menu, "Quit", "quit");

    gsni_item_set_action_group(item, G_ACTION_GROUP(actions));
    gsni_item_set_menu(item, G_MENU_MODEL(menu));

    /* Connect signals */
    g_signal_connect(item, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(item, "scroll", G_CALLBACK(on_scroll), NULL);

    gsni_item_register(item, NULL);

    g_print("Tray icon with menu registered.\n"
            "Click the icon to see the context menu.\n"
            "Scroll to test scroll events.\n"
            "Press Ctrl+C to exit.\n");

    g_main_loop_run(loop);

    return 0;
}
