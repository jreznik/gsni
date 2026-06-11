/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Internal DBusMenu (com.canonical.dbusmenu) adapter.
 * Translates GMenuModel + GActionGroup → the DBusMenu wire protocol.
 */

#ifndef GSNI_DBUSMENU_H
#define GSNI_DBUSMENU_H

#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GsniDBusMenu GsniDBusMenu;

GsniDBusMenu *gsni_dbusmenu_new        (GDBusConnection *connection,
                                        const gchar     *object_path,
                                        GMenuModel      *menu,
                                        GActionGroup    *actions);

gboolean      gsni_dbusmenu_export     (GsniDBusMenu *self,
                                        GError      **error);
void          gsni_dbusmenu_unexport   (GsniDBusMenu *self);
void          gsni_dbusmenu_free       (GsniDBusMenu *self);

G_END_DECLS

#endif /* GSNI_DBUSMENU_H */
