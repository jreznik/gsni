/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * D-Bus vtable for org.kde.StatusNotifierItem.
 *
 * Parses the interface XML once (statically), registers the object at
 * the item's path, and dispatches method calls / property reads.
 */

#include "gsni-item-dbus.h"
#include "gsni-dbusmenu.h"
#include "gsni/gsni-pixbuf.h"
#include "gsni/gsni-enums.h"

/*
 * Embedded org.kde.StatusNotifierItem introspection XML.
 */
static const gchar sni_item_xml[] =
    "<node>"
    "  <interface name='org.kde.StatusNotifierItem'>"
    "    <property name='Category' type='s' access='read'/>"
    "    <property name='Id' type='s' access='read'/>"
    "    <property name='Title' type='s' access='read'/>"
    "    <property name='Status' type='s' access='read'/>"
    "    <property name='WindowId' type='i' access='read'/>"
    "    <property name='IconThemePath' type='s' access='read'/>"
    "    <property name='Menu' type='o' access='read'/>"
    "    <property name='ItemIsMenu' type='b' access='read'/>"
    "    <property name='IconName' type='s' access='read'/>"
    "    <property name='IconPixmap' type='a(iiay)' access='read'/>"
    "    <property name='OverlayIconName' type='s' access='read'/>"
    "    <property name='OverlayIconPixmap' type='a(iiay)' access='read'/>"
    "    <property name='AttentionIconName' type='s' access='read'/>"
    "    <property name='AttentionIconPixmap' type='a(iiay)' access='read'/>"
    "    <property name='AttentionMovieName' type='s' access='read'/>"
    "    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
    "    <method name='ContextMenu'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='Activate'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='SecondaryActivate'>"
    "      <arg type='i' name='x' direction='in'/>"
    "      <arg type='i' name='y' direction='in'/>"
    "    </method>"
    "    <method name='Scroll'>"
    "      <arg type='i' name='delta' direction='in'/>"
    "      <arg type='s' name='orientation' direction='in'/>"
    "    </method>"
    "    <method name='ProvideXdgActivationToken'>"
    "      <arg type='s' name='token' direction='in'/>"
    "    </method>"
    "    <signal name='NewTitle'/>"
    "    <signal name='NewIcon'/>"
    "    <signal name='NewAttentionIcon'/>"
    "    <signal name='NewOverlayIcon'/>"
    "    <signal name='NewMenu'/>"
    "    <signal name='NewToolTip'/>"
    "    <signal name='NewStatus'>"
    "      <arg type='s' name='status'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static GDBusInterfaceInfo *sni_iface_info = NULL;

struct _GsniItemDbus {
    GsniItem        *item;
    GDBusConnection *connection;
    gchar           *object_path;
    gchar           *menu_path;
    guint            reg_id;

    /* Menu export */
    GsniDBusMenu    *dbusmenu;
    guint            gio_menu_id;
    guint            gio_action_id;
};

