/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GTK4 system tray host example.
 *
 * Left-click activates the item. Right-click shows its DBusMenu context menu.
 */

#include <gtk/gtk.h>
#include "gsni/gsni.h"
#include "gsni/gsni-host.h"
#include "gsni/gsni-host-item.h"

typedef struct {
    GtkWindow     *window;
    GtkBox        *tray_box;
    GsniHost      *host;
    GHashTable    *item_widgets;
} AppData;

static void add_tray_icon(AppData *app, GsniHostItem *item);
static void remove_tray_icon(AppData *app, GsniHostItem *item);
static void load_existing_items(AppData *app);

/* ── DBusMenu helpers ──────────────────────────────────────── */

static GVariant *
fetch_menu_layout(const gchar *bus_name, const gchar *menu_path)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) return NULL;

    GVariantBuilder filter;
    g_variant_builder_init(&filter, G_VARIANT_TYPE_STRING_ARRAY);
    const gchar *props[] = {"type","label","enabled","children-display",
                            "toggle-type","toggle-state", NULL};
    for (const gchar **p = props; *p; p++)
        g_variant_builder_add(&filter, "s", *p);

    GError *err = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        conn, bus_name, menu_path, "com.canonical.dbusmenu", "GetLayout",
        g_variant_new("(ii@as)", 0, -1, g_variant_builder_end(&filter)),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (!result) { g_clear_error(&err); }
    g_object_unref(conn);
    return result;
}

static void
trigger_menu_event(const gchar *bus_name, const gchar *menu_path, gint id)
{
    if (!bus_name || !bus_name[0] || !menu_path) return;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) return;
    g_dbus_connection_call_sync(conn, bus_name, menu_path,
        "com.canonical.dbusmenu", "Event",
        g_variant_new("(i&s@vu)", id, "clicked",
                      g_variant_new_variant(g_variant_new_string("")), 0U),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
    g_object_unref(conn);
}

/* ── Custom popover menu built from DBusMenu layout ────────── */

typedef struct {
    gchar       *bus_name;
    gchar       *menu_path;
    gint         item_id;
} MenuCallbackData;

static void
menu_callback_data_free(gpointer data)
{
    MenuCallbackData *mcd = data;
    g_free(mcd->bus_name);
    g_free(mcd->menu_path);
    g_free(mcd);
}

static void
on_popover_item_clicked(GtkGestureClick *gesture, gint np, gdouble x, gdouble y,
                        gpointer data)
{
    MenuCallbackData *mcd = data;
    trigger_menu_event(mcd->bus_name, mcd->menu_path, mcd->item_id);

    GtkWidget *popover = GTK_WIDGET(
        g_object_get_data(G_OBJECT(gesture), "popover"));
    if (popover)
        gtk_popover_popdown(GTK_POPOVER(popover));
}

static void
add_menu_item_to_box(GtkBox *box, const gchar *label, gboolean enabled,
                     const gchar *bus_name, const gchar *menu_path,
                     gint item_id, GtkWidget *popover)
{
    GtkWidget *btn = gtk_button_new_with_label(label ? label : "");
    gtk_widget_set_sensitive(btn, enabled);
    gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
    gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);

    MenuCallbackData *mcd = g_new(MenuCallbackData, 1);
    mcd->bus_name = g_strdup(bus_name);
    mcd->menu_path = g_strdup(menu_path);
    mcd->item_id = item_id;

    GtkGesture *click = gtk_gesture_click_new();
    g_object_set_data(G_OBJECT(click), "popover", popover);
    g_signal_connect_data(click, "pressed",
                          G_CALLBACK(on_popover_item_clicked), mcd,
                          (GClosureNotify)menu_callback_data_free, 0);
    gtk_widget_add_controller(btn, GTK_EVENT_CONTROLLER(click));

    gtk_box_append(box, btn);
}

