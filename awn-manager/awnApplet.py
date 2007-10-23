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

import sys, os, time, urllib
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.gdk as gdk
except:
    sys.exit(1)

import awn
import awnDefs as defs
import tarfile
from xdg.DesktopEntry import DesktopEntry

defs.i18nize(globals())

class awnApplet:

    def __init__(self, glade):
        # DIRS
        if not os.path.isdir(defs.HOME_APPLET_DIR):
          os.mkdir(defs.HOME_APPLET_DIR)

        self.client = awn.Config()
        self.client.ensure_group(defs.AWN)

        self.treeview_current = None
        self.load_finished = False

        self.wTree = glade
        self.scrollwindow = self.wTree.get_widget("applet_scrollwindow")
        self.make_model()
        self.treeview_available =  self.wTree.get_widget("applet_treeview_available")
        self.load_applets()
        self.applet_remove = self.wTree.get_widget("applet_remove")
        self.applet_remove.connect("clicked", self.remove_clicked)
        self.applet_add = self.wTree.get_widget("applet_add")
        self.applet_add.connect("clicked", self.add_applet)

        self.applet_delete = self.wTree.get_widget("deleteapplet")
        self.applet_delete.connect("clicked", self.delete_applet)

        self.applet_install = self.wTree.get_widget("installapplet")
        self.applet_install.connect("clicked", self.add)

        self.treeview_available.enable_model_drag_dest([('text/plain', 0, 0)],
                  gdk.ACTION_DEFAULT | gdk.ACTION_MOVE)
        self.treeview_available.connect("drag_data_received", self.drag_data_received_data)

    def drag_data_received_data(self, treeview, context, x, y, selection, info, etime, do_apply=False):
        data = urllib.unquote(selection.data)
        data = data.replace("file://", "").replace("\r\n", "")
        if tarfile.is_tarfile(data):
            self.extract_file(data, do_apply)

    def add(self, widget, data=None):
        dialog = gtk.FileChooserDialog(title=None,action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                  buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)

        filter = gtk.FileFilter()
        filter.set_name("AWN Applet Package")
        filter.add_pattern("*.tar.gz")
        filter.add_pattern("*.tgz")
        filter.add_pattern("*.bz2")
        filter.add_pattern("*.awn")
        dialog.add_filter(filter)

        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            file = dialog.get_filename()
            dialog.destroy()
            if tarfile.is_tarfile(file):
              self.extract_file(file, False)
        else:
          dialog.destroy()

    def check_path(self, appletpath):
        df = DesktopEntry(appletpath)
        icon_path = df.getIcon()
        if not icon_path.startswith('/') and '/' not in icon_path:
            df.set("Icon", os.path.join(defs.HOME_APPLET_DIR, icon_path))
            df.write()

    def extract_file(self, filename, do_apply):
        appletpath = ""
        applet_exists = False
        tar = tarfile.open(filename, "r:gz")
        for member in tar.getmembers():
            if member.name.endswith(".desktop"):
                appletpath = os.path.join(defs.HOME_APPLET_DIR, member.name)

        applet_exists = os.path.exists(appletpath)

        [tar.extract(f, defs.HOME_APPLET_DIR) for f in tar.getnames()]
        tar.close()

        if appletpath:
            self.check_path(appletpath)

            if do_apply:
                self.install_applet(appletpath, True, applet_exists)
                self.install_applet(appletpath, False, applet_exists, False)
            else:
                self.install_applet(appletpath, False, applet_exists)
        else:
            message = "Applet Installation Failed"
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def install_applet(self, appletpath, do_apply, applet_exists, msg=True):
        if do_apply:
            model = self.model
        else:
            model = self.appmodel

        if applet_exists:
            message = "Applet Successfully Updated"
        else:
            icon, text = self.make_row (appletpath)
            if len (text) > 2:
                row = model.append ()
                model.set_value (row, 0, icon)
                model.set_value (row, 1, text)
                model.set_value (row, 2, appletpath)
            if do_apply:
                uid = "%d" % int(time.time())
                self.model.set_value (row, 3, uid)
                self._apply ()
            if msg:
                message = "Applet Successfully Added"
            else:
                message = "Applet Installation Failed"

        if msg:
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def add_applet (self, button):
        select = self.treeview_available.get_selection()
        if not select:
            print "no selection"
            return
        model, iterator = select.get_selected ()
        path = model.get_value (iterator, 2)
        icon, text = self.make_row (path)
        uid = "%d" % int(time.time())
        if len (text) < 2:
            print "cannot load desktop file %s" % path
            return

        row = self.model.append ()
        self.model.set_value (row, 0, icon)
        self.model.set_value (row, 1, text)
        self.model.set_value (row, 2, path)
        self.model.set_value (row, 3, uid)

        self._apply ()

    def row_active (self, q, w, e):
        self.add_applet (None)

    def test_active(self, model, path, iterator, sel_path):
        if model.get_value (iterator, 2) == sel_path:
            self.active_found = True
            return True

    def delete_applet(self,widget):
        self.active_found = False
        select = self.treeview_available.get_selection()
        if not select:
            return
        model, iterator = select.get_selected ()
        path = model.get_value (iterator, 2)
        item = DesktopEntry (path)

        self.model.foreach(self.test_active, path)
        if self.active_found:
            self.popup_msg("Can not delete active applet")
            return

        dialog = gtk.Dialog("Delete Applet",
                            None,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                            gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        label = gtk.Label("<b>Delete %s?</b>" % item.getName())
        label.set_use_markup(True)
        align = gtk.Alignment()
        align.set_padding(5,5,5,5)
        align.add(label)
        dialog.vbox.add(align)
        dialog.show_all()
        result = dialog.run()

        if result == -3:
            execpath = item.get_exec()
            fullpath = os.path.join(defs.HOME_APPLET_DIR, os.path.split(execpath)[0])

            if os.path.exists(fullpath) and ".config" in path:
                model.remove (iterator)
                self.remove_applet_dir(fullpath, path)
                self._apply ()
                dialog.destroy()
            else:
                dialog.destroy()
                self.popup_msg("Unable to Delete Applet")
        else:
            dialog.destroy()

    def remove_clicked (self, button):
        select = self.treeview_current.get_selection()
        if not select:
            return
        model, iterator = select.get_selected ()
        self.remove_keys (model, iterator)
        model.remove (iterator)
        self._apply ()

    def remove_keys (self, model, iterator):
        uid = model.get_value (iterator, 3)
        applet_client = awn.config_for_applet(uid)
        applet_client.clear()

    def remove_applet_dir(self, dirPath, filename):
        namesHere = os.listdir(dirPath)
        for name in namesHere:
            path = os.path.join(dirPath, name)
            if not os.path.isdir(path):
                os.remove(path)
            else:
                self.remove_applet_dir(path, filename)
        os.rmdir(dirPath)
        if os.path.exists(filename):
          os.unlink(filename)

    def _apply (self):
        l = []
        it = self.model.get_iter_first ()
        while (it):
            path = self.model.get_value (it, 2)
            uid = self.model.get_value (it, 3)
            s = "%s::%s" % (path, uid)
            l.append (s)
            it= self.model.iter_next (it)

        self.client.set_list(defs.AWN, defs.APPLET_LIST, awn.CONFIG_LIST_STRING, l)

    def up_clicked (self, button):
        select = self.treeview.get_selection()
        model, iterator = select.get_selected ()
        uri = model.get_value (iterator, 2)
        prev = None
        it = model.get_iter_first ()
        while it:
            if model.get_value (it, 2) == uri:
                break
            prev = it
            it = model.iter_next (it)

        if prev:
            model.move_before (iterator, prev)
        self._apply ()

    def down_clicked (self, button):
        select = self.treeview.get_selection()
        model, iterator = select.get_selected ()
        next = model.iter_next (iterator)
        if next:
            model.move_after (iterator, next)
        self._apply ()

    def make_model (self):
        self.treeview_current = gtk.TreeView()
        self.treeview_current.set_reorderable(True)
        self.treeview_current.set_headers_visible(True)

        self.scrollwindow.add(self.treeview_current)

        self.model = model = gtk.ListStore(gdk.Pixbuf, str, str, str)
        self.treeview_current.set_model (model)
        self.model.connect("row-changed", self.reordered)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Active Applets", ren, pixbuf=0)

        ren = gtk.CellRendererText()
        col.pack_start(ren, False)
        col.add_attribute(ren, 'markup', 1)
        ren.set_property('xalign', 0)

        self.treeview_current.append_column (col)
        self.treeview_current.show()

        applets = self.client.get_list(defs.AWN, defs.APPLET_LIST, awn.CONFIG_LIST_STRING)

        self.refresh_tree (applets)

    def make_row (self, path):
        text = ""
        try:
            item = DesktopEntry (path)
            text = "<b>%s</b>\n%s" % (item.getName(), item.getComment())
        except:
            return None, ""
        return self.make_icon (item.getIcon()), text

    def make_icon (self, name):
        icon = None

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

        if icon is None and "/" in name and os.path.exists(name):
            icon = gdk.pixbuf_new_from_file_at_size (name, 32, 32)
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
        self.treeview_available.set_model (model)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Available Applets", ren, pixbuf=0)

        ren = gtk.CellRendererText()
        col.pack_start(ren, False)
        col.add_attribute(ren, 'markup', 1)
        ren.set_property('xalign', 0)

        self.treeview_available.append_column (col)

    def load_applets (self):
        self.make_appmodel ()
        model = self.appmodel

        prefixes = ["/usr/lib", "/usr/local/lib", "/usr/lib64", "/usr/local/lib64",
                    os.path.join(defs.PREFIX, "lib"), os.path.expanduser("~/.config")]
        dirs = [os.path.join(prefix, "awn", "applets") for prefix in prefixes]
        applets = []
        for d in dirs:
            if not os.path.exists (d):
                continue
            if not os.path.realpath(d) == d and os.path.realpath(d) in dirs:
                continue

            applets += [os.path.join(d, a) for a in os.listdir(d) if a.endswith(".desktop")]

        for a in applets:
            icon, text = self.make_row (a)
            if len (text) < 2:
                continue;
            row = model.append ()
            model.set_value (row, 0, icon)
            model.set_value (row, 1, text)
            model.set_value (row, 2, a)
        self.load_finished = True

    def popup_msg(self, message):
        success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING,
                                    buttons=gtk.BUTTONS_OK, message_format=message)
        success.run()
        success.destroy()

    def reordered(self, model, path, iterator, data=None):
        cur_index = self.model.get_path(iterator)[0]
        cur_uri = self.model.get_value (iterator, 2)
        cur_uid = self.model.get_value (iterator, 3)
        cur_s = "%s::%s" % (cur_uri, cur_uid)
        l = {}
        it = self.model.get_iter_first ()
        while (it):
            path = self.model.get_value (it, 2)
            uid = self.model.get_value (it, 3)
            s = "%s::%s" % (path, uid)
            l[self.model.get_path(it)[0]] = s
            it = self.model.iter_next (it)

        remove = None
        for item in l:
            if l[item] == cur_s and cur_index != item:
                remove = item
                break
        if remove >= 0:
            del l[remove]

        applets = l.values()

        if not None in applets and self.load_finished:
            self.client.set_list(defs.AWN, defs.APPLET_LIST, awn.CONFIG_LIST_STRING, applets)
# vim: set et ts=4 sts=4 sw=4 :