static void
gsni_item_dbus_method_call(GDBusConnection       *connection,
                           const gchar           *sender,
                           const gchar           *path,
                           const gchar           *iface,
                           const gchar           *method,
                           GVariant              *params,
                           GDBusMethodInvocation *invocation,
                           gpointer               user_data)
{
    GsniItemDbus *self = user_data;

    if (g_str_equal(method, "Activate")) {
        gint x, y;
        g_variant_get(params, "(ii)", &x, &y);
        gsni_item_emit_activate(self->item, x, y);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else if (g_str_equal(method, "SecondaryActivate")) {
        gint x, y;
        g_variant_get(params, "(ii)", &x, &y);
        gsni_item_emit_secondary_activate(self->item, x, y);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else if (g_str_equal(method, "ContextMenu")) {
        /* Return UnknownMethod — same as ksni. Tells the host to
         * use the DBusMenu interface from the Menu property instead. */
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
            "Not supported, please use the Menu property");

    } else if (g_str_equal(method, "Scroll")) {
        gint delta;
        const gchar *orientation;
        g_variant_get(params, "(i&s)", &delta, &orientation);

        GsniScrollOrientation ori = GSNI_SCROLL_DOWN;
        if (g_str_equal(orientation, "horizontal")) {
            ori = (delta >= 0) ? GSNI_SCROLL_RIGHT : GSNI_SCROLL_LEFT;
        } else {
            ori = (delta >= 0) ? GSNI_SCROLL_DOWN : GSNI_SCROLL_UP;
        }

        gsni_item_emit_scroll(self->item, delta < 0 ? -delta : delta, ori);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else if (g_str_equal(method, "ProvideXdgActivationToken")) {
        const gchar *token;
        g_variant_get(params, "(&s)", &token);
        gsni_item_set_activation_token(self->item, token);
        g_dbus_method_invocation_return_value(invocation, NULL);

    } else {
        g_dbus_method_invocation_return_error(invocation,
            G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD,
            "Unknown method: %s", method);
    }
}

static GVariant *
gsni_item_dbus_get_property(GDBusConnection *connection,
                            const gchar     *sender,
                            const gchar     *path,
                            const gchar     *iface,
                            const gchar     *property,
                            GError         **error,
                            gpointer         user_data)
{
    GsniItemDbus *self = user_data;

    if (g_str_equal(property, "Category")) {
        GEnumValue *ev = g_enum_get_value(
            g_type_class_peek(GSNI_TYPE_CATEGORY),
            gsni_item_get_category(self->item));
        return g_variant_new_string(ev ? ev->value_nick : "ApplicationStatus");
    }

    if (g_str_equal(property, "Id")) {
        const gchar *id_str = gsni_item_get_id(self->item);
        return g_variant_new_string(id_str ? id_str : "");
    }

    if (g_str_equal(property, "Title")) {
        const gchar *t = gsni_item_get_title(self->item);
        return g_variant_new_string(t ? t : "");
    }

    if (g_str_equal(property, "Status")) {
        GEnumValue *ev = g_enum_get_value(
            g_type_class_peek(GSNI_TYPE_STATUS),
            gsni_item_get_status(self->item));
        return g_variant_new_string(ev ? ev->value_nick : "Passive");
    }

    if (g_str_equal(property, "WindowId"))
        return g_variant_new_int32(gsni_item_get_window_id(self->item));

    if (g_str_equal(property, "IconThemePath")) {
        const gchar *p = gsni_item_get_icon_theme_path(self->item);
        return g_variant_new_string(p ? p : "");
    }

    if (g_str_equal(property, "Menu"))
        return g_variant_new_object_path(self->menu_path);

    if (g_str_equal(property, "ItemIsMenu"))
        return g_variant_new_boolean(gsni_item_get_item_is_menu(self->item));

    if (g_str_equal(property, "IconName")) {
        const gchar *n = gsni_item_get_icon_name(self->item);
        return g_variant_new_string(n ? n : "");
    }

    if (g_str_equal(property, "IconPixmap")) {
        GdkPixbuf *pb = gsni_item_get_icon_pixbuf(self->item);
        return gsni_pixbuf_to_dbus_image_vector(pb);
    }

    if (g_str_equal(property, "OverlayIconName")) {
        const gchar *n = gsni_item_get_overlay_icon_name(self->item);
        return g_variant_new_string(n ? n : "");
    }

    if (g_str_equal(property, "OverlayIconPixmap"))
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);

    if (g_str_equal(property, "AttentionIconName")) {
        const gchar *n = gsni_item_get_attention_icon_name(self->item);
        return g_variant_new_string(n ? n : "");
    }

    if (g_str_equal(property, "AttentionIconPixmap"))
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);

    if (g_str_equal(property, "AttentionMovieName")) {
        const gchar *n = gsni_item_get_attention_movie_name(self->item);
        return g_variant_new_string(n ? n : "");
    }

    if (g_str_equal(property, "ToolTip")) {
        GsniToolTip *tip = gsni_item_get_tooltip(self->item);
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE("(sa(iiay)ss)"));
        g_variant_builder_add(&b, "s", tip ? (tip->icon_name ? tip->icon_name : "") : "");
        GVariantBuilder child;
        g_variant_builder_init(&child, G_VARIANT_TYPE("a(iiay)"));
        g_variant_builder_add_value(&b, g_variant_builder_end(&child));
        g_variant_builder_add(&b, "s", tip ? (tip->title ? tip->title : "") : "");
        g_variant_builder_add(&b, "s", tip ? (tip->subtitle ? tip->subtitle : "") : "");
        return g_variant_builder_end(&b);
    }

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                "Unknown property: %s", property);
    return NULL;
}

static const GDBusInterfaceVTable sni_vtable = {
    .method_call  = gsni_item_dbus_method_call,
    .get_property = gsni_item_dbus_get_property,
    .set_property = NULL,
};

