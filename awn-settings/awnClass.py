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
#  Edited By: Ryan Rushton <ryan@rrdesign.ca>
#
#  Notes: Avant Window Navigator preferences window

import sys
import os
import socket
import time
import urllib
import cairo
import array
from ConfigParser import ConfigParser
try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO
try:
    import gobject
    import gtk
    import gtk.gdk as gdk
    import pango
except Exception, e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)
from xdg.DesktopEntry import DesktopEntry

import awn
import awnDefs as defs
from desktopagnostic import config
from desktopagnostic import vfs
from desktopagnostic.config import GROUP_DEFAULT
from desktopagnostic.ui import LauncherEditorDialog
import tarfile
import shutil
import tempfile
import dbus

try:
    from bzrlib import branch
    from bzrlib.builtins import cmd_branch, cmd_pull
    from bzrlib.plugins.launchpad.lp_directory import LaunchpadDirectory
    support_bzr = True
except:
    support_bzr = False

defs.i18nize(globals())

class AwnListStore(gtk.ListStore, gtk.TreeDragSource, gtk.TreeDragDest):

    __gsignals__ = {
        # we can't do TreePath easily, so lets use just int
        # (change to string representation of TreePath if necessary)
        'foreign-drop':
            (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                [int, gtk.SelectionData.__gtype__])
    }

    def do_drag_data_received(self, dest, selection_data):
        model, row = None, None
        if selection_data.target == 'GTK_TREE_MODEL_ROW':
            model, row = selection_data.tree_get_row_drag_data()

        if model == self:
            # drop from the same widget, handle reorder ourselves
            src = row[0]
            dst = dest[0]
            if (src == dst):
                return False

            new_array = []
            iter = self.get_iter_root()
            while iter != None:
                iter = self.iter_next(iter)
                new_array.append(len(new_array))

            # new_array[oldpos] = newpos
            if src < dst:
                new_array[src] = dst-1
                for i in range(src+1, dst):
                    new_array[i] = i-1
            else:
                new_array[src] = dst
                for i in range(dst, src):
                    new_array[i] = i+1

            # need to map it to > new_order[newpos] = oldpos
            new_order = new_array[:]
            for i in range(len(new_array)): new_order[new_array[i]] = i
            # emit change signals
            self.reorder(new_order)
        else:
            self.emit("foreign-drop", dest[0], selection_data)

        return False

    def do_row_drop_possible(self, dest_path, selection_data):
        return True

    def do_drag_data_delete(self, path):
        return True

    def do_drag_data_get(self, path, selection_data):
        # if we return False the default handler will do exactly what we want
        return False

gobject.type_register (AwnListStore)


