#!/usr/bin/env python3
import subprocess, platform
from gi.repository import Gtk, GLib

class SleepTimer(Gtk.Builder):
    def __init__(self):
        super().__init__()
        self.add_from_file("main.glade")
        self.connect_signals(self)

        self.spin_buttons = (
            self.get_object("spinbutton_h"),
            self.get_object("spinbutton_min"),
            self.get_object("spinbutton_s"),
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
                        if platform.system() == "Windows":
                            subprocess.check_call("nircmd.exe mutesysvolume 1")
                        else:
                            subprocess.check_call("pactl set-sink-mute 1 1", shell=True)

                    if platform.system() == "Windows":
                        subprocess.check_call("nircmd.exe " + (
                            "hibernate" if self.get_object("hibernate").get_active() else "standby"
                        ))
                    else:
                        subprocess.call("systemctl " + (
                            "hibernate" if self.get_object("hibernate").get_active() else "suspend"
                        ), shell=True)

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

    def on_time_changed(self):
        self.get_object("togglebutton1").set_sensitive(
            self.spin_buttons[0].get_value() != 0 or
            self.spin_buttons[1].get_value() != 0 or
            self.spin_buttons[2].get_value() != 0
        )

    def on_h_changed(self, spin_button):
        self.on_time_changed()

    def on_min_changed(self, spin_button):
        """
        When minutes drop below 0 deincrease hours and when they get above 59 increase hours
        """
        while spin_button.get_value() < 0:
            if self.spin_buttons[0].get_value() == 0:
                spin_button.set_value(0)
            else:
                spin_button.set_value(spin_button.get_value() + 60)
                self.spin_buttons[0].set_value(self.spin_buttons[0].get_value() - 1)
        while spin_button.get_value() > 59:
            spin_button.set_value(spin_button.get_value() - 60)
            self.spin_buttons[0].set_value(self.spin_buttons[0].get_value() + 1)
        self.on_time_changed()

    def on_s_changed(self, spin_button):
        """
        When seconds drop below 0 deincrease minutes and when they get above 59 increase minutes
        """
        while spin_button.get_value() < 0:
            if self.spin_buttons[0].get_value() == 0 and self.spin_buttons[1].get_value() == 0:
                spin_button.set_value(0)
            else:
                spin_button.set_value(spin_button.get_value() + 60)
                self.spin_buttons[1].set_value(self.spin_buttons[1].get_value() - 1)
        while spin_button.get_value() > 59:
            spin_button.set_value(spin_button.get_value() - 60)
            self.spin_buttons[1].set_value(self.spin_buttons[1].get_value() + 1)
        self.on_time_changed()

    def on_delete_window(self, *args):
        Gtk.main_quit(*args)

SleepTimer()
Gtk.main()
