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
except Exception, e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)

import gtk.gdk as gdk
import gconf, gnomedesktop, gobject, subprocess, locale, gettext
import awnDefs as defs

APP = 'avant-window-navigator'
DIR = defs.LOCALEDIR
I18N_DOMAIN = "avant-window-navigator"

locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext

class awnLauncher:

    def __init__(self, glade, config_dir):
        self.wTree = glade
        self.AWN_CONFIG_LAUNCH_DIR = os.path.join(config_dir, 'launchers')
        if not os.path.exists(self.AWN_CONFIG_LAUNCH_DIR):
            os.makedirs(self.AWN_CONFIG_LAUNCH_DIR)

        self.load_finished = False

        # GCONF KEYS
        self.LAUNCHER_DIR = "/apps/avant-window-navigator/window_manager"
        self.LAUNCHER_PATH = "/apps/avant-window-navigator/window_manager/launchers"

        self.client = gconf.client_get_default()
        self.client.add_dir(self.LAUNCHER_DIR, gconf.CLIENT_PRELOAD_NONE)

        self.scrollwindow = self.wTree.get_widget("launcher_scrollwindow")
        self.make_model()

        self.applet_remove = self.wTree.get_widget("launcher_remove")
        self.applet_remove.connect("clicked", self.remove)
        self.applet_add = self.wTree.get_widget("launcher_add")
        self.applet_add.connect("clicked", self.add)

    def reordered(self, model, path, iter, data=None):
        cur_index = self.model.get_path(iter)[0]
        cur_uri = self.model.get_value (iter, 2)
        l = {}
        it = self.model.get_iter_first ()
        while (it):
            uri = self.model.get_value (it, 2)
            l[self.model.get_path(it)[0]] = uri
            it = self.model.iter_next (it)

        remove = None
        for item in l:
            if l[item] == cur_uri and cur_index != item:
                remove = item
                break
        if remove >= 0:
            del l[remove]

        launchers = []
        for item in l:
            launchers.append(l[item])

        if not None in launchers and self.load_finished:
            self.client.set_list(self.LAUNCHER_PATH, gconf.VALUE_STRING, launchers)

    def make_model (self):

        self.treeview = gtk.TreeView()
        self.treeview.set_reorderable(True)
        self.treeview.set_headers_visible(False)

        self.scrollwindow.add(self.treeview)

        self.model = model = gtk.ListStore(gdk.Pixbuf, str, str)
        self.treeview.set_model (model)

        model.connect("row-changed", self.reordered)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
        self.treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Name", ren, markup=1)
        self.treeview.append_column (col)

        self.treeview.show()

        uris = self.client.get_list(self.LAUNCHER_PATH, gconf.VALUE_STRING)
        self.uris = uris

        self.refresh_tree(uris)

        self.load_finished = True

    def refresh_tree (self, uris):
        self.model.clear()
        for i in uris:
            text = self.make_row (i)
            if len(text) > 2:
                row = self.model.append ()
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
        try:
            item = gnomedesktop.item_new_from_file (uri, 0)
            name = item.get_string(gnomedesktop.KEY_ICON)
            if name is None:
                return icon
        except:
            return icon

        theme = gtk.icon_theme_get_default ()
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
		try:
			icon = gtk.gdk.pixbuf_new_from_file_at_size (name, 32, 32)
		except:
			icon = None

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


    #   Code below taken from:
    #   Alacarte Menu Editor - Simple fd.o Compliant Menu Editor
    #   Copyright (C) 2006  Travis Watkins
    #   Edited by Ryan Rushton

    def add(self, button):
        file_path = os.path.join(self.AWN_CONFIG_LAUNCH_DIR, self.getUniqueFileId('awn_launcher', '.desktop'))
        process = subprocess.Popen(['gnome-desktop-item-edit', file_path], env=os.environ)
        gobject.timeout_add(100, self.waitForNewItemProcess, process, file_path)

    def remove(self, button):
        selection = self.treeview.get_selection()
        (model, iter) = selection.get_selected()
        uri = model.get_value(iter, 2)
        if os.path.exists(uri):
            uris = self.client.get_list(self.LAUNCHER_PATH, gconf.VALUE_STRING)
            uris.remove(uri)
            if uri.startswith(self.AWN_CONFIG_LAUNCH_DIR):
                os.remove(uri)
            self.refresh_tree(uris)

    def waitForNewItemProcess(self, process, file_path):
        if process.poll() != None:
            if os.path.isfile(file_path):
                uris = self.client.get_list(self.LAUNCHER_PATH, gconf.VALUE_STRING)
                uris.append(file_path)
                self.client.set_list(self.LAUNCHER_PATH, gconf.VALUE_STRING, uris)
                self.refresh_tree(uris)
            return False
        return True

    def getUniqueFileId(self, name, extension):
        append = 0
        while 1:
            if append == 0:
                filename = name + extension
            else:
                filename = name + '-' + str(append) + extension
            if extension == '.desktop':
                if not os.path.isfile(os.path.join(self.AWN_CONFIG_LAUNCH_DIR, filename)):
                    break
            append += 1
        return filename
