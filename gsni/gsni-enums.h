/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Gsni enumerated types (Category, Status, ScrollOrientation).
 */

#ifndef GSNI_ENUMS_H
#define GSNI_ENUMS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSNI_TYPE_STATUS             (gsni_status_get_type())
#define GSNI_TYPE_CATEGORY           (gsni_category_get_type())
#define GSNI_TYPE_SCROLL_ORIENTATION  (gsni_scroll_orientation_get_type())

typedef enum {
    GSNI_STATUS_PASSIVE,
    GSNI_STATUS_ACTIVE,
    GSNI_STATUS_NEEDS_ATTENTION,
} GsniStatus;

GType gsni_status_get_type(void) G_GNUC_CONST;

typedef enum {
    GSNI_CATEGORY_APPLICATION_STATUS,
    GSNI_CATEGORY_COMMUNICATIONS,
    GSNI_CATEGORY_SYSTEM_SERVICES,
    GSNI_CATEGORY_HARDWARE,
} GsniCategory;

GType gsni_category_get_type(void) G_GNUC_CONST;

typedef enum {
    GSNI_SCROLL_UP,
    GSNI_SCROLL_DOWN,
    GSNI_SCROLL_LEFT,
    GSNI_SCROLL_RIGHT,
} GsniScrollOrientation;

GType gsni_scroll_orientation_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif /* GSNI_ENUMS_H */
