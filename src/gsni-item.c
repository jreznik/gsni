/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GsniItem — a StatusNotifierItem exported over D-Bus.
 *
 * This is a GObject that wraps the org.kde.StatusNotifierItem D-Bus
 * interface.  Set properties, register with the session bus, and the
 * system tray will show your icon.  Call gsni_item_register() when ready.
 */

#include "gsni/gsni-item.h"
#include "gsni/gsni-pixbuf.h"
#include "gsni-item-dbus.h"
#include "gsni-watcher.h"

/*
 * Protocol constants.
 */
#define GSNI_PROTOCOL_VERSION  0

struct _GsniItem
{
    GObject parent_instance;
};

enum {
    PROP_0,
    PROP_ID,
    PROP_CATEGORY,
    PROP_TITLE,
    PROP_STATUS,
    PROP_ICON_NAME,
    PROP_ICON_PIXBUF,
    PROP_OVERLAY_ICON_NAME,
    PROP_ATTENTION_ICON_NAME,
    PROP_ATTENTION_MOVIE_NAME,
    PROP_ITEM_IS_MENU,
    PROP_ICON_THEME_PATH,
    PROP_MENU,
    PROP_ACTION_GROUP,
    PROP_WINDOW_ID,
    PROP_CONNECTED,
    PROP_CONNECTION,
    N_PROPERTIES
};

enum {
    SIGNAL_ACTIVATE,
    SIGNAL_SECONDARY_ACTIVATE,
    SIGNAL_SCROLL,
    SIGNAL_QUIT_REQUESTED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

typedef struct {
    gchar            *id;
    gchar            *object_path;

    GsniCategory      category;
    GsniStatus        status;
    gchar            *title;
    gchar            *icon_name;
    GdkPixbuf        *icon_pixbuf;
    gchar            *overlay_icon_name;
    gchar            *attention_icon_name;
    gchar            *attention_movie_name;
    gboolean          item_is_menu;
    gchar            *icon_theme_path;
    GMenuModel       *menu;
    GActionGroup     *action_group;
    gint              window_id;

    GsniToolTip      *tooltip;

    GDBusConnection  *connection;
    gboolean          is_registered;

    GsniItemDbus     *dbus;
    GsniWatcher      *watcher;

    gchar            *activation_token;
} GsniItemPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GsniItem, gsni_item, G_TYPE_OBJECT)

/*
 * Work around the fact that G_DEFINE_TYPE_WITH_PRIVATE's getter
 * returns gpointer (void *) — GCC with strict C17 rejects -> on void *.
 */
#define PRIV(item) ((GsniItemPrivate *) gsni_item_get_instance_private(item))

static void
gsni_item_constructed(GObject *object)
{
    GsniItem *self = GSNI_ITEM(object);
    GsniItemPrivate *priv = PRIV(self);

    G_OBJECT_CLASS(gsni_item_parent_class)->constructed(object);

    if (priv->connection == NULL)
        priv->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

    if (priv->connection)
        g_object_ref(priv->connection);

    priv->object_path = g_strdup_printf("/org/libgsni/%s", priv->id);
}

static void
gsni_item_dispose(GObject *object)
{
    GsniItem *self = GSNI_ITEM(object);
    GsniItemPrivate *priv = PRIV(self);

    gsni_item_unregister(self);

    g_clear_object(&priv->menu);
    g_clear_object(&priv->action_group);
    g_clear_object(&priv->icon_pixbuf);
    g_clear_object(&priv->connection);

    G_OBJECT_CLASS(gsni_item_parent_class)->dispose(object);
}

