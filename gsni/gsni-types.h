/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Gsni utility types shared between the item and host sides.
 */

#ifndef GSNI_TYPES_H
#define GSNI_TYPES_H

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

typedef struct _GsniIconPixmap {
    gint      width;
    gint      height;
    GdkPixbuf *pixbuf;
} GsniIconPixmap;

void          gsni_icon_pixmap_free     (GsniIconPixmap *pixmap);
GsniIconPixmap *gsni_icon_pixmap_copy   (const GsniIconPixmap *pixmap);

GType          gsni_icon_pixmap_get_type(void) G_GNUC_CONST;
#define GSNI_TYPE_ICON_PIXMAP (gsni_icon_pixmap_get_type())

typedef struct _GsniToolTip {
    gchar    *icon_name;
    GdkPixbuf *icon_pixbuf;
    gchar    *title;
    gchar    *subtitle;
} GsniToolTip;

void          gsni_tool_tip_free        (GsniToolTip *tip);
GsniToolTip  *gsni_tool_tip_copy        (const GsniToolTip *tip);

GType         gsni_tool_tip_get_type    (void) G_GNUC_CONST;
#define GSNI_TYPE_TOOL_TIP (gsni_tool_tip_get_type())

G_END_DECLS

#endif /* GSNI_TYPES_H */
