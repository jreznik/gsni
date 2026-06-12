/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHostItem — proxy for a remote StatusNotifierItem.
 *
 * Reads all properties via org.freedesktop.DBus.Properties.GetAll on
 * construction, subscribes to property change signals, and exposes
 * values as GObject properties.
 */

#include "gsni/gsni-host-item.h"
#include "gsni/gsni-enums.h"
#include "gsni/gsni-pixbuf.h"

#include <string.h>

#define SNI_IFACE  "org.kde.StatusNotifierItem"
#define PROPS_IFACE "org.freedesktop.DBus.Properties"

struct _GsniHostItem {
    GObject          parent_instance;
    GDBusConnection *connection;
    gchar           *service;       /* full string from watcher */
    gchar           *bus_name;      /* D-Bus well-known name (without path) */
    gchar           *object_path;   /* /StatusNotifierItem */
    GDBusProxy      *properties_proxy;

    gchar           *id;
    gchar           *title;
    GsniStatus       status;
    GsniCategory     category;
    gchar           *icon_name;
    GdkPixbuf       *icon_pixbuf;
    gchar           *menu_path;
    gchar           *overlay_icon_name;
    gchar           *attention_icon_name;
    gboolean         item_is_menu;
    gint             window_id;
    gboolean         loaded;

    guint            props_changed_sig;
};

G_DEFINE_FINAL_TYPE(GsniHostItem, gsni_host_item, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_ID,
    PROP_TITLE,
    PROP_STATUS,
    PROP_CATEGORY,
    PROP_ICON_NAME,
    PROP_ICON_PIXBUF,
    PROP_MENU_PATH,
    PROP_OVERLAY_ICON_NAME,
    PROP_ATTENTION_ICON_NAME,
    PROP_ITEM_IS_MENU,
    PROP_WINDOW_ID,
    PROP_SERVICE,
    PROP_LOADED,
    N_PROPS
};


static GsniStatus
parse_status(const gchar *s)
{
    if (g_str_equal(s, "Active"))          return GSNI_STATUS_ACTIVE;
    if (g_str_equal(s, "NeedsAttention"))  return GSNI_STATUS_NEEDS_ATTENTION;
    return GSNI_STATUS_PASSIVE;
}

static GsniCategory
parse_category(const gchar *s)
{
    if (g_str_equal(s, "Communications"))     return GSNI_CATEGORY_COMMUNICATIONS;
    if (g_str_equal(s, "SystemServices"))     return GSNI_CATEGORY_SYSTEM_SERVICES;
    if (g_str_equal(s, "Hardware"))           return GSNI_CATEGORY_HARDWARE;
    return GSNI_CATEGORY_APPLICATION_STATUS;
}

static void
gsni_host_item_load_properties(GsniHostItem *self)
{
    GError *error = NULL;
    GVariant *result = g_dbus_connection_call_sync(self->connection,
        self->bus_name, self->object_path, PROPS_IFACE, "GetAll",
        g_variant_new("(s)", SNI_IFACE),
        NULL, G_DBUS_CALL_FLAGS_NONE, 500, NULL, &error);

    if (result == NULL) {
        g_debug("GsniHostItem: GetAll failed for %s: %s", self->service, error->message);
        g_clear_error(&error);
        return;
    }

    g_autoptr(GVariant) inner = g_variant_get_child_value(result, 0);
    g_variant_unref(result);

    if (inner == NULL) {
        g_warning("GsniHostItem: GetAll reply has unexpected structure");
        return;
    }

    {
        GVariantIter iter;
        g_variant_iter_init(&iter, inner);
        const gchar *prop_name;
        GVariant *entry_val;

        while (g_variant_iter_next(&iter, "{&sv}", &prop_name, &entry_val)) {

            if (g_str_equal(prop_name, "Id")) {
                g_free(self->id);
                self->id = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "Title")) {
                g_free(self->title);
                self->title = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "Status")) {
                self->status = parse_status(g_variant_get_string(entry_val, NULL));
            } else if (g_str_equal(prop_name, "Category")) {
                self->category = parse_category(g_variant_get_string(entry_val, NULL));
            } else if (g_str_equal(prop_name, "IconName")) {
                g_free(self->icon_name);
                self->icon_name = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "Menu")) {
                g_free(self->menu_path);
                self->menu_path = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "OverlayIconName")) {
                g_free(self->overlay_icon_name);
                self->overlay_icon_name = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "AttentionIconName")) {
                g_free(self->attention_icon_name);
                self->attention_icon_name = g_variant_dup_string(entry_val, NULL);
            } else if (g_str_equal(prop_name, "ItemIsMenu")) {
                self->item_is_menu = g_variant_get_boolean(entry_val);
            } else if (g_str_equal(prop_name, "WindowId")) {
                self->window_id = g_variant_get_int32(entry_val);
            } else if (g_str_equal(prop_name, "IconPixmap")) {
                g_clear_object(&self->icon_pixbuf);
                if (g_variant_n_children(entry_val) > 0) {
                    g_autoptr(GVariant) first = g_variant_get_child_value(entry_val, 0);
                    self->icon_pixbuf = gsni_dbus_image_to_pixbuf(first);
                }
            }
            g_variant_unref(entry_val);
        }
    }

    self->loaded = TRUE;
    g_object_notify(G_OBJECT(self), "loaded");
}

