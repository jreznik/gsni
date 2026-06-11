/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Internal D-Bus adaptor for GsniItem — the org.kde.StatusNotifierItem vtable.
 */

#ifndef GSNI_ITEM_DBUS_H
#define GSNI_ITEM_DBUS_H

#include <gio/gio.h>
#include "gsni/gsni-item.h"

G_BEGIN_DECLS

typedef struct _GsniItemDbus GsniItemDbus;

GsniItemDbus *gsni_item_dbus_new        (GsniItem  *item);
gboolean      gsni_item_dbus_export     (GsniItemDbus *self,
                                         GError      **error);
void          gsni_item_dbus_unexport   (GsniItemDbus *self);
void          gsni_item_dbus_free       (GsniItemDbus *self);

void          gsni_item_dbus_emit_signal(GsniItemDbus *self,
                                         const gchar  *signal_name,
                                         GVariant     *parameters);

void          gsni_item_dbus_update_menu(GsniItemDbus  *self,
                                         GMenuModel    *menu,
                                         GActionGroup  *actions);

/* Internal accessors used by the D-Bus and watcher layers */
GDBusConnection *gsni_item_get_connection   (GsniItem *item);
const gchar     *gsni_item_get_object_path  (GsniItem *item);
GsniItemDbus    *gsni_item_get_dbus         (GsniItem *item);

/* Internal functions called by the D-Bus adaptor */
void          gsni_item_set_activation_token(GsniItem *item, const gchar *token);
void          gsni_item_emit_activate        (GsniItem *item, gint x, gint y);
void          gsni_item_emit_secondary_activate(GsniItem *item, gint x, gint y);
void          gsni_item_emit_scroll          (GsniItem *item, gint delta,
                                              GsniScrollOrientation orientation);

G_END_DECLS

#endif /* GSNI_ITEM_DBUS_H */