class awnBzr(gobject.GObject):
        #Utils Bzr
    def lp_path_normalize(self, path):
        '''     Get a "lp:" format url and return a http url
                path: a url from a branch
                return: the http format of a lp: format, or the same url
        '''
        if support_bzr == True:
            directory = LaunchpadDirectory()
            return directory._resolve(path).replace("bzr+ssh","http")
        else:
            return path

    def read_list(self, file_path):
        '''     Read a flat file and return the content in a list
                file_path : path of the file
                return: a list of the elements of the file
        '''
        flat_list = []
        f = open(file_path, 'r')
        for line in f:
            flat_list.append(line.strip())
        f.close()
        return flat_list

    #Bzr
    def create_branch(self, bzr_dir, path):
        '''     Create a new bzr branch.
                path: the path of the branch
                bzr_dir: the location of the futur tree.
        '''
        if support_bzr == False:
            print (_("Bzr support is not enable, try to install bzr"))
        else:
            if os.path.exists(path):
                print (_("Error, the path already exist"))
            else:
                try:
                    bzr_branch = cmd_branch()
                    status = StringIO()
                    status = bzr_branch._setup_outf()
                    bzr_branch.run(from_location=self.lp_path_normalize(bzr_dir), to_location=path)
                except socket.gaierror:
                    print (_('Socket error, could not create branch.'))

    def update_branch(self, path):
        '''     Update a local branch
                path: Location of the branch
                Return the output of the command
        '''
        if support_bzr == False:
            print (_("Bzr support is not enable, try to install bzr"))
        else:
            bzr_pull = cmd_pull()
            status = StringIO()
            status = bzr_pull._setup_outf()
            bzr_pull.run(directory=path)

    def get_revision_from_path(self, path):
        '''     Return the last revision number of the branch
                specify with path parameter
        '''
        if support_bzr == False:
            print (_("Bzr support is not enable, try to install bzr"))
            return 0
        else:
            tree = branch.Branch.open(path)
            revision_number, revision_id = tree.last_revision_info()
            return revision_number

    #Sources.list
    def dict_from_sources_list(self, config=defs.HOME_CONFIG_DIR):
        '''
        Return a dictionnay of the sources.list
        config: the directory to read sources.list.
        '''
        sources_dict = {}
        sources_list_path = os.path.join(config, "sources.list")
        f = open(sources_list_path)
        sources_list = f.readlines()
        sources_list = [elem.replace("\n","") for elem in sources_list]
        sources_list= [elem.split(" ") for elem in sources_list]
        for i in sources_list:
            sources_dict[i[0]] = i[1]
        f.close()
        return sources_dict

    def list_from_sources_list(self, config=defs.HOME_CONFIG_DIR):
        '''
        Return a list for the sources.list.
        Each element of the list is another list of 2 elements ['sources','directory']
        config: the directory to read sources.list.
        '''
        sources_list = []
        sources_list_path = os.path.join(config, "sources.list")
        f = open(sources_list_path)
        sources_list = f.readlines()
        sources_list = [elem.replace("\n","") for elem in sources_list]
        sources_list= [elem.split(" ") for elem in sources_list]
        f.close()
        return sources_list

    def sources_from_sources_list(self, config=defs.HOME_CONFIG_DIR):
        '''
        Return a list of all sources (without the directory)
        config: the directory to read sources.list.
        '''
        list_sources = []
        list_sources_list = self.list_from_sources_list(config)
        [list_sources.append(elem[0]) for elem in list_sources_list]
        return list_sources

    def create_sources_list(self, config = defs.HOME_CONFIG_DIR, default = defs.DEFAULT_SOURCES_LIST):
        '''     Create a sources_list with all the sources (bzr branch or local files)
                Should be unique for an installation
                config: the directory to write sources.list.
                default: the default content of the sources.list
        '''
        sources_list_path = os.path.join(config,"sources.list")
        if not os.path.isfile(sources_list_path):
            self.write_default_sources_list(sources_list_path)
        else: print (_("Error, a sources.list already exist"))

    def write_default_sources_list(self, sources_list_path, config = defs.HOME_CONFIG_DIR, default = defs.DEFAULT_SOURCES_LIST):
        '''     Write default sources in the sources list
                If a sources.list exist, it will write default sources at the
                end of the file
        '''
        if os.path.isfile(sources_list_path):
            f = open(sources_list_path, 'a')
        else:
            f = open(sources_list_path, 'w')
        [f.write(i[0] + " " + i[1] + "\n")
                for i
                in default if i[0] <> '']
        f.close()


    def update_sources_list(self, directories = defs.HOME_THEME_DIR, progressbar = None):
        '''
                Update all sources of the sources list
                If the directory doesn't exist, a new branch will be create
                directories = default locations of the local branches to update
                progress = A widget of a progress bar to update.
        '''
        if progressbar is not None:
            progressbar.pulse()
            progressbar.show()

        sources_list = self.list_from_sources_list()
        sources_to_update = []

        [ sources_to_update.append(elem)
                for elem in sources_list
                if not elem[0].startswith("/") or not elem[1] == "local" ]
        total = len(sources_to_update)
        counter = 0

        for elem in sources_to_update:
            if not os.path.isdir(os.path.join(directories , elem[1])):
                print self.create_branch(elem[0], os.path.join(directories , elem[1]))

            else:
                print self.update_branch(os.path.join(directories , elem[1]))
            counter = counter+1
            if progressbar is not None:
                progressbar.set_fraction(int(counter) / int(total))




    def add_source(self, path, source_type="web", config = defs.HOME_CONFIG_DIR, directories = defs.HOME_THEME_DIR):
        '''     Add a source to the sources list.
                path: path to the branch
                source_type: type of source web or local
                config: the directory to write sources.list.
                directories = the directory to write sources
        '''
        if not self.sources_check_sanity(path):
            if source_type == "local":
                path = path +" "+"local"+"\n"
            elif source_type == "web":
                number = len(os.listdir(directories))
                path = path +" "+"web-sources-"+str(number)+"\n"
            source = os.path.join(config,"sources.list")
            f = open(source, 'a')
            f.write(path)
            f.close()
        else:
            print (_("Error, the path is empty or already exist"))

    def remove_source(self, source, config = defs.HOME_CONFIG_DIR):
        '''     Remove a source from the sources.list
                path is the path of the source (url or local path)
        '''
        list_sources = self.list_from_sources_list(config)
        sources_list_path = os.path.join(config,"sources.list")
        new_sources_list = []
        for elem in list_sources :
            if not elem[0] == source:
                new_sources_list.append(elem)

        print list_sources
        print new_sources_list

        f = open(sources_list_path, 'w')
        [f.write(i[0] + " " + i[1] + "\n")
                for i
                in new_sources_list if i[0] <> '']
        f.close()

    def create_branch_config_directory(self, source):
        '''
                Create the directory of a source in config directory.
                source is a dictionnary of 1 entry {'url','directory'}.
                source is 1 line of the sources list.
        '''
        if not source[0].startswith("/") or source[1] == "local":
            self.create_branch((source[0], source[1]))
        else:
            print "Error, this is a local path"

    def sources_list_check_double_path(self, path):
        '''     Check if a path is already in the sources.list
                path: location of the source
                Return True if the path already in.
        '''
        sources_list = self.list_from_sources_list()
        error = 0
        for elem in sources_list:
            if elem[0] == path:
                error=error+1
        if error >= 1:
            return True
        else:
            return False
    def sources_check_sanity(self, path):
        '''     Do some check to be sure the source is ok
                path: location of the source
                return True if the source is not ok
        '''
        #Test If the source already in the sources.list
        test = self.sources_list_check_double_path(path)

        #Test if the source is not null
        if path == '':
            test = True

        #Test if it's not a fantasic path
        if not path.startswith("/") or not path.startswith("http://"):
            test = True

        #TODO Add more tests

    #Desktop files
    def read_desktop(self, file_path):
        '''     Read a desktop file and return a dictionnary with all fields
                API still unstable
        '''

        #API of the desktop file (unstable, WIP etc ...)
        # keys should be a string of the type
        # value could be
        #       '' if there is nothing,
        #       a string when there is no settings
        #       a list when there is a setting : [value of the setting, group settings, key setting]
        struct = {
            'type': '',             # Applet
            'location':'',          # Location of the desktop file
            'name': '',             # Name of the Type
            'comment':'',           # Comments
            'version':'',           # Version of the
            'copyright':'',         # Copyright
            'author':'',            # Author
            'licence_code':'',      # Licence for the code
            'licence_icons':'',     # Licence for the icons
            'icon':'',              # Icon for the type
            'exec':'',              # Execution path, for applet
            'applet_type':'',       # Type of teh applet (C, Vala or Python)
            'applet_category':'',   # Category for the applet
            'singleton':'',         # True or False, use only one instance.
        }

        desktop_entry = DesktopEntry(file_path)
        struct['type'] = desktop_entry.get('X-AWN-Type')
        struct['location'] = file_path
        struct['name'] = desktop_entry.get('Name')
        #TODO More to add
        struct['icon'] = desktop_entry.get('Icon')
        struct['exec'] = desktop_entry.get('X-AWN-AppletExec')
        struct['applet_type'] = desktop_entry.get('X-AWN-AppletType')
        struct['applet_category'] = desktop_entry.get('X-AWN-AppletCategory').rsplit(",")

        return struct

    def get_files_from_source(self, source, file_type, directories = defs.HOME_THEME_DIR):
        '''     Read all desktop file from a source
                source is a list of 2 entries ('url','directory').
                source is 1 line of the sources list.
        '''
        if source[1] == "local":
            path = source[0]
        else:
            path= str(directories + "/" + source[1])
        try:
            list_files = os.listdir(path)
            files =  [path + "/" + elem for elem in list_files if os.path.splitext(elem)[1] ==file_type]
            return files
        except:
            print(_("Error %s is missing on your system, please remove it from the sources.list") % path)
            return None


    def catalog_from_sources_list(self, file_type='.desktop'):
        '''
                Return a catalog (list of desktop files from the sources list)
        '''
        catalog=[]
        sources_list = self.list_from_sources_list()
        for elem in sources_list:
            desktops = self.get_files_from_source(elem, file_type)
            if desktops is not None:
                [ catalog.append(i) for i in desktops ]
        return catalog

    def type_catalog_from_sources_list(self, type_catalog='Theme'):
        '''
        Return a catalog of themes or applets (list of desktop files from the sources list)
        type_catalog is the type of catalog (Theme of Applets)
        '''
        extension = '.awn-theme' if type_catalog == 'Theme' else '.desktop'
        catalog = self.catalog_from_sources_list(extension)

        final_catalog = []

        if type_catalog == 'Theme':
            final_catalog = catalog
        else:
            for elem in catalog:
                desktop = DesktopEntry(elem)
                if desktop.get('X-AWN-Type') == type_catalog:
                    final_catalog.append(elem)
                else:
                    if desktop.get('X-AWN-AppletType') and type_catalog =='Applet':
                        final_catalog.append(elem)

        return final_catalog

    def load_settings_from_theme(self, path):
        '''     Load settings from desktop file
        '''
        parser = ConfigParser()
        parser.read(path)

        for section in parser.sections():
            if section.startswith('config'):
                ids = section.split('/')
                group, instance_id = None, None
                if len(ids) > 2:
                    group = ids[1]
                    instance_id = ids[2]
                else:
                    group = ids[1]

                client = self.client

                if instance_id is not None:
                    client = awn.config_get_default(int(instance_id))

                for key, value in parser.items(section):
                    try:
                        if value.isdigit():
                            value = int(value)
                        elif len(value.split('.')) == 2:
                            value = float(value)
                        elif value.lower() in ['true', 'false']:
                            value = True if value.lower() == 'true' else False
                        elif value.startswith('#'):
                            client.set_string(group, key, value)
                            continue
                        client.set_value(group, key, value)
                    except:
                        print "Unable to set value %s for %s/%s" % (value, group, key)

    def list_applets_categories(self):
        '''     List all categories of applets
        '''
        catalog = self.type_catalog_from_sources_list(type_catalog='Applet')
        desktop_list = [self.read_desktop(elem) for elem in catalog]
        categories_list = []
        for applet in desktop_list:
            for elem in applet['applet_category']:
                if not elem in categories_list and elem is not "":
                    categories_list.append(elem)
        categories_list.append("All")
        return categories_list

    def applets_by_categories(self, categories = ""):
        '''     Return applets uris of this categories
                categories = list of Categories
        '''
        catalog = self.type_catalog_from_sources_list(type_catalog='Applet')
        desktop_list = [self.read_desktop(elem) for elem in catalog]
        if categories is "":
            list_applets = catalog
        else:
            list_applets = [applet['location'] for applet in desktop_list if categories in applet['applet_category']]

        return list_applets


    #GUI functions
    def make_row (self, path):
        '''     Make a row for a list of applets or launchers
                path : path of the desktop file
        '''
        icon = None
        text = ""
        name = ""
        try:
            if not os.path.exists(path):
                raise IOError("Desktop file does not exist!")

            if os.path.splitext(path)[1] == '.desktop':
                item = DesktopEntry (path)
                name = item.getName()
                text = "<b>%s</b>\n%s" % (name, item.getComment())
                icon_name = item.getIcon()
                icon = self.make_icon(icon_name)
            else:
                parser = ConfigParser()
                parser.read(path)
                name = parser.get('theme-info', 'Name')
                text = "<b>%s</b> %s" % (name,
                                         parser.get('theme-info', 'Version'))
                if parser.has_option('theme-info', 'Author'):
                    text += "\n<span style='italic'>by</span> %s" % parser.get('theme-info', 'Author')
                #icon = self.make_icon(parser.get('theme-info', 'Icon'))
                icon = gdk.pixbuf_new_from_file(parser.get('theme-info', 'Icon'))
        except:
            pass
        return icon, text, name

    def make_icon (self, name, icon_size=32):
        ''' Extract an icon from a desktop file
        '''
        icon_final = None
        theme = gtk.icon_theme_get_default ()
        pixmaps_path = [os.path.join(p, "share", "pixmaps") for p in ("/usr", "/usr/local", defs.PREFIX)]
        applets_path = [os.path.join(p, "share", "avant-window-navigator","applets") for p in ("/usr", "/usr/local", defs.PREFIX)]
        list_icons = (
            ('theme icon', self.search_icon(
                name, type_icon="theme", size=icon_size)
            ),
            ('stock_icon', self.search_icon(
                name, type_icon="stock", size=icon_size)
            ),
            ('file_icon', self.search_icon(
                name, size=icon_size)
            ),
            ('pixmap_png_icon', self.search_icon(
                name, pixmaps_path, extension=".png", size=icon_size)
            ),
            ('pixmap_icon', self.search_icon(
                name, pixmaps_path, size=icon_size)
            ),
            ('applets_png_icon', self.search_icon(
                name, applets_path, extension=".png", size=icon_size)
            ),
            ('applets_svg_icon', self.search_icon(
                name, applets_path, extension=".svg", size=icon_size)
            )
        )

        for i in list_icons:
            if i[1] is not None:
                icon_final = i[1]
                break
        if icon_final is not None:
            return icon_final
        else:
            return theme.load_icon('gtk-execute', icon_size, 0)

    def search_icon (self, name, path="", extension="",
                     type_icon="pixmap", size=32):
        ''' Search a icon'''
        theme = gtk.icon_theme_get_default ()
        icon = None
        if type_icon is "theme":
            try:
                icon = theme.load_icon (name, size, 0)
            except:
                icon = None
        elif type_icon is "stock":
            try:
                image = gtk.image_new_from_stock(name, size)
                icon = image.get_pixbuf()
            except:
                icon = None
        elif type_icon is "pixmap":
            
            for i in path:
                if os.path.exists(os.path.join(i,name+extension)):
                    try:
                        icon = gdk.pixbuf_new_from_file_at_size(os.path.join(i,name+extension), size, size)
                    except:
                        icon = None

        return icon

    def add_uris_to_model(self, model, uris):
        for uri in uris:
            if os.path.isfile(uri):
                icon, text, name = self.make_row (uri)
                if len(text) > 2:
                    row = model.append ()
                    model.set_value (row, 0, icon)
                    model.set_value (row, 1, text)
                    model.set_value (row, 2, uri)
                    model.set_value (row, 3, name)

    def make_applet_model(self, uris, treeview):
        self.applet_model = model = AwnListStore(gdk.Pixbuf, str, str, str)
        treeview.set_model (model)

        def deactivate_applet(applet_model, dest, selection_data):
            model, src = selection_data.tree_get_row_drag_data()
            itr = model.get_iter(src)
            model.remove(itr)
            self.apply_applet_list_changes()
            
            infobar = self.wTree.get_object("tm_infobar")
            if self.check_for_task_manager():
                infobar.get_parent().hide_all()
            else:
                infobar.get_parent().show_all()
                

        self.applet_model.connect("foreign-drop", deactivate_applet)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
        col.set_expand (False)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        ren.props.ellipsize = pango.ELLIPSIZE_END
        col = gtk.TreeViewColumn ("Description", ren, markup=1)
        col.set_expand (True)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Desktop", ren)
        col.set_visible (False)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Name", ren)
        col.set_visible (False)
        treeview.append_column (col)

        # Plus icon
        #ren = gtk.CellRendererPixbuf()
        #ren.props.icon_name = "gtk-add"
        #ren.props.stock_size = gtk.ICON_SIZE_DND
        #col = gtk.TreeViewColumn ("AddIcon", ren)
        #col.set_expand (False)
        #treeview.append_column (col)

        self.add_uris_to_model(model, uris)

        treeview.show()

        return model

    def make_launchers_model(self, uris, treeview):
        model = AwnListStore(gdk.Pixbuf, str, str, str)
        treeview.set_model (model)

        def add_launcher(model, dest, selection):
            data = urllib.unquote(selection.data)
            data = data.replace("file://", "").replace("\r\n", "")
            if data.endswith('.desktop'):
                paths = self.client_taskman.get_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST)
                paths.append(data)
                self.client_taskman.set_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST, paths)
            else:
                print "This widget accepts only desktop files!"

        model.connect('foreign-drop', add_launcher)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Description", ren, markup=1)
        col.set_expand (True)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        ren.props.visible = False
        col = gtk.TreeViewColumn ("Desktop", ren)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        ren.props.visible = False
        col = gtk.TreeViewColumn ("Name", ren)
        treeview.append_column (col)

        self.add_uris_to_model(model, uris)

        treeview.show()

        return model

    def make_model(self, uris, treeview):
        model = gtk.ListStore(gdk.Pixbuf, str, str, str)
        treeview.set_model (model)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Description", ren, markup=1)
        col.set_expand (True)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        ren.props.visible = False
        col = gtk.TreeViewColumn ("Desktop", ren)
        treeview.append_column (col)

        ren = gtk.CellRendererText()
        ren.props.visible = False
        col = gtk.TreeViewColumn ("Name", ren)
        treeview.append_column (col)

        self.add_uris_to_model(model, uris)

        treeview.show()

        return model

    def refresh_tree (self, uris, model):
        model.clear()
        self.add_uris_to_model(model, uris)

    def create_dropdown(self, widget, entries, matrix_settings=False):
        '''     Create a dropdown.
                widget : The widget where create the dropdown
                entries : entries to add to the dropdown
                matrix_settings : connect to settings (not implemented yet)
        '''
        model = gtk.ListStore(str)

        [model.append([elem]) for elem in entries]
        widget.set_model(model)
        cell = gtk.CellRendererText()
        widget.pack_start(cell)
        widget.add_attribute(cell,'text',0)

    def check_for_task_manager(self):
		applets = self.client.get_list(defs.PANEL, defs.APPLET_LIST)
		for applet in applets:
			tokens = applet.split("::")
			if tokens[0].find('taskmanager.desktop') > 0:
				return True
		return False
        
