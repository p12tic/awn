#!/usr/bin/python
# -*- coding: utf-8 -*-
#  
#  Copyright © 2009 Michal Hruby <michal.mhr@gmail.com>
#  Copyright © 2009 Mark Lee <avant-wn@lazymalevolence.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.

import gtk
import awn


class Main:

    def __init__(self):
        # Create GUI objects
        rgbaColormap = gtk.gdk.screen_get_default().get_rgba_colormap()
        if rgbaColormap != None:
            gtk.gdk.screen_get_default().set_default_colormap(rgbaColormap)
        self.window = gtk.Window()
        self.window.connect("delete-event", self.OnQuit)

        self.vbox = gtk.VBox(False)
        self.effectsHBox = gtk.HBox()

        self.icon = awn.ThemedIcon()
        self.icon.set_info_simple("Applet", "123", "awn-manager")

        self.icon.connect("button-release-event", self.OnButton)
        self.icon.connect("enter-notify-event", self.OnMouseOver)
        self.icon.connect("leave-notify-event", self.OnMouseOut)
        self.effectsHBox.pack_start(self.icon, True, True)

        self.vbox.pack_start(self.effectsHBox)

        self.table = gtk.Table(2, 3)
        self.quitButton = gtk.Button(stock='gtk-quit')
        self.quitButton.connect("clicked", self.OnQuit)
        self.textOvrly = gtk.ToggleButton("Text overlay")
        self.textOvrly.connect("toggled", self.TextOverlay)
        self.throOvrly = gtk.ToggleButton("Throbber overlay")
        self.throOvrly.connect("toggled", self.ThrobberOverlay)
        self.thioOvrly = gtk.ToggleButton("ThemedIcon overlay")
        self.thioOvrly.connect("toggled", self.ThemedIconOverlay)
        self.pixbOvrly = gtk.ToggleButton("Pixbuf overlay")
        self.pixbOvrly.connect("toggled", self.PixbufOverlay)
        self.pixfOvrly = gtk.ToggleButton("Pixbuf file overlay")
        self.pixfOvrly.connect("toggled", self.PixbufFileOverlay)

        self.table.attach(self.textOvrly, 0, 1, 0, 1)
        self.table.attach(self.throOvrly, 1, 2, 0, 1)
        self.table.attach(self.thioOvrly, 2, 3, 0, 1)

        self.table.attach(self.pixbOvrly, 0, 1, 1, 2)
        self.table.attach(self.pixfOvrly, 1, 2, 1, 2)

        self.table.attach(self.quitButton, 2, 3, 1, 2)

        self.vbox.pack_start(self.table, False)
        self.window.add(self.vbox)

        self.window.show_all()

    def TextOverlay(self, widget, *args, **kwargs):
        if widget.get_active():
            self.textOverlay = awn.OverlayText()
            self.textOverlay.props.text = "hello"
            self.icon.get_effects().add_overlay(self.textOverlay)
        elif self.textOverlay != None:
            self.icon.get_effects().remove_overlay(self.textOverlay)
            self.textOverlay = None

    def ThrobberOverlay(self, widget, *args, **kwargs):
        if widget.get_active():
            self.throOverlay = awn.OverlayThrobber(self.icon)
            self.icon.get_effects().add_overlay(self.throOverlay)
        elif self.throOverlay != None:
            self.icon.get_effects().remove_overlay(self.throOverlay)
            self.throOverlay = None

    def ThemedIconOverlay(self, widget, *args, **kwargs):
        if widget.get_active():
            self.thioOverlay = awn.OverlayThemedIcon(self.icon, "awn-manager",
                                                     "something")
            self.icon.get_effects().add_overlay(self.thioOverlay)
        elif self.thioOverlay != None:
            self.icon.get_effects().remove_overlay(self.thioOverlay)
            self.thioOverlay = None

    def PixbufOverlay(self, widget, *args, **kwargs):
        if widget.get_active():
            pixbuf = self.icon.get_icon_at_size(24)
            self.pixbOverlay = awn.OverlayPixbuf(pixbuf)
            self.icon.get_effects().add_overlay(self.pixbOverlay)
        elif self.pixbOverlay != None:
            self.icon.get_effects().remove_overlay(self.pixbOverlay)
            self.pixbOverlay = None

    def PixbufFileOverlay(self, widget, *args, **kwargs):
        if widget.get_active():
            icon_path = "/usr/share/icons/gnome/22x22/actions/window-new.png"
            self.pixfOverlay = awn.OverlayPixbufFile(icon_path)
            self.icon.get_effects().add_overlay(self.pixfOverlay)
        elif self.pixfOverlay != None:
            self.icon.get_effects().remove_overlay(self.pixfOverlay)
            self.pixfOverlay = None

    def OnMouseOver(self, widget, *args, **kwargs):
        pass

    def OnMouseOut(self, widget, *args, **kwargs):
        pass

    def OnButton(self, widget, event):
        pass

    def OnQuit(self, widget, event=None):
        gtk.main_quit()

start = Main()
gtk.main()