static void
on_properties_changed(GDBusConnection *conn, const gchar *sender,
                      const gchar *path, const gchar *iface,
                      const gchar *signal_name, GVariant *params,
                      gpointer user_data)
{
    GsniHostItem *self = GSNI_HOST_ITEM(user_data);
    /* Re-fetch all properties on any change */
    gsni_host_item_load_properties(self);
}


static void
gsni_host_item_get_property(GObject *obj, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
    GsniHostItem *self = GSNI_HOST_ITEM(obj);
    switch (prop_id) {
    case PROP_ID:          g_value_set_string(value, self->id); break;
    case PROP_TITLE:       g_value_set_string(value, self->title); break;
    case PROP_STATUS:      g_value_set_enum(value, self->status); break;
    case PROP_CATEGORY:    g_value_set_enum(value, self->category); break;
    case PROP_ICON_NAME:   g_value_set_string(value, self->icon_name); break;
    case PROP_ICON_PIXBUF: g_value_set_object(value, self->icon_pixbuf); break;
    case PROP_MENU_PATH:   g_value_set_string(value, self->menu_path); break;
    case PROP_OVERLAY_ICON_NAME:    g_value_set_string(value, self->overlay_icon_name); break;
    case PROP_ATTENTION_ICON_NAME:  g_value_set_string(value, self->attention_icon_name); break;
    case PROP_ITEM_IS_MENU:         g_value_set_boolean(value, self->item_is_menu); break;
    case PROP_WINDOW_ID:   g_value_set_int(value, self->window_id); break;
    case PROP_SERVICE:     g_value_set_string(value, self->service); break;
    case PROP_LOADED:      g_value_set_boolean(value, self->loaded); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec); break;
    }
}

static void
gsni_host_item_finalize(GObject *object)
{
    GsniHostItem *self = GSNI_HOST_ITEM(object);
    g_free(self->service);
    g_free(self->bus_name);
    g_free(self->object_path);
    g_free(self->id);
    g_free(self->title);
    g_free(self->icon_name);
    g_clear_object(&self->icon_pixbuf);
    g_free(self->menu_path);
    g_free(self->overlay_icon_name);
    g_free(self->attention_icon_name);
    if (self->props_changed_sig) {
        g_dbus_connection_signal_unsubscribe(self->connection, self->props_changed_sig);
        self->props_changed_sig = 0;
    }
    g_clear_object(&self->connection);
    G_OBJECT_CLASS(gsni_host_item_parent_class)->finalize(object);
}