static void
populate_popover_from_layout(GtkBox *box, GVariant *layout_node,
                             const gchar *bus_name, const gchar *menu_path,
                             GtkWidget *popover, gboolean is_root)
{
    gint32 id;
    g_autoptr(GVariant) props = g_variant_get_child_value(layout_node, 1);
    g_autoptr(GVariant) children = g_variant_get_child_value(layout_node, 2);
    id = g_variant_get_int32(g_variant_get_child_value(layout_node, 0));

    const gchar *type_str = "standard";
    gchar *label_str = NULL;
    gboolean enabled = TRUE;

    {
        GVariantIter iter;
        const gchar *key;
        GVariant *val;
        g_variant_iter_init(&iter, props);
        while (g_variant_iter_next(&iter, "{&sv}", &key, &val)) {
            if (g_str_equal(key, "type"))
                type_str = g_variant_get_string(val, NULL);
            else if (g_str_equal(key, "label"))
                label_str = g_variant_dup_string(val, NULL);
            else if (g_str_equal(key, "enabled"))
                enabled = g_variant_get_boolean(val);
            g_variant_unref(val);
        }
    }

    guint n_children = g_variant_n_children(children);

    if (is_root) {
        for (guint i = 0; i < n_children; i++) {
            g_autoptr(GVariant) child = g_variant_get_child_value(children, i);
            /* Children in av array are wrapped in v — unwrap */
            g_autoptr(GVariant) unwrapped = g_variant_get_variant(child);
            populate_popover_from_layout(box, unwrapped, bus_name, menu_path,
                                         popover, FALSE);
        }
        g_free(label_str);
        return;
    }

    if (g_str_equal(type_str, "separator")) {
        GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_append(box, sep);
        g_free(label_str);
        return;
    }

    if (n_children > 0) {
        GtkWidget *sub_label = gtk_label_new(label_str ? label_str : "");
        gtk_widget_set_halign(sub_label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(sub_label, 8);
        gtk_box_append(box, sub_label);

        for (guint i = 0; i < n_children; i++) {
            g_autoptr(GVariant) child = g_variant_get_child_value(children, i);
            g_autoptr(GVariant) unwrapped = g_variant_get_variant(child);
            populate_popover_from_layout(box, unwrapped, bus_name, menu_path,
                                         popover, FALSE);
        }

        GtkWidget *sep2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_append(box, sep2);
    } else {
        add_menu_item_to_box(box, label_str, enabled, bus_name, menu_path,
                             id, popover);
    }
    g_free(label_str);
}

static void
show_context_menu(GsniHostItem *item, GtkWidget *row, gdouble x, gdouble y)
{
    const gchar *service = gsni_host_item_get_service(item);
    gchar *bus_name = g_strdup(service);
    gchar *slash = strchr(bus_name, '/');
    if (slash) *slash = '\0';

    const gchar *menu_path = gsni_host_item_get_menu_path(item);
    if (!menu_path || !menu_path[0]) { g_free(bus_name); return; }

    GVariant *reply = fetch_menu_layout(bus_name, menu_path);
    if (!reply) { g_free(bus_name); return; }

    g_autoptr(GVariant) layout = g_variant_get_child_value(reply, 1);
    g_variant_unref(reply);

    GtkWidget *popover = gtk_popover_new();
    gtk_popover_set_pointing_to(GTK_POPOVER(popover),
        &(GdkRectangle){ (gint)x, (gint)y, 1, 1 });
    gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(popover), box);

    populate_popover_from_layout(GTK_BOX(box), layout, bus_name, menu_path,
                                 popover, TRUE);

    gtk_widget_set_parent(popover, row);
    gtk_popover_popup(GTK_POPOVER(popover));
    g_free(bus_name);
}

/* ── Click handlers ────────────────────────────────────────── */

static void
activate_item(GsniHostItem *item, gdouble x, gdouble y)
{
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn) return;

    const gchar *service = gsni_host_item_get_service(item);
    gchar *bus_name = g_strdup(service);
    gchar *slash = strchr(bus_name, '/');
    if (slash) *slash = '\0';

    GError *err = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        conn, bus_name, "/StatusNotifierItem",
        "org.kde.StatusNotifierItem", "Activate",
        g_variant_new("(ii)", (gint)x, (gint)y),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (result) g_variant_unref(result);
    else { g_warning("Activate failed: %s", err->message); g_clear_error(&err); }
    g_free(bus_name);
    g_object_unref(conn);
}

static void
on_icon_pressed(GtkGestureClick *gesture, gint n_press,
                gdouble x, gdouble y, gpointer data)
{
    GsniHostItem *item = GSNI_HOST_ITEM(data);
    guint button = gtk_gesture_single_get_current_button(
        GTK_GESTURE_SINGLE(gesture));
    GtkWidget *row = gtk_event_controller_get_widget(
        GTK_EVENT_CONTROLLER(gesture));

    if (button == GDK_BUTTON_SECONDARY)
        show_context_menu(item, row, x, y);
    else
        activate_item(item, x, y);
}

/* ── Item lifecycle ────────────────────────────────────────── */

static void
on_item_added(GsniHost *host, GsniHostItem *item, gpointer data)
{
    AppData *app = data;
    g_print("Item added: %s\n", gsni_host_item_get_id(item));
    add_tray_icon(app, item);
}
static void
on_item_removed(GsniHost *host, GsniHostItem *item, gpointer data)
{
    AppData *app = data;
    g_print("Item removed: %s\n", gsni_host_item_get_id(item));
    remove_tray_icon(app, item);
}

