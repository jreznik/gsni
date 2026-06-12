/* SPDX-License-Identifier: LGPL-2.1-or-later
 * SPDX-FileCopyrightText: 2024 libgsni contributors
 *
 * Music controller tray icon — real-world example demonstrating
 * dynamic menu updates, stateful actions, radio groups, checkmarks,
 * and stdin-driven runtime state changes.
 *
 * Build: meson setup build && ninja -C build
 * Run:   ./build/examples/music-controller
 */

#include <gio/gio.h>
#include "gsni/gsni.h"

typedef enum {
    STATE_STOPPED,
    STATE_PLAYING,
    STATE_PAUSED,
} PlaybackState;

typedef struct {
    GsniItem           *item;
    GMenu              *menu;
    GSimpleActionGroup *actions;
    GMainLoop          *loop;

    PlaybackState  state;
    gint           selected_radio;
    gboolean       shuffle_on;
    gboolean       repeat_on;
    gint           track_number;
    const gchar   *track_title;
} AppData;

static const gchar *track_list[] = {
    "Distant Signals",
    "Midnight Transit",
    "Warm Glow",
    "After Hours",
    NULL
};

static void rebuild_menu(AppData *app);

static void
transition_state(AppData *app, PlaybackState new_state)
{
    PlaybackState old = app->state;
    app->state = new_state;

    switch (new_state) {
    case STATE_PLAYING:
        g_print("▶ Playing: %s\n", app->track_title);
        break;
    case STATE_PAUSED:
        g_print("⏸ Paused\n");
        break;
    case STATE_STOPPED:
        g_print("⏹ Stopped\n");
        break;
    }

    if (new_state != old) {
        gsni_item_set_title(app->item, new_state == STATE_PLAYING
            ? "▶ Now Playing" : "⏸ Music Controller");
        rebuild_menu(app);
    }
}

static void
on_play(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    transition_state(app, STATE_PLAYING);
}

static void
on_pause(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    transition_state(app, STATE_PAUSED);
}

static void
on_stop(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    transition_state(app, STATE_STOPPED);
}

static void
on_next(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    app->track_number = (app->track_number + 1);
    while (track_list[app->track_number] == NULL)
        app->track_number = 0;
    app->track_title = track_list[app->track_number];
    g_print("⏭ Next: %s\n", app->track_title);
    if (app->state == STATE_PLAYING)
        gsni_item_set_title(app->item, "▶ Now Playing");
    rebuild_menu(app);
}

static void
on_previous(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    if (app->track_number == 0) {
        gint i = 0;
        while (track_list[i + 1] != NULL) i++;
        app->track_number = i;
    } else {
        app->track_number--;
    }
    app->track_title = track_list[app->track_number];
    g_print("⏮ Previous: %s\n", app->track_title);
    if (app->state == STATE_PLAYING)
        gsni_item_set_title(app->item, "▶ Now Playing");
    rebuild_menu(app);
}

static void
on_toggle_shuffle(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)param;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean current = g_variant_get_boolean(state);
    g_variant_unref(state);

    app->shuffle_on = !current;
    g_simple_action_set_state(action, g_variant_new_boolean(app->shuffle_on));
    g_print("🔀 Shuffle: %s\n", app->shuffle_on ? "ON" : "OFF");
}

static void
on_toggle_repeat(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)param;
    GVariant *state = g_action_get_state(G_ACTION(action));
    gboolean current = g_variant_get_boolean(state);
    g_variant_unref(state);

    app->repeat_on = !current;
    g_simple_action_set_state(action, g_variant_new_boolean(app->repeat_on));
    g_print("🔁 Repeat: %s\n", app->repeat_on ? "ON" : "OFF");
}

static void
on_set_radio(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    app->selected_radio = g_variant_get_int32(param);
    g_simple_action_set_state(action, param);

    const gchar *label = "1";
    switch (app->selected_radio) {
    case 10: label = "1"; break;
    case 20: label = "2"; break;
    case 30: label = "3"; break;
    }
    g_print("Radio: option %s\n", label);
}

