/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Real-world example: a "music controller" tray icon with interactive
 * controls. Demonstrates dynamic menu updates, radio groups, checkmarks,
 * and runtime tray state changes — all driven from stdin commands.
 *
 * Build: meson setup build && ninja -C build
 * Run:   ./build/examples/music-controller
 */

#include <gio/gio.h>
#include "gsni/gsni.h"

typedef struct {
    GsniItem    *item;
    GMenu       *menu;
    GSimpleActionGroup *actions;
    GMainLoop   *loop;
    gboolean     playing;
    gint         selected_radio;
    gboolean     shuffle_on;
} AppData;

/* ── Action callbacks ─────────────────────────────────────── */

static void
on_play_pause(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    app->playing = !app->playing;
    g_print("Playback: %s\n", app->playing ? "▶ Playing" : "⏸ Paused");

    /* Rebuild the menu to reflect new state */
    gsni_item_set_menu(app->item, G_MENU_MODEL(app->menu));
    gsni_item_set_title(app->item,
        app->playing ? "▶ Now Playing" : "⏸ Music Stopped");
}

static void
on_next(GSimpleAction *action, GVariant *param, gpointer data)
{
    (void)action; (void)param; (void)data;
    g_print("⏭ Next track\n");
}

static void
on_previous(GSimpleAction *action, GVariant *param, gpointer data)
{
    (void)action; (void)param; (void)data;
    g_print("⏮ Previous track\n");
}

static void
on_toggle_shuffle(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean current = g_variant_get_boolean(state);
    g_variant_unref(state);

    app->shuffle_on = !current;
    g_simple_action_set_state(action, g_variant_new_boolean(app->shuffle_on));
    g_print("Shuffle: %s\n", app->shuffle_on ? "ON" : "OFF");
}

static void
on_set_radio(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    app->selected_radio = g_variant_get_int32(param);
    g_simple_action_set_state(action, param);

    const gchar *label = "1";
    if (app->selected_radio == 10) label = "1";
    else if (app->selected_radio == 20) label = "2";
    else if (app->selected_radio == 30) label = "3";
    g_print("Radio: option %s\n", label);
}

static void
on_quit_cb(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    g_print("Quitting.\n");
    g_main_loop_quit(app->loop);
}

/* ── Menu builder ─────────────────────────────────────────── */

static void
rebuild_menu(AppData *app)
{
    g_menu_remove_all(app->menu);

    /* Play/Pause (dynamic label) */
    g_menu_append(app->menu,
        app->playing ? "⏸ Pause" : "▶ Play",
        "play-pause");

    g_menu_append(app->menu, "⏭ Next", "next");
    g_menu_append(app->menu, "⏮ Previous", "prev");

    GMenuItem *shuffle = g_menu_item_new("🔀 Shuffle", "shuffle");
    g_menu_append_item(app->menu, shuffle);
    g_object_unref(shuffle);

    /* Separator */
    GMenuItem *sep = g_menu_item_new_section(NULL, NULL);
    g_menu_append_item(app->menu, sep);
    g_object_unref(sep);

    /* Radio group (simulated via submenu + stateful actions) */
    GMenu *radio_menu = g_menu_new();
    GMenuItem *r1 = g_menu_item_new("Option 1", "radio(10)");
    g_menu_append_item(radio_menu, r1);
    g_object_unref(r1);
    GMenuItem *r2 = g_menu_item_new("Option 2", "radio(20)");
    g_menu_append_item(radio_menu, r2);
    g_object_unref(r2);
    GMenuItem *r3 = g_menu_item_new("Option 3", "radio(30)");
    g_menu_append_item(radio_menu, r3);
    g_object_unref(r3);

    g_menu_append_submenu(app->menu, "⚙ Options", G_MENU_MODEL(radio_menu));
    g_object_unref(radio_menu);

    /* Separator */
    GMenuItem *sep2 = g_menu_item_new_section(NULL, NULL);
    g_menu_append_item(app->menu, sep2);
    g_object_unref(sep2);

    g_menu_append(app->menu, "❌ Quit", "quit");
}

/* ── stdin handler ────────────────────────────────────────── */