class awnPreferences(awnBzr):
    def setup_font(self, group, key, font_btn):
        """sets up font chooser"""
        self.load_font(group, key, font_btn)
        font_btn.connect("font-set", self.font_changed, (group, key))
        self.client.notify_add(group, key, self.reload_font, font_btn)

    def load_font(self, group, key, font_btn):
        font = self.client.get_string(group, key)
        if font == None:
            font = "Sans 10"
        font_btn.set_font_name(font)

    def reload_font(self, group, key, value, font_btn):
        self.load_font(group, key, font_btn)

    def font_changed(self, font_btn, groupkey):
        group, key = groupkey
        self.client.set_string(group, key, font_btn.get_font_name())

    def load_effect(self, group, key, dropdown):
        dropdown.set_active(self.client.get_int(group, key))

    def reload_effect(self, group, key, value, dropdown):
        self.load_effect(group, key, dropdown)

    def setup_custom_effects(self, group, key):
        self.effect_drop = []
        for drop in ['open', 'close', 'hover', 'launch', 'attention']:
            d = self.wTree.get_object('effect_'+drop)
            self.effect_drop.append(d)
            model = gtk.ListStore(str)
            model.append([_("None")])
            model.append([_("Simple")])
            model.append([_("Classic")])
            model.append([_("Fade")])
            model.append([_("Spotlight")])
            model.append([_("Zoom")])
            model.append([_("Squish")])
            model.append([_("3D Turn")])
            model.append([_("3D Spotlight Turn")])
            model.append([_("Glow")])
            d.set_model(model)
            cell = gtk.CellRendererText()
            d.pack_start(cell)
            d.add_attribute(cell,'text',0)

        self.load_effect_custom(group,key)
        self.client.notify_add(group, key, self.reload_effect_custom)
        
        for drop in ['open', 'close', 'hover', 'launch', 'attention']:
            d = self.wTree.get_object('effect_'+drop)
            d.connect("changed", self.effect_custom_changed, (group, key))

    def load_effect_custom(self, group, key):
        effect_settings = self.client.get_int(group, key)
        effect_dict = {'open': 0xF, 'close': 0xF0, 'hover': 0xF00,
                       'launch': 0xF000, 'attention': 0xF0000}
        for drop in effect_dict.keys():
            d = self.wTree.get_object('effect_'+drop)
            current_effect = effect_settings & effect_dict[drop]
            while current_effect > 15: current_effect >>= 4
            if current_effect == 15:
                d.set_active(0)
            else:
                d.set_active(current_effect+1)

    def reload_effect_custom(self, group, key, value):
        self.load_effect_custom(group, key)

    def effect_custom_changed(self, dropdown, groupkey):
        group, key = groupkey
        if (dropdown.get_active() != self.effects_dd.get_active()):
            self.effects_dd.set_active(10) #Custom
            new_effects = self.get_custom_effects()
            self.client.set_int(group, key, new_effects)
            print "Setting effects to:", "0x%0.8X" % new_effects

    def get_custom_effects(self):
        effects = 0
        dropdowns = ['open', 'close', 'hover', 'launch', 'attention']
        dropdowns.reverse()
        for drop in dropdowns:
            d = self.wTree.get_object('effect_'+drop)
            if(d.get_active() == 0):
                effects = effects << 4 | 15
            else:
                effects = effects << 4 | int(d.get_active())-1
        return effects

    def setup_effects(self, group, key, dropdown):
        self.effects_dd = dropdown
        model = gtk.ListStore(str)
        model.append([_("None")])
        model.append([_("Simple")])
        model.append([_("Classic")])
        model.append([_("Fade")])
        model.append([_("Spotlight")])
        model.append([_("Zoom")])
        model.append([_("Squish")])
        model.append([_("3D Turn")])
        model.append([_("3D Spotlight Turn")])
        model.append([_("Glow")])
        model.append([_("Custom")]) ##Always last
        dropdown.set_model(model)
        cell = gtk.CellRendererText()
        dropdown.pack_start(cell)
        dropdown.add_attribute(cell,'text',0)

        self.load_effect(group,key,dropdown)

        dropdown.connect("changed", self.effect_changed, (group, key))
        self.client.notify_add(group, key, self.reload_effect, dropdown)

    def load_effect(self, group, key, dropdown):
        effect_settings = self.client.get_int(group, key)
        hover_effect = effect_settings & 15 # not really hover
        bundle = 0
        for i in range(5):
            bundle = bundle << 4 | hover_effect

        if (bundle == effect_settings):
            if (hover_effect == 15):
                active = 0
            else:
                active = hover_effect+1
        else:
            active = 10 #Custom
            self.btn_edit_custom_effects.show()

        dropdown.set_active(int(active))

    def reload_effect(self, group, key, value, dropdown):
        self.load_effect(group, key, dropdown)

    def effect_changed(self, dropdown, groupkey):
        group, key = groupkey
        new_effects = 0
        effect = 0
        if dropdown.get_active() != 10: # not Custom
            if dropdown.get_active() == 0:
                effect = 15
            else:
                effect = dropdown.get_active() - 1
            for i in range(5):
                new_effects = new_effects << 4 | effect
            self.client.set_int(group, key, new_effects)
            self.btn_edit_custom_effects.hide()
            print "Setting effects to: ", "0x%0.8X" % new_effects
        else:
            self.btn_edit_custom_effects.show()
            response = self.custom_effects_dialog.run()
            self.custom_effects_dialog.hide()

    def setup_autostart(self, check):
        """sets up checkboxes"""
        self.load_autostart(check)
        check.connect("toggled", self.autostart_changed)

    def load_autostart(self, check):
        autostart_file = self.get_autostart_file_path()
        check.set_active(os.path.isfile(autostart_file))

    def reload_autostart(self, group, key, value, check):
        self.load_autostart(group, key, check)

    def autostart_changed(self, check):
        if check.get_active():
            self.create_autostarter()
        else:
            self.delete_autostarter()

    # The following code is adapted from screenlets-manager.py
    def get_autostart_file_path(self):
        if os.environ.has_key('DESKTOP_SESSION') and os.environ['DESKTOP_SESSION'].startswith('kde'):
            autostart_dir = os.path.join(os.environ['HOME'], '.kde', 'Autostart')
        else:
            autostart_dir = os.path.join(os.environ['HOME'], '.config', 'autostart')
        return os.path.join(autostart_dir, 'awn.desktop')

    def create_autostarter(self):
        '''Create an autostart entry for Awn.'''
        def err_dialog():
            err_msg = _('Automatic creation failed. Please manually create the directory:\n%s') % autostart_dir
            msg = gtk.MessageDialog(parent, type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK, message_format=err_msg)
            msg.show_all()
            msg.run()
            msg.destroy()
        autostart_file = self.get_autostart_file_path()
        autostart_dir = os.path.dirname(autostart_file)
        if not os.path.isdir(autostart_dir):
            # create autostart directory, if not existent
            parent = self.wTree.get_object('awnManagerWindow')
            dialog = gtk.Dialog(_('Confirm directory creation'), parent, gtk.DIALOG_MODAL, ((gtk.STOCK_NO, gtk.RESPONSE_NO, gtk.STOCK_YES, gtk.RESPONSE_YES)))
            dialog.vbox.add(gtk.Label(_('There is no existing autostart directory for your user account yet.\nDo you want me to automatically create it for you?')))
            dialog.show_all()
            response = dialog.run()
            dialog.destroy()
            if response == gtk.RESPONSE_YES:
                try:
                    os.mkdir(autostart_dir)
                except Exception, e:
                    err_dialog()
                    raise e
            else:
                err_dialog()
                return
        if not os.path.isfile(autostart_file):
            # create the autostart entry
            starter_item = DesktopEntry(autostart_file)
            starter_item.set('Name', 'Avant Window Navigator')
            starter_item.set('Exec', 'avant-window-navigator --startup')
            starter_item.set('Icon', 'avant-window-navigator')
            starter_item.set('X-GNOME-Autostart-enabled', 'true')
            starter_item.write()

    def delete_autostarter(self):
        '''Delete the autostart entry for the dock.'''
        autostart_file = self.get_autostart_file_path()
        if os.path.isfile(autostart_file):
            os.remove(autostart_file)

    def test_bzr_themes(self, widget, data=None):
        if widget.get_active() == True:
            self.create_sources_list()
            self.update_sources_list()
            print self.catalog_from_sources_list()
        else:
            pass

