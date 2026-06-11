/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 */

#include "gsni/gsni-enums.h"

GType
gsni_status_get_type(void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        static const GEnumValue values[] = {
            { GSNI_STATUS_PASSIVE,         "Passive status",          "Passive"         },
            { GSNI_STATUS_ACTIVE,          "Active status",           "Active"          },
            { GSNI_STATUS_NEEDS_ATTENTION, "Needs attention status",  "NeedsAttention"  },
            { 0, NULL, NULL }
        };
        type = g_enum_register_static("GsniStatus", values);
    }
    return type;
}

GType
gsni_category_get_type(void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        static const GEnumValue values[] = {
            { GSNI_CATEGORY_APPLICATION_STATUS, "Application status",   "ApplicationStatus" },
            { GSNI_CATEGORY_COMMUNICATIONS,     "Communications",       "Communications"    },
            { GSNI_CATEGORY_SYSTEM_SERVICES,    "System services",      "SystemServices"    },
            { GSNI_CATEGORY_HARDWARE,           "Hardware",             "Hardware"          },
            { 0, NULL, NULL }
        };
        type = g_enum_register_static("GsniCategory", values);
    }
    return type;
}

GType
gsni_scroll_orientation_get_type(void)
{
    static GType type = 0;
    if (G_UNLIKELY(type == 0)) {
        static const GEnumValue values[] = {
            { GSNI_SCROLL_UP,    "Scroll up",    "up"    },
            { GSNI_SCROLL_DOWN,  "Scroll down",  "down"  },
            { GSNI_SCROLL_LEFT,  "Scroll left",  "left"  },
            { GSNI_SCROLL_RIGHT, "Scroll right", "right" },
            { 0, NULL, NULL }
        };
        type = g_enum_register_static("GsniScrollOrientation", values);
    }
    return type;
}
