# -*- Mode: python; coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*-
#
# Copyright (C) 2007 - Neil J Patel
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.

import rhythmdb
import rb
import gtk, gobject
import dbus
import dbus.decorators
import dbus.glib


BUS_NAME = "com.google.code.Awn"
OBJ_PATH = "/com/google/code/Awn"
IFACE_NAME = "com.google.code.Awn"

class AwnMenuPlugin (rb.Plugin):
	def __init__ (self):
		rb.Plugin.__init__ (self)

	def activate (self, shell):
		self.shell = shell
		bus = dbus.SessionBus()
		bus.add_signal_receiver(self.menu_clicked, dbus_interface = "com.google.code.Awn", signal_name = "MenuItemClicked")
		bus.add_signal_receiver(self.check_clicked, dbus_interface = "com.google.code.Awn", signal_name = "CheckItemClicked")
		self.awn_obj = awn_obj = bus.get_object(BUS_NAME, OBJ_PATH)
		self.create_menus()

	def menu_clicked (self, id):
		sp = self.shell.get_player ()
		if id == self.go_back:
			sp.do_previous()
		elif id == self.go_forward:
			sp.do_next()
		else:
			return

	def check_clicked (self, id, bool):
		if id == self.toggle_play:
			sp = self.shell.get_player ()
			if bool:
				sp.play()
			else:
				sp.pause()

	def create_menus (self):
		try:
			sp = self.shell.get_player ()
			self.go_back = self.awn_obj.AddTaskMenuItemByName ("rhythmbox", "gtk-media-previous", " ")
			self.go_forward = self.awn_obj.AddTaskMenuItemByName ("rhythmbox", "gtk-media-next", " ")
			self.toggle_play = self.awn_obj.AddTaskCheckItemByName("rhythmbox", "P_lay", sp.get_playing())
		except dbus.DBusException, e:
			print "Unavailable"

	def deactivate (self, shell):
		del self.shell
		

	def playing_changed (self, sp, playing):
		self.awn_obj.SetTaskCheckItemByName("rhythmbox", self.toggle_play, sp.get_playing())