class awnManager:
    def __init__(self):
        self.theme = gtk.icon_theme_get_default()

    def safe_load_icon(self, name, size, flags=0):
        '''Loads an icon, with gtk-missing-image being the fallback.
        :param name: Either the name of an icon, or a list of icon names.
        :param size: The preferred size of the icon.
        :param flags: The flags used to determine how the icon is loaded. See
        gtk.IconLookupFlags for details.
        :returns: a gdk.Pixbuf on success, None on failure.
        '''
        icon = None
        if isinstance(name, (list, tuple)):
            icons = list(name) + ['gtk-missing-image']
            # Gtk.IconTheme.choose_icon() does not do what I want it to do...
            for icon_name in icons:
                if self.theme.has_icon(icon_name):
                    try:
                        icon = self.theme.load_icon(icon_name, size, flags)
                        break
                    except gobject.GError:
                        pass
            # if gtk-missing-image doesn't exist, we have a real problem...
            if icon is None:
                sys.stderr.write(_('Could not locate any of these icons: %s\n')
                                 % ', '.join(icons))
        else:
            try:
                icon = self.theme.load_icon(name, size, flags)
            except gobject.GError:
                icon = self.theme.load_icon('gtk-missing-image', size, flags)
                sys.stderr.write(_('Could not locate the following icon: %s\n')
                                 % name)
        return icon

    def make_menu_model(self):
        self.menu_model = gtk.ListStore(str, gtk.gdk.Pixbuf)

        pixbuf = self.safe_load_icon("gtk-preferences", 48, gtk.ICON_LOOKUP_FORCE_SVG)
        self.menu_model.append(['General', pixbuf])
        pixbuf = self.safe_load_icon("gtk-sort-ascending", 48, gtk.ICON_LOOKUP_FORCE_SVG)
        self.menu_model.append(['Applets', pixbuf])
        pixbuf = self.safe_load_icon("gtk-fullscreen", 48, gtk.ICON_LOOKUP_FORCE_SVG)
        self.menu_model.append(['Launchers', pixbuf])
        pixbuf = self.safe_load_icon("gtk-home", 48, gtk.ICON_LOOKUP_FORCE_SVG)
        self.menu_model.append(['Themes', pixbuf])

    def changeTab(self, iconView):
        self.notebook.set_current_page(iconView.get_cursor()[0][0])

    def about(self, button):
        self.about = gtk.AboutDialog()
        self.about.set_name(_("Avant Window Navigator"))
        version = defs.VERSION
        extra_version = defs.EXTRA_VERSION
        if len(extra_version) > 0:
            version += extra_version
        self.about.set_version(version)
        self.about.set_copyright("Copyright (C) 2007-2010 Awn-core team")
        self.about.set_authors([
            'Neil Jagdish Patel <njpatel@gmail.com>',
            'haytjes <hv1989@gmail.com>',
            'Miika-Petteri Matikainen <miikapetteri@gmail.com>',
            'Anthony Arobone  <aarobone@gmail.com>',
            'Ryan Rushton <ryan@rrdesign.ca>',
            'Michal Hruby  <michal.mhr@gmail.com>',
            'Julien Lavergne <julien.lavergne@gmail.com>',
            'Mark Lee <avant-wn@lazymalevolence.com>',
            'Rodney Cryderman <rcryderman@gmail.com>'
            ])
        self.about.set_comments(_("Fully customisable dock-like window navigator for GNOME."))
        self.about.set_license(
_("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.") +
"\n\n" +
_("This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.") +
"\n\n" +
_("You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA."))
        self.about.set_wrap_license(True)
        self.about.set_website('https://launchpad.net/awn/')
        self.about.set_logo(self.safe_load_icon('avant-window-navigator', 96, gtk.ICON_LOOKUP_USE_BUILTIN))
        self.about.set_documenters(["More to come..."])
        self.about.set_artists(["More to come..."])
        #self.about.set_translator_credits()
        if button is None:
            self.about.set_skip_taskbar_hint(True)
        self.about.run()
        self.about.destroy()

    def win_destroy(self, button, w):
        w.destroy()

    def main(self):
        gtk.main()

