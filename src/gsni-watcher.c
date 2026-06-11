/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniWatcher — manages the org.kde.StatusNotifierWatcher lifecycle.
 *
 * One watcher per GDBusConnection.  Watches for the watcher service
 * appearing and disappearing, and registers/unregisters all GsniItem
 * instances on that connection.
 */

#include "gsni-watcher.h"
#include "gsni-item-dbus.h"

#define GSNI_WATCHER_NAME      "org.kde.StatusNotifierWatcher"
#define GSNI_WATCHER_PATH      "/StatusNotifierWatcher"
#define GSNI_WATCHER_IFACE     "org.kde.StatusNotifierWatcher"
#define GSNI_PROTOCOL_VERSION  0

static const gchar watcher_xml[] =
    "<node>"
    "  <interface name='org.kde.StatusNotifierWatcher'>"
    "    <property name='ProtocolVersion' type='i' access='read'/>"
    "    <property name='RegisteredStatusNotifierItems' type='as' access='read'/>"
    "    <property name='IsStatusNotifierHostRegistered' type='b' access='read'/>"
    "    <method name='RegisterStatusNotifierItem'>"
    "      <arg type='s' name='service' direction='in'/>"
    "    </method>"
    "    <method name='RegisterStatusNotifierHost'>"
    "      <arg type='s' name='service' direction='in'/>"
    "    </method>"
    "    <signal name='StatusNotifierItemRegistered'>"
    "      <arg type='s' name='service'/>"
    "    </signal>"
    "    <signal name='StatusNotifierItemUnregistered'>"
    "      <arg type='s' name='service'/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static GDBusInterfaceInfo *watcher_iface_info = NULL;
static GQuark watcher_quark = 0;

struct _GsniWatcher {
    GDBusConnection *connection;
    GDBusProxy      *proxy;
    guint            watch_id;
    GList           *items;         /* GsniItem * (borrowed) */
    gboolean         watcher_present;
};

static void register_pending_items(GsniWatcher *self);

static void
on_watcher_appeared(GDBusConnection *connection,
                    const gchar     *name,
                    const gchar     *owner,
                    gpointer         user_data)
{
    GsniWatcher *self = user_data;
    GError *error = NULL;

    if (watcher_iface_info == NULL) {
        GDBusNodeInfo *node = g_dbus_node_info_new_for_xml(watcher_xml,
                                                           &error);
        if (node == NULL) {
            g_warning("Failed to parse watcher XML: %s", error->message);
            g_clear_error(&error);
            return;
        }
        watcher_iface_info = g_dbus_node_info_lookup_interface(node,
            GSNI_WATCHER_IFACE);
        g_dbus_interface_info_ref(watcher_iface_info);
        g_dbus_node_info_unref(node);
    }

    self->proxy = g_dbus_proxy_new_sync(connection,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
        G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
        watcher_iface_info,
        GSNI_WATCHER_NAME,
        GSNI_WATCHER_PATH,
        GSNI_WATCHER_IFACE,
        NULL, &error);

    if (self->proxy == NULL) {
        g_warning("Failed to create watcher proxy: %s", error->message);
        g_clear_error(&error);
        return;
    }

    self->watcher_present = TRUE;
    register_pending_items(self);
}

static void
on_watcher_vanished(GDBusConnection *connection,
                    const gchar     *name,
                    gpointer         user_data)
{
    GsniWatcher *self = user_data;

    self->watcher_present = FALSE;

    g_clear_object(&self->proxy);

    /* Mark all items as disconnected */
    for (GList *l = self->items; l; l = l->next) {
        GsniItem *item = GSNI_ITEM(l->data);
        g_object_notify(G_OBJECT(item), "connected");
    }
}

static void
register_pending_items(GsniWatcher *self)
{
    for (GList *l = self->items; l; l = l->next) {
        GsniItem *item = GSNI_ITEM(l->data);

        GDBusConnection *conn = gsni_item_get_connection(item);
        const gchar *unique_name = g_dbus_connection_get_unique_name(conn);

        GError *error = NULL;
        GVariant *result = g_dbus_proxy_call_sync(self->proxy,
            "RegisterStatusNotifierItem",
            g_variant_new("(s)", unique_name),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

        if (result == NULL) {
            g_warning("Failed to register item: %s", error->message);
            g_clear_error(&error);
        } else {
            g_variant_unref(result);
            g_object_notify(G_OBJECT(item), "connected");
        }
    }
}

GsniWatcher *
gsni_watcher_get_for_connection(GDBusConnection *connection)
{
    g_return_val_if_fail(G_IS_DBUS_CONNECTION(connection), NULL);

    if (watcher_quark == 0)
        watcher_quark = g_quark_from_static_string("gsni-watcher");

    GsniWatcher *w = g_object_get_qdata(G_OBJECT(connection), watcher_quark);
    if (w != NULL)
        return w;

    w = g_new0(GsniWatcher, 1);
    w->connection = connection;

    w->watch_id = g_bus_watch_name_on_connection(connection,
        GSNI_WATCHER_NAME,
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        on_watcher_appeared,
        on_watcher_vanished,
        w, NULL);

    g_object_set_qdata_full(G_OBJECT(connection), watcher_quark, w,
                            g_free);

    return w;
}

void
gsni_watcher_register_item(GsniWatcher *self, GsniItem *item)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(GSNI_IS_ITEM(item));

    /* Avoid duplicates */
    if (g_list_find(self->items, item))
        return;

    self->items = g_list_prepend(self->items, item);

    if (self->watcher_present) {
        GDBusConnection *conn = gsni_item_get_connection(item);
        const gchar *unique = g_dbus_connection_get_unique_name(conn);

        GError *error = NULL;
        GVariant *result = g_dbus_proxy_call_sync(self->proxy,
            "RegisterStatusNotifierItem",
            g_variant_new("(s)", unique),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

        if (result == NULL) {
            g_warning("Failed to register item: %s", error->message);
            g_clear_error(&error);
        } else {
            g_variant_unref(result);
        }
    }
}

void
gsni_watcher_unregister_item(GsniWatcher *self, GsniItem *item)
{
    g_return_if_fail(self != NULL);
    g_return_if_fail(GSNI_IS_ITEM(item));

    self->items = g_list_remove(self->items, item);
}
