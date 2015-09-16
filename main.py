#!/usr/bin/env python3
import subprocess
from gi.repository import Gtk, GLib

class SleepTimer(Gtk.Builder):
    def __init__(self):
        super().__init__()
        self.add_from_file("main.glade")
        self.connect_signals(self)

        self.spin_buttons = (
            self.get_object("spinbutton1"),
            self.get_object("spinbutton2"),
            self.get_object("spinbutton3"),
        )

        window = self.get_object("window1")
        window.show_all()

    def on_timer(self):
        """
        Deincreases by one second
        """
        if not self.get_object("togglebutton1").get_active():
            return False
        seconds = self.spin_buttons[2].get_value_as_int()
        if seconds == 0:
            seconds = 60
            minutes = self.spin_buttons[1].get_value_as_int()
            if minutes == 0:
                minutes = 60
                hours = self.spin_buttons[0].get_value_as_int()
                if hours == 0:
                    if self.get_object("checkbutton1").get_active():
                        subprocess.check_call("pactl set-sink-mute 1 1", shell=True)
                    subprocess.call("systemctl suspend", shell=True)
                    Gtk.main_quit()
                    return False
                self.spin_buttons[0].set_value(hours - 1)
            self.spin_buttons[1].set_value(minutes - 1)
        self.spin_buttons[2].set_value(seconds - 1)
        return True

    def on_toggled(self, button):
        """
        Start button toggled
        """
        for spin_button in self.spin_buttons:
            spin_button.set_sensitive(not button.get_active())

        if button.get_active():
            GLib.timeout_add(1000, self.on_timer)

    def on_delete_window(self, *args):
        Gtk.main_quit(*args)

SleepTimer()
Gtk.main()
