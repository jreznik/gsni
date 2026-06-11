/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniHost — consumes SNI items from the session bus.
 *
 * This is for building system trays.  Register a GsniHost, and it
 * watches for StatusNotifierItems and surfaces them as GsniHostItem
 * proxy objects via a GListModel.
 */

#ifndef GSNI_HOST_H
#define GSNI_HOST_H

#include <gio/gio.h>
#include "gsni/gsni-enums.h"

G_BEGIN_DECLS

#define GSNI_TYPE_HOST (gsni_host_get_type())
G_DECLARE_FINAL_TYPE(GsniHost, gsni_host, GSNI, HOST, GObject)

GsniHost    *gsni_host_new               (GDBusConnection *connection);

gboolean     gsni_host_register          (GsniHost   *self,
                                          GError    **error);
void         gsni_host_unregister        (GsniHost   *self);

GListModel  *gsni_host_get_items         (GsniHost   *self);

G_END_DECLS

#endif /* GSNI_HOST_H */
