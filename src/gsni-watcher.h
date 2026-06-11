/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Internal watcher for the StatusNotifierWatcher lifecycle.
 */

#ifndef GSNI_WATCHER_H
#define GSNI_WATCHER_H

#include <gio/gio.h>
#include "gsni/gsni-item.h"

G_BEGIN_DECLS

typedef struct _GsniWatcher GsniWatcher;

GsniWatcher *gsni_watcher_get_for_connection(GDBusConnection *connection);

void gsni_watcher_register_item  (GsniWatcher *self, GsniItem *item,
                                   const gchar *bus_name);
void gsni_watcher_unregister_item(GsniWatcher *self, GsniItem *item);

G_END_DECLS

#endif /* GSNI_WATCHER_H */