static gboolean
on_stdin(GIOChannel *channel, GIOCondition cond, gpointer data)
{
    AppData *app = data;
    gchar *line = NULL;
    gsize len;
    GError *err = NULL;

    if (g_io_channel_read_line(channel, &line, &len, NULL, &err)
        != G_IO_STATUS_NORMAL) {
        g_clear_error(&err);
        return TRUE;
    }

    g_strchomp(line);

    if (g_str_equal(line, "toggle") || g_str_equal(line, "t"))
        on_play_pause(NULL, NULL, app);
    else if (g_str_equal(line, "next") || g_str_equal(line, "n"))
        on_next(NULL, NULL, app);
    else if (g_str_equal(line, "prev") || g_str_equal(line, "p"))
        on_previous(NULL, NULL, app);
    else if (g_str_equal(line, "shuffle") || g_str_equal(line, "s"))
        on_toggle_shuffle(NULL, NULL, app);
    else if (g_str_equal(line, "quit") || g_str_equal(line, "q"))
        on_quit_cb(NULL, NULL, app);
    else if (line[0])
        g_print("Commands: toggle (t), next (n), prev (p), "
                "shuffle (s), quit (q)\n");

    g_free(line);
    return TRUE;
}

/* ── main ─────────────────────────────────────────────────── */

int
main(int argc, char *argv[])
{
    g_autoptr(GApplication) gapp = g_application_new(
        "org.libgsni.MusicController", G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(gapp, NULL, NULL);

    GDBusConnection *bus = g_application_get_dbus_connection(gapp);
    g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

    AppData app = { .loop = loop, .playing = FALSE,
                    .selected_radio = 10, .shuffle_on = FALSE };

    /* ── Set up actions ──────────────────────────────────── */
    app.actions = g_simple_action_group_new();

    GSimpleAction *act_play = g_simple_action_new("play-pause", NULL);
    g_signal_connect(act_play, "activate", G_CALLBACK(on_play_pause), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_play));
    g_object_unref(act_play);

    GSimpleAction *act_next = g_simple_action_new("next", NULL);
    g_signal_connect(act_next, "activate", G_CALLBACK(on_next), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_next));
    g_object_unref(act_next);

    GSimpleAction *act_prev = g_simple_action_new("prev", NULL);
    g_signal_connect(act_prev, "activate", G_CALLBACK(on_previous), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_prev));
    g_object_unref(act_prev);

    GSimpleAction *act_shuffle = g_simple_action_new_stateful(
        "shuffle", NULL, g_variant_new_boolean(FALSE));
    g_signal_connect(act_shuffle, "activate",
        G_CALLBACK(on_toggle_shuffle), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_shuffle));
    g_object_unref(act_shuffle);

    GSimpleAction *act_radio = g_simple_action_new_stateful(
        "radio", G_VARIANT_TYPE_INT32, g_variant_new_int32(10));
    g_signal_connect(act_radio, "activate", G_CALLBACK(on_set_radio), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_radio));
    g_object_unref(act_radio);

    GSimpleAction *act_quit = g_simple_action_new("quit", NULL);
    g_signal_connect(act_quit, "activate", G_CALLBACK(on_quit_cb), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_quit));
    g_object_unref(act_quit);

    /* ── Build tray icon ──────────────────────────────────── */
    app.item = gsni_item_new("music-controller", bus, NULL);
    gsni_item_set_title(app.item, "⏸ Music Stopped");
    gsni_item_set_icon_name(app.item, "audio-x-generic");
    gsni_item_set_status(app.item, GSNI_STATUS_ACTIVE);
    gsni_item_set_tooltip(app.item, NULL,
        "Music Controller", "Click for controls");

    app.menu = g_menu_new();
    rebuild_menu(&app);
    gsni_item_set_menu(app.item, G_MENU_MODEL(app.menu));
    gsni_item_set_action_group(app.item, G_ACTION_GROUP(app.actions));
    gsni_item_register(app.item, NULL);

    /* ── stdin listener ───────────────────────────────────── */
    GIOChannel *stdin_ch = g_io_channel_unix_new(STDIN_FILENO);
    g_io_add_watch(stdin_ch, G_IO_IN, on_stdin, &app);

    g_print("Music Controller — tray icon registered.\n"
            "Commands: toggle|t  next|n  prev|p  shuffle|s  quit|q\n"
            "Click the tray icon for the menu.\n");

    g_main_loop_run(loop);

    g_io_channel_unref(stdin_ch);
    gsni_item_unregister(app.item);
    g_object_unref(app.menu);
    g_object_unref(app.actions);
    g_object_unref(app.item);
    g_object_unref(bus);

    return 0;
}
