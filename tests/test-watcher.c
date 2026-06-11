/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Unit tests for GsniWatcher.
 */

#include <glib.h>
#include <gio/gio.h>
#include "src/gsni-watcher.h"
#include "gsni/gsni-item.h"

static void
test_watcher_get_for_connection(void)
{
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (conn == NULL) {
        g_test_skip("No session bus available");
        return;
    }

    GsniWatcher *w = gsni_watcher_get_for_connection(conn);
    g_assert_nonnull(w);

    GsniWatcher *w2 = gsni_watcher_get_for_connection(conn);
    g_assert_true(w == w2);  /* singleton per connection */

    g_object_unref(conn);
}

static void
test_watcher_register_unregister(void)
{
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (conn == NULL) {
        g_test_skip("No session bus available");
        return;
    }

    GsniItem *item = gsni_item_new("test-watcher", conn, NULL);
    GsniWatcher *w = gsni_watcher_get_for_connection(conn);

    gsni_watcher_register_item(w, item);
    gsni_watcher_unregister_item(w, item);

    g_object_unref(item);
    g_object_unref(conn);
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/gsni/watcher/get-for-connection",
                    test_watcher_get_for_connection);
    g_test_add_func("/gsni/watcher/register-unregister",
                    test_watcher_register_unregister);

    return g_test_run();
}
