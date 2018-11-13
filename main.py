#!/usr/bin/env python3
import subprocess, platform
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib, Gdk

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

        self.css_provider = Gtk.CssProvider()
        self.get_object("togglebutton1").get_style_context().add_provider(
            self.css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        self.start_seconds_left = 0
        self.window = self.get_object("window1")
        self.window.show_all()

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
                    try:
                        if self.get_object("checkbutton1").get_active():
                            if platform.system() == "Windows":
                                subprocess.check_output("nircmd.exe mutesysvolume 1")
                            else:
                                subprocess.check_output("pactl set-sink-mute 0 1", shell=True)

                        verb = "hibernate"
                        if platform.system() == "Windows":
                            if self.get_object("standby").get_active():
                                verb = "standby"
                            elif self.get_object("shutdown").get_active():
                                verb = "exitwin poweroff"
                            subprocess.check_output("nircmd.exe " + verb)
                        else:
                            if self.get_object("standby").get_active():
                                verb = "suspend"
                            elif self.get_object("shutdown").get_active():
                                verb = "poweroff"
                            subprocess.check_output("systemctl " + verb + " -i", shell=True,
                                                    stderr=subprocess.STDOUT)

                    except subprocess.CalledProcessError as err:
                        dialog = Gtk.MessageDialog(
                            parent=self.window, message_type=Gtk.MessageType.ERROR,
                            buttons=Gtk.ButtonsType.CLOSE,
                            text="`{}` failed with exit code {}".format(err.cmd, err.returncode))
                        dialog.format_secondary_text(err.stdout.decode('utf-8', 'ignore').strip())
                        dialog.run()
                        dialog.destroy()
                    Gtk.main_quit()
                    return False
                self.spin_buttons[0].set_value(hours - 1)
            self.spin_buttons[1].set_value(minutes - 1)
        self.spin_buttons[2].set_value(seconds - 1)
        self.css_provider.load_from_data(".install-progress {{ background-size: {}%; }}".format(
                int(self.get_seconds_left() * 100 / self.start_seconds_left)
        ).encode())
        return True

    def on_toggled(self, button):
        """
        Start button toggled
        """
        self.spin_buttons[2].set_sensitive(not button.get_active()) # seconds

        context = button.get_style_context()
        if button.get_active():
            context.add_class("install-progress")
            context.remove_class("suggested-action")
            self.css_provider.load_from_data(b".install-progress { background-size: 100%; }")
            self.start_seconds_left = self.get_seconds_left()
            self.previous_label = button.get_label()
            button.set_label("_Stop")
        else:
            context.remove_class("install-progress")
            context.add_class("suggested-action")
            button.set_label(self.previous_label)

        if button.get_active():
            GLib.timeout_add(1000, self.on_timer)

    def on_time_changed(self):
        self.get_object("togglebutton1").set_sensitive(
            self.spin_buttons[0].get_value() != 0 or
            self.spin_buttons[1].get_value() != 0 or
            self.spin_buttons[2].get_value() != 0
        )
        # If the user increases the time while it's running this could result in a negative
        # percentage for the progress bar. Adjust the start time so that it never happens:
        self.start_seconds_left = max(self.start_seconds_left, self.get_seconds_left())

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

    def get_seconds_left(self):
        return self.spin_buttons[0].get_value() * 3600 + self.spin_buttons[1].get_value() * 60 + \
            self.spin_buttons[2].get_value()


style_provider = Gtk.CssProvider()
style_provider.load_from_data(b""".install-progress {
	background-image: linear-gradient(to top, @theme_selected_bg_color 2px, alpha(@theme_selected_bg_color, 0) 2px);
	background-repeat: no-repeat;
	background-position: 0 bottom;
	transition: none;
}
.install-progress { background-position: 100% bottom; }
""")
Gtk.StyleContext.add_provider_for_screen(
    Gdk.Screen.get_default(), style_provider,
    Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
)
SleepTimer()
Gtk.main()