static void
gsni_host_item_class_init(GsniHostItemClass *klass)
{
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->get_property = gsni_host_item_get_property;
    oc->finalize = gsni_host_item_finalize;

    g_object_class_install_property(oc, PROP_ID,
        g_param_spec_string("id", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_TITLE,
        g_param_spec_string("title", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_STATUS,
        g_param_spec_enum("status", NULL, NULL, GSNI_TYPE_STATUS, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_CATEGORY,
        g_param_spec_enum("category", NULL, NULL, GSNI_TYPE_CATEGORY, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_ICON_NAME,
        g_param_spec_string("icon-name", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_ICON_PIXBUF,
        g_param_spec_object("icon-pixbuf", NULL, NULL, GDK_TYPE_PIXBUF, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_MENU_PATH,
        g_param_spec_string("menu-path", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_OVERLAY_ICON_NAME,
        g_param_spec_string("overlay-icon-name", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_ATTENTION_ICON_NAME,
        g_param_spec_string("attention-icon-name", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_ITEM_IS_MENU,
        g_param_spec_boolean("item-is-menu", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_WINDOW_ID,
        g_param_spec_int("window-id", NULL, NULL, 0, G_MAXINT32, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_SERVICE,
        g_param_spec_string("service", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(oc, PROP_LOADED,
        g_param_spec_boolean("loaded", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gsni_host_item_init(GsniHostItem *self)
{
    self->status = GSNI_STATUS_PASSIVE;
    self->category = GSNI_CATEGORY_APPLICATION_STATUS;
}


GsniHostItem *
gsni_host_item_new(const gchar *service, GDBusConnection *connection)
{
    g_return_val_if_fail(service != NULL, NULL);

    GsniHostItem *self = g_object_new(GSNI_TYPE_HOST_ITEM, NULL);
    self->connection = g_object_ref(connection);
    self->service = g_strdup(service);

    /* service format is "well-known-name" or "well-known-name/object-path".
     * Extract the bus name for signal subscriptions. */
    gchar *bus_name = g_strdup(service);
    const gchar *slash = strchr(bus_name, '/');
    if (slash) {
        self->object_path = g_strdup(slash);
        *((gchar *)slash) = '\0';
    } else {
        self->object_path = g_strdup("/StatusNotifierItem");
    }
    self->bus_name = g_strdup(bus_name);

    /* Subscribe to property changes — use bus_name only (no path suffix) */
    self->props_changed_sig = g_dbus_connection_signal_subscribe(
        connection, bus_name, PROPS_IFACE, "PropertiesChanged",
        self->object_path, SNI_IFACE, G_DBUS_SIGNAL_FLAGS_NONE,
        on_properties_changed, g_object_ref(self), g_object_unref);

    /* Load initial properties */
    gsni_host_item_load_properties(self);

    g_free(bus_name);
    return self;
}


/**
 * gsni_host_item_get_id:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the item ID or %NULL
 */
const gchar *
gsni_host_item_get_id(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->id;
}
/**
 * gsni_host_item_get_title:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the title or %NULL
 */
const gchar *
gsni_host_item_get_title(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->title;
}
/**
 * gsni_host_item_get_status:
 * @self: a #GsniHostItem
 *
 * Returns: the status
 */
GsniStatus
gsni_host_item_get_status(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), GSNI_STATUS_PASSIVE);
    return self->status;
}
/**
 * gsni_host_item_get_category:
 * @self: a #GsniHostItem
 *
 * Returns: the category
 */
GsniCategory
gsni_host_item_get_category(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), GSNI_CATEGORY_APPLICATION_STATUS);
    return self->category;
}
/**
 * gsni_host_item_get_icon_name:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the icon name or %NULL
 */
const gchar *
gsni_host_item_get_icon_name(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->icon_name;
}
/**
 * gsni_host_item_get_icon_pixbuf:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none): the icon pixbuf
 */
GdkPixbuf *
gsni_host_item_get_icon_pixbuf(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->icon_pixbuf;
}
/**
 * gsni_host_item_get_menu_path:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the DBusMenu object path or %NULL
 */
const gchar *
gsni_host_item_get_menu_path(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->menu_path;
}
/**
 * gsni_host_item_get_overlay_icon_name:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the overlay icon name or %NULL
 */
const gchar *
gsni_host_item_get_overlay_icon_name(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->overlay_icon_name;
}
/**
 * gsni_host_item_get_attention_icon_name:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none) (nullable): the attention icon name or %NULL
 */
const gchar *
gsni_host_item_get_attention_icon_name(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->attention_icon_name;
}
/**
 * gsni_host_item_get_item_is_menu:
 * @self: a #GsniHostItem
 *
 * Returns: %TRUE if the item acts as a DBusMenu provider
 */
gboolean
gsni_host_item_get_item_is_menu(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), FALSE);
    return self->item_is_menu;
}
/**
 * gsni_host_item_get_service:
 * @self: a #GsniHostItem
 *
 * Returns: (transfer none): the D-Bus service name
 */
const gchar *
gsni_host_item_get_service(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), NULL);
    return self->service;
}
/**
 * gsni_host_item_get_window_id:
 * @self: a #GsniHostItem
 *
 * Returns: the X11 window ID
 */
gint
gsni_host_item_get_window_id(GsniHostItem *self)
{
    g_return_val_if_fail(GSNI_IS_HOST_ITEM(self), 0);
    return self->window_id;
}
