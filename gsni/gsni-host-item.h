/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHostItem — a passive proxy for a remote StatusNotifierItem.
 */

#ifndef GSNI_HOST_ITEM_H
#define GSNI_HOST_ITEM_H

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gsni/gsni-enums.h"
#include "gsni/gsni-types.h"

G_BEGIN_DECLS

#define GSNI_TYPE_HOST_ITEM (gsni_host_item_get_type())
G_DECLARE_FINAL_TYPE(GsniHostItem, gsni_host_item, GSNI, HOST_ITEM, GObject)

const gchar   *gsni_host_item_get_id          (GsniHostItem *self);
const gchar   *gsni_host_item_get_title       (GsniHostItem *self);
GsniStatus     gsni_host_item_get_status      (GsniHostItem *self);
GsniCategory   gsni_host_item_get_category    (GsniHostItem *self);
const gchar   *gsni_host_item_get_icon_name   (GsniHostItem *self);
GdkPixbuf     *gsni_host_item_get_icon_pixbuf (GsniHostItem *self);
GsniToolTip   *gsni_host_item_get_tooltip     (GsniHostItem *self);
const gchar   *gsni_host_item_get_menu_path   (GsniHostItem *self);

G_END_DECLS

#endif /* GSNI_HOST_ITEM_H */
