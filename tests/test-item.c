/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Unit tests for GsniItem.
 */

#include <glib.h>
#include <gio/gio.h>
#include "gsni/gsni-item.h"

static void
test_item_new(void)
{
    GError *error = NULL;
    GsniItem *item = gsni_item_new("test-id", NULL, &error);

    g_assert_no_error(error);
    g_assert_nonnull(item);
    g_assert_true(GSNI_IS_ITEM(item));

    g_object_unref(item);
}

static void
test_item_defaults(void)
{
    GsniItem *item = gsni_item_new("test-defaults", NULL, NULL);

    g_assert_cmpint(gsni_item_get_status(item), ==, GSNI_STATUS_PASSIVE);
    g_assert_cmpint(gsni_item_get_category(item), ==, GSNI_CATEGORY_APPLICATION_STATUS);
    g_assert_null(gsni_item_get_title(item));
    g_assert_null(gsni_item_get_icon_name(item));
    g_assert_false(gsni_item_get_item_is_menu(item));
    g_assert_false(gsni_item_get_connected(item));
    g_assert_cmpint(gsni_item_get_window_id(item), ==, 0);

    g_object_unref(item);
}

static void
test_item_set_properties(void)
{
    GsniItem *item = gsni_item_new("test-props", NULL, NULL);

    gsni_item_set_title(item, "Test Title");
    g_assert_cmpstr(gsni_item_get_title(item), ==, "Test Title");

    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    g_assert_cmpint(gsni_item_get_status(item), ==, GSNI_STATUS_ACTIVE);

    gsni_item_set_category(item, GSNI_CATEGORY_HARDWARE);
    g_assert_cmpint(gsni_item_get_category(item), ==, GSNI_CATEGORY_HARDWARE);

    gsni_item_set_icon_name(item, "test-icon");
    g_assert_cmpstr(gsni_item_get_icon_name(item), ==, "test-icon");

    gsni_item_set_overlay_icon_name(item, "overlay");
    g_assert_cmpstr(gsni_item_get_overlay_icon_name(item), ==, "overlay");

    gsni_item_set_attention_icon_name(item, "attention");
    g_assert_cmpstr(gsni_item_get_attention_icon_name(item), ==, "attention");

    gsni_item_set_attention_movie_name(item, "/tmp/movie.mng");
    g_assert_cmpstr(gsni_item_get_attention_movie_name(item), ==, "/tmp/movie.mng");

    gsni_item_set_item_is_menu(item, TRUE);
    g_assert_true(gsni_item_get_item_is_menu(item));

    gsni_item_set_icon_theme_path(item, "/usr/share/icons");
    g_assert_cmpstr(gsni_item_get_icon_theme_path(item), ==, "/usr/share/icons");

    gsni_item_set_window_id(item, 12345);
    g_assert_cmpint(gsni_item_get_window_id(item), ==, 12345);

    g_object_unref(item);
}

static void
test_item_tooltip(void)
{
    GsniItem *item = gsni_item_new("test-tooltip", NULL, NULL);

    gsni_item_set_tooltip(item, "tip-icon", "Tip Title", "Tip Subtitle");

    GsniToolTip *tip = gsni_item_get_tooltip(item);
    g_assert_nonnull(tip);
    g_assert_cmpstr(tip->icon_name, ==, "tip-icon");
    g_assert_cmpstr(tip->title, ==, "Tip Title");
    g_assert_cmpstr(tip->subtitle, ==, "Tip Subtitle");

    g_object_unref(item);
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/gsni/item/new", test_item_new);
    g_test_add_func("/gsni/item/defaults", test_item_defaults);
    g_test_add_func("/gsni/item/set-properties", test_item_set_properties);
    g_test_add_func("/gsni/item/tooltip", test_item_tooltip);

    return g_test_run();
}