class awnLauncher(awnBzr):
    
    def callback_launcher_selection(self, selection, data=None):
        (model, iter) = selection.get_selected()
        if iter is not None:
            self.launcher_remove.set_sensitive(True)
            self.launcher_edit.set_sensitive(True)
        else:
            if hasattr(self, 'launcher_remove'): 
                self.launcher_remove.set_sensitive(False)
            if hasattr(self, 'launcher_edit'): 
                self.launcher_edit.set_sensitive(False)
        
    def launchers_reordered(self, model, path, iterator, data=None):
        data = []
        it = model.get_iter_first ()
        while (it):
            uri = model.get_value (it, 2)
            data.append(uri)
            it = model.iter_next (it)

        data = filter(None, data)

        if (self.last_uris != data):
            self.last_uris = data[:]
            self.client_taskman.set_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST, data)

    def refresh_launchers (self, group, key, value, extra):
        self.last_uris = value
        self.refresh_tree (self.last_uris, extra)

    #   Code below taken from:
    #   Alacarte Menu Editor - Simple fd.o Compliant Menu Editor
    #   Copyright (C) 2006  Travis Watkins
    #   Edited by Ryan Rushton
    
    def create_unique_launcher_file(self):
        path = os.path.join(defs.HOME_LAUNCHERS_DIR,
                            self.getUniqueFileId('awn_launcher', '.desktop'))
        return vfs.File.for_path(path)

    def edit(self, button):
        selection = self.treeview_launchers.get_selection()
        (model, iter) = selection.get_selected()
        path = model.get_value(iter, 2)
        vfile = vfs.File.for_path(path)
        if vfile.is_writable():
            output = None
        else:
            output = self.create_unique_launcher_file()
        editor = LauncherEditorDialog(vfile, output)
        editor.show_all()
        response = editor.run()
        if response == gtk.RESPONSE_APPLY and output is not None:
            paths = self.client_taskman.get_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST)
            idx = paths.index(path)
            paths[idx] = output.props.path
            self.client_taskman.set_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST, paths)

    def add(self, button):
        selection = self.treeview_launchers.get_selection()
        (model, iter) = selection.get_selected()
        vfile = self.create_unique_launcher_file()
        editor = LauncherEditorDialog(vfile, None)
        editor.show_all()
        response = editor.run()
        if response == gtk.RESPONSE_APPLY:
            paths = self.client_taskman.get_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST)
            paths.append(vfile.props.path)
            self.client_taskman.set_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST, paths)

    def remove(self, button):
        selection = self.treeview_launchers.get_selection()
        (model, iter) = selection.get_selected()
        path = model.get_value(iter, 2)

        paths = self.client_taskman.get_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST)
        paths.remove(path)
        if os.path.exists(path) and path.startswith(defs.HOME_LAUNCHERS_DIR):
            os.remove(path)
        self.client_taskman.set_list(GROUP_DEFAULT, defs.LAUNCHERS_LIST, paths)
        self.refresh_tree(paths, model)

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

