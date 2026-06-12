/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * GTK4 system tray host example.
 *
 * Discovers SNI items via GsniHost and displays them in a window.
 * Click an icon to activate the associated application.
 */

#include <gtk/gtk.h>
#include "gsni/gsni.h"
#include "gsni/gsni-host.h"
#include "gsni/gsni-host-item.h"

typedef struct {
    GtkWindow     *window;
    GtkBox        *tray_box;
    GsniHost      *host;
    GHashTable    *item_widgets;  /* GsniHostItem * -> GtkWidget * */
} AppData;

static void add_tray_icon(AppData *app, GsniHostItem *item);
static void remove_tray_icon(AppData *app, GsniHostItem *item);

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
on_icon_clicked(GtkGestureClick *gesture, gint n_press,
                gdouble x, gdouble y, gpointer data)
{
    GsniHostItem *item = GSNI_HOST_ITEM(data);

    if (gsni_host_item_get_item_is_menu(item)) {
        /* DBusMenu-based item — the host shows the menu.
         * For now, just activate (most items treat this as toggle). */
        g_print("Item %s is menu-type (no direct activation)\n",
                gsni_host_item_get_id(item));
        return;
    }

    /* Call Activate on the remote SNI item via D-Bus */
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn)
        return;

    const gchar *service = gsni_host_item_get_service(item);
    gchar *bus_name = g_strdup(service);
    gchar *slash = strchr(bus_name, '/');
    if (slash)
        *slash = '\0';

    GError *err = NULL;
    GVariant *result = g_dbus_connection_call_sync(
        conn, bus_name, "/StatusNotifierItem",
        "org.kde.StatusNotifierItem", "Activate",
        g_variant_new("(ii)", (gint)x, (gint)y),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);

    if (result)
        g_variant_unref(result);
    else
        g_warning("Activate failed for %s: %s", bus_name, err->message);
    g_clear_error(&err);
    g_free(bus_name);
    g_object_unref(conn);
}

static void
add_tray_icon(AppData *app, GsniHostItem *item)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    /* Icon */
    GtkWidget *image = gtk_image_new();
    const gchar *icon_name = gsni_host_item_get_icon_name(item);
    if (icon_name && icon_name[0])
        gtk_image_set_from_icon_name(GTK_IMAGE(image), icon_name);
    else
        gtk_image_set_from_icon_name(GTK_IMAGE(image), "image-missing");
    gtk_image_set_pixel_size(GTK_IMAGE(image), 24);
    gtk_box_append(GTK_BOX(row), image);

    /* Title */
    const gchar *title = gsni_host_item_get_title(item);
    const gchar *item_id = gsni_host_item_get_id(item);
    gchar *label_text = g_strdup_printf("<b>%s</b>\n<small>%s</small>",
        title ? title : "(untitled)",
        item_id ? item_id : "");
    GtkWidget *label = gtk_label_new(label_text);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    gtk_box_append(GTK_BOX(row), label);
    g_free(label_text);

    /* Status indicator */
    GsniStatus status = gsni_host_item_get_status(item);
    GtkWidget *status_label = gtk_label_new("");
    switch (status) {
    case GSNI_STATUS_ACTIVE:
        gtk_label_set_text(GTK_LABEL(status_label), "● Active");
        break;
    case GSNI_STATUS_NEEDS_ATTENTION:
        gtk_label_set_text(GTK_LABEL(status_label), "⚠ Attention");
        break;
    default:
        gtk_label_set_text(GTK_LABEL(status_label), "○ Passive");
        break;
    }
    gtk_widget_set_margin_start(status_label, 8);
    gtk_box_append(GTK_BOX(row), status_label);

    /* Make clickable */
    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed", G_CALLBACK(on_icon_clicked),
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

    /* Create and register the host */
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

    /* Pump the main loop to let the watcher appear and items load */
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
