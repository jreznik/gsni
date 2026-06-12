/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHost — watches for and manages remote StatusNotifierItems.
 *
 * Registers as a StatusNotifierHost with the watcher, subscribes to
 * item registration signals, reads the RegisteredStatusNotifierItems
 * property on startup, and surfaces items as GsniHostItem proxies
 * through a GListModel.
 */

#include "gsni/gsni-host.h"
#include "gsni/gsni-host-item.h"
#include "gsni/gsni-enums.h"
#include "gsni/gsni-pixbuf.h"

#define WATCHER_NAME  "org.kde.StatusNotifierWatcher"
#define WATCHER_PATH  "/StatusNotifierWatcher"
#define WATCHER_IFACE "org.kde.StatusNotifierWatcher"
#define PROPS_IFACE   "org.freedesktop.DBus.Properties"

struct _GsniHost {
    GObject          parent_instance;
    GDBusConnection *connection;
    GDBusProxy      *watcher_proxy;
    GListStore      *items;
    gboolean         is_registered;

    guint            signal_item_registered;
    guint            signal_item_unregistered;
    guint            watch_id;

    /* Our own bus name (well-known) for host registration */
    gchar           *host_name;
};

G_DEFINE_FINAL_TYPE(GsniHost, gsni_host, G_TYPE_OBJECT)

enum {
    SIGNAL_ITEM_ADDED,
    SIGNAL_ITEM_REMOVED,
    N_SIGNALS
};

static guint host_signals[N_SIGNALS];

static void gsni_host_load_existing_items(GsniHost *self);

static GsniHostItem *
gsni_host_find_item(GsniHost *self, const gchar *service)
{
    guint n = g_list_model_get_n_items(G_LIST_MODEL(self->items));
    for (guint i = 0; i < n; i++) {
        g_autoptr(GsniHostItem) item =
            g_list_model_get_item(G_LIST_MODEL(self->items), i);
        if (g_str_equal(gsni_host_item_get_service(item), service))
            return item;
    }
    return NULL;
}

static void
on_item_registered(GDBusConnection *conn, const gchar *sender, const gchar *path,
                   const gchar *iface, const gchar *signal_name, GVariant *params,
                   gpointer user_data)
{
    GsniHost *self = GSNI_HOST(user_data);
    const gchar *service;
    g_variant_get(params, "(&s)", &service);

    if (gsni_host_find_item(self, service))
        return;

    GsniHostItem *item = gsni_host_item_new(service, self->connection);
    if (item) {
        g_list_store_append(self->items, item);
        g_signal_emit(self, host_signals[SIGNAL_ITEM_ADDED], 0, item);
        g_object_unref(item);
    }
}

static void
on_item_unregistered(GDBusConnection *conn, const gchar *sender, const gchar *path,
                     const gchar *iface, const gchar *signal_name, GVariant *params,
                     gpointer user_data)
{
    GsniHost *self = GSNI_HOST(user_data);
    const gchar *service;
    g_variant_get(params, "(&s)", &service);

    guint n = g_list_model_get_n_items(G_LIST_MODEL(self->items));
    for (guint i = 0; i < n; i++) {
        g_autoptr(GsniHostItem) item =
            g_list_model_get_item(G_LIST_MODEL(self->items), i);
        if (g_str_equal(gsni_host_item_get_service(item), service)) {
            g_signal_emit(self, host_signals[SIGNAL_ITEM_REMOVED], 0, item);
            g_list_store_remove(self->items, i);
            return;
        }
    }
}

