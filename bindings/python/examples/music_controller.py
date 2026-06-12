#!/usr/bin/env python3
"""1:1 Python port of examples/music-controller.c.

Music controller tray icon — dynamic menu updates, stateful actions,
radio groups, checkmarks, and stdin-driven runtime state changes.

Run:
    GI_TYPELIB_PATH=build/src LD_LIBRARY_PATH=build/src \
        python3 bindings/python/examples/music_controller.py
"""

import sys
import gi
gi.require_version("Gio", "2.0")
gi.require_version("GLib", "2.0")
from gi.repository import Gio, GLib

from gsni import StatusNotifierItem

# ── state ────────────────────────────────────────────────────

STATE_STOPPED = 0
STATE_PLAYING = 1
STATE_PAUSED = 2

TRACK_LIST = ["Distant Signals", "Midnight Transit",
              "Warm Glow", "After Hours"]


class MusicController:
    def __init__(self):
        self.state = STATE_STOPPED
        self.selected_radio = 10
        self.shuffle_on = False
        self.repeat_on = False
        self.track_number = 0
        self.track_title = TRACK_LIST[0]
        self.loop = GLib.MainLoop.new(None, False)

        self.actions = Gio.SimpleActionGroup.new()
        self._build_actions()

        self.item = StatusNotifierItem("music-controller",
                                       icon_name="audio-x-generic",
                                       title="\u23f8 Music Controller")
        self.item.status = 1  # ACTIVE
        self.item.set_tooltip(None, "Music Controller",
                              "Click for controls")
        self.rebuild_menu()
        self.item.action_group = self.actions
        self.item.register()

    def _build_actions(self):
        a = self.actions
        s = Gio.SimpleAction

        act = s.new("play", None)
        act.connect("activate", lambda *a: self.transition(STATE_PLAYING))
        a.add_action(act)

        act = s.new("pause", None)
        act.connect("activate", lambda *a: self.transition(STATE_PAUSED))
        a.add_action(act)

        act = s.new("stop", None)
        act.connect("activate", lambda *a: self.transition(STATE_STOPPED))
        a.add_action(act)

        act = s.new("next", None)
        act.connect("activate", self._on_next)
        a.add_action(act)

        act = s.new("prev", None)
        act.connect("activate", self._on_previous)
        a.add_action(act)

        act = s.new_stateful("shuffle",
                             None,
                             GLib.Variant.new_boolean(False))
        act.connect("activate", self._on_toggle_shuffle)
        a.add_action(act)

        act = s.new_stateful("repeat",
                             None,
                             GLib.Variant.new_boolean(False))
        act.connect("activate", self._on_toggle_repeat)
        a.add_action(act)

        param_type = GLib.VariantType.new("i")
        act = s.new_stateful("radio",
                             param_type,
                             GLib.Variant.new_int32(10))
        act.connect("activate", self._on_set_radio)
        a.add_action(act)

        act = s.new("quit", None)
        act.connect("activate", self._on_quit)
        a.add_action(act)

    # ── state transitions ────────────────────────────────────

    def transition(self, new_state):
        old = self.state
        self.state = new_state

        if new_state == STATE_PLAYING:
            print(f"\u25b6 Playing: {self.track_title}")
        elif new_state == STATE_PAUSED:
            print("\u23f8 Paused")
        elif new_state == STATE_STOPPED:
            print("\u23f9 Stopped")

        if new_state != old:
            title = ("\u25b6 Now Playing" if new_state == STATE_PLAYING
                     else "\u23f8 Music Controller")
            self.item.title = title
            self.rebuild_menu()

    # ── actions ─────────────────────────────────────────────

    def _on_next(self, action, param):
        self.track_number = (self.track_number + 1) % len(TRACK_LIST)
        self.track_title = TRACK_LIST[self.track_number]
        print(f"\u23ed Next: {self.track_title}")
        if self.state == STATE_PLAYING:
            self.rebuild_menu()

    def _on_previous(self, action, param):
        if self.track_number == 0:
            self.track_number = len(TRACK_LIST) - 1
        else:
            self.track_number -= 1
        self.track_title = TRACK_LIST[self.track_number]
        print(f"\u23ee Previous: {self.track_title}")
        if self.state == STATE_PLAYING:
            self.rebuild_menu()

    def _on_toggle_shuffle(self, action, param):
        state = action.get_state().get_boolean()
        self.shuffle_on = not state
        action.set_state(GLib.Variant.new_boolean(self.shuffle_on))
        print(f"\U0001f500 Shuffle: {'ON' if self.shuffle_on else 'OFF'}")

    def _on_toggle_repeat(self, action, param):
        state = action.get_state().get_boolean()
        self.repeat_on = not state
        action.set_state(GLib.Variant.new_boolean(self.repeat_on))
        print(f"\U0001f501 Repeat: {'ON' if self.repeat_on else 'OFF'}")

    def _on_set_radio(self, action, param):
        val = param.get_int32()
        self.selected_radio = val
        action.set_state(param)
        labels = {10: "1", 20: "2", 30: "3"}
        print(f"Radio: option {labels.get(val, '?')}")

    def _on_quit(self, action, param):
        print("Quitting.")
        self.transition(STATE_STOPPED)
        self.loop.quit()

    # ── menu ─────────────────────────────────────────────────

    def rebuild_menu(self):
        menu = Gio.Menu.new()

        if self.state == STATE_STOPPED:
            menu.append("\u25b6 Play", "play")
            menu.append("\u23ed Next", "next")
            menu.append("\u23ee Previous", "prev")
        elif self.state == STATE_PLAYING:
            menu.append("\u23ef Pause", "pause")
            menu.append("\u23ed Next", "next")
            menu.append("\u23ee Previous", "prev")
            menu.append("\u23f9 Stop", "stop")
        elif self.state == STATE_PAUSED:
            menu.append("\u25b6 Resume", "play")
            menu.append("\u23f9 Stop", "stop")

        menu.append_section(None, Gio.Menu.new())
        menu.append(f"\U0001f3b5 {self.track_title}", None)
        menu.append_section(None, Gio.Menu.new())
        menu.append("\U0001f500 Shuffle", "shuffle")
        menu.append("\U0001f501 Repeat", "repeat")
        menu.append_section(None, Gio.Menu.new())

        radio = Gio.Menu.new()
        radio.append("Quality: Low", "radio(10)")
        radio.append("Quality: Medium", "radio(20)")
        radio.append("Quality: High", "radio(30)")
        menu.append_submenu("\u2699 Quality", radio)

        menu.append_section(None, Gio.Menu.new())
        menu.append("\u274c Quit", "quit")

        self.item.menu = menu

    # ── stdin ────────────────────────────────────────────────

    def on_stdin(self, source, condition):
        try:
            status, line, length, term_pos = source.read_line()
        except Exception:
            return True
        if status != GLib.IOStatus.NORMAL or not line:
            return True

        line = line.strip()
        if not line:
            return True

        if line in ("play", "p"):
            self.transition(STATE_PLAYING)
        elif line == "pause":
            self.transition(STATE_PAUSED)
        elif line in ("stop", "s"):
            self.transition(STATE_STOPPED)
        elif line in ("next", "n"):
            self._on_next(None, None)
        elif line == "prev":
            self._on_previous(None, None)
        elif line == "shuffle":
            self._on_toggle_shuffle(self.actions.lookup_action("shuffle"), None)
        elif line in ("repeat", "r"):
            self._on_toggle_repeat(self.actions.lookup_action("repeat"), None)
        elif line in ("quit", "q"):
            self._on_quit(None, None)
        else:
            print("Commands: play(p) pause stop(s) next(n) prev "
                  "shuffle repeat(r) quit(q)")
        return True

    def run(self):
        stdin_chan = GLib.IOChannel.unix_new(sys.stdin.fileno())
        GLib.io_add_watch(stdin_chan, GLib.PRIORITY_DEFAULT,
                          GLib.IO_IN, self.on_stdin)
        print("Music Controller — tray icon registered.")
        print("Commands: play(p) pause stop(s) next(n) prev "
              "shuffle repeat(r) quit(q)")
        print("Click the tray icon for the menu.")
        self.loop.run()


if __name__ == "__main__":
    app = MusicController()
    app.run()
