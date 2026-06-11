/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 */

#include "gsni/gsni-types.h"
#include "gsni/gsni-pixbuf.h"

GType
gsni_icon_pixmap_get_type(void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        type = g_boxed_type_register_static("GsniIconPixmap",
                (GBoxedCopyFunc) gsni_icon_pixmap_copy,
                (GBoxedFreeFunc) gsni_icon_pixmap_free);
    }
    return type;
}

void
gsni_icon_pixmap_free(GsniIconPixmap *pixmap)
{
    if (pixmap == NULL)
        return;
    g_clear_object(&pixmap->pixbuf);
    g_free(pixmap);
}

GsniIconPixmap *
gsni_icon_pixmap_copy(const GsniIconPixmap *pixmap)
{
    GsniIconPixmap *copy;

    g_return_val_if_fail(pixmap != NULL, NULL);

    copy = g_new(GsniIconPixmap, 1);
    copy->width = pixmap->width;
    copy->height = pixmap->height;
    copy->pixbuf = pixmap->pixbuf ? g_object_ref(pixmap->pixbuf) : NULL;
    return copy;
}

GType
gsni_tool_tip_get_type(void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        type = g_boxed_type_register_static("GsniToolTip",
                (GBoxedCopyFunc) gsni_tool_tip_copy,
                (GBoxedFreeFunc) gsni_tool_tip_free);
    }
    return type;
}

void
gsni_tool_tip_free(GsniToolTip *tip)
{
    if (tip == NULL)
        return;
    g_free(tip->icon_name);
    g_clear_object(&tip->icon_pixbuf);
    g_free(tip->title);
    g_free(tip->subtitle);
    g_free(tip);
}

GsniToolTip *
gsni_tool_tip_copy(const GsniToolTip *tip)
{
    GsniToolTip *copy;

    g_return_val_if_fail(tip != NULL, NULL);

    copy = g_new(GsniToolTip, 1);
    copy->icon_name = g_strdup(tip->icon_name);
    copy->icon_pixbuf = tip->icon_pixbuf ? g_object_ref(tip->icon_pixbuf) : NULL;
    copy->title = g_strdup(tip->title);
    copy->subtitle = g_strdup(tip->subtitle);
    return copy;
}
