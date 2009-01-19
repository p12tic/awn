
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

from ConfigParser import ConfigParser
import shutil
import tarfile
import os
import gtk
import gobject

import awn
import awnDefs as defs

def stringbool( value ):
    if( value == "False"):
        return False
    else:
        return bool(value)

CONFIG_TYPE_MAP = {
    bool: 'bool',
    stringbool: 'bool',
    float: 'float',
    int: 'int',
    str: 'string'
    }

def get_typefunc(ptype, prefix):
    return '%s_%s' % (prefix, CONFIG_TYPE_MAP[ptype])

class Pref:
    pref_list = {}
    def __init__(self, name, ptype):
        self.name = name
        self.ptype = ptype
        Pref.pref_list[self.name] = self

    @staticmethod
    def lookup(name):
        if name in Pref.pref_list:
            return Pref.pref_list[name]
        else:
            return None

    def typecast(self, value):
        return self.ptype(value)

class AwnThemeManager:

    def __init__(self, glade):
        self.wTree = glade
        self.theme_treeview = self.wTree.get_widget('theme_treeview')
        self.theme_add = self.wTree.get_widget('theme_add')
        self.theme_add.connect("clicked", self.add)
        self.theme_remove = self.wTree.get_widget('theme_remove')
        self.theme_remove.connect("clicked", self.delete)
        self.theme_save = self.wTree.get_widget('theme_save')
        self.theme_save.connect("clicked", self.save)
        self.theme_apply = self.wTree.get_widget('theme_apply')
        self.theme_apply.connect("clicked", self.apply_theme)

        self.AWN_CONFIG = 'theme.awn'
        self.AWN_THUMB = 'thumb.png'
        self.AWN_CUSTOM_ICONS = 'custom-icons'
        self.AWN_CURRENT = os.path.join(defs.HOME_THEME_DIR, 'current.awn')
        self.CONFIG = awn.Config()
        self.BUILD_DIR = "/tmp/awn_theme_build"
        self.THEME_PREFS = {
            '': [Pref('autohide', stringbool)],
            'app': [Pref('active_png', str), Pref('alpha_effect', stringbool),
                    Pref('arrow_color', str),Pref('hover_bounce_effect', stringbool),
                    Pref('tasks_have_arrows', stringbool)],
            'bar': [Pref('bar_angle', int), Pref('bar_height', int),
                    Pref('border_color', str), Pref('corner_radius', float),
                    Pref('glass_histep_1', str), Pref('glass_histep_2', str),
                    Pref('glass_step_1', str), Pref('glass_step_2', str),
                    Pref('hilight_color', str), Pref('icon_offset', int),
                    Pref('pattern_alpha', float), Pref('pattern_uri', str),
                    Pref('render_pattern', stringbool), Pref('rounded_corners', stringbool),
                    Pref('sep_color', str), Pref('show_separator', stringbool)],
            'title': [Pref('background', str), Pref('font_face', str),
                      Pref('shadow_color', str), Pref('text_color', str)]
            }
        self.theme_list = []
        self.currItr = None
        self.model = None
        self.window = self.wTree.get_widget("main_window")
        self.make_model()

    def list_themes(self, model):
        self.model = model
        curr_name = ''
        curr_version = ''
        if os.path.exists(self.AWN_CURRENT):
            curr = open(self.AWN_CURRENT, "rb")
            lines = curr.readlines()
            if len(lines) == 2:
                curr_name = lines[0].rstrip('\n')
                curr_version = lines[1].rstrip('\n')
                curr.close()

        if not os.path.exists(defs.HOME_THEME_DIR) and not os.path.isdir(defs.HOME_THEME_DIR):
            os.makedirs(defs.HOME_THEME_DIR)

        theme_dirs = []
        if os.path.exists(defs.SYS_THEME_DIR):
            theme_dirs = map(lambda x: os.path.join(defs.SYS_THEME_DIR, x),
                             self.list_dirs(defs.SYS_THEME_DIR))

        theme_dirs = theme_dirs + map(lambda x: 
                     os.path.join(defs.HOME_THEME_DIR, x),
                     self.list_dirs(defs.HOME_THEME_DIR))

        for full_dir in theme_dirs:
            theme_path = os.path.join(full_dir, self.AWN_CONFIG)
            thumb_path = os.path.join(full_dir, self.AWN_THUMB)

            if os.path.exists(theme_path):
                cfg = self.read(theme_path)
                if cfg:
                    if os.path.exists(thumb_path):
                        self.pixbuf = gtk.gdk.pixbuf_new_from_file (thumb_path)
                    else:
                        self.pixbuf = None

                    setRadio = curr_name == cfg['details']['name'] and curr_version == cfg['details']['version']

                    row = model.append (None, (setRadio, self.pixbuf, "Theme: %s\nVersion: %s\nAuthor: %s\nDate: %s" % (cfg['details']['name'], cfg['details']['version'], cfg['details']['author'], cfg['details']['date'])))

                    self.theme_list.append({
                        'name': cfg['details']['name'],
                        'version': cfg['details']['version'],
                        'full-dir': full_dir
                        })
                    if setRadio:
                        self.currItr = row
                    else:
                        self.currItr = ''


    def apply_theme(self, widget, data=None):
        if self.currItr is not None:
            
            self.model.foreach(self.update_radio)
            
            index = self.model.get_path(self.currItr)[0]
            name = self.theme_list[index]['name']
            version = self.theme_list[index]['version']
            directory = self.theme_list[index]['full-dir']

            curr = open(self.AWN_CURRENT, 'w')
            curr.write(name+"\n")
            curr.write(version+"\n")
            curr.close()

            theme_config = self.read(os.path.join(directory, self.AWN_CONFIG))

            for group, entries in theme_config.iteritems():
                if group != 'details':
                    if group == "root":
                        group = awn.CONFIG_DEFAULT_GROUP
                    for key, value in entries.iteritems():
                        pref = Pref.lookup(key)
                        if pref is not None:
                            getattr(self.CONFIG, get_typefunc(pref.ptype, 'set'))(group, key, pref.typecast(value))

            if self.CONFIG.get_bool(defs.BAR, defs.RENDER_PATTERN):
                self.CONFIG.set_string(defs.BAR, defs.PATTERN_URI, os.path.join(directory, "pattern.png"))

    def add(self, widget, data=None):
        dialog = gtk.FileChooserDialog(title=None,action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                  buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)

        ffilter = gtk.FileFilter()
        ffilter.set_name("Compressed Files")
        ffilter.add_pattern("*.tar.gz")
        ffilter.add_pattern("*.tgz")
        ffilter.add_pattern("*.bz2")
        dialog.add_filter(ffilter)

        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            fname = dialog.get_filename()
            if not self.extract_file(fname):
                invalid = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format="Invalid Theme Format")
                invalid.run()
                invalid.destroy()
        dialog.destroy()

    def extract_file(self, fname):
        tar = tarfile.open(fname, "r:gz")
        filelist = tar.getmembers()
        theme_found = False
        thumb_found = False
        for f in filelist:
            path = os.path.split(f.name)
            if path[1] == self.AWN_CONFIG:
                theme_found = True
            if path[1] == self.AWN_THUMB:
                thumb_found = True
        if theme_found and thumb_found:
            if hasattr(tar, 'extractall'):
                tar.extractall(defs.HOME_THEME_DIR) #new in python 2.5
            else:
                [tar.extract(f, defs.HOME_THEME_DIR) for f in tar.getnames()]
            tar.close()
            self.add_row(os.path.join(defs.HOME_THEME_DIR, path[0]))
            message = "Theme Successfully Added"
            message2 = ""
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            icon_path = os.path.join(defs.HOME_THEME_DIR, path[0], self.AWN_CUSTOM_ICONS)
            if os.path.exists(icon_path):
                message2 = "Custom icons included can be found at:\n " + os.path.join(defs.HOME_THEME_DIR, path[0], self.AWN_CUSTOM_ICONS)
            success.format_secondary_text(message2)
            success.run()
            success.destroy()
            return True
        else:
            return False

    def save(self, widget, data=None):
        detailsWindow = gtk.Dialog("Save Current Theme", self.window, gtk.DIALOG_DESTROY_WITH_PARENT, buttons=None)
        detailsWindow.add_buttons(gtk.STOCK_CANCEL, 47, gtk.STOCK_SAVE, 48)
        entries = {}
        hbox = gtk.HBox(homogeneous=False, spacing=5)
        detailsWindow.vbox.pack_start(hbox)
        hbox.pack_start(gtk.Label("Author:"), expand=True, fill=False)
        entries["author"] = gtk.Entry(max=0)
        hbox.pack_start(entries["author"], expand=False, fill=False)

        hbox = gtk.HBox(homogeneous=False, spacing=5)
        detailsWindow.vbox.pack_start(hbox)
        hbox.pack_start(gtk.Label("Theme Name:"), expand=True, fill=False)
        entries["name"] = gtk.Entry(max=0)
        hbox.pack_start(entries["name"], expand=False, fill=False)

        hbox = gtk.HBox(homogeneous=False, spacing=5)
        detailsWindow.vbox.pack_start(hbox)
        hbox.pack_start(gtk.Label("Date:"), expand=True, fill=False)
        entries["date"] = gtk.Entry(max=0)
        hbox.pack_start(entries["date"], expand=False, fill=False)

        hbox = gtk.HBox(homogeneous=False, spacing=5)
        detailsWindow.vbox.pack_start(hbox)
        hbox.pack_start(gtk.Label("Version:"), expand=True, fill=False)
        entries["version"] = gtk.Entry(max=0)
        hbox.pack_start(entries["version"], expand=False, fill=False)

        custom_icons_found = False
        if os.path.exists(defs.HOME_CUSTOM_ICONS_DIR):
            if len(os.listdir(defs.HOME_CUSTOM_ICONS_DIR)) > 0:
                hbox = gtk.HBox(homogeneous=False, spacing=5)
                detailsWindow.vbox.pack_start(hbox)
                entries["save_icons"] = gtk.CheckButton("Save Custom Icons")
                hbox.pack_start(entries["save_icons"], expand=False, fill=False)
                custom_icons_found = True

        detailsWindow.show_all()

        response = detailsWindow.run()
        if response == 48:
            theme_name = entries['name'].get_text()
            self.THEME_PREFS['details'] = {
                'author': entries['author'].get_text(),
                'name': theme_name,
                'date': entries['date'].get_text(),
                'version': entries['version'].get_text()
                }

            if theme_name == '':
                foldername = 'awn_theme'
            else:
                foldername = theme_name.replace(" ", "_")

            os.makedirs(os.path.join(self.BUILD_DIR, foldername))

            self.write(os.path.join(self.BUILD_DIR, foldername, self.AWN_CONFIG))

            if bool(self.CONFIG.get_bool(defs.BAR, defs.RENDER_PATTERN)):
                pattern_path = str(self.CONFIG.get_string(defs.BAR, defs.PATTERN_URI))
                if os.path.isfile(pattern_path):
                    shutil.copy(pattern_path, os.path.join(self.BUILD_DIR, foldername, "pattern.png"))

            img = self.get_img()
            if (img != None):
                img.save(os.path.join(self.BUILD_DIR, foldername, self.AWN_THUMB),"png")

            if custom_icons_found:
                if entries["save_icons"].get_active():
                    self.save_custom_icons(foldername)

            os.chdir(self.BUILD_DIR)
            tar = tarfile.open('./'+foldername+".tgz", "w:gz")
            tar.add('./'+foldername)
            tar.close()

            desktop = os.path.expanduser("~/Desktop")
            shutil.move(os.path.join(self.BUILD_DIR, foldername + ".tgz"), desktop)

            self.clean_tmp(self.BUILD_DIR)

            detailsWindow.destroy()

            msg = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK, message_format="The theme was saved to your Desktop folder as \"%s\"" % (foldername + ".tgz"))
            msg.run()
            msg.destroy()
        else:
            detailsWindow.destroy()

    def save_custom_icons(self, foldername):
        build_icon_path = os.path.join(self.BUILD_DIR, foldername, self.AWN_CUSTOM_ICONS)
        shutil.copytree(defs.HOME_CUSTOM_ICONS_DIR, build_icon_path)

    def delete(self, widget, data=None):
        if self.currItr is not None:
            index = self.model.get_path(self.currItr)[0]
            
            name = self.theme_list[index]['name']
            version = self.theme_list[index]['version']
            directory = self.theme_list[index]['full-dir']

            try:
                self.clean_tmp(directory)
            except (IOError, OSError), e:
                dialog = gtk.MessageDialog(None, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, 'Unable to remove the theme. Error: %s' % e)
                dialog.run()
                dialog.destroy()
                return

            if os.path.exists(self.AWN_CURRENT):
                curr = open(self.AWN_CURRENT, "rb")
                lines = curr.readlines()
                if len(lines) == 2:
                    curr_name = lines[0].rstrip('\n')
                    curr_version = lines[1].rstrip('\n')
                    curr.close()
                    if curr_name == name and curr_version == version:
                        curr = open(self.AWN_CURRENT, 'w')
                        curr.write("")
                        curr.close()

            self.theme_list.pop(index)
            self.model.remove(self.currItr)

    def add_row(self, directory):
        theme_path = os.path.join(directory, self.AWN_CONFIG)
        thumb_path = os.path.join(directory, self.AWN_THUMB)

        if os.path.exists(theme_path):
            cfg = self.read(theme_path)

            if os.path.exists(thumb_path):
                self.pixbuf = gtk.gdk.pixbuf_new_from_file(thumb_path)
            else:
                self.pixbuf = None

            self.model.append (None, (False, self.pixbuf, "Theme: %s\nVersion: %s\nAuthor: %s\nDate: %s" % (cfg['details']['name'], cfg['details']['version'], cfg['details']['author'], cfg['details']['date'])))
            self.theme_list.append({
                'name': cfg['details']['name'],
                'version': cfg['details']['version'],
                'full-dir':directory
                })

    def write(self, path):
        cfg = ConfigParser()
        for group, contents in self.THEME_PREFS.iteritems():
            if group == '':
                group = 'root'
                cfg_group = awn.CONFIG_DEFAULT_GROUP
            else:
                cfg_group = group
            cfg.add_section(group)
            if type(contents) is dict:
                [cfg.set(group, key, value) for key, value in contents.iteritems()]
            else:
                for pref in contents:
                    # uses the proper retrieval function based on the declared type
                    value = getattr(self.CONFIG, get_typefunc(pref.ptype, 'get'))(cfg_group, pref.name)
                    cfg.set(cfg_group, pref.name, value)
        cfg.write(open(path, 'w'))

    def read(self, path):
        cp = ConfigParser()
        cp.read(path)
        cfg_list = {}
        for sec in cp.sections():
            grp_list = {}
            opts = cp.options(sec)
            for opt in opts:
                grp_list[opt] = cp.get(sec, opt)
            cfg_list[sec] = grp_list
        return cfg_list

    def clean_tmp(self, dirPath):
        namesHere = os.listdir(dirPath)
        for name in namesHere:
            path = os.path.join(dirPath, name)
            if not os.path.isdir(path):
                os.remove(path)
            else:
                self.clean_tmp(path)
        os.rmdir(dirPath)

    def list_dirs(self, path):
        return list(os.listdir(path))

    def get_img(self):
        img_height = 75
        cmd = 'xwininfo -int -name  "awn_elements"'
        x = gtk.gdk.screen_width()/2
        y = gtk.gdk.screen_height()-img_height
        for line in os.popen(cmd).readlines():
            if line.find("Absolute upper-left X:  ") > 0:
                x = int(line.split("  ")[2])-20
            elif line.find("Absolute upper-left Y:  ") > 0:
                y = int(line.split("  ")[2])+int(img_height/2)
        w = gtk.gdk.get_default_root_window()
        sz = w.get_size()
        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB,False,8,150,75)
        pb = pb.get_from_drawable(w,w.get_colormap(),x,y,0,0,150,75)
        return pb

    def on_select(self, selection):
        model, selection_iter = selection.get_selected()
        self.currItr = selection_iter

    def update_radio(self, model, path, iterator, data=None):
        if path == self.model.get_path(self.currItr):
            model.set_value(iterator, 0, 1)
        else:
            model.set_value(iterator, 0, 0)

    def drag_data_received_data(self, treeview, context, x, y, selection, info, etime):
        data = selection.data
        data = data.replace('file://',"")
        data = data.replace("\r\n","")
        if tarfile.is_tarfile(data):
            self.extract_file(data)

    def str_to_bool(self, string):
        return string.lower() == "true"

    def make_model(self):
        self.theme_model = gtk.TreeStore (gobject.TYPE_BOOLEAN, gtk.gdk.Pixbuf, str)
        COL_SELECT = 0
        COL_PREVIEW = 1
        COL_AUTHOR = 2

        self.theme_treeview.set_model (self.theme_model)

        self.theme_treeview.enable_model_drag_dest([('text/plain', 0, 0)],
                  gtk.gdk.ACTION_DEFAULT | gtk.gdk.ACTION_MOVE)
        self.theme_treeview.connect("drag_data_received", self.drag_data_received_data)

        self.renderer = gtk.CellRendererToggle ()
        self.renderer.set_radio (True)
        self.renderer.set_property( 'activatable', True )
        selection = self.theme_treeview.get_selection()
        selection.connect('changed', self.on_select)

        self.theme_treeview.insert_column_with_attributes (-1, 'Select', self.renderer, active=COL_SELECT)
        self.theme_treeview.insert_column_with_attributes (-1, 'Preview', gtk.CellRendererPixbuf (), pixbuf=COL_PREVIEW)
        self.theme_treeview.insert_column_with_attributes (-1, 'Author', gtk.CellRendererText (), text=COL_AUTHOR)

        self.list_themes(self.theme_model)
