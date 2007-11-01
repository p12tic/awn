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
    import gtk.glade
except Exception, e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)

import gconf
import gnomedesktop
import gtk.gdk as gdk
import awnDefs as defs
import tarfile

APP = 'avant-window-navigator'
DIR = defs.LOCALEDIR
I18N_DOMAIN = "avant-window-navigator"

import locale
import gettext
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext





class awnApplet:

    def __init__(self, glade):
        # GCONF KEYS
        self.APPLETS_DIR = "/apps/avant-window-navigator"
        self.APPLETS_PATH = "/apps/avant-window-navigator/applets_list"

        # DIRS
        self.AWN_APPLET_DIR = os.path.join(os.path.expanduser('~'), ".config/awn/applets")
        if not os.path.isdir(self.AWN_APPLET_DIR):
          os.mkdir(self.AWN_APPLET_DIR)

        self.client = gconf.client_get_default()
        self.client.add_dir(self.APPLETS_DIR, gconf.CLIENT_PRELOAD_NONE)

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
                  gtk.gdk.ACTION_DEFAULT | gtk.gdk.ACTION_MOVE)
        self.treeview_available.connect("drag_data_received", self.drag_data_received_data)

    def drag_data_received_data(self, treeview, context, x, y, selection, info, etime, apply=False):
        data = selection.data
        data = urllib.unquote(data)
        data = data.replace('file://',"")
        data = data.replace("\r\n","")
        if tarfile.is_tarfile(data):
            self.extract_file(data, apply)

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
      df = gnomedesktop.item_new_from_file (appletpath, 0)
      icon_path = df.get_string(gnomedesktop.KEY_ICON)
      if(not icon_path.startswith('/') and icon_path.find('/') != -1):
        df.set_string(gnomedesktop.KEY_ICON, os.path.join(self.AWN_APPLET_DIR, icon_path))
        df.save(appletpath, False)

    def extract_file(self, file, apply):
      appletpath = ""
      applet_exists = False
      tar = tarfile.open(file, "r:gz")
      filelist = tar.getmembers()
      for file in filelist:
        if ".desktop" in file.name:
          appletpath = os.path.join(self.AWN_APPLET_DIR, file.name)

      if os.path.exists(appletpath):
        applet_exists = True

      for file in tar.getnames():
        tar.extract(file, self.AWN_APPLET_DIR)
      tar.close()

      if appletpath:
        self.check_path(appletpath)

        if apply:
          self.install_applet(appletpath, True, applet_exists)
          self.install_applet(appletpath, False, applet_exists, False)
        else:
          self.install_applet(appletpath, False, applet_exists)
      else:
        message = "Applet Installation Failed"
        success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
        success.run()
        success.destroy()

    def install_applet(self, appletpath, apply, applet_exists, msg=True):
      if apply:
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
          if apply:
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
            model, iter = select.get_selected ()
            path = model.get_value (iter, 2)
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

    def test_active(self, model, path, iter, sel_path):
      if model.get_value (iter, 2) == sel_path:
        self.active_found = True
        return True

    def delete_applet(self,widget):
      self.active_found = False
      select = self.treeview_available.get_selection()
      if not select:
              return
      model, iter = select.get_selected ()
      path = model.get_value (iter, 2)
      item = gnomedesktop.item_new_from_file (path, 0)

      self.model.foreach(self.test_active, path)
      if self.active_found:
        self.popup_msg("Can not delete active applet")
        return

      dialog = gtk.Dialog("Delete Applet",
                     None,
                     gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                     (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                      gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
      label = gtk.Label("<b>Delete "+item.get_string(gnomedesktop.KEY_NAME) +"?</b>")
      label.set_use_markup(True)
      align = gtk.Alignment()
      align.set_padding(5,5,5,5)
      align.add(label)
      dialog.vbox.add(align)
      dialog.show_all()
      result = dialog.run()

      if result == -3:
        execpath = item.get_string(gnomedesktop.KEY_EXEC)
        fullpath = os.path.join(self.AWN_APPLET_DIR, os.path.split(execpath)[0])

        if os.path.exists(fullpath) and ".config" in path:
          model.remove (iter)
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

            self.client.set_list(self.APPLETS_PATH, gconf.VALUE_STRING, l)

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

        applets = self.client.get_list(self.APPLETS_PATH, gconf.VALUE_STRING)

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
                    if "/" in name and os.path.exists(name):
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
            self.treeview_available.set_model (model)

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
        install_prefix = os.path.join(defs.PREFIX, "lib")
        if install_prefix not in prefixes:
            prefixes.append(install_prefix)
        prefixes.append(os.path.expanduser("~/.config"))
        dirs = [os.path.join(prefix, "awn", "applets") for prefix in prefixes]
        applets = []
        for d in dirs:
                if not os.path.exists (d):
                        continue
                if not os.path.realpath(d) == d and os.path.realpath(d) in dirs:
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
        self.load_finished = True

    def popup_msg(self, message):
      success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
      success.run()
      success.destroy()

    def reordered(self, model, path, iter, data=None):
        cur_index = self.model.get_path(iter)[0]
        cur_uri = self.model.get_value (iter, 2)
        cur_uid = self.model.get_value (iter, 3)
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

        applets = []
        for item in l:
            applets.append(l[item])

        if not None in applets and self.load_finished:
            self.client.set_list(self.APPLETS_PATH, gconf.VALUE_STRING, applets)