class awnApplet(awnBzr):
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
        filter.set_name(_("Awn Applet Package"))
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
            message = _("Applet Installation Failed")
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def register_applet(self, appletpath, do_apply, applet_exists, msg=True):
        if do_apply:
            model = self.model
        else:
            model = self.appmodel

        if applet_exists:
            message = _("Applet Successfully Updated")
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
                    self.apply_applet_list_changes ()
                else:
                    model.set_value (row, 3, name)

            if msg:
                message = _("Applet Successfully Added")
            else:
                message = _("Applet Installation Failed")

        if msg:
            success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_OK, message_format=message)
            success.run()
            success.destroy()

    def activate_applet (self, treeview, path, col):
        select = treeview.get_selection()
        if not select:
            return
        model, iterator = select.get_selected ()
        if not iterator:
            return
        path = model.get_value (iterator, 2)
        icon, text, name = self.make_row (path)
        uid = "%d" % int(time.time())
        if len (text) < 2:
            print "cannot load desktop file %s" % path
            return

        self.active_model.append([icon, path, uid, text])

        self.apply_applet_list_changes()
        hs = self.scrollwindow1.get_hscrollbar()
        ha = self.scrollwindow1.get_hadjustment()
        hs.set_value(ha.upper - ha.page_size)
    
    def activate_applet_btn(self, widget, data=None):
        self.activate_applet(self.treeview_available, None, None)
        
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
            self.popup_msg(_("Can not delete active applet"))
            return

        dialog = gtk.Dialog(_("Delete Applet"),
                            None,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                            gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
        label = gtk.Label(_("<b>Delete %s?</b>") % item.getName())
        label.set_use_markup(True)
        align = gtk.Alignment()
        align.set_padding(5,5,5,5)
        align.add(label)
        dialog.vbox.add(align)
        dialog.show_all()
        result = dialog.run()

        if result == -3:
            execpath = item.get('X-AWN-AppletExec')
            fullpath = os.path.join(defs.HOME_APPLET_DIR, os.path.split(execpath)[0])

            if os.path.exists(fullpath) and ".config" in path:
                model.remove (iterator)
                self.remove_applet_dir(fullpath, path)
                self.apply_applet_list_changes()
                dialog.destroy()
            else:
                dialog.destroy()
                self.popup_msg(_("Unable to Delete Applet"))
        else:
            dialog.destroy()
            
    def deactivate_applet(self, button):
        cursor = self.icon_view.get_cursor()
        if not cursor:
            return
        itr = self.active_model.get_iter(cursor[0])
        name = os.path.splitext(os.path.basename(self.active_model.get_value(itr, 1)))[0]
        uid = self.active_model.get_value (itr, 2)
        #applet_client = awn.Config(name, uid)
        #applet_client.clear()
        self.active_model.remove(itr)
        self.apply_applet_list_changes()

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

    def apply_applet_list_changes (self):
        applets_list = []
        ua_list = []

        it = self.active_model.get_iter_first ()
        while (it):
            path = self.active_model.get_value (it, 1)
            uid = self.active_model.get_value (it, 2)
            s = "%s::%s" % (path, uid)
            if path.endswith(".desktop"):
                applets_list.append(s)
            else:
                ua_list.append(s)

            it= self.active_model.iter_next (it)

        self.client.set_list(defs.PANEL, defs.APPLET_LIST, applets_list)
        self.client.set_list(defs.PANEL, defs.UA_LIST, ua_list)

    def make_active_applets_model (self):
        self.active_model = AwnListStore(gtk.gdk.Pixbuf, str, str, str)
        self.active_model.connect("rows-reordered", self.applet_reorder)

        def activate_applet(active_model, dst_row, selection_data):
            model, src_row = selection_data.tree_get_row_drag_data()
            iter = model.get_iter(src_row)
            path = model.get_value (iter, 2)
            icon, text, name = self.make_row (path)
            uid = "%d" % int(time.time())
            active_model.insert(dst_row, [icon, path, uid, text])
            self.apply_applet_list_changes()
            
            infobar = self.wTree.get_object("tm_infobar")
            if self.check_for_task_manager():
                infobar.get_parent().hide_all()
            else:
                infobar.get_parent().show_all()
            
        self.active_model.connect("foreign-drop", activate_applet)

        self.icon_view = gtk.IconView(self.active_model)
        self.icon_view.set_pixbuf_column(0)
        self.icon_view.set_orientation(gtk.ORIENTATION_HORIZONTAL)
        self.icon_view.set_selection_mode(gtk.SELECTION_SINGLE)
        if hasattr(self.icon_view, 'set_tooltip_column'):
            self.icon_view.set_tooltip_column(3)
        self.icon_view.set_item_width(-1)
        self.icon_view.set_size_request(48, -1)
        self.icon_view.set_columns(100)
 
        self.scrollwindow1.add(self.icon_view)
        self.scrollwindow1.show_all()
        
        self.icon_view.connect('selection-changed', self.callback_active_applet_selection)
        
        applets = self.client.get_list(defs.PANEL, defs.APPLET_LIST)

        ua_applets = self.client.get_list(defs.PANEL, defs.UA_LIST)

        for ua in ua_applets:
            tokens = ua.split("::")
            applets.insert(int(tokens[1]), ua)

        self.refresh_icon_list (applets, self.active_model)
        
    def refresh_icon_list (self, applets, model):
        for a in applets:
            tokens = a.split("::")
            if tokens[0].endswith(".desktop"):
                path = tokens[0]
                uid = tokens[1]
                icon, text, name = self.make_row(path)
                if len (text) < 2:
                    continue;

                model.append([icon, path, uid, text])
            else:
                path = tokens[0]
                uid = tokens[1]
                theme = gtk.icon_theme_get_default ()
                icon = theme.load_icon ("screenlets", 32, 0)
                text = tokens[0]
                model.append([icon, path, uid, text])

    def load_applets (self):
        applets = self.applets_by_categories()
        model = self.make_applet_model(applets, self.treeview_available)

        model.set_sort_column_id(1, gtk.SORT_ASCENDING)
        self.treeview_available.set_search_column (3)

        
    def update_applets(self, list_applets):
        if list_applets == "All":
            list_applets = ''
        applets = self.applets_by_categories(list_applets)
        self.refresh_tree(applets, self.treeview_available.get_model())


    def popup_msg(self, message):
        success = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING,
                                    buttons=gtk.BUTTONS_OK, message_format=message)
        success.run()
        success.destroy()

    def applet_reorder(self, model, path, iterator, data=None):
        l = []
        it = model.get_iter_root ()
        while (it):
            path = model.get_value (it, 1)
            uid = model.get_value (it, 2)
            s = "%s::%s" % (path, uid)
            l.append(s)
            it = model.iter_next (it)

        applets = l
        applets = filter(None, applets)

        applets_list = []
        ua_list = []
        for a in applets:
            tokens = a.split("::")
            path = tokens[0]
            if path.endswith(".desktop"):
                applets_list.append(a)
            else:
                position = applets.index(a)
                ua = tokens[0] + "::" + str(position)
                ua_list.append(ua)

        self.client.set_list(defs.PANEL, defs.APPLET_LIST, applets_list)
        self.client.set_list(defs.PANEL, defs.UA_LIST, ua_list)

    def callback_widget_filter_applets(self, data=None):
        model = self.choose_categorie.get_model()
        select_cat = model.get_value(self.choose_categorie.get_active_iter(),0)
        self.update_applets(select_cat)

    def callback_widget_filter_applets_view(self, selection, data=None):
        (model, iter) = selection.get_selected()
        if iter is not None:
            self.update_applets(model.get_value(iter, 0))
    
    def callback_applet_selection(self, selection, data=None):
        (model, iter) = selection.get_selected()
        if iter is not None:
            self.appletActivate.set_sensitive(True)
            if os.access(model.get_value(iter, 2), os.W_OK):
                self.btn_delete.set_sensitive(True)
            else:
                self.btn_delete.set_sensitive(False)
        else:
            self.btn_delete.set_sensitive(False)

    def callback_active_applet_selection(self, iconview, data=None):
        sel = iconview.get_selected_items()
        if len(sel):
            model = iconview.get_model()
            iter = model.get_iter(sel[0])
            if iter is not None:
                self.appletDeactivate.set_sensitive(True)
            else:
                self.appletDeactivate.set_sensitive(False)
        else:
            self.appletDeactivate.set_sensitive(False)
                