static void
ensure_interface_info(void)
{
    if (sni_iface_info != NULL)
        return;

    GError *error = NULL;
    GDBusNodeInfo *node = g_dbus_node_info_new_for_xml(sni_item_xml, &error);
    if (node == NULL) {
        g_error("Failed to parse SNI item XML: %s", error->message);
        g_error_free(error);
        return;
    }

    sni_iface_info = g_dbus_node_info_lookup_interface(node,
        "org.kde.StatusNotifierItem");
    if (sni_iface_info == NULL)
        g_error("Missing org.kde.StatusNotifierItem in XML");

    g_dbus_interface_info_ref(sni_iface_info);
    g_dbus_node_info_unref(node);
}

GsniItemDbus *
gsni_item_dbus_new(GsniItem *item)
{
    GsniItemDbus *self;

    g_return_val_if_fail(GSNI_IS_ITEM(item), NULL);

    self = g_new0(GsniItemDbus, 1);
    self->item = item;

    self->connection = gsni_item_get_connection(item);
    self->object_path = g_strdup(gsni_item_get_object_path(item));
    self->menu_path = g_strdup("/MenuBar");

    return self;
}

gboolean
gsni_item_dbus_export(GsniItemDbus *self, GError **error)
{
    g_return_val_if_fail(self != NULL, FALSE);
    g_return_val_if_fail(self->connection != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    ensure_interface_info();

    self->reg_id = g_dbus_connection_register_object(
        self->connection, self->object_path, sni_iface_info,
        &sni_vtable, self, NULL, error);

    return self->reg_id != 0;
}

static void
clear_menu_exports(GsniItemDbus *self)
{
    if (self->dbusmenu) {
        gsni_dbusmenu_unexport(self->dbusmenu);
        gsni_dbusmenu_free(self->dbusmenu);
        self->dbusmenu = NULL;
    }
    if (self->gio_menu_id) {
        g_dbus_connection_unexport_menu_model(self->connection, self->gio_menu_id);
        self->gio_menu_id = 0;
    }
    if (self->gio_action_id) {
        g_dbus_connection_unexport_action_group(self->connection, self->gio_action_id);
        self->gio_action_id = 0;
    }
}

void
gsni_item_dbus_unexport(GsniItemDbus *self)
{
    g_return_if_fail(self != NULL);

    clear_menu_exports(self);

    if (self->reg_id) {
        g_dbus_connection_unregister_object(self->connection,
                                            self->reg_id);
        self->reg_id = 0;
    }
}

void
gsni_item_dbus_free(GsniItemDbus *self)
{
    g_return_if_fail(self != NULL);

    g_free(self->object_path);
    g_free(self->menu_path);
    g_free(self);
}

void
gsni_item_dbus_emit_signal(GsniItemDbus *self, const gchar *signal_name,
                           GVariant *parameters)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(signal_name != NULL);
    g_return_if_fail(self->connection != NULL);

    g_dbus_connection_emit_signal(self->connection,
                                  NULL,  /* destination: broadcast */
                                  self->object_path,
                                  "org.kde.StatusNotifierItem",
                                  signal_name,
                                  parameters,
                                  NULL);
}

void
gsni_item_dbus_update_menu(GsniItemDbus *self, GMenuModel *menu,
                           GActionGroup *actions)
{
    g_return_if_fail(self != NULL);

    clear_menu_exports(self);

    if (menu == NULL)
        return;

    GError *error = NULL;

    /* Export com.canonical.dbusmenu (our adapter) */
    self->dbusmenu = gsni_dbusmenu_new(self->connection, self->menu_path,
                                        menu, actions);
    gsni_dbusmenu_export(self->dbusmenu, &error);
    if (error) {
        g_warning("Failed to export dbusmenu: %s", error->message);
        g_clear_error(&error);
    }

    /* Export org.gtk.Menus (GIO native) */
    self->gio_menu_id = g_dbus_connection_export_menu_model(
        self->connection, self->menu_path, menu, &error);
    if (error) {
        g_warning("Failed to export GIO menus: %s", error->message);
        g_clear_error(&error);
    }

    /* Export org.gtk.Actions (GIO native) */
    if (actions) {
        self->gio_action_id = g_dbus_connection_export_action_group(
            self->connection, self->menu_path, actions, &error);
        if (error) {
            g_warning("Failed to export GIO actions: %s", error->message);
            g_clear_error(&error);
        }
    }
}