static void
on_quit_cb(GSimpleAction *action, GVariant *param, gpointer data)
{
    AppData *app = data;
    (void)action; (void)param;
    g_print("Quitting.\n");
    transition_state(app, STATE_STOPPED);
    g_main_loop_quit(app->loop);
}

static void
rebuild_menu(AppData *app)
{
    g_menu_remove_all(app->menu);

    /* Transport controls — change based on playback state */
    switch (app->state) {
    case STATE_STOPPED:
        g_menu_append(app->menu, "▶ Play", "play");
        g_menu_append(app->menu, "⏭ Next", "next");
        g_menu_append(app->menu, "⏮ Previous", "prev");
        break;
    case STATE_PLAYING:
        g_menu_append(app->menu, "⏯ Pause", "pause");
        g_menu_append(app->menu, "⏭ Next", "next");
        g_menu_append(app->menu, "⏮ Previous", "prev");
        g_menu_append(app->menu, "⏹ Stop", "stop");
        break;
    case STATE_PAUSED:
        g_menu_append(app->menu, "▶ Resume", "play");
        g_menu_append(app->menu, "⏹ Stop", "stop");
        break;
    }

    GMenu *empty = g_menu_new();
    GMenuItem *sep = g_menu_item_new_section(NULL, G_MENU_MODEL(empty));
    g_menu_append_item(app->menu, sep);
    g_object_unref(sep);
    g_object_unref(empty);

    /* Track info */
    gchar *track_label = g_strdup_printf("🎵 %s", app->track_title);
    g_menu_append(app->menu, track_label, NULL);
    g_free(track_label);

    GMenuItem *sep2 = g_menu_item_new_section(NULL, NULL);
    g_menu_append_item(app->menu, sep2);
    g_object_unref(sep2);

    /* Toggles */
    GMenuItem *shuffle = g_menu_item_new("🔀 Shuffle", "shuffle");
    g_menu_append_item(app->menu, shuffle);
    g_object_unref(shuffle);

    GMenuItem *repeat = g_menu_item_new("🔁 Repeat", "repeat");
    g_menu_append_item(app->menu, repeat);
    g_object_unref(repeat);

    GMenuItem *sep3 = g_menu_item_new_section(NULL, NULL);
    g_menu_append_item(app->menu, sep3);
    g_object_unref(sep3);

    /* Radio group */
    GMenu *radio_menu = g_menu_new();
    GMenuItem *r1 = g_menu_item_new("Quality: Low", "radio(10)");
    g_menu_append_item(radio_menu, r1);
    g_object_unref(r1);
    GMenuItem *r2 = g_menu_item_new("Quality: Medium", "radio(20)");
    g_menu_append_item(radio_menu, r2);
    g_object_unref(r2);
    GMenuItem *r3 = g_menu_item_new("Quality: High", "radio(30)");
    g_menu_append_item(radio_menu, r3);
    g_object_unref(r3);

    g_menu_append_submenu(app->menu, "⚙ Quality", G_MENU_MODEL(radio_menu));
    g_object_unref(radio_menu);

    GMenuItem *sep4 = g_menu_item_new_section(NULL, NULL);
    g_menu_append_item(app->menu, sep4);
    g_object_unref(sep4);

    g_menu_append(app->menu, "❌ Quit", "quit");
}

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

    if (g_str_equal(line, "play") || g_str_equal(line, "p"))
        transition_state(app, STATE_PLAYING);
    else if (g_str_equal(line, "pause"))
        transition_state(app, STATE_PAUSED);
    else if (g_str_equal(line, "stop") || g_str_equal(line, "s"))
        transition_state(app, STATE_STOPPED);
    else if (g_str_equal(line, "next") || g_str_equal(line, "n"))
        on_next(NULL, NULL, app);
    else if (g_str_equal(line, "prev"))
        on_previous(NULL, NULL, app);
    else if (g_str_equal(line, "shuffle"))
        on_toggle_shuffle(NULL, NULL, app);
    else if (g_str_equal(line, "repeat") || g_str_equal(line, "r"))
        on_toggle_repeat(NULL, NULL, app);
    else if (g_str_equal(line, "quit") || g_str_equal(line, "q"))
        on_quit_cb(NULL, NULL, app);
    else if (line[0])
        g_print("Commands: play(p) pause stop(s) next(n) prev "
                "shuffle repeat(r) quit(q)\n");

    g_free(line);
    return TRUE;
}

