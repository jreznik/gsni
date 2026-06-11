/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Unit tests for Gsni pixbuf serialization.
 */

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gsni/gsni-pixbuf.h"

static void
test_pixbuf_to_dbus_null(void)
{
    GVariant *v = gsni_pixbuf_to_dbus_image(NULL);
    g_assert_nonnull(v);
    g_assert_true(g_variant_is_of_type(v, G_VARIANT_TYPE_BYTESTRING));
    g_variant_unref(v);
}

static void
test_pixbuf_to_dbus_roundtrip(void)
{
    GdkPixbuf *original = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 16, 16);
    gdk_pixbuf_fill(original, 0xff0000ff);

    GVariant *v = gsni_pixbuf_to_dbus_image(original);
    g_assert_nonnull(v);
    g_assert_true(g_variant_is_of_type(v, G_VARIANT_TYPE("(iiay)")));

    GdkPixbuf *restored = gsni_dbus_image_to_pixbuf(v);
    g_assert_nonnull(restored);
    g_assert_cmpint(gdk_pixbuf_get_width(restored), ==, 16);
    g_assert_cmpint(gdk_pixbuf_get_height(restored), ==, 16);

    g_object_unref(restored);
    g_variant_unref(v);
    g_object_unref(original);
}

static void
test_pixbuf_vector_empty(void)
{
    GVariant *v = gsni_pixbuf_to_dbus_image_vector(NULL);
    g_assert_nonnull(v);
    g_assert_true(g_variant_is_of_type(v, G_VARIANT_TYPE("a(iiay)")));
    g_assert_cmpint(g_variant_n_children(v), ==, 0);
    g_variant_unref(v);
}

static void
test_pixbuf_vector(void)
{
    GdkPixbuf *original = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 48, 48);
    gdk_pixbuf_fill(original, 0xff0000ff);

    GVariant *v = gsni_pixbuf_to_dbus_image_vector(original);
    g_assert_nonnull(v);
    g_assert_true(g_variant_is_of_type(v, G_VARIANT_TYPE("a(iiay)")));
    g_assert_cmpint(g_variant_n_children(v), >, 0);

    g_variant_unref(v);
    g_object_unref(original);
}

int
main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/gsni/pixbuf/to-dbus-null", test_pixbuf_to_dbus_null);
    g_test_add_func("/gsni/pixbuf/roundtrip", test_pixbuf_to_dbus_roundtrip);
    g_test_add_func("/gsni/pixbuf/vector-empty", test_pixbuf_vector_empty);
    g_test_add_func("/gsni/pixbuf/vector", test_pixbuf_vector);

    return g_test_run();
}