static void
on_watcher_appeared(GDBusConnection *connection, const gchar *name,
                    const gchar *owner, gpointer user_data)
{
    GsniHost *self = user_data;
    GError *error = NULL;

    self->watcher_proxy = g_dbus_proxy_new_sync(connection,
        G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
        G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
        NULL, WATCHER_NAME, WATCHER_PATH, WATCHER_IFACE,
        NULL, &error);
    if (self->watcher_proxy == NULL) {
        g_warning("GsniHost: failed to create watcher proxy: %s", error->message);
        g_clear_error(&error);
        return;
    }

    /* Register ourselves as a host */
    GVariant *r = g_dbus_proxy_call_sync(self->watcher_proxy,
        "RegisterStatusNotifierHost",
        g_variant_new("(s)", self->host_name),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (r)
        g_variant_unref(r);
    else
        g_warning("RegisterStatusNotifierHost failed: %s", error->message);
    g_clear_error(&error);

    /* Subscribe to item registration signals */
    self->signal_item_registered = g_dbus_connection_signal_subscribe(
        connection, WATCHER_NAME, WATCHER_IFACE,
        "StatusNotifierItemRegistered", WATCHER_PATH, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_item_registered,
        g_object_ref(self), g_object_unref);
    self->signal_item_unregistered = g_dbus_connection_signal_subscribe(
        connection, WATCHER_NAME, WATCHER_IFACE,
        "StatusNotifierItemUnregistered", WATCHER_PATH, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_item_unregistered,
        g_object_ref(self), g_object_unref);

    gsni_host_load_existing_items(self);
}

static void
on_watcher_vanished(GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    GsniHost *self = user_data;
    g_clear_object(&self->watcher_proxy);

    if (self->signal_item_registered) {
        g_dbus_connection_signal_unsubscribe(connection, self->signal_item_registered);
        self->signal_item_registered = 0;
    }
    if (self->signal_item_unregistered) {
        g_dbus_connection_signal_unsubscribe(connection, self->signal_item_unregistered);
        self->signal_item_unregistered = 0;
    }
}

static void
gsni_host_load_existing_items(GsniHost *self)
{
    GError *error = NULL;
    GVariant *prop = g_dbus_proxy_call_sync(self->watcher_proxy,
        "org.freedesktop.DBus.Properties.Get", g_variant_new("(ss)",
            WATCHER_IFACE, "RegisteredStatusNotifierItems"),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if (prop == NULL) {
        g_warning("GsniHost: failed to read registered items: %s", error->message);
        g_clear_error(&error);
        return;
    }

    /* g_dbus_proxy_call_sync returns out-args tuple (v).
     * Properties.Get returns v, so proxy returns (v). */
    g_autoptr(GVariant) result = g_variant_get_child_value(prop, 0);
    g_variant_unref(prop);

    if (!g_variant_is_of_type(result, G_VARIANT_TYPE_VARIANT)) {
        g_warning("GsniHost: unexpected GetAll reply type");
        return;
    }
    g_autoptr(GVariant) inner = g_variant_get_variant(result);

    GVariantIter iter;
    g_variant_iter_init(&iter, inner);
    const gchar *service;
    while (g_variant_iter_next(&iter, "&s", &service)) {
        if (gsni_host_find_item(self, service))
            continue;
        GsniHostItem *item = gsni_host_item_new(service, self->connection);
        if (item) {
            g_list_store_append(self->items, item);
            g_signal_emit(self, host_signals[SIGNAL_ITEM_ADDED], 0, item);
            g_object_unref(item);
        }
    }
}


static void
gsni_host_dispose(GObject *object)
{
    GsniHost *self = GSNI_HOST(object);
    gsni_host_unregister(self);
    g_clear_object(&self->connection);
    g_clear_object(&self->items);
    G_OBJECT_CLASS(gsni_host_parent_class)->dispose(object);
}

static void
gsni_host_finalize(GObject *object)
{
    GsniHost *self = GSNI_HOST(object);
    g_free(self->host_name);
    G_OBJECT_CLASS(gsni_host_parent_class)->finalize(object);
}

static void
gsni_host_class_init(GsniHostClass *klass)
{
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->dispose = gsni_host_dispose;
    oc->finalize = gsni_host_finalize;

    host_signals[SIGNAL_ITEM_ADDED] = g_signal_new("item-added",
        GSNI_TYPE_HOST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
        G_TYPE_NONE, 1, GSNI_TYPE_HOST_ITEM);

    host_signals[SIGNAL_ITEM_REMOVED] = g_signal_new("item-removed",
        GSNI_TYPE_HOST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
        G_TYPE_NONE, 1, GSNI_TYPE_HOST_ITEM);
}

static void
gsni_host_init(GsniHost *self)
{
    self->items = g_list_store_new(GSNI_TYPE_HOST_ITEM);
}


GsniHost *
gsni_host_new(GDBusConnection *connection)
{
    GsniHost *self = g_object_new(GSNI_TYPE_HOST, NULL);
    self->connection = connection ? g_object_ref(connection)
                     : g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (self->connection == NULL) {
        g_warning("GsniHost: no session bus available");
        g_object_unref(self);
        return NULL;
    }

    /* Generate a well-known name for our host */
    self->host_name = g_strdup_printf("org.libgsni.Host-%u-%d",
        (guint)getpid(),
        (gint)g_random_int());

    return self;
}

gboolean
gsni_host_register(GsniHost *self, GError **error)
{
    g_return_val_if_fail(GSNI_IS_HOST(self), FALSE);

    if (self->is_registered)
        return TRUE;

    /* Watch for the watcher service */
    self->watch_id = g_bus_watch_name_on_connection(self->connection,
        WATCHER_NAME, G_BUS_NAME_WATCHER_FLAGS_NONE,
        on_watcher_appeared, on_watcher_vanished,
        g_object_ref(self), g_object_unref);

    self->is_registered = TRUE;
    return TRUE;
}

void
gsni_host_unregister(GsniHost *self)
{
    g_return_if_fail(GSNI_IS_HOST(self));
    if (!self->is_registered)
        return;

    if (self->watch_id) {
        g_bus_unwatch_name(self->watch_id);
        self->watch_id = 0;
    }
    if (self->signal_item_registered) {
        g_dbus_connection_signal_unsubscribe(self->connection,
            self->signal_item_registered);
        self->signal_item_registered = 0;
    }
    if (self->signal_item_unregistered) {
        g_dbus_connection_signal_unsubscribe(self->connection,
            self->signal_item_unregistered);
        self->signal_item_unregistered = 0;
    }
    g_clear_object(&self->watcher_proxy);
    self->is_registered = FALSE;
}

gboolean
gsni_host_get_is_registered(GsniHost *self)
{
    g_return_val_if_fail(GSNI_IS_HOST(self), FALSE);
    return self->is_registered;
}

/**
 * gsni_host_get_items:
 * @self: a #GsniHost
 *
 * Returns: (transfer none): the list of items
 */
GListModel *
gsni_host_get_items(GsniHost *self)
{
    g_return_val_if_fail(GSNI_IS_HOST(self), NULL);
    return G_LIST_MODEL(self->items);
}