static void
add_tray_icon(AppData *app, GsniHostItem *item)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    GtkWidget *image = gtk_image_new();
    const gchar *icon_name = gsni_host_item_get_icon_name(item);
    if (icon_name && icon_name[0])
        gtk_image_set_from_icon_name(GTK_IMAGE(image), icon_name);
    else
        gtk_image_set_from_icon_name(GTK_IMAGE(image), "image-missing");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 24);
    gtk_box_append(GTK_BOX(row), image);

    const gchar *title = gsni_host_item_get_title(item);
    const gchar *item_id = gsni_host_item_get_id(item);
    gchar *label_text = g_strdup_printf("<b>%s</b>\n<small>%s</small>",
        title ? title : "(untitled)", item_id ? item_id : "");
    GtkWidget *label = gtk_label_new(label_text);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_box_append(GTK_BOX(row), label);
    g_free(label_text);

    GsniStatus status = gsni_host_item_get_status(item);
    GtkWidget *sl = gtk_label_new("");
    switch (status) {
    case GSNI_STATUS_ACTIVE:
        gtk_label_set_text(GTK_LABEL(sl), "● Active"); break;
    case GSNI_STATUS_NEEDS_ATTENTION:
        gtk_label_set_text(GTK_LABEL(sl), "⚠ Attention"); break;
    default:
        gtk_label_set_text(GTK_LABEL(sl), "○ Passive"); break;
    }
    gtk_widget_set_margin_start(sl, 8);
    gtk_box_append(GTK_BOX(row), sl);

    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
    g_signal_connect(click, "pressed", G_CALLBACK(on_icon_pressed),
                     g_object_ref(item));
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(click));

    gtk_box_append(app->tray_box, row);
    g_hash_table_insert(app->item_widgets, g_object_ref(item), row);
}

static void
remove_tray_icon(AppData *app, GsniHostItem *item)
{
    GtkWidget *row = g_hash_table_lookup(app->item_widgets, item);
    if (row) {
        gtk_box_remove(app->tray_box, row);
        g_hash_table_remove(app->item_widgets, item);
    }
}

static void
load_existing_items(AppData *app)
{
    GListModel *model = gsni_host_get_items(app->host);
    guint n = g_list_model_get_n_items(model);
    for (guint i = 0; i < n; i++) {
        GsniHostItem *item = GSNI_HOST_ITEM(g_list_model_get_item(model, i));
        add_tray_icon(app, item);
        g_object_unref(item);
    }
}

/* ── Application ───────────────────────────────────────────── */

static void
app_activate(GApplication *gapp)
{
    GtkApplication *app = GTK_APPLICATION(gapp);
    AppData *data = g_new0(AppData, 1);
    g_object_set_data_full(G_OBJECT(app), "tray-host-data", data, g_free);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "libgsni — System Tray Host");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    data->window = GTK_WINDOW(window);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_window_set_child(GTK_WINDOW(window), scrolled);

    data->tray_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 2));
    gtk_box_set_spacing(data->tray_box, 4);
    gtk_widget_set_margin_start(GTK_WIDGET(data->tray_box), 8);
    gtk_widget_set_margin_top(GTK_WIDGET(data->tray_box), 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                  GTK_WIDGET(data->tray_box));

    data->item_widgets = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                               g_object_unref, NULL);

    data->host = gsni_host_new(NULL);
    g_signal_connect(data->host, "item-added",
                     G_CALLBACK(on_item_added), data);
    g_signal_connect(data->host, "item-removed",
                     G_CALLBACK(on_item_removed), data);

    GError *err = NULL;
    if (!gsni_host_register(data->host, &err)) {
        GtkWidget *msg = gtk_label_new("No StatusNotifierWatcher available.\n"
                                       "Run this on a session with system tray support.");
        gtk_box_append(data->tray_box, msg);
        g_print("Host registration failed: %s\n", err->message);
        g_clear_error(&err);
    }

    gint n = 0;
    while (n++ < 50)
        g_main_context_iteration(NULL, FALSE);
    load_existing_items(data);
    gtk_window_present(GTK_WINDOW(window));
}

static void
app_shutdown(GApplication *gapp)
{
    AppData *data = g_object_get_data(G_OBJECT(gapp), "tray-host-data");
    if (data && data->host) {
        gsni_host_unregister(data->host);
        g_object_unref(data->host);
        data->host = NULL;
    }
}

int
main(int argc, char *argv[])
{
    GtkApplication *app = gtk_application_new("org.libgsni.TrayHost",
                                              G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(app_shutdown), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