static void
gsni_item_finalize(GObject *object)
{
    GsniItemPrivate *priv = gsni_item_get_instance_private(GSNI_ITEM(object));

    g_free(priv->id);
    g_free(priv->object_path);
    g_free(priv->title);
    g_free(priv->icon_name);
    g_free(priv->overlay_icon_name);
    g_free(priv->attention_icon_name);
    g_free(priv->attention_movie_name);
    g_free(priv->icon_theme_path);
    g_free(priv->activation_token);

    if (priv->tooltip) {
        gsni_tool_tip_free(priv->tooltip);
        priv->tooltip = NULL;
    }

    G_OBJECT_CLASS(gsni_item_parent_class)->finalize(object);
}

static void
gsni_item_get_property(GObject *object, guint prop_id,
                       GValue *value, GParamSpec *pspec)
{
    GsniItemPrivate *priv = gsni_item_get_instance_private(GSNI_ITEM(object));

    switch (prop_id) {
    case PROP_ID:
        g_value_set_string(value, priv->id);
        break;
    case PROP_CATEGORY: {
        GEnumValue *ev = g_enum_get_value(
            g_type_class_ref(GSNI_TYPE_CATEGORY), priv->category);
        g_value_set_string(value, ev ? ev->value_nick : "");
        break;
    }
    case PROP_TITLE:
        g_value_set_string(value, priv->title);
        break;
    case PROP_STATUS: {
        GEnumValue *ev = g_enum_get_value(
            g_type_class_ref(GSNI_TYPE_STATUS), priv->status);
        g_value_set_string(value, ev ? ev->value_nick : "");
        break;
    }
    case PROP_ICON_NAME:
        g_value_set_string(value, priv->icon_name);
        break;
    case PROP_ICON_PIXBUF:
        g_value_set_object(value, priv->icon_pixbuf);
        break;
    case PROP_OVERLAY_ICON_NAME:
        g_value_set_string(value, priv->overlay_icon_name);
        break;
    case PROP_ATTENTION_ICON_NAME:
        g_value_set_string(value, priv->attention_icon_name);
        break;
    case PROP_ATTENTION_MOVIE_NAME:
        g_value_set_string(value, priv->attention_movie_name);
        break;
    case PROP_ITEM_IS_MENU:
        g_value_set_boolean(value, priv->item_is_menu);
        break;
    case PROP_ICON_THEME_PATH:
        g_value_set_string(value, priv->icon_theme_path);
        break;
    case PROP_MENU:
        g_value_set_object(value, priv->menu);
        break;
    case PROP_ACTION_GROUP:
        g_value_set_object(value, priv->action_group);
        break;
    case PROP_WINDOW_ID:
        g_value_set_int(value, priv->window_id);
        break;
    case PROP_CONNECTED:
        g_value_set_boolean(value, priv->is_registered);
        break;
    case PROP_CONNECTION:
        g_value_set_object(value, priv->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gsni_item_set_property(GObject *object, guint prop_id,
                       const GValue *value, GParamSpec *pspec)
{
    GsniItem *self = GSNI_ITEM(object);
    GsniItemPrivate *priv = PRIV(self);

    switch (prop_id) {
    case PROP_ID:
        g_free(priv->id);
        priv->id = g_value_dup_string(value);
        break;
    case PROP_CATEGORY:
        gsni_item_set_category(self, g_value_get_enum(value));
        break;
    case PROP_TITLE:
        gsni_item_set_title(self, g_value_get_string(value));
        break;
    case PROP_STATUS:
        gsni_item_set_status(self, g_value_get_enum(value));
        break;
    case PROP_ICON_NAME:
        gsni_item_set_icon_name(self, g_value_get_string(value));
        break;
    case PROP_ICON_PIXBUF:
        gsni_item_set_icon_pixbuf(self, g_value_get_object(value));
        break;
    case PROP_OVERLAY_ICON_NAME:
        gsni_item_set_overlay_icon_name(self, g_value_get_string(value));
        break;
    case PROP_ATTENTION_ICON_NAME:
        gsni_item_set_attention_icon_name(self, g_value_get_string(value));
        break;
    case PROP_ATTENTION_MOVIE_NAME:
        gsni_item_set_attention_movie_name(self, g_value_get_string(value));
        break;
    case PROP_ITEM_IS_MENU:
        gsni_item_set_item_is_menu(self, g_value_get_boolean(value));
        break;
    case PROP_ICON_THEME_PATH:
        gsni_item_set_icon_theme_path(self, g_value_get_string(value));
        break;
    case PROP_MENU:
        gsni_item_set_menu(self, G_MENU_MODEL(g_value_get_object(value)));
        break;
    case PROP_ACTION_GROUP:
        gsni_item_set_action_group(self, G_ACTION_GROUP(g_value_get_object(value)));
        break;
    case PROP_WINDOW_ID:
        gsni_item_set_window_id(self, g_value_get_int(value));
        break;
    case PROP_CONNECTION:
        priv->connection = g_value_dup_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gsni_item_class_init(GsniItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->constructed = gsni_item_constructed;
    object_class->dispose = gsni_item_dispose;
    object_class->finalize = gsni_item_finalize;
    object_class->get_property = gsni_item_get_property;
    object_class->set_property = gsni_item_set_property;

    g_object_class_install_property(object_class, PROP_ID,
        g_param_spec_string("id", "ID", "Unique item identifier",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                            G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_CATEGORY,
        g_param_spec_enum("category", "Category", "Item category",
                          GSNI_TYPE_CATEGORY, GSNI_CATEGORY_APPLICATION_STATUS,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_TITLE,
        g_param_spec_string("title", "Title", "Item title",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_STATUS,
        g_param_spec_enum("status", "Status", "Item status",
                          GSNI_TYPE_STATUS, GSNI_STATUS_PASSIVE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ICON_NAME,
        g_param_spec_string("icon-name", "Icon name", "Themed icon name",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ICON_PIXBUF,
        g_param_spec_object("icon-pixbuf", "Icon pixbuf", "Icon as pixbuf",
                            GDK_TYPE_PIXBUF,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_OVERLAY_ICON_NAME,
        g_param_spec_string("overlay-icon-name", "Overlay icon name",
                            "Overlay icon theme name",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ATTENTION_ICON_NAME,
        g_param_spec_string("attention-icon-name", "Attention icon name",
                            "Attention icon theme name",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ATTENTION_MOVIE_NAME,
        g_param_spec_string("attention-movie-name", "Attention movie name",
                            "Attention movie path",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ITEM_IS_MENU,
        g_param_spec_boolean("item-is-menu", "Item is menu",
                             "Only context menu, no activation",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ICON_THEME_PATH,
        g_param_spec_string("icon-theme-path", "Icon theme path",
                            "Extra icon theme search path",
                            NULL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_MENU,
        g_param_spec_object("menu", "Menu", "Context menu model",
                            G_TYPE_MENU_MODEL,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_ACTION_GROUP,
        g_param_spec_object("action-group", "Action group",
                            "Actions for menu items",
                            G_TYPE_ACTION_GROUP,
                            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_WINDOW_ID,
        g_param_spec_int("window-id", "Window ID",
                         "X11 window ID for the associated window",
                         0, G_MAXINT32, 0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_CONNECTED,
        g_param_spec_boolean("connected", "Connected",
                             "Whether registered with the watcher",
                             FALSE,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(object_class, PROP_CONNECTION,
        g_param_spec_object("connection", "Connection",
                            "GDBusConnection for D-Bus communication",
                            G_TYPE_DBUS_CONNECTION,
                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                            G_PARAM_STATIC_STRINGS));

    signals[SIGNAL_ACTIVATE] = g_signal_new(
        "activate", GSNI_TYPE_ITEM, G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

    signals[SIGNAL_SECONDARY_ACTIVATE] = g_signal_new(
        "secondary-activate", GSNI_TYPE_ITEM, G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);

    signals[SIGNAL_SCROLL] = g_signal_new(
        "scroll", GSNI_TYPE_ITEM, G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT,
        GSNI_TYPE_SCROLL_ORIENTATION);

    signals[SIGNAL_QUIT_REQUESTED] = g_signal_new(
        "quit-requested", GSNI_TYPE_ITEM, G_SIGNAL_RUN_LAST,
        0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gsni_item_init(GsniItem *self)
{
    GsniItemPrivate *priv = PRIV(self);
    priv->category = GSNI_CATEGORY_APPLICATION_STATUS;
    priv->status = GSNI_STATUS_PASSIVE;
}

/* --- public API --- */

GsniItem *
gsni_item_new(const gchar *id, GDBusConnection *connection, GError **error)
{
    g_return_val_if_fail(id != NULL, NULL);
    g_return_val_if_fail(connection == NULL || G_IS_DBUS_CONNECTION(connection), NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    return g_object_new(GSNI_TYPE_ITEM,
                        "id", id,
                        "connection", connection,
                        NULL);
}

gboolean
gsni_item_register(GsniItem *self, GError **error)
{
    GsniItemPrivate *priv;

    g_return_val_if_fail(GSNI_IS_ITEM(self), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    priv = PRIV(self);

    if (priv->is_registered)
        return TRUE;

    if (priv->connection == NULL) {
        g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
                            "No D-Bus connection available");
        return FALSE;
    }

    if (priv->dbus == NULL) {
        priv->dbus = gsni_item_dbus_new(self);
        if (!gsni_item_dbus_export(priv->dbus, error))
            return FALSE;
    }

    if (priv->watcher == NULL) {
        priv->watcher = gsni_watcher_get_for_connection(priv->connection);
    }

    gsni_watcher_register_item(priv->watcher, self);

    priv->is_registered = TRUE;
    g_object_notify(G_OBJECT(self), "connected");

    return TRUE;
}

void
gsni_item_unregister(GsniItem *self)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (!priv->is_registered)
        return;

    if (priv->watcher)
        gsni_watcher_unregister_item(priv->watcher, self);

    if (priv->dbus) {
        gsni_item_dbus_unexport(priv->dbus);
        gsni_item_dbus_free(priv->dbus);
        priv->dbus = NULL;
    }

    priv->is_registered = FALSE;
    g_object_notify(G_OBJECT(self), "connected");
}

gboolean
gsni_item_get_connected(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), FALSE);
    return PRIV(self)->is_registered;
}

void
gsni_item_set_title(GsniItem *self, const gchar *title)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->title, title) == 0)
        return;

    g_free(priv->title);
    priv->title = g_strdup(title);
    g_object_notify(G_OBJECT(self), "title");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewTitle", NULL);
}

const gchar *
gsni_item_get_title(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->title;
}

void
gsni_item_set_category(GsniItem *self, GsniCategory category)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);
    priv->category = category;
    g_object_notify(G_OBJECT(self), "category");
}

GsniCategory
gsni_item_get_category(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), GSNI_CATEGORY_APPLICATION_STATUS);
    return PRIV(self)->category;
}

void
gsni_item_set_status(GsniItem *self, GsniStatus status)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (priv->status == status)
        return;

    priv->status = status;
    g_object_notify(G_OBJECT(self), "status");

    if (priv->dbus) {
        GEnumValue *ev = g_enum_get_value(
            g_type_class_ref(GSNI_TYPE_STATUS), status);
        GVariant *body = g_variant_new("(s)", ev ? ev->value_nick : "Passive");
        gsni_item_dbus_emit_signal(priv->dbus, "NewStatus", body);
    }
}

GsniStatus
gsni_item_get_status(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), GSNI_STATUS_PASSIVE);
    return PRIV(self)->status;
}

void
gsni_item_set_icon_name(GsniItem *self, const gchar *name)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->icon_name, name) == 0)
        return;

    g_free(priv->icon_name);
    priv->icon_name = g_strdup(name);
    g_object_notify(G_OBJECT(self), "icon-name");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewIcon", NULL);
}

const gchar *
gsni_item_get_icon_name(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->icon_name;
}

void
gsni_item_set_icon_pixbuf(GsniItem *self, GdkPixbuf *pixbuf)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (pixbuf)
        g_object_ref(pixbuf);
    g_clear_object(&priv->icon_pixbuf);
    priv->icon_pixbuf = pixbuf;

    g_object_notify(G_OBJECT(self), "icon-pixbuf");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewIcon", NULL);
}

GdkPixbuf *
gsni_item_get_icon_pixbuf(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->icon_pixbuf;
}

void
gsni_item_set_overlay_icon_name(GsniItem *self, const gchar *name)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->overlay_icon_name, name) == 0)
        return;

    g_free(priv->overlay_icon_name);
    priv->overlay_icon_name = g_strdup(name);
    g_object_notify(G_OBJECT(self), "overlay-icon-name");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewOverlayIcon", NULL);
}

const gchar *
gsni_item_get_overlay_icon_name(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->overlay_icon_name;
}

void
gsni_item_set_attention_icon_name(GsniItem *self, const gchar *name)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->attention_icon_name, name) == 0)
        return;

    g_free(priv->attention_icon_name);
    priv->attention_icon_name = g_strdup(name);
    g_object_notify(G_OBJECT(self), "attention-icon-name");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewAttentionIcon", NULL);
}

const gchar *
gsni_item_get_attention_icon_name(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->attention_icon_name;
}

void
gsni_item_set_attention_movie_name(GsniItem *self, const gchar *name)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->attention_movie_name, name) == 0)
        return;

    g_free(priv->attention_movie_name);
    priv->attention_movie_name = g_strdup(name);
    g_object_notify(G_OBJECT(self), "attention-movie-name");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewAttentionIcon", NULL);
}

