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

import sys, os, subprocess
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gobject
    import gtk
    import gtk.gdk as gdk
except Exception, e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)

import awn
from xdg.DesktopEntry import DesktopEntry
import awnDefs as defs
from awnLauncherEditor import awnLauncherEditor

defs.i18nize(globals())

class awnLauncher:

    def __init__(self, glade):
        self.wTree = glade
        if not os.path.exists(defs.HOME_LAUNCHERS_DIR):
            os.makedirs(defs.HOME_LAUNCHERS_DIR)

        self.load_finished = False
        self.idle_id = 0

        self.client = awn.Config()
        self.client.ensure_group(defs.WINMAN)

        self.scrollwindow = self.wTree.get_widget("launcher_scrollwindow")
        self.make_model()

        self.applet_remove = self.wTree.get_widget("launcher_remove")
        self.applet_remove.connect("clicked", self.remove)
        self.applet_add = self.wTree.get_widget("launcher_add")
        self.applet_add.connect("clicked", self.add)
        self.launcher_edit = self.wTree.get_widget("launcher_edit")
        self.launcher_edit.connect("clicked", self.edit)

    def reordered(self, model, path, iterator, data=None):
        cur_index = self.model.get_path(iterator)[0]
        cur_uri = self.model.get_value (iterator, 2)
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

        launchers = filter(None, launchers)

        if self.load_finished:
            if (self.idle_id != 0):
                gobject.source_remove(self.idle_id)
            self.idle_id = gobject.idle_add(self.check_changes, launchers)

    def check_changes (self, data):
        self.idle_id = 0
        if (self.last_uris != data):
            self.last_uris = data[:]
            self.client.set_list(defs.WINMAN, defs.LAUNCHERS, awn.CONFIG_LIST_STRING, data)
        
        return False
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

        uris = []
        if self.client.exists(defs.WINMAN, defs.LAUNCHERS):
            uris = self.client.get_list(defs.WINMAN, defs.LAUNCHERS, awn.CONFIG_LIST_STRING)
        self.last_uris = uris[:] # make a copy
        self.client.notify_add(defs.WINMAN, defs.LAUNCHERS, self.refresh_launchers, self)

        self.refresh_tree(uris)

        self.load_finished = True

    def refresh_launchers (self, entry, extra):
        self.last_uris = entry['value']
        self.refresh_tree (self.last_uris)

    def refresh_tree (self, uris):
        self.model.clear()
        for i in uris:
            text = self.make_row (i)
            if len(text) > 2:
                row = self.model.append ()
                self.model.set_value (row, 0, self.make_icon (i))
                self.model.set_value (row, 1, text)
                self.model.set_value (row, 2, i)
        if len(uris) == 0:
            if (self.idle_id != 0):
                gobject.source_remove(self.idle_id)
            self.idle_id = gobject.idle_add(self.check_changes, [])

    def make_row (self, uri):
        try:
            if not os.path.exists(uri):
                raise IOError("Desktop file does not exist!")
            item = DesktopEntry (uri)
            text = "<b>%s</b>\n%s" % (item.getName(), item.getComment())
        except:
            return "<b>%s</b>" % _("Invalid launcher")
        return text

    def make_icon (self, uri):
        icon = None
        theme = gtk.icon_theme_get_default ()
        try:
            item = DesktopEntry (uri)
            name = item.getIcon()
            if name is None:
                return icon
        except:
            return icon

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

        if icon is None and "/" in name:
            try:
                icon = gdk.pixbuf_new_from_file_at_size (name, 32, 32)
            except:
                icon = None
        if icon is None:
            dirs = [os.path.join(p, "share", "pixmaps")
                    for p in ("/usr", "/usr/local", defs.PREFIX)]
            for d in dirs:
                n = name
                if not name.endswith(".png"):
                    n = name + ".png"
                path = os.path.join (d, n)
                try:
                    icon = gdk.pixbuf_new_from_file_at_size (path, 32, 32)
                    if icon is not None:
                        break
                except:
                    icon = None
        if icon is None and "pixmaps" in name:
            for d in dirs:
                path = os.path.join(d, name)
                try:
                    icon = gdk.pixbuf_new_from_file_at_size (path, 32, 32)
                    if icon is not None:
                        break
                except:
                    icon = None
        if icon is None:
            icon = theme.load_icon('gtk-execute', 32, 0)
        return icon


    #   Code below taken from:
    #   Alacarte Menu Editor - Simple fd.o Compliant Menu Editor
    #   Copyright (C) 2006  Travis Watkins
    #   Edited by Ryan Rushton

    def edit(self, button):
        selection = self.treeview.get_selection()
        (model, iter) = selection.get_selected()
        uri = model.get_value(iter, 2)
        editor = awnLauncherEditor(uri, self)
        editor.run()

    def add(self, button):
        file_path = os.path.join(defs.HOME_LAUNCHERS_DIR, self.getUniqueFileId('awn_launcher', '.desktop'))
        editor = awnLauncherEditor(file_path, self)
        editor.run()

    def remove(self, button):
        selection = self.treeview.get_selection()
        (model, iter) = selection.get_selected()
        uri = model.get_value(iter, 2)
        #don't check if it exists, perhaps it's invalid
        uris = self.last_uris[:]
        uris.remove(uri)
        if uri.startswith(defs.HOME_LAUNCHERS_DIR):
            os.remove(uri)
        self.refresh_tree(uris)

    def getUniqueFileId(self, name, extension):
        append = 0
        while 1:
            if append == 0:
                filename = name + extension
            else:
                filename = name + '-' + str(append) + extension
            if extension == '.desktop':
                if not os.path.isfile(os.path.join(defs.HOME_LAUNCHERS_DIR, filename)):
                    break
            append += 1
        return filename
