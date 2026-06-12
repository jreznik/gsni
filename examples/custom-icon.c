/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Custom pixmap icon example — programmatically generates an icon
 * from raw pixel data and sets it via gsni_item_set_icon_pixbuf().
 */

#include <gio/gio.h>
#include "gsni/gsni.h"

static GdkPixbuf *
generate_tray_icon(void)
{
    gint size = 48;
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, size, size);
    guint8 *pixels = gdk_pixbuf_get_pixels(pixbuf);
    gint stride = gdk_pixbuf_get_rowstride(pixbuf);
    gint half = size / 2;

    for (gint y = 0; y < size; y++) {
        for (gint x = 0; x < size; x++) {
            gboolean inside_circle =
                (x - half) * (x - half) + (y - half) * (y - half)
                < half * half;

            guint8 *p = pixels + (gsize)y * stride + (gsize)x * 4;

            if (inside_circle) {
                p[0] = 0x50 + (guint8)(x * 180 / size); /* red   */
                p[1] = 0x80 + (guint8)(y * 100 / size); /* green */
                p[2] = 0xcc;                              /* blue  */
                p[3] = 0xff;                              /* alpha */
            } else {
                p[0] = p[1] = p[2] = 0x00;
                p[3] = 0x00; /* transparent */
            }
        }
    }

    return pixbuf;
}

int
main(int argc, char *argv[])
{
    g_autoptr(GApplication) app = g_application_new(
        "org.libgsni.CustomIcon", G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(app, NULL, NULL);

    GDBusConnection *bus = g_application_get_dbus_connection(app);
    g_autoptr(GsniItem) item = gsni_item_new("custom-icon", bus, NULL);

    gsni_item_set_title(item, "Custom Icon Demo");
    gsni_item_set_status(item, GSNI_STATUS_ACTIVE);
    gsni_item_set_tooltip(item, NULL,
        "Custom Icon", "Icon generated from raw pixel data");

    /* Set icon as pixbuf instead of themed name */
    g_autoptr(GdkPixbuf) icon = generate_tray_icon();
    gsni_item_set_icon_pixbuf(item, icon);

    gsni_item_register(item, NULL);

    g_print("Tray icon registered with custom pixmap.\n"
            "The icon is a programmatically generated colored circle.\n"
            "Press Ctrl+C to exit.\n");

    g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}
