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
#  Notes: Avant Window Navigator applet preferences window

import sys, os, time
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

APPLETS_DIR = "/apps/avant-window-navigator"
APPLETS_PATH = "/apps/avant-window-navigator/applets_list"

DATA_DIR = "@PKGDATADIR@"


class main:
	
	def __init__(self):
		
		self.client = gconf.client_get_default()
		self.client.add_dir(APPLETS_DIR, gconf.CLIENT_PRELOAD_NONE)
		
		#Set the Glade file
		gtk.glade.bindtextdomain(APP, DIR)
		gtk.glade.textdomain(APP)
		self.gladefile = os.path.join(DATA_DIR, "avant-applets.glade") 
		print self.gladefile 
	        self.wTree = gtk.glade.XML(self.gladefile, domain=I18N_DOMAIN) 
		
		#Get the Main Window, and connect the "destroy" event
		self.window = self.wTree.get_widget("main_window")
		self.window.connect("delete-event", gtk.main_quit)
		
		self.treeview =  self.wTree.get_widget("treeview")
		self.make_model ()
		
		close = self.wTree.get_widget("closebutton")
		close.connect("clicked", gtk.main_quit)
		
		self.add = self.wTree.get_widget ("add")
		self.add.connect("clicked", self.add_clicked)
		
		self.remove = self.wTree.get_widget ("remove")
		self.remove.connect("clicked", self.remove_clicked)
		
		self.up = self.wTree.get_widget ("up")
		self.up.connect("clicked", self.up_clicked)
		
		self.down = self.wTree.get_widget ("down")
		self.down.connect("clicked", self.down_clicked)
		
		self.addwindow = self.wTree.get_widget("addapplet")
		self.addwindow.hide ()
		
		self.addview =  self.wTree.get_widget("treeviewnew")
		self.addview.connect ("row-activated", self.row_active)
		self.load_applets ()

		self.addaccept = self.wTree.get_widget ("addaccept")
		self.addaccept.connect("clicked", self.add_applet)
		
		self.addcancel = self.wTree.get_widget ("addcancel")
		self.addcancel.connect("clicked", self.cancel_clicked)		
		
		

	def cancel_clicked (self, button):
                self.addwindow.hide ()

        def add_applet (self, button):
                select = self.addview.get_selection()
                if not select:
                        print "no selection"
                        self.addwindow.hide ()
                        return
                model, iter = select.get_selected ()
                path = model.get_value (iter, 2)
                icon, text = self.make_row (path)
                uid = "%d" % int(time.time())
                if len (text) < 2:
                        print "cannot load desktop file %s" % path
                        self.addwindow.hide ()
                        return
                
                row = self.model.append ()
                self.model.set_value (row, 0, icon)
                self.model.set_value (row, 1, text)
                self.model.set_value (row, 2, path)
                self.model.set_value (row, 3, uid)
                
                self._apply ()
                self.addwindow.hide ()

        def row_active (self, q, w, e):
                self.add_applet (None)
        
        def add_clicked (self, button):
                self.addwindow.show ()
                
        def remove_clicked (self, button):
                select = self.treeview.get_selection()
                if not select:
                        return
                model, iter = select.get_selected ()
                self.remove_keys (model, iter)
                model.remove (iter)
                self._apply ()
        
        def remove_keys (self, model, iter):
                engine = gconf.engine_get_default ()
                uid = model.get_value (iter, 3)
                key = "/apps/avant-window-navigator/applets/%s" % uid
                try:
                        engine.remove_dir (key)
                except:
                        pass                        
                
                
	def win_destroy(self, button, w):
		w.destroy()
        
        def _apply (self):
                l = []
                it = self.model.get_iter_first ()
                while (it):
                        path = self.model.get_value (it, 2)
                        uid = self.model.get_value (it, 3)
                        s = "%s::%s" % (path, uid)
                        l.append (s)
                        it= self.model.iter_next (it)
                
                self.client.set_list(APPLETS_PATH, gconf.VALUE_STRING, l)

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
                self._apply ()
        
        def down_clicked (self, button):
                select = self.treeview.get_selection()
                model, iter = select.get_selected ()
                next = model.iter_next (iter)
                if next:
                        model.move_after (iter, next)
                self._apply ()
        
        def make_model (self):
                self.model = model = gtk.ListStore(gdk.Pixbuf, str, str, str)
                self.treeview.set_model (model)
                
                ren = gtk.CellRendererPixbuf()
                col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
                self.treeview.append_column (col)
                
                ren = gtk.CellRendererText()
                col = gtk.TreeViewColumn ("Name", ren, markup=1)
                self.treeview.append_column (col)
                
                applets = self.client.get_list(APPLETS_PATH, gconf.VALUE_STRING)
                
                self.refresh_tree (applets)                
                        
        def make_row (self, path):
                text = ""
                try:
                        item = gnomedesktop.item_new_from_file (path, 0)
                        text = "<b>%s</b>\n%s" % (item.get_string(gnomedesktop.KEY_NAME), item.get_string (gnomedesktop.KEY_COMMENT))
                except:
                        return None, ""
                return self.make_icon (item.get_string(gnomedesktop.KEY_ICON)), text   
        
        def make_icon (self, name):
                icon = None

                theme = gtk.IconTheme ()
                try:
                        icon = theme.load_icon (name, 32, 0)
                except:
                        icon = None
                #Hack hack hack
                if icon is None:
                        try:
                                i = gtk.image_new_from_stock (name, 32)
                                icon = i.get_pixbuf ()
                        except:
                                icon = None
                
                if icon is None:
                        if "/" in name:
                                icon = gtk.gdk.pixbuf_new_from_file_at_size (name, 32, 32)
                if icon is None:
                        dirs = ["/usr/share/pixmaps", "/usr/local/share/pixmaps"]
                        for d in dirs:
                                n = name
                                if not ".png" in name:
                                        n = name + ".png"
                                path = os.path.join (d, n)
                                if icon is None:
                                        try:
                                                icon = gtk.gdk.pixbuf_new_from_file_at_size (path, 32, 32)
                                        except: 
                                                icon = None
                if icon is None:
                        if "pixmaps" in name:
                                path1 = os.path.join ("/usr/share/pixmaps", name)
                                path2 = os.path.join ("/usr/local/share/pixmaps",
                                                      name)
                                try:
                                        icon = gtk.gdk.pixbuf_new_from_file_at_size (path1, 32, 32)
                                except:
                                        icon = None
                                        
                                if icon is None:
                                        try:
                                                icon = gtk.gdk.pixbuf_new_from_file_at_scale (path2, 32, 32)
                                        except:
                                                icon = None
                return icon       
	
        def refresh_tree (self, applets):
                
                for a in applets:
                        tokens = a.split("::")
                        path = tokens[0]
                        uid = tokens[1]
                        icon, text = self.make_row(path)
                        if len (text) < 2:
                                continue;
                        
                        row = self.model.append ()
                        self.model.set_value (row, 0, icon)
                        self.model.set_value (row, 1, text)
                        self.model.set_value (row, 2, path)
                        self.model.set_value (row, 3, uid)
        
        def make_appmodel (self):
                self.appmodel = model = gtk.ListStore(gdk.Pixbuf, str, str)
                self.addview.set_model (model)
                
                ren = gtk.CellRendererPixbuf()
                col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
                self.addview.append_column (col)
                
                ren = gtk.CellRendererText()
                col = gtk.TreeViewColumn ("Name", ren, markup=1)
                self.addview.append_column (col)
                
        def load_applets (self):
                self.make_appmodel ()
                model = self.appmodel
                
                hdir = os.path.join (os.environ["HOME"], ".awn/applets")
                dirs = ["/usr/lib/awn/applets", 
                        "/usr/local/lib/awn/applets",
                        "/usr/lib64/awn/applets",
                        "/usr/local/lib64/awn/applets",
                        hdir]
                applets = []
                for d in dirs:
                        if not os.path.exists (d):
                                continue
                        apps = os.listdir (d)
                        for a in apps:
                                if ".desktop" in a:
                                        path = os.path.join (d, a)
                                        applets.append (path)
                
                for a in applets:
                        icon, text = self.make_row (a)
                        if len (text) < 2:
                                continue;
                        row = model.append ()
                        model.set_value (row, 0, icon)
                        model.set_value (row, 1, text)
                        model.set_value (row, 2, a)                       


if __name__ == "__main__":
	gettext.textdomain(I18N_DOMAIN)
	gtk.glade.bindtextdomain(I18N_DOMAIN, "/usr/share/locale")
	app = main()
	gtk.main()

