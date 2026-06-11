/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Minimal example: create a tray icon with libgsni.
 *
 * Build: meson setup build && ninja -C build
 * Run:   ./build/examples/simple-item
 *
 * You need a system tray implementation running (org.kde.StatusNotifierWatcher
 * on the session bus) for the icon to actually appear.
 */

#include <gio/gio.h>
#include "gsni/gsni.h"

int
main(int argc, char *argv[])
{
    g_autoptr(GApplication) app = NULL;
    g_autoptr(GDBusConnection) bus = NULL;
    g_autoptr(GsniItem) item = NULL;
    g_autoptr(GMainLoop) loop = NULL;

    app = g_application_new("org.example.SimpleTray",
                            G_APPLICATION_DEFAULT_FLAGS);

    if (!g_application_register(app, NULL, NULL)) {
        g_printerr("Failed to register application\n");
        return 1;
    }

    bus = g_application_get_dbus_connection(app);
    if (bus == NULL) {
        g_printerr("No D-Bus connection\n");
        return 1;
    }

    /* Create the tray icon */
    item = gsni_item_new("simple-tray", bus, NULL);
    gsni_item_set_title(item, "Simple Tray Demo");
    gsni_item_set_icon_name(item, "applications-other");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);

    gsni_item_set_tooltip(item, NULL,
                          "Simple Tray Demo",
                          "This is a libgsni example");

    gsni_item_register(item, NULL);

    g_print("Tray icon registered.\n"
            "Look for 'Simple Tray Demo' in your system tray.\n"
            "Press Ctrl+C to exit.\n");

    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
