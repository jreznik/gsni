/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GdkPixbuf ↔ D-Bus a(iiay) serialization (public API).
 */

#ifndef GSNI_PIXBUF_H
#define GSNI_PIXBUF_H

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

GVariant   *gsni_pixbuf_to_dbus_image        (GdkPixbuf *pixbuf);
GdkPixbuf  *gsni_dbus_image_to_pixbuf        (GVariant  *variant);
GVariant   *gsni_pixbuf_to_dbus_image_vector  (GdkPixbuf *pixbuf);

G_END_DECLS

#endif /* GSNI_PIXBUF_H */