const gchar *
gsni_item_get_attention_movie_name(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->attention_movie_name;
}

void
gsni_item_set_item_is_menu(GsniItem *self, gboolean is_menu)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (priv->item_is_menu == is_menu)
        return;

    priv->item_is_menu = is_menu;
    g_object_notify(G_OBJECT(self), "item-is-menu");
}

gboolean
gsni_item_get_item_is_menu(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), FALSE);
    return PRIV(self)->item_is_menu;
}

void
gsni_item_set_icon_theme_path(GsniItem *self, const gchar *path)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (g_strcmp0(priv->icon_theme_path, path) == 0)
        return;

    g_free(priv->icon_theme_path);
    priv->icon_theme_path = g_strdup(path);
    g_object_notify(G_OBJECT(self), "icon-theme-path");

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewIconThemePath",
                                   g_variant_new("(s)", path ? path : ""));
}

const gchar *
gsni_item_get_icon_theme_path(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->icon_theme_path;
}

void
gsni_item_set_menu(GsniItem *self, GMenuModel *menu)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if ((GObject *)priv->menu == (GObject *)menu)
        return;

    if (priv->menu)
        g_object_unref(priv->menu);
    priv->menu = menu ? g_object_ref(menu) : NULL;
    g_object_notify(G_OBJECT(self), "menu");

    if (priv->dbus) {
        gsni_item_dbus_update_menu(priv->dbus, menu, priv->action_group);
        gsni_item_dbus_emit_signal(priv->dbus, "NewMenu", NULL);
    }
}

