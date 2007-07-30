#!/usr/bin/env python
#
#  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
#
#  Author: Neil Jagdish Patel <njpatel@gmail.com>
#
#  Notes: Avant Window Navigator preferences window

import sys, os
try:
 	import pygtk
  	pygtk.require("2.0")
except:
  	pass
try:
	import gtk
  	import gtk.glade
except:
	sys.exit(1)

import gconf
import gnomedesktop
import gtk.gdk as gdk

APP = 'avant-window-navigator'
DIR = '/usr/share/locale'
I18N_DOMAIN = "avant-window-navigator"

import locale
import gettext
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext


# GCONF KEYS

LAUNCHER_DIR = "/apps/avant-window-navigator/window_manager"
LAUNCHER_PATH = "/apps/avant-window-navigator/window_manager/launchers"

DATA_DIR = "@PKGDATADIR@"


class main:
	
	def __init__(self):
		
		self.client = gconf.client_get_default()
		self.client.add_dir(LAUNCHER_DIR, gconf.CLIENT_PRELOAD_NONE)
		
		#Set the Glade file
		gtk.glade.bindtextdomain(APP, DIR)
		gtk.glade.textdomain(APP)
		self.gladefile = os.path.join(DATA_DIR, "avant-launchers.glade") 
		print self.gladefile 
	        self.wTree = gtk.glade.XML(self.gladefile, domain=I18N_DOMAIN) 
		
		#Get the Main Window, and connect the "destroy" event
		self.window = self.wTree.get_widget("main_window")
		self.window.connect("delete-event", gtk.main_quit)
		
		close = self.wTree.get_widget("closebutton")
		close.connect("clicked", gtk.main_quit)
		
		applybut = self.wTree.get_widget("applybutton")
		applybut.connect("clicked", self.apply_clicked)
		
		self.treeview =  self.wTree.get_widget("treeview")
		self.make_model ()
		
		self.add = self.wTree.get_widget ("add")
		self.remove = self.wTree.get_widget ("remove")
		
		self.up = self.wTree.get_widget ("up")
		self.up.connect("clicked", self.up_clicked)
		
		self.down = self.wTree.get_widget ("down")
		self.down.connect("clicked", self.down_clicked)

	def win_destroy(self, button, w):
		w.destroy()
        
        def apply_clicked (self, button):
                l = []
                it = self.model.get_iter_first ()
                while (it):
                        uri = self.model.get_value (it, 2)
                        l.append (uri)
                        it= self.model.iter_next (it)
                
                self.client.set_list(LAUNCHER_PATH, gconf.VALUE_STRING, l)

        def up_clicked (self, button):
                select = self.treeview.get_selection()
                model, iter = select.get_selected ()
                uri = model.get_value (iter, 2)
                prev = None
                it = model.get_iter_first ()
                while it:
                        if model.get_value (it, 2) == uri:
                                break
                        prev = it
                        it = model.iter_next (it)
                
                if prev:
                        model.move_before (iter, prev)
        
        def down_clicked (self, button):
                select = self.treeview.get_selection()
                model, iter = select.get_selected ()
                next = model.iter_next (iter)
                if next:
                        model.move_after (iter, next)
        
        def make_model (self):
                self.model = model = gtk.ListStore(gdk.Pixbuf, str, str)
                self.treeview.set_model (model)
                
                ren = gtk.CellRendererPixbuf()
                col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
                self.treeview.append_column (col)
                
                ren = gtk.CellRendererText()
                col = gtk.TreeViewColumn ("Name", ren, markup=1)
                self.treeview.append_column (col)
                
                
                uris = self.client.get_list(LAUNCHER_PATH, gconf.VALUE_STRING)
                self.uris = uris
                
                for i in uris:
                        text = self.make_row (i)
                        if len(text) > 2:
                                row = model.append ()
                                self.model.set_value (row, 0, self.make_icon (i))
                                self.model.set_value (row, 1, text)
                                self.model.set_value (row, 2, i)
        
        def make_row (self, uri):
                try:
                        item = gnomedesktop.item_new_from_file (uri, 0)
                        text = "<b>%s</b>\n%s" % (item.get_string(gnomedesktop.KEY_NAME), item.get_string (gnomedesktop.KEY_COMMENT))
                except:
                        return ""
                return text   
        
        def make_icon (self, uri):
                icon = None
                return icon       
	


if __name__ == "__main__":
	gettext.textdomain(I18N_DOMAIN)
	gtk.glade.bindtextdomain(I18N_DOMAIN, "/usr/share/locale")
	app = main()
	gtk.main()

