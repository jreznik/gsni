/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHostItem — proxy for a remote SNI item.
 */

#include "gsni/gsni-host-item.h"

struct _GsniHostItem {
    GObject parent_instance;
    gchar    *id;
    gchar    *title;
    gchar    *icon_name;
    gchar    *menu_path;
    GsniStatus status;
    GsniCategory category;
};

G_DEFINE_FINAL_TYPE(GsniHostItem, gsni_host_item, G_TYPE_OBJECT)

static void
gsni_host_item_finalize(GObject *object)
{
    GsniHostItem *self = GSNI_HOST_ITEM(object);
    g_free(self->id);
    g_free(self->title);
    g_free(self->icon_name);
    g_free(self->menu_path);
    G_OBJECT_CLASS(gsni_host_item_parent_class)->finalize(object);
}

static void
gsni_host_item_class_init(GsniHostItemClass *klass)
{
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->finalize = gsni_host_item_finalize;
}

static void
gsni_host_item_init(GsniHostItem *self)
{
    self->status = GSNI_STATUS_PASSIVE;
    self->category = GSNI_CATEGORY_APPLICATION_STATUS;
}

const gchar *
gsni_host_item_get_id(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->id;
}

const gchar *
gsni_host_item_get_title(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->title;
}

GsniStatus
gsni_host_item_get_status(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), GSNI_STATUS_PASSIVE);
    return self->status;
}

GsniCategory
gsni_host_item_get_category(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self),
                         GSNI_CATEGORY_APPLICATION_STATUS);
    return self->category;
}

const gchar *
gsni_host_item_get_icon_name(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->icon_name;
}

GdkPixbuf *
gsni_host_item_get_icon_pixbuf(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return NULL;
}

GsniToolTip *
gsni_host_item_get_tooltip(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return NULL;
}

const gchar *
gsni_host_item_get_menu_path(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->menu_path;
}
