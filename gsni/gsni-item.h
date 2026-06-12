/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniItem — a StatusNotifierItem exported over D-Bus.
 */

#ifndef GSNI_ITEM_H
#define GSNI_ITEM_H

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gsni/gsni-enums.h"
#include "gsni/gsni-types.h"

G_BEGIN_DECLS

#define GSNI_TYPE_ITEM (gsni_item_get_type())
G_DECLARE_FINAL_TYPE(GsniItem, gsni_item, GSNI, ITEM, GObject)

GsniItem      *gsni_item_new               (const gchar       *id,
                                            GDBusConnection   *connection,
                                            GError           **error);

gboolean       gsni_item_register          (GsniItem    *self,
                                            GError     **error);
void           gsni_item_unregister        (GsniItem    *self);
gboolean       gsni_item_get_connected     (GsniItem    *self);

void           gsni_item_set_title         (GsniItem    *self,
                                            const gchar *title);
const gchar   *gsni_item_get_title         (GsniItem    *self);

const gchar   *gsni_item_get_id            (GsniItem    *self);

const gchar   *gsni_item_get_bus_name      (GsniItem    *self);

const gchar   *gsni_item_get_activation_token(GsniItem *self);

void           gsni_item_set_category      (GsniItem    *self,
                                            GsniCategory category);
GsniCategory   gsni_item_get_category      (GsniItem    *self);

void           gsni_item_set_status        (GsniItem    *self,
                                            GsniStatus   status);
GsniStatus     gsni_item_get_status        (GsniItem    *self);

void           gsni_item_set_icon_name     (GsniItem    *self,
                                            const gchar *name);
const gchar   *gsni_item_get_icon_name     (GsniItem    *self);

void           gsni_item_set_icon_pixbuf   (GsniItem    *self,
                                            GdkPixbuf   *pixbuf);
GdkPixbuf     *gsni_item_get_icon_pixbuf   (GsniItem    *self);

void           gsni_item_set_overlay_icon_name(GsniItem    *self,
                                               const gchar *name);
const gchar   *gsni_item_get_overlay_icon_name(GsniItem    *self);

void           gsni_item_set_attention_icon_name(GsniItem    *self,
                                                  const gchar *name);
const gchar   *gsni_item_get_attention_icon_name(GsniItem    *self);

void           gsni_item_set_attention_movie_name(GsniItem    *self,
                                                   const gchar *name);
const gchar   *gsni_item_get_attention_movie_name(GsniItem    *self);

void           gsni_item_set_item_is_menu  (GsniItem    *self,
                                            gboolean     is_menu);
gboolean       gsni_item_get_item_is_menu  (GsniItem    *self);

void           gsni_item_set_icon_theme_path(GsniItem    *self,
                                              const gchar *path);
const gchar   *gsni_item_get_icon_theme_path(GsniItem    *self);

void           gsni_item_set_menu          (GsniItem    *self,
                                            GMenuModel  *menu);
GMenuModel    *gsni_item_get_menu          (GsniItem    *self);

void           gsni_item_set_action_group  (GsniItem    *self,
                                            GActionGroup *actions);
GActionGroup  *gsni_item_get_action_group  (GsniItem    *self);

void           gsni_item_set_tooltip       (GsniItem    *self,
                                            const gchar *icon_name,
                                            const gchar *title,
                                            const gchar *subtitle);
GsniToolTip   *gsni_item_get_tooltip       (GsniItem    *self);

void           gsni_item_set_window_id     (GsniItem    *self,
                                            gint         window_id);
gint           gsni_item_get_window_id     (GsniItem    *self);

void           gsni_item_show_notification  (GsniItem    *self,
                                             const gchar *title,
                                             const gchar *body,
                                             const gchar *icon_name,
                                             gint         timeout_ms);

const gchar   *gsni_item_get_bus_name     (GsniItem    *self);

G_END_DECLS

#endif /* GSNI_ITEM_H */
