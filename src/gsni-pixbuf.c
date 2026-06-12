/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 */

#include "gsni/gsni-pixbuf.h"

/*
 * Maximum icon dimensions we accept.
 */
#define GSNI_MAX_ICON_SIZE  4096

/*
 * Default sizes we export when the caller does not provide specific sizes.
 * These are common system tray icon sizes.
 */
static const gint default_icon_sizes[] = { 16, 22, 24, 32, 48, 64, 0 };

static void
swap_to_network_byte_order(guint8 *data, gsize len)
{
    guint32 *p = (guint32 *)data;
    gsize count = len / sizeof(guint32);

    for (gsize i = 0; i < count; i++) {
        *p = GUINT32_TO_BE(*p);
        p++;
    }
}

static void
swap_from_network_byte_order(guint8 *data, gsize len)
{
    guint32 *p = (guint32 *)data;
    gsize count = len / sizeof(guint32);

    for (gsize i = 0; i < count; i++) {
        *p = GUINT32_FROM_BE(*p);
        p++;
    }
}

/*
 * gsni_pixbuf_to_dbus_image:
 * @pixbuf: a #GdkPixbuf to serialize
 *
 * Converts a #GdkPixbuf to a D-Bus image struct variant of type (iiay).
 * The pixbuf is stored as width, height, and ARGB32 data in network
 * byte order.
 *
 * Returns: (transfer full): a floating #GVariant of type (iiay), or
 *     an empty byte array variant if @pixbuf is %NULL.
 */
GVariant *
gsni_pixbuf_to_dbus_image(GdkPixbuf *pixbuf)
{
    g_return_val_if_fail(GDK_IS_PIXBUF(pixbuf) || pixbuf == NULL,
                         g_variant_new_array(G_VARIANT_TYPE_BYTE, NULL, 0));

    if (pixbuf == NULL)
        return g_variant_new_array(G_VARIANT_TYPE_BYTE, NULL, 0);

    gint width = gdk_pixbuf_get_width(pixbuf);
    gint height = gdk_pixbuf_get_height(pixbuf);

    g_return_val_if_fail(width > 0 && width <= GSNI_MAX_ICON_SIZE,
                         g_variant_new_array(G_VARIANT_TYPE_BYTE, NULL, 0));
    g_return_val_if_fail(height > 0 && height <= GSNI_MAX_ICON_SIZE,
                         g_variant_new_array(G_VARIANT_TYPE_BYTE, NULL, 0));

    g_autoptr(GdkPixbuf) argb = NULL;

    if (gdk_pixbuf_get_colorspace(pixbuf) != GDK_COLORSPACE_RGB ||
        !gdk_pixbuf_get_has_alpha(pixbuf) ||
        gdk_pixbuf_get_bits_per_sample(pixbuf) != 8) {
        argb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
        gdk_pixbuf_copy_area(pixbuf, 0, 0, width, height, argb, 0, 0);
    } else {
        argb = g_object_ref(pixbuf);
    }

    gsize data_len = (gsize)height * gdk_pixbuf_get_rowstride(argb);
    g_autoptr(GBytes) bytes = g_bytes_new(gdk_pixbuf_get_pixels(argb), data_len);
    gsize actual_len;
    const guint8 *raw = g_bytes_get_data(bytes, &actual_len);

    guint8 *copy = g_memdup2(raw, actual_len);
    swap_to_network_byte_order(copy, actual_len);

    GVariant *result = g_variant_new("(ii@ay)", width, height,
        g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, copy, actual_len, 1));
    g_free(copy);
    return result;
}

/*
 * gsni_pixbuf_to_dbus_image_vector:
 * @pixbuf: a #GdkPixbuf
 *
 * Builds an a(iiay) array with multiple sizes of the icon, so the
 * system tray can pick the most appropriate one.
 *
 * Returns: (transfer full): a #GVariant of type a(iiay).
 */
GVariant *
gsni_pixbuf_to_dbus_image_vector(GdkPixbuf *pixbuf)
{
    g_return_val_if_fail(pixbuf == NULL || GDK_IS_PIXBUF(pixbuf),
                         g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0));

    if (pixbuf == NULL)
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(iiay)"));

    for (const gint *size = default_icon_sizes; *size != 0; size++) {
        g_autoptr(GdkPixbuf) scaled = gdk_pixbuf_scale_simple(
            pixbuf, *size, *size, GDK_INTERP_BILINEAR);
        if (scaled == NULL)
            continue;

        GVariant *image = gsni_pixbuf_to_dbus_image(scaled);
        g_variant_builder_add_value(&builder, image);
    }

    return g_variant_builder_end(&builder);
}

/*
 * gsni_dbus_image_to_pixbuf:
 * @variant: a #GVariant of type (iiay), as returned by the SNI protocol
 *
 * Converts a D-Bus icon pixmap struct back to a #GdkPixbuf.
 *
 * Returns: (transfer full): a new #GdkPixbuf, or %NULL on error.
 */
GdkPixbuf *
gsni_dbus_image_to_pixbuf(GVariant *variant)
{
    g_return_val_if_fail(variant != NULL, NULL);
    g_return_val_if_fail(g_variant_is_of_type(variant, G_VARIANT_TYPE("(iiay)")), NULL);

    gint width, height;
    g_autoptr(GVariant) data_variant = NULL;
    g_variant_get(variant, "(ii@ay)", &width, &height, &data_variant);

    if (width <= 0 || height <= 0 || width > GSNI_MAX_ICON_SIZE ||
        height > GSNI_MAX_ICON_SIZE)
        return NULL;

    gsize data_len;
    const guint8 *raw = g_variant_get_fixed_array(data_variant, &data_len, 1);

    if (raw == NULL || data_len == 0)
        return NULL;

    guint8 *copy = g_memdup2(raw, data_len);
    swap_from_network_byte_order(copy, data_len);

    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                       width, height);
    gsize rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    gsize expected = (gsize)height * rowstride;

    if (data_len >= expected) {
        memcpy(gdk_pixbuf_get_pixels(pixbuf), copy, expected);
    } else {
        memset(gdk_pixbuf_get_pixels(pixbuf), 0, expected);
        for (gint row = 0; row < height; row++) {
            gsize src_row = (gsize)row * width * 4;
            gsize dst_row = (gsize)row * rowstride;
            if (src_row >= data_len)
                break;
            gsize remaining = data_len - src_row;
            gsize to_copy = MIN((gsize)(width * 4), remaining);
            if (to_copy > 0)
                memcpy(gdk_pixbuf_get_pixels(pixbuf) + dst_row,
                       copy + src_row, to_copy);
        }
    }

    g_free(copy);
    return pixbuf;
}
