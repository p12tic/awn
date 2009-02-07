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

import sys, os, os.path, time, urllib
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gtk
    import gtk.gdk as gdk
except Exception, e:
    sys.stderr.write(str(e) + '\n')
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

        self.scrollwindow = self.wTree.get_widget("appletScrollActive1")
        self.make_active_model()
        self.treeview_available =  self.wTree.get_widget("appletTreeviewAvailable")

        self.load_applets()

        self.btn_deactivate = self.wTree.get_widget("appletDeactivate")
        self.btn_deactivate.connect("clicked", self.deactivate_applet)

        self.btn_activate = self.wTree.get_widget("appletActivate")
        self.btn_activate.connect("clicked", self.activate_applet)

        self.btn_delete = self.wTree.get_widget("appletDelete")
        self.btn_delete.connect("clicked", self.delete_applet)

        self.btn_install = self.wTree.get_widget("appletInstall")
        self.btn_install.connect("clicked", self.install_applet)

        self.treeview_available.enable_model_drag_dest([('text/plain', 0, 0)],
                  gdk.ACTION_DEFAULT | gdk.ACTION_MOVE)
        self.treeview_available.connect("drag_data_received", self.drag_data_received_data)

    def drag_data_received_data(self, treeview, context, x, y, selection, info, etime, do_apply=False):
        data = urllib.unquote(selection.data)
        data = data.replace("file://", "").replace("\r\n", "")
        if tarfile.is_tarfile(data):
            self.extract_file(data, do_apply)

    def install_applet(self, widget, data=None):
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
                self.register_applet(appletpath, True, applet_exists)
                self.register_applet(appletpath, False, applet_exists, False)
            else:
                self.register_applet(appletpath, False, applet_exists)
        else:
            message = "Applet Installation Failed"
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def register_applet(self, appletpath, do_apply, applet_exists, msg=True):
        if do_apply:
            model = self.model
        else:
            model = self.appmodel

        if applet_exists:
            message = "Applet Successfully Updated"
        else:
            icon, text, name = self.make_row (appletpath)
            if len (text) > 2:
                row = model.append ()
                model.set_value (row, 0, icon)
                model.set_value (row, 1, text)
                model.set_value (row, 2, appletpath)
                if do_apply:
                    uid = "%d" % int(time.time())
                    self.model.set_value (row, 3, uid)
                    self._apply ()
                else:
                    model.set_value (row, 3, name)                   

            if msg:
                message = "Applet Successfully Added"
            else:
                message = "Applet Installation Failed"

        if msg:
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def activate_applet (self, button):
        select = self.treeview_available.get_selection()
        if not select:
            print "no selection"
            return
        model, iterator = select.get_selected ()
        path = model.get_value (iterator, 2)
        icon, text, name = self.make_row (path)
        uid = "%d" % int(time.time())
        if len (text) < 2:
            print "cannot load desktop file %s" % path
            return

        self.active_model.append([icon, path, uid, text])

        self._apply ()

    def row_active (self, q, w, e):
        self.activate_applet (None)

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

        self.active_model.foreach(self.test_active, path)
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
            execpath = item.getExec()
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

    def deactivate_applet (self, button):
        cursor = self.icon_view.get_cursor()
        if not cursor:
            return
        itr = self.active_model.get_iter(cursor[0])
        name = os.path.splitext(os.path.basename(self.active_model.get_value(itr, 1)))[0]
        uid = self.active_model.get_value (itr, 2)
        applet_client = awn.Config(name, uid)
        applet_client.clear()
        self.active_model.remove(itr)
        self._apply()

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
        it = self.active_model.get_iter_first ()
        while (it):
            path = self.active_model.get_value (it, 1)
            uid = self.active_model.get_value (it, 2)
            s = "%s::%s" % (path, uid)
            l.append (s)
            it= self.active_model.iter_next (it)

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

    def make_active_model (self):
        self.active_model = gtk.ListStore(gtk.gdk.Pixbuf, str, str, str)
        self.active_model.connect("row-changed", self.applet_reorder)

        self.icon_view = gtk.IconView(self.active_model)
        self.icon_view.set_pixbuf_column(0)
        self.icon_view.set_orientation(gtk.ORIENTATION_HORIZONTAL)
        self.icon_view.set_selection_mode(gtk.SELECTION_SINGLE)
        if hasattr(self.icon_view, 'set_tooltip_column'):
            self.icon_view.set_tooltip_column(3)
        self.icon_view.set_item_width(-1)
        self.icon_view.set_size_request(48, -1)
        self.icon_view.set_reorderable(True)
        self.icon_view.set_columns(100)

        self.scrollwindow.add(self.icon_view)
        self.scrollwindow.show()

        applets = self.client.get_list(defs.AWN, defs.APPLET_LIST, awn.CONFIG_LIST_STRING)

        self.refresh_icon_list (applets, self.active_model)

    def make_row (self, path):
        text = ""
        name = ""
        try:
            item = DesktopEntry (path)
            text = "<b>%s</b>\n%s" % (item.getName(), item.getComment())
            name = item.getName();
        except:
            return None, "", ""
        return self.make_icon (item.getIcon()), text, name

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
            try:
                icon = gdk.pixbuf_new_from_file_at_size (name, 32, 32)
            except:
                print "Error loading icon " + name
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
            dirs = [os.path.join(p, "share", "avant-window-navigator","applets")
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
        if icon is None:
            dirs = [os.path.join(p, "share", "avant-window-navigator","applets")
                    for p in ("/usr", "/usr/local", defs.PREFIX)]
            for d in dirs:
                n = name
                if not name.endswith(".svg"):
                    n = name + ".svg"
                path = os.path.join (d, n)
                try:
                    icon = gdk.pixbuf_new_from_file_at_size (path, 32, 32)
                    if icon is not None:
                        break
                except:
                    icon = None
        if icon is None:
            icon = theme.load_icon('gtk-execute', 32, 0)
        return icon

    def refresh_icon_list (self, applets, model):
        for a in applets:
            tokens = a.split("::")
            path = tokens[0]
            uid = tokens[1]
            icon, text, name = self.make_row(path)
            if len (text) < 2:
                continue;

            model.append([icon, path, uid, text])

    def refresh_tree (self, applets):
        for a in applets:
            tokens = a.split("::")
            path = tokens[0]
            uid = tokens[1]
            icon, text, name = self.make_row(path)
            if len (text) < 2:
                continue;

            row = self.model.append ()
            self.model.set_value (row, 0, icon)
            self.model.set_value (row, 1, text)
            self.model.set_value (row, 2, path)
            self.model.set_value (row, 3, uid)

    def make_appmodel (self):

        self.appmodel = model = gtk.ListStore(gdk.Pixbuf, str, str, str)
        self.appmodel.set_sort_column_id(1, gtk.SORT_ASCENDING)
        self.treeview_available.set_model (model)
        self.treeview_available.set_search_column (3)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Available Applets", ren, pixbuf=0)

        ren = gtk.CellRendererText()
        ren = gtk.CellRendererText()
        col.pack_start(ren, False)
        col.add_attribute(ren, 'markup', 1)
        ren.set_property('xalign', 0)

        self.treeview_available.append_column (col)

    def load_applets (self):
        self.make_appmodel ()
        model = self.appmodel

        prefixes = ["/usr/lib", "/usr/local/lib", "/usr/lib64", "/usr/local/lib64"]
        prefixes.append(os.path.expanduser("~/.config"))
        dirs = [os.path.join(prefix, "awn", "applets") for prefix in prefixes]
        dirs.insert(0, os.path.join(defs.PREFIX, "share", "avant-window-navigator", "applets"))
        applets = []
        for d in dirs:
            if not os.path.exists (d):
                continue
            if not os.path.realpath(d) == d and os.path.realpath(d) in dirs:
                continue

            applets += [os.path.join(d, a) for a in os.listdir(d) if a.endswith(".desktop")]

        for a in applets:
            icon, text, name = self.make_row (a)
            if len (text) < 2:
                continue;
            row = model.append ()
            model.set_value (row, 0, icon)
            model.set_value (row, 1, text)
            model.set_value (row, 2, a)
            model.set_value (row, 3, name)
        self.load_finished = True

    def popup_msg(self, message):
        success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING,
                                    buttons=gtk.BUTTONS_OK, message_format=message)
        success.run()
        success.destroy()

    def applet_reorder(self, model, path, iterator, data=None):
        cur_index = model.get_path(iterator)[0]
        cur_uri = model.get_value (iterator, 1)
        cur_uid = model.get_value (iterator, 2)
        cur_s = "%s::%s" % (cur_uri, cur_uid)
        l = {}
        it = model.get_iter_first ()
        while (it):
            path = model.get_value (it, 1)
            uid = model.get_value (it, 2)
            s = "%s::%s" % (path, uid)
            l[model.get_path(it)[0]] = s
            it = model.iter_next (it)

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