int
main(int argc, char *argv[])
{
    g_autoptr(GApplication) gapp = g_application_new(
        "org.libgsni.MusicController", G_APPLICATION_DEFAULT_FLAGS);
    g_application_register(gapp, NULL, NULL);

    GDBusConnection *bus = g_application_get_dbus_connection(gapp);
    g_autoptr(GMainLoop) loop = g_main_loop_new(NULL, FALSE);

    AppData app = { .loop = loop, .state = STATE_STOPPED,
                    .selected_radio = 10, .shuffle_on = FALSE,
                    .repeat_on = FALSE, .track_number = 0,
                    .track_title = track_list[0] };

    app.actions = g_simple_action_group_new();

    GSimpleAction *act_play = g_simple_action_new("play", NULL);
    g_signal_connect(act_play, "activate", G_CALLBACK(on_play), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_play));
    g_object_unref(act_play);

    GSimpleAction *act_pause = g_simple_action_new("pause", NULL);
    g_signal_connect(act_pause, "activate", G_CALLBACK(on_pause), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_pause));
    g_object_unref(act_pause);

    GSimpleAction *act_stop = g_simple_action_new("stop", NULL);
    g_signal_connect(act_stop, "activate", G_CALLBACK(on_stop), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_stop));
    g_object_unref(act_stop);

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

    GSimpleAction *act_repeat = g_simple_action_new_stateful(
        "repeat", NULL, g_variant_new_boolean(FALSE));
    g_signal_connect(act_repeat, "activate",
        G_CALLBACK(on_toggle_repeat), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_repeat));
    g_object_unref(act_repeat);

    GSimpleAction *act_radio = g_simple_action_new_stateful(
        "radio", G_VARIANT_TYPE_INT32, g_variant_new_int32(10));
    g_signal_connect(act_radio, "activate", G_CALLBACK(on_set_radio), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_radio));
    g_object_unref(act_radio);

    GSimpleAction *act_quit = g_simple_action_new("quit", NULL);
    g_signal_connect(act_quit, "activate", G_CALLBACK(on_quit_cb), &app);
    g_action_map_add_action(G_ACTION_MAP(app.actions), G_ACTION(act_quit));
    g_object_unref(act_quit);

    app.item = gsni_item_new("music-controller", bus, NULL);
    gsni_item_set_title(app.item, "⏸ Music Controller");
    gsni_item_set_icon_name(app.item, "audio-x-generic");
    gsni_item_set_status(app.item, GSNI_STATUS_ACTIVE);
    gsni_item_set_tooltip(app.item, NULL,
        "Music Controller", "Click for controls");

    app.menu = g_menu_new();
    rebuild_menu(&app);
    gsni_item_set_menu(app.item, G_MENU_MODEL(app.menu));
    gsni_item_set_action_group(app.item, G_ACTION_GROUP(app.actions));
    gsni_item_register(app.item, NULL);

    GIOChannel *stdin_ch = g_io_channel_unix_new(STDIN_FILENO);
    g_io_add_watch(stdin_ch, G_IO_IN, on_stdin, &app);

    g_print("Music Controller — tray icon registered.\n"
            "Commands: play(p) pause stop(s) next(n) prev "
            "shuffle repeat(r) quit(q)\n"
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
