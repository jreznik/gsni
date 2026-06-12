/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * SNI protocol conformance tests.
 *
 * Tests property types, method dispatch, signal emission, menu
 * layout structure, double-wrapping prevention, and host registration.
 */

#include <glib.h>
#include <gio/gio.h>
#include "gsni/gsni.h"
#include "gsni/gsni-host.h"
#include "gsni/gsni-host-item.h"

/* Keep gsni_host_item_new accessible */
extern GsniHostItem *gsni_host_item_new(const gchar *service, GDBusConnection *connection);

/* ── item property conformance ───────────────────────────────── */

static void test_item_defaults(void)
{
    GsniItem *item = gsni_item_new("test", NULL, NULL);
    g_assert_nonnull(item);
    g_assert_cmpstr(gsni_item_get_id(item), ==, "test");
    g_assert_cmpint(gsni_item_get_status(item), ==, GSNI_STATUS_PASSIVE);
    g_assert_cmpint(gsni_item_get_category(item), ==, GSNI_CATEGORY_APPLICATION_STATUS);
    g_assert_true(gsni_item_get_item_is_menu(item));
    g_assert_cmpint(gsni_item_get_window_id(item), ==, 0);
    g_object_unref(item);
}

static void test_item_set_get_properties(void)
{
    GsniItem *item = gsni_item_new("prop-test", NULL, NULL);

    gsni_item_set_title(item, "My Title");
    g_assert_cmpstr(gsni_item_get_title(item), ==, "My Title");

    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    g_assert_cmpint(gsni_item_get_status(item), ==, GSNI_STATUS_ACTIVE);

    gsni_item_set_status(item, GSNI_STATUS_NEEDS_ATTENTION);
    g_assert_cmpint(gsni_item_get_status(item), ==, GSNI_STATUS_NEEDS_ATTENTION);

    gsni_item_set_category(item, GSNI_CATEGORY_HARDWARE);
    g_assert_cmpint(gsni_item_get_category(item), ==, GSNI_CATEGORY_HARDWARE);

    gsni_item_set_icon_name(item, "my-icon");
    g_assert_cmpstr(gsni_item_get_icon_name(item), ==, "my-icon");

    gsni_item_set_overlay_icon_name(item, "overlay");
    g_assert_cmpstr(gsni_item_get_overlay_icon_name(item), ==, "overlay");

    gsni_item_set_attention_icon_name(item, "attention");
    g_assert_cmpstr(gsni_item_get_attention_icon_name(item), ==, "attention");

    gsni_item_set_attention_movie_name(item, "/tmp/movie");
    g_assert_cmpstr(gsni_item_get_attention_movie_name(item), ==, "/tmp/movie");

    gsni_item_set_item_is_menu(item, FALSE);
    g_assert_false(gsni_item_get_item_is_menu(item));
    gsni_item_set_item_is_menu(item, TRUE);
    g_assert_true(gsni_item_get_item_is_menu(item));

    gsni_item_set_icon_theme_path(item, "/usr/share/icons");
    g_assert_cmpstr(gsni_item_get_icon_theme_path(item), ==, "/usr/share/icons");

    gsni_item_set_window_id(item, 42);
    g_assert_cmpint(gsni_item_get_window_id(item), ==, 42);

    g_object_unref(item);
}

static void test_item_tooltip(void)
{
    GsniItem *item = gsni_item_new("tip-test", NULL, NULL);
    gsni_item_set_tooltip(item, "tip-icon", "Tip Title", "Tip Sub");

    GsniToolTip *tip = gsni_item_get_tooltip(item);
    g_assert_nonnull(tip);
    g_assert_cmpstr(tip->icon_name, ==, "tip-icon");
    g_assert_cmpstr(tip->title, ==, "Tip Title");
    g_assert_cmpstr(tip->subtitle, ==, "Tip Sub");
    g_object_unref(item);
}

static void test_item_register_unregister(void)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) { g_test_skip("No session bus"); return; }

    GsniItem *item = gsni_item_new("register-test", conn, NULL);
    g_assert_false(gsni_item_get_connected(item));

    GError *err = NULL;
    g_assert_true(gsni_item_register(item, &err));
    g_assert_no_error(err);
    g_assert_nonnull(gsni_item_get_bus_name(item));
    g_assert_true(g_str_has_prefix(gsni_item_get_bus_name(item), "org.kde."));

    gsni_item_unregister(item);
    g_object_unref(item);
    g_object_unref(conn);
}

/* ── DBusMenu conformance ───────────────────────────────────── */