class awnThemeCustomize(awnBzr):

    def export_theme(self, config, filename, newfilename, panel_id, save_pattern):
        tmpdir = tempfile.gettempdir()
        themedir = os.path.join(tmpdir, filename)
        themefile = os.path.join(tmpdir, filename+'.awn-theme')
        if os.path.exists(themefile):
            os.remove(themefile)
            if os.path.exists(themedir):
                shutil.rmtree(themedir)
        if os.path.exists(themedir):
            self.hide_export_dialog(None)
            # Translators: This string is preceded by a filename
            msg = _("%s already exists, unable to export theme.") % (themedir)
            self.theme_message(msg)
            return

        os.mkdir(themedir)

        f = open(themefile, "w")
        config.write(f)
        f.close()

        arrow_path = self.client.get_string(defs.EFFECTS, defs.ARROW_ICON)
        if os.path.exists(arrow_path):
            shutil.copy(arrow_path, themedir+'/arrow.png')

        pattern_path = self.client.get_string(defs.THEME, defs.PATTERN_FILENAME)
        if save_pattern and os.path.exists(pattern_path):
            root, ext = os.path.splitext(pattern_path)
            shutil.copy(pattern_path, "%s/pattern%s" % (themedir, ext))

        active_icon_path = self.client.get_string(defs.EFFECTS, defs.ACTIVE_RECT_ICON)
        if active_icon_path and os.path.exists(active_icon_path):
            shutil.copy(active_icon_path, themedir+'/active_icon.png')

        self.get_dock_image(themedir, panel_id)

        tarpath = os.path.join(tmpdir, filename+'.tgz')
        tFile = tarfile.open(tarpath, "w:gz")
        tFile.add(themedir, os.path.basename(themedir))
        tFile.add(themefile, os.path.basename(themefile))
        tFile.close()
	
        shutil.move(tarpath, newfilename)
        shutil.rmtree(themedir)
        os.remove(themefile)
        self.hide_export_dialog(None)

    def get_dock_image(self, themedir, panel_id):
        bus = dbus.SessionBus()
        panel = bus.get_object('org.awnproject.Awn',
                               '/org/awnproject/Awn/Panel%d' % (panel_id),
                               'org.awnproject.Awn.Panel')
        data = panel.GetSnapshot(byte_arrays=True)
        width, height, rowstride, has_alpha, bits_per_sample, n_channels, pixels = data
        pixels = array.array('c', pixels)
        surface = cairo.ImageSurface.create_for_data(pixels, cairo.FORMAT_ARGB32, width, height, rowstride)
        # get only a subimage
        newsurface = surface.create_similar(cairo.CONTENT_COLOR_ALPHA, 150, height)
        cr = cairo.Context(newsurface)
        cr.set_operator(cairo.OPERATOR_SOURCE)
        cr.set_source_surface(surface)
        cr.paint()
        newsurface.write_to_png(themedir+'/thumb.png')
            
    def install_theme(self, file):
        goodTheme = False
        customArrow = False
        customActiveIcon = False
        pattern = False
        if tarfile.is_tarfile(file):
            tar = tarfile.open(file, "r:gz")
            for member in tar.getmembers():
                if member.name.endswith(".awn-theme"):
                    goodTheme = member.name
                elif member.name.endswith("arrow.png"):
                    customArrow = member.name
                elif member.name.endswith("active_icon.png"):
                    customActiveIcon = member.name
                elif member.name.find("pattern.") >= 0:
                    pattern = member.name
            
            if goodTheme:
                themefile = os.path.join(defs.HOME_THEME_DIR, goodTheme)
                (filename, fileext) = os.path.splitext(goodTheme)
                themedir = os.path.join(defs.HOME_THEME_DIR, filename)
                
                if os.path.exists(themefile):
                    msg = _("Theme already installed, do you wish to overwrite it?")
                    message = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_WARNING, buttons=gtk.BUTTONS_YES_NO, message_format=msg)
                    resp = message.run()
                    if resp != gtk.RESPONSE_YES:
                        message.destroy()
                        return
                    else:
                        model = self.treeview_themes.get_model()
                        model.foreach(self.search_treeview, themefile)
                        message.destroy()
                
                tar.extractall(defs.HOME_THEME_DIR)
                tar.close()
                 
                config = ConfigParser()
                config.read(themefile)
                self.check_version(config.get('theme-info', 'Awn-Theme-Version'))
                
                config.set('theme-info', 'Icon', themedir+'/thumb.png')
                if customArrow:
                    config.set("config/effects", 'arrow_icon',
                               themedir+'/arrow.png')
                if customActiveIcon:
                    config.set("config/effects", 'active_background_icon',
                               themedir+'/active_icon.png')
                if pattern:
                    index = pattern.find('/pattern.')
                    config.set("config/theme", 'pattern_filename',
                               themedir+pattern[index:])
                f = open(themefile, "w")
                config.write(f)
                f.close()
                self.add_uris_to_model(self.treeview_themes.get_model(),[themefile])
            else:
                msg = _("This is an incompatible theme file.")
                self.theme_message(msg)
                
    def delete_theme(self):
        model = self.treeview_themes.get_model()
        selection = self.treeview_themes.get_selection()
        (model, iter) = selection.get_selected()
        if iter is not None:
            themefile = model.get_value(iter, 2)
            (themedir, fileExtension) = os.path.splitext(themefile)
            if os.path.isdir(themedir):
				shutil.rmtree(themedir)
            if os.path.exists(themefile):
                os.remove(themefile)
            model.remove(iter)
            self.deleteTheme.set_sensitive(False)
    
    def check_version(self, version):   
        req_version = defs.THEME_VERSION.split('.')
        version = version.split('.')
        if version[0] < req_version[0]:
            print "Major version is low"
        else:
            if version[1] < req_version[1]:
                print "Minor version is low"
            else:
                if version[2] < req_version[2]:
                    print "Micro version is low"
        
    def theme_message(self, msg):
        message = gtk.MessageDialog(parent=None, flags=0, type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_CLOSE, message_format=msg)
        resp = message.run()
        if resp == gtk.RESPONSE_CLOSE:
            message.destroy()
    
    def search_treeview(self, model, path, iter, filename=None):
        if model.get_value(iter, 2) == filename:
            model.remove(iter)
            return True
        
                                   
class awnTaskManager(awnBzr):
    
    def runDockmanagerSettings(self, xid):
        bus = dbus.SessionBus()
        daemon = bus.get_object('net.launchpad.DockManager.Daemon',
                                '/net/launchpad/DockManager/Daemon')
        daemon_interface = dbus.Interface(daemon, 
                                          'net.launchpad.DockManager.Daemon')
        daemon_interface.EmbedPreferences(xid, {'no-install': True})

    def ding(self):
        pass