GMenuModel *
gsni_item_get_menu(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->menu;
}

void
gsni_item_set_action_group(GsniItem *self, GActionGroup *actions)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if ((GObject *)priv->action_group == (GObject *)actions)
        return;

    if (priv->action_group)
        g_object_unref(priv->action_group);
    priv->action_group = actions ? g_object_ref(actions) : NULL;
    g_object_notify(G_OBJECT(self), "action-group");

    if (priv->dbus && priv->menu) {
        gsni_item_dbus_update_menu(priv->dbus, priv->menu, actions);
    }
}

GActionGroup *
gsni_item_get_action_group(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->action_group;
}

void
gsni_item_set_tooltip(GsniItem *self, const gchar *icon_name,
                      const gchar *title, const gchar *subtitle)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);

    if (priv->tooltip)
        gsni_tool_tip_free(priv->tooltip);

    priv->tooltip = g_new(GsniToolTip, 1);
    priv->tooltip->icon_name = g_strdup(icon_name);
    priv->tooltip->icon_pixbuf = NULL;
    priv->tooltip->title = g_strdup(title);
    priv->tooltip->subtitle = g_strdup(subtitle);

    if (priv->dbus)
        gsni_item_dbus_emit_signal(priv->dbus, "NewToolTip", NULL);
}