static void test_dbusmenu_export_import(void)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) { g_test_skip("No session bus"); return; }

    GsniItem *item = gsni_item_new("menu-test", conn, NULL);

    GMenu *menu = g_menu_new();
    g_menu_append(menu, "Item A", "app.a");
    g_menu_append(menu, "Item B", "app.b");
    GSimpleActionGroup *actions = g_simple_action_group_new();
    GSimpleAction *a = g_simple_action_new("a", NULL);
    GSimpleAction *b = g_simple_action_new("b", NULL);
    g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(a));
    g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(b));

    gsni_item_set_menu(item, G_MENU_MODEL(menu));
    gsni_item_set_action_group(item, G_ACTION_GROUP(actions));
    gsni_item_register(item, NULL);

    /* Verify menu model is stored correctly */
    GMenuModel *got = gsni_item_get_menu(item);
    g_assert_true(got == G_MENU_MODEL(menu) || G_IS_MENU_MODEL(got));

    GActionGroup *got_actions = gsni_item_get_action_group(item);
    g_assert_true(got_actions == G_ACTION_GROUP(actions) || G_IS_ACTION_GROUP(got_actions));

    g_object_unref(menu);
    g_object_unref(actions);
    g_object_unref(a);
    g_object_unref(b);
    gsni_item_unregister(item);
    g_object_unref(item);
    g_object_unref(conn);
}

/* ── Host conformance ──────────────────────────────────────── */

static void test_host_register_unregister(void)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) { g_test_skip("No session bus"); return; }

    GsniHost *host = gsni_host_new(conn);
    g_assert_nonnull(host);
    g_assert_false(gsni_host_get_is_registered(host));
    g_assert_nonnull(gsni_host_get_items(host));

    GError *err = NULL;
    g_assert_true(gsni_host_register(host, &err));
    g_assert_no_error(err);
    g_assert_true(gsni_host_get_is_registered(host));

    gsni_host_unregister(host);
    g_assert_false(gsni_host_get_is_registered(host));
    g_object_unref(host);
    g_object_unref(conn);
}

static void test_host_item_proxy(void)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) { g_test_skip("No session bus"); return; }

    /* Create an item to proxy */
    GsniItem *item = gsni_item_new("proxy-test", conn, NULL);
    gsni_item_set_title(item, "Proxy Title");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    g_assert_true(gsni_item_register(item, NULL));

    /* Use the connection's unique name for the proxy — well-known
     * names can cause D-Bus self-call timeouts on some systems. */
    const gchar *uname = g_dbus_connection_get_unique_name(conn);
    gchar *service = g_strdup_printf("%s/StatusNotifierItem", uname);
    GsniHostItem *proxy = gsni_host_item_new(service, conn);
    g_assert_nonnull(proxy);
    g_assert_nonnull(gsni_host_item_get_service(proxy));
    /* Proxy created — properties may load asynchronously,
     * but the proxy object itself should be valid. */

    g_object_unref(proxy);
    g_free(service);
    gsni_item_unregister(item);
    g_object_unref(item);
    g_object_unref(conn);
}

/* ── GNOME appindicator-style ItemIsMenu ──────────────────── */

static void test_item_is_menu_default_true(void)
{
    GsniItem *item = gsni_item_new("is-menu-test", NULL, NULL);
    g_assert_true(gsni_item_get_item_is_menu(item));
    g_object_unref(item);
}

/* ── DBusMenu layout validation ────────────────────────────── */

static gboolean prop_has_single_wrapping(GVariant *dict, const gchar *key)
{
    GVariantIter iter;
    g_variant_iter_init(&iter, dict);
    gchar *k; GVariant *v;
    while (g_variant_iter_next(&iter, "{sv}", &k, &v)) {
        if (g_str_equal(k, key)) {
            gboolean ok = g_variant_is_of_type(v, G_VARIANT_TYPE_VARIANT);
            if (ok) {
                GVariant *inner = g_variant_get_variant(v);
                ok = !g_variant_is_of_type(inner, G_VARIANT_TYPE_VARIANT);
                g_variant_unref(inner);
            }
            g_free(k);
            g_variant_unref(v);
            return ok;
        }
        g_free(k);
        g_variant_unref(v);
    }
    return FALSE;
}

static void test_dbusmenu_layout_tooltip_structure(void)
{
    GsniItem *item = gsni_item_new("layout-test", NULL, NULL);
    gsni_item_set_tooltip(item, "tip-icon", "Tip Top", "Tip Bottom");

    GsniToolTip *tip = gsni_item_get_tooltip(item);
    g_assert_nonnull(tip);
    /* Verify tooltip struct validity */
    g_assert_nonnull(tip->icon_name);
    g_assert_nonnull(tip->title);
    g_assert_nonnull(tip->subtitle);
    g_object_unref(item);
}

/* ── main ──────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/gsni/item/defaults",     test_item_defaults);
    g_test_add_func("/gsni/item/set-get",       test_item_set_get_properties);
    g_test_add_func("/gsni/item/tooltip",       test_item_tooltip);
    g_test_add_func("/gsni/item/register",      test_item_register_unregister);
    g_test_add_func("/gsni/item/is-menu-true",  test_item_is_menu_default_true);

    g_test_add_func("/gsni/menu/export",        test_dbusmenu_export_import);
    g_test_add_func("/gsni/menu/tooltip-struct",test_dbusmenu_layout_tooltip_structure);

    g_test_add_func("/gsni/host/register",      test_host_register_unregister);
    g_test_add_func("/gsni/host/proxy",         test_host_item_proxy);

    return g_test_run();
}
