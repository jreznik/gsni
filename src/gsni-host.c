/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHost — consumes/watches SNI items on the session bus.
 */

#include "gsni/gsni-host.h"
#include "gsni/gsni-host-item.h"

#define GSNI_WATCHER_NAME  "org.kde.StatusNotifierWatcher"
#define GSNI_WATCHER_PATH  "/StatusNotifierWatcher"
#define GSNI_WATCHER_IFACE "org.kde.StatusNotifierWatcher"

struct _GsniHost {
    GObject          parent_instance;
    GDBusConnection *connection;
    GDBusProxy      *watcher_proxy;
    GListStore      *items;          /* GsniHostItem */
    guint            signal_sub_id;
};

G_DEFINE_FINAL_TYPE(GsniHost, gsni_host, G_TYPE_OBJECT)

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
gsni_host_class_init(GsniHostClass *klass)
{
    GObjectClass *oc = G_OBJECT_CLASS(klass);
    oc->dispose = gsni_host_dispose;
}

static void
gsni_host_init(GsniHost *self)
{
    self->items = g_list_store_new(GSNI_TYPE_HOST_ITEM);
}

GsniHost *
gsni_host_new(GDBusConnection *connection)
{
    g_return_val_if_fail(connection == NULL || G_IS_DBUS_CONNECTION(connection),
                         NULL);

    GsniHost *self = g_object_new(GSNI_TYPE_HOST, NULL);
    self->connection = connection ? g_object_ref(connection)
                     : g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    return self;
}

gboolean
gsni_host_register(GsniHost *self, GError **error)
{
    g_return_val_if_fail(GSNI_IS_HOST(self), FALSE);

    /* TODO: register as a StatusNotifierHost with the watcher,
     * subscribe to StatusNotifierItemRegistered/unregistered signals,
     * and create GsniHostItem proxies */

    return TRUE;
}

void
gsni_host_unregister(GsniHost *self)
{
    g_return_if_fail(GSNI_IS_HOST(self));

    if (self->signal_sub_id) {
        g_dbus_connection_signal_unsubscribe(self->connection,
                                              self->signal_sub_id);
        self->signal_sub_id = 0;
    }
}

GListModel *
gsni_host_get_items(GsniHost *self)
{
    g_return_val_if_fail(GSNI_IS_HOST(self), NULL);
    return G_LIST_MODEL(self->items);
}