GsniToolTip *
gsni_item_get_tooltip(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->tooltip;
}

void
gsni_item_set_window_id(GsniItem *self, gint window_id)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);
    priv->window_id = window_id;
    g_object_notify(G_OBJECT(self), "window-id");
}

gint
gsni_item_get_window_id(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), 0);
    return PRIV(self)->window_id;
}

void
gsni_item_show_notification(GsniItem *self, const gchar *title,
                            const gchar *body, const gchar *icon_name,
                            gint timeout_ms)
{
    GsniItemPrivate *priv;
    g_autoptr(GDBusConnection) conn = NULL;
    g_autoptr(GVariant) result = NULL;
    GError *error = NULL;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);
    conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (conn == NULL) {
        g_warning("Cannot get session bus for notification: %s", error->message);
        g_clear_error(&error);
        return;
    }

    GVariantBuilder hints;
    g_variant_builder_init(&hints, G_VARIANT_TYPE_VARDICT);

    const gchar *display_title = priv->title ? priv->title :
                                 g_get_application_name();

    result = g_dbus_connection_call_sync(conn,
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify",
        g_variant_new("(susssasa{sv}i)",
                      display_title ? display_title : "",
                      (guint32)0,
                      icon_name ? icon_name : "",
                      title ? title : "",
                      body ? body : "",
                      NULL,  /* actions */
                      &hints,
                      timeout_ms),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);

    if (result == NULL) {
        g_warning("Failed to show notification: %s", error->message);
        g_clear_error(&error);
    }
}

