from ConfigParser import ConfigParser
import shutil
import tarfile
import os
import gconf
import gconf
import gnomedesktop
import gtk
import gobject

class AwnThemeManager:

    def list(self, model):
        self.model = model
        curr_name = ''
        curr_version = ''
        if(os.path.exists(self.AWN_CURRENT)):
            curr = open(self.AWN_CURRENT, "rb")
            lines = curr.readlines()
            if(len(lines) == 2):
                curr_name = lines[0].rstrip('\n')
                curr_version = lines[1].rstrip('\n')
                curr.close()

        if(not os.path.exists(self.AWN_THEME_DIR) and not os.path.isdir(self.AWN_THEME_DIR)):
            os.makedirs(self.AWN_THEME_DIR)

        for dir in self.list_dirs(self.AWN_THEME_DIR):
            theme_path = os.path.join(os.path.join(self.AWN_THEME_DIR, dir), self.AWN_CONFIG)
            thumb_path = os.path.join(os.path.join(self.AWN_THEME_DIR, dir), self.AWN_THUMB)

            if os.path.exists(theme_path):
                cfg = self.read(theme_path)
                if(cfg):
                    if(os.path.exists(thumb_path)):
                        self.pixbuf = gtk.gdk.pixbuf_new_from_file (thumb_path)
                    else:
                        self.pixbuf = None

                    if(curr_name == cfg['details']['name'] and curr_version == cfg['details']['version']):
                        setRadio = True
                    else:
                        setRadio = False

                    row = model.append (None, (setRadio, self.pixbuf, "Theme: "+cfg['details']['name']+"\nVersion: "+cfg['details']['version']+"\nAuthor: "+cfg['details']['author']+"\nDate: "+cfg['details']['date']))
                    path = model.get_path(row)[0]
                    self.theme_list[path] = {'row':row, 'name':cfg['details']['name'], 'version':cfg['details']['version'], 'dir':dir}
                    if setRadio:
                        self.currItr = row
                    else:
                        self.currItr = ''


    def apply(self, widget, data=None):
        if(self.currItr != None):
            index = self.model.get_path(self.currItr)[0]
            name = self.theme_list[index]['name']
            version = self.theme_list[index]['version']
            dir = self.theme_list[index]['dir']

            curr = open(self.AWN_CURRENT, 'w')
            curr.write(name+"\n")
            curr.write(version+"\n")
            curr.close()

            gconf_insert = self.read(os.path.join(os.path.join(self.AWN_THEME_DIR, dir), self.AWN_CONFIG))

            self.gconf_client = gconf.client_get_default ()

            for group in gconf_insert:
                if group != 'details':
                    if group == "root":
                        key_path = self.AWN_GCONF
                    else:
                        key_path = os.path.join(self.AWN_GCONF, group)
                    style = gconf_insert[group]
                    for key in style:
                        full_key_path = os.path.join(key_path, key)
                        if(self.gconf_client.get(full_key_path) != None):
                            type = self.gconf_client.get(full_key_path).type
                            if type == gconf.VALUE_STRING:
                                self.gconf_client.set_string(full_key_path, style[key])
                            elif type == gconf.VALUE_INT:
                                self.gconf_client.set_int(full_key_path, int(style[key]))
                            elif type == gconf.VALUE_FLOAT:
                                self.gconf_client.set_float(full_key_path, float(style[key]))
                            elif type == gconf.VALUE_BOOL:
                                self.gconf_client.set_bool(full_key_path, self.str_to_bool(style[key]))

            if self.gconf_client.get_bool(self.AWN_GCONF+"/bar/render_pattern"):
                self.gconf_client.set_string(self.AWN_GCONF+"/bar/pattern_uri", os.path.join(self.AWN_THEME_DIR, dir, "pattern.png"))

    def add(self, widget, data=None):
        dialog = gtk.FileChooserDialog(title=None,action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                  buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_CANCEL,gtk.STOCK_OPEN,gtk.RESPONSE_OK))
        dialog.set_default_response(gtk.RESPONSE_OK)

        filter = gtk.FileFilter()
        filter.set_name("Compressed Files")
        filter.add_pattern("*.tar.gz")
        filter.add_pattern("*.tgz")
        filter.add_pattern("*.bz2")
        dialog.add_filter(filter)

        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            file = dialog.get_filename()
            if not self.extract_file(file):
                invalid = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format="Invalid Theme Format")
                invalid.run()
                invalid.destroy()
        dialog.destroy()

    def extract_file(self, file):
        tar = tarfile.open(file, "r:gz")
        filelist = tar.getmembers()
        theme_found = False
        thumb_found = False
        for file in filelist:
            path = os.path.split(file.name)
            if(path[1] == self.AWN_CONFIG):
                theme_found = True
            if(path[1] == self.AWN_THUMB):
                thumb_found = True
        if(theme_found and thumb_found):
            for file in tar.getnames():
                tar.extract(file, self.AWN_THEME_DIR)
            #tar.extractall(self.AWN_THEME_DIR) #new in python 2.5
            tar.close()
            self.add_row(path[0])
            message = "Theme Successfully Added"
            message2 = ""
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            icon_path = os.path.join(self.AWN_THEME_DIR, path[0], self.AWN_CUSTOM_ICONS)
            if os.path.exists(icon_path):
                message2 = "Custom icons included can be found at:\n "+str(os.path.join(self.AWN_THEME_DIR, path[0], self.AWN_CUSTOM_ICONS))
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
        icon_path = os.path.join(self.AWN_CONFIG_DIR, self.AWN_CUSTOM_ICONS)
        if os.path.exists(icon_path):
            if len(os.listdir(icon_path)) > 0:
                hbox = gtk.HBox(homogeneous=False, spacing=5)
                detailsWindow.vbox.pack_start(hbox)
                entries["save_icons"] = gtk.CheckButton("Save Custom Icons")
                hbox.pack_start(entries["save_icons"], expand=False, fill=False)
                custom_icons_found = True

        detailsWindow.show_all()

        response = detailsWindow.run()
        if response == 48:
            gconfKeys = {}
            gconfKeys[""] = ["appactive_png", "apparrow_color", "apptasks_have_arrows", "auto_hide"]
            gconfKeys["app"] = ["active_png", "alpha_effect", "arrow_color", "fade_effect", "hover_bounce_effect", "tasks_have_arrows"]
            gconfKeys["bar"] = ["bar_angle", "bar_height", "border_color", "corner_radius", "glass_histep_1", "glass_histep_2", "glass_step_1",
                            "glass_step_2", "hilight_color", "icon_offset", "pattern_alpha", "pattern_uri", "render_pattern",
                            "rounded_corners", "sep_color", "show_separator"]
            gconfKeys["title"] = ["background", "bold", "font_face", "font_size", "italic", "shadow_color", "text_color"]
            gconfKeys["details"] = {"author":entries["author"].get_text(),"name":entries["name"].get_text(),"date":entries["date"].get_text(),"version":entries["version"].get_text()}
            gconf_client = gconf.client_get_default()

            foldername = entries["name"].get_text().replace(" ", "_")
            if(foldername == ''): foldername = "awn_theme"

            os.makedirs(self.BUILD_DIR+"/"+foldername)

            self.write(self.BUILD_DIR+'/'+foldername+"/"+self.AWN_CONFIG, gconfKeys)

            if bool(self.GCONF.get_bool(self.AWN_GCONF+"/bar/render_pattern")):
                pattern_path = str(self.GCONF.get_string(self.AWN_GCONF+"/bar/pattern_uri"))
                if os.path.isfile(pattern_path):
                    shutil.copy(pattern_path,self.BUILD_DIR+'/'+foldername+"/pattern.png")

            img = self.get_img()
            if (img != None):
                img.save(self.BUILD_DIR+'/'+foldername+"/"+self.AWN_THUMB,"png")

            if custom_icons_found:
                if entries["save_icons"].get_active():
                    self.save_custom_icons(foldername)

            os.chdir(self.BUILD_DIR)
            tar = tarfile.open('./'+foldername+".tgz", "w:gz")
            tar.add('./'+foldername)
            tar.close()

            desktop = os.path.expanduser("~/Desktop")
            shutil.move(self.BUILD_DIR+'/'+foldername+".tgz", desktop)

            self.clean_tmp(self.BUILD_DIR)

            detailsWindow.destroy()
        else:
            detailsWindow.destroy()

    def save_custom_icons(self, foldername):
        build_icon_path = os.path.join(self.BUILD_DIR, foldername, self.AWN_CUSTOM_ICONS)
        icon_path = os.path.join(self.AWN_CONFIG_DIR, self.AWN_CUSTOM_ICONS)
        shutil.copytree(icon_path, build_icon_path)

    def delete(self, widget, data=None):
        if(self.currItr != None):
            index = self.model.get_path(self.currItr)[0]
            name = self.theme_list[index]['name']
            version = self.theme_list[index]['version']
            dir = self.theme_list[index]['dir']

            self.clean_tmp(os.path.join(self.AWN_THEME_DIR, dir))

            #os.remove(self.AWN_THEME_DIR+'/'+dir+"/"+self.AWN_THUMB)
            #os.remove(self.AWN_THEME_DIR+'/'+dir+"/"+self.AWN_CONFIG)
            #os.rmdir(self.AWN_THEME_DIR+'/'+dir)

            if(os.path.exists(self.AWN_CURRENT)):
                curr = open(self.AWN_CURRENT, "rb")
                lines = curr.readlines()
                if(len(lines) == 2):
                    curr_name = lines[0].rstrip('\n')
                    curr_version = lines[1].rstrip('\n')
                    curr.close()
                    if curr_name == name and curr_version == version:
                        curr = open(self.AWN_CURRENT, 'w')
                        curr.write("")
                        curr.close()

            del self.theme_list[index]
            self.model.remove(self.currItr)

    def add_row(self, dir):
        theme_path = os.path.join(os.path.join(self.AWN_THEME_DIR, dir), self.AWN_CONFIG)
        thumb_path = os.path.join(os.path.join(self.AWN_THEME_DIR, dir), self.AWN_THUMB)

        if os.path.exists(theme_path):
            cfg = self.read(theme_path)

            if(os.path.exists(thumb_path)):
                self.pixbuf = gtk.gdk.pixbuf_new_from_file(thumb_path)
            else:
                self.pixbuf = None

            row = self.model.append (None, (False, self.pixbuf, "Theme: "+cfg['details']['name']+"\nVersion: "+cfg['details']['version']+"\nAuthor: "+cfg['details']['author']+"\nDate: "+cfg['details']['date']))
            path = self.model.get_path(row)[0]
            self.theme_list[path] = {'row':row, 'name':cfg['details']['name'], 'version':cfg['details']['version'], 'dir':dir}

    def gconf_get_key(self,full_key):
        if(self.GCONF.get(full_key) != None):
            type = self.GCONF.get(full_key).type
            if type == gconf.VALUE_STRING:
                key = str(self.GCONF.get_string(full_key))
            elif type == gconf.VALUE_INT:
                key = str(self.GCONF.get_int(full_key))
            elif type == gconf.VALUE_FLOAT:
                key = str(self.GCONF.get_float(full_key))
            elif type == gconf.VALUE_BOOL:
                key = str(self.GCONF.get_bool(full_key))
            else:
                key = ''
            return key

    def write(self, path, gconfKeys):
        cfg = ConfigParser()
        for item in gconfKeys:
            if item == "":
                cfg.add_section("root")
            else:
                cfg.add_section(item)
            for key in gconfKeys[item]:
                if item == "details":
                    value = gconfKeys[item][key]
                else:
                    value = self.gconf_get_key(os.path.join(os.path.join(self.AWN_GCONF, item), key))
                if item == "":
                    itm = "root"
                else:
                    itm = item
                cfg.set(itm, key, value)
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
        dirs = []
        for dir in os.listdir(path):
            dirs.append(dir)
        return dirs

    def get_img(self):
        img_height = 75
        cmd = 'xwininfo -int -name  "awn_elements"'
        x = gtk.gdk.screen_width()/2
        y = gtk.gdk.screen_height()-img_height
        for file in os.popen(cmd).readlines():
            if file.find("Absolute upper-left X:  ") > 0:
                x = int(file.split("  ")[2])-20
            elif file.find("Absolute upper-left Y:  ") > 0:
                y = int(file.split("  ")[2])+int(img_height/2)
        w = gtk.gdk.get_default_root_window()
        sz = w.get_size()
        pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB,False,8,150,75)
        pb = pb.get_from_drawable(w,w.get_colormap(),x,y,0,0,150,75)
        return pb

    def on_toggle(self, widget, event, data=None):
        widget.foreach(self.update_radio, event)

    def update_radio(self, model, path, iter, data):
        if(path[0] == int(data)):
            model.set_value(iter, 0, 1)
            self.currItr = iter
        else:
            model.set_value(iter, 0, 0)

    def drag_data_received_data(self, treeview, context, x, y, selection, info, etime):
        data = selection.data
        data = data.replace('file://',"")
        data = data.replace("\r\n","")
        if tarfile.is_tarfile(data):
            self.extract_file(data)

    def str_to_bool(self, str):
        if(str.lower() == "true"):
            return True
        else:
            return False

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
        self.renderer.connect_object( 'toggled', self.on_toggle, self.theme_model )

        self.theme_treeview.insert_column_with_attributes (-1, 'Select', self.renderer, active=COL_SELECT)
        self.theme_treeview.insert_column_with_attributes (-1, 'Preview', gtk.CellRendererPixbuf (), pixbuf=COL_PREVIEW)
        self.theme_treeview.insert_column_with_attributes (-1, 'Author', gtk.CellRendererText (), text=COL_AUTHOR)

        self.list(self.theme_model)

    def __init__(self, glade, config_dir):
        self.AWN_CONFIG_DIR = config_dir
        self.wTree = glade
        self.theme_treeview = self.wTree.get_widget('theme_treeview')
        self.theme_add = self.wTree.get_widget('theme_add')
        self.theme_add.connect("clicked", self.add)
        self.theme_remove = self.wTree.get_widget('theme_remove')
        self.theme_remove.connect("clicked", self.delete)
        self.theme_save = self.wTree.get_widget('theme_save')
        self.theme_save.connect("clicked", self.save)
        self.theme_apply = self.wTree.get_widget('theme_apply')
        self.theme_apply.connect("clicked", self.apply)

        self.AWN = 'avant-window-navigator'
        self.AWN_GCONF = '/apps/'+self.AWN
        self.AWN_THEME_DIR = os.path.join(self.AWN_CONFIG_DIR, "themes/")
        self.AWN_CONFIG = 'theme.awn'
        self.AWN_THUMB = 'thumb.png'
        self.AWN_CUSTOM_ICONS = 'custom-icons'
        self.AWN_CURRENT = os.path.join(self.AWN_THEME_DIR, 'current.awn')
        self.GCONF = gconf.client_get_default()
        self.BUILD_DIR = "/tmp/awn_theme_build"
        self.theme_list = {}
        self.currItr = None
        self.model = None
        self.window = self.wTree.get_widget("main_window")
        self.make_model()