const gchar *
gsni_item_get_activation_token(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->activation_token;
}

/* --- internal API (used by gsni-item-dbus and gsni-watcher) --- */

GsniItemDbus *
gsni_item_get_dbus(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->dbus;
}

GDBusConnection *
gsni_item_get_connection(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->connection;
}

const gchar *
gsni_item_get_object_path(GsniItem *self)
{
    g_return_val_if_fail(GSNI_IS_ITEM(self), NULL);
    return PRIV(self)->object_path;
}

void
gsni_item_set_activation_token(GsniItem *self, const gchar *token)
{
    GsniItemPrivate *priv;

    g_return_if_fail(GSNI_IS_ITEM(self));

    priv = PRIV(self);
    g_free(priv->activation_token);
    priv->activation_token = g_strdup(token);
}

void
gsni_item_emit_activate(GsniItem *self, gint x, gint y)
{
    g_return_if_fail(GSNI_IS_ITEM(self));
    g_signal_emit(self, signals[SIGNAL_ACTIVATE], 0, x, y);
}

void
gsni_item_emit_secondary_activate(GsniItem *self, gint x, gint y)
{
    g_return_if_fail(GSNI_IS_ITEM(self));
    g_signal_emit(self, signals[SIGNAL_SECONDARY_ACTIVATE], 0, x, y);
}

void
gsni_item_emit_scroll(GsniItem *self, gint delta, GsniScrollOrientation ori)
{
    g_return_if_fail(GSNI_IS_ITEM(self));
    g_signal_emit(self, signals[SIGNAL_SCROLL], 0, delta, ori);
}
