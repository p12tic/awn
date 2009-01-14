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

import sys, os, time, urllib, StringIO
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
from xdg.DesktopEntry import DesktopEntry

import awn
import awnDefs as defs
from awnLauncherEditor import awnLauncherEditor
import tarfile

from bzrlib.builtins import cmd_branch, cmd_pull
from bzrlib.plugins.launchpad.lp_directory import LaunchpadDirectory 

defs.i18nize(globals())

def dec2hex(n):
    """return the hexadecimal string representation of integer n"""
    n = int(n)
    if n == 0:
        return "00"
    return "%0.2X" % n

def hex2dec(s):
    """return the integer value of a hexadecimal string s"""
    return int(s, 16)

def make_color(hexi):
    """returns a gtk.gdk.Color from a hex string RRGGBBAA"""
    color = gtk.gdk.color_parse('#' + hexi[:6])
    alpha = hex2dec(hexi[6:])
    alpha = (float(alpha)/255)*65535
    return color, int(alpha)

def make_color_string(color, alpha):
    """makes avant-readable string from gdk.color & alpha (0-65535) """
    string = ""

    string = string + dec2hex(int( (float(color.red) / 65535)*255))
    string = string + dec2hex(int( (float(color.green) / 65535)*255))
    string = string + dec2hex(int( (float(color.blue) / 65535)*255))
    string = string + dec2hex(int( (float(alpha) / 65535)*255))

    #hack
    return string

EMPTY = "none";

class awnBzr:
	#Utils
	def lp_path_normalize(self, path):
		'''     Get a "lp:" format url and return a http url
                        path: a url from a branch
                        return: the http format of a lp: format, or the same url
		'''
                directory = LaunchpadDirectory()
                return directory._resolve(path).replace("bzr+ssh","http") 

	def read_list(self, file_path):
		'''	Read a flat file and return the content in a list
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
		'''	Create a new bzr branch.
			path: the path of the branch
			bzr_dir: the location of the futur tree.
		'''
		if os.path.exists(path):
			print ("Error, the path already exist")
		else:
			bzr_branch = cmd_branch()
			status = StringIO.StringIO()
			status = bzr_branch._setup_outf()
			bzr_branch.run(from_location=self.lp_path_normalize(bzr_dir), to_location=path) 

	def update_branch(self, path):
		''' 	Update a local branch
			path: Location of the branch
			Return the output of the command
		'''
		bzr_pull = cmd_pull()
		status = StringIO.StringIO()
		status = bzr_pull._setup_outf()
		bzr_pull.run(directory=path)

	def get_revision_from_path(self, path):
		''' 	Return the last revision number of the branch 
			specify with path parameter
		'''
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
		'''	Create a sources_list with all the sources (bzr branch or local files)
			Should be unique for an installation
			config: the directory to write sources.list.
			default: the default content of the sources.list
		'''
		sources_list_path = os.path.join(config,"sources.list")
		if not os.path.isfile(sources_list_path):
			f = open(sources_list_path, 'w')
			[f.write(i[0] + " " + i[1] + "\n") 
				for i 
				in default if i[0] <> '']
			f.close()
		else: print ("Error, a sources.list already exist")

	def update_sources_list(self, directories = defs.HOME_THEME_DIR):
		''' 	
			Update all sources of the sources list
			If the directory doesn't exist, a new branch will be create
			directories = default locations of the local branches to update
		'''
		sources_list = self.list_from_sources_list()
		sources_to_update = []
		[ sources_to_update.append(elem) 
			for elem in sources_list 
			if not elem[0].startswith("/") or not elem[1] == "local" ]
		
		for elem in sources_to_update:
			if not os.path.isdir(os.path.join(directories , elem[1])):
				print self.create_branch(elem[0], os.path.join(directories , elem[1]))
			else:
				print self.update_branch(os.path.join(directories , elem[1]))		

	def add_source(self, path, source_type="web", config = defs.HOME_CONFIG_DIR, directories = defs.HOME_THEME_DIR):
		'''	Add a source to the sources list.
			path: path to the branch
			source_type: type of source web or local
			config: the directory to write sources.list.
			directories = the directory to write sources
		'''
		if not self.sources_list_check_double_path(path) and path <> '':
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
			print ("Error, the path is empty or already exist")

	def remove_source(self, source, config = defs.HOME_CONFIG_DIR):
		'''	Remove a source from the sources.list
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
		''' 	Check if a path is already in the sources.list
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


	#Desktop files
    	def read_desktop(self, file_path):
		'''	Read a desktop file and return a dictionnary with all fields
			API still unstable
		'''

		#API of the desktop file (unstable, WIP etc ...)
		struct= {	'type': '',		# Applet or Theme
				'location':'',		# Location of the bzr branch
				'name': '',		# Name of the Type
				'comment':'',		# Comments
				'version':'',		# Version of the 
				'copyright':'',		# Copyright
				'author':'',		# Author
				'licence_code':'',	# Licence for the code
				'licence_icons':'',	# Licence for the icons
				'icon':'',		# Icon for the type
				# Applet specific
				'exec':'',		# Execution path, for applet
				'applet_type':'',	# Type of teh applet (C, Vala or Python)
				'applet_category':'',	# Category for the applet
				# Theme specific
				'effects':'',
				'orientation':'',
				'size':'',
				'gtk_theme_mode':'',	
				'corner_radius':'',	
				'panel_angle':'',	
				'curviness':'',		
			}
	
		desktop_entry = DesktopEntry(file_path)
		struct['type'] = desktop_entry.get('X-AWN-Type')
		struct['location'] = desktop_entry.get('X-AWN-Location')
		struct['name'] = desktop_entry.get('Name')
		#TODO More to add
		struct['icon'] = desktop_entry.get('Icon')
		struct['exec'] = desktop_entry.get('Exec')
		struct['applet_type'] = desktop_entry.get('X-AWN-AppletType')
		struct['applet_category'] = desktop_entry.get('X-AWN-AppletCategory')
		struct['effects'] = int(desktop_entry.get('X-AWN-ThemeEffects'))
		struct['orientation'] = int(desktop_entry.get('X-AWN-ThemeOrientation'))
		struct['size'] = int(desktop_entry.get('X-AWN-ThemeSize'))

		return struct

    	def load_element_from_desktop(self, file_path, parameter, group, key):
		'''	
			Read a desktop file, and load the paramater setting.
		'''
		struct = self.read_desktop(file_path)
		if struct['type'] == 'Theme':
			#Read the settings
			if not struct[parameter] == '':
				if type(struct[parameter]) is int:
					self.client.set_int(group, key, struct[parameter])
				elif type(struct[parameter]) is float:
					self.client.set_float(group, key, struct[parameter])
			#TODO more type settings
		else: 
			print "Error, the desktop file is not for a theme"


	def read_desktop_files_from_source(self, source, directories = defs.HOME_THEME_DIR):
		'''	Read all desktop file from a source
			source is a list of 2 entries ('url','directory').
			source is 1 line of the sources list.
		'''
		if source[1] == "local":
			path = source[0]
		else:
			path= str(directories + "/" + source[1])
		list_files = os.listdir(path)
		desktops =  [path + "/" + elem for elem in list_files if os.path.splitext(elem)[1] =='.desktop']
		return desktops

	def catalog_from_sources_list(self):
		'''	
			Return a catalog (list of desktop files from the sources list)
		'''
		catalog=[]
		sources_list = self.list_from_sources_list()
		for elem in sources_list:
			desktops = self.read_desktop_files_from_source(elem)
			if not desktops == []:
				[ catalog.append(i) for i in desktops ]
		return catalog

	def type_catalog_from_sources_list(self, type_catalog="Theme"):
		'''
		Return a catalog of themes or applets (list of desktop files from the sources list)
		type_catalog is the type of catalog (Theme of Applets)
		'''
		catalog = self.catalog_from_sources_list()
		type_catalog = []
		for elem in catalog:
			desktop = DesktopEntry(elem)
			if desktop.get('X-AWN-Type') == type_catalog:
				type_catalog.append(elem)
		return type_catalog

class awnPreferences(awnBzr):
    """This is the main class, duh"""
    def __init__(self, wTree):
        self.wTree = wTree

        self.client = awn.Config()
        self.client.ensure_group(defs.PANEL)
        self.client.ensure_group(defs.THEME)
        self.client.ensure_group(defs.EFFECTS)

    def setup_color(self, group, key, colorbut, show_opacity_scale = True):
        self.load_color(group, key, colorbut, show_opacity_scale)
        colorbut.connect("color-set", self.color_changed, (group, key))
        self.client.notify_add(group, key, self.reload_color, (colorbut,show_opacity_scale))

    def load_color(self, group, key, colorbut, show_opacity_scale = True):
        try:
            color, alpha = make_color(self.client.get_string(group, key))
        except TypeError:
            raise "\nKey: [%s]%s isn't set.\nRestarting AWN usually solves this issue\n" % (group, key)
        colorbut.set_color(color)
        if show_opacity_scale:
            colorbut.set_alpha(alpha)
        else:
            colorbut.set_use_alpha(False)

    def reload_color(self, entry, (colorbut,show_opacity_scale)):
        self.load_color(entry['group'], entry['key'], colorbut, show_opacity_scale)

    def color_changed(self, colorbut, groupkey):
        group, key = groupkey
        string =  make_color_string(colorbut.get_color(), colorbut.get_alpha())
        self.client.set_string(group, key, string)

    def setup_scale(self, group, key, scale):
        self.load_scale(group, key, scale)
        scale.connect("value-changed", self.scale_changed, (group, key))
        self.client.notify_add(group, key, self.reload_scale, scale)

    def load_scale(self, group, key, scale):
        try:
            val = self.client.get_float(group, key)
        except TypeError:
            raise "\nKey: [%s]%s isn't set.\nRestarting AWN usually solves this issue\n" % (group, key)
        val = 100 - (val * 100)
        scale.set_value(val)

    def reload_scale(self, entry, scale):
        self.load_scale(entry['group'], entry['key'], scale)

    def scale_changed(self, scale, groupkey):
        group, key = groupkey
        val = scale.get_value()
        val = 100 - val
        if (val):
            val = val/100
        self.client.set_float(group, key, val)

    def setup_spin(self, group, key, spin):
        self.load_spin(group, key, spin)
        spin.connect("value-changed", self.spin_changed, (group, key))
        self.client.notify_add(group, key, self.reload_spin, spin)

    def load_spin(self, group, key, spin):
        try:
            spin.set_value(self.client.get_float(group, key))
        except TypeError:
            raise "\nKey: [%s]%s isn't set.\nRestarting AWN usually solves this issue\n" % (group, key)
        except gobject.GError, err:
            spin.set_value(float(self.client.get_int(group, key)))

    def reload_spin(self, entry, spin):
        self.load_spin(entry['group'], entry['key'], spin)

    def spin_changed(self, spin, groupkey):
        group, key = groupkey
        try:
            self.client.get_float(group, key) #gives error if it is an int
            self.client.set_float(group, key, spin.get_value())
        except gobject.GError, err:
            self.client.set_int(group, key, int(spin.get_value()))

    def setup_chooser(self, group, key, chooser):
        """sets up png choosers"""
        fil = gtk.FileFilter()
        fil.set_name(_("PNG Files"))
        fil.add_pattern("*.png")
        fil.add_pattern("*.PNG")
        chooser.add_filter(fil)
        preview = gtk.Image()
        chooser.set_preview_widget(preview)

        self.load_chooser(group, key, chooser)
        self.client.notify_add(group, key, self.reload_chooser, chooser)

        chooser.connect("update-preview", self.update_preview, preview)
        chooser.connect("selection-changed", self.chooser_changed, (group, key))

    def load_chooser(self, group, key, chooser):
        try:
            filename = self.client.get_string(group, key)
            if os.path.exists(filename):
                chooser.set_uri(filename)
            elif(filename != "~"):
                self.client.set_string(group, key, "~")
        except TypeError:
            raise "\nKey: [%s]%s isn't set.\nRestarting AWN usually solves this issue\n" % (group, key)

    def reload_chooser(self, entry, chooser):
        self.load_chooser(entry['group'], entry['key'], chooser)

    def chooser_changed(self, chooser, groupkey):
        group, key = groupkey
        f = chooser.get_filename()
        if f == None:
            return
        self.client.set_string(group, key, f)

    def update_preview(self, chooser, preview):
        f = chooser.get_preview_filename()
        try:
            pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(f, 128, 128)
            preview.set_from_pixbuf(pixbuf)
            have_preview = True
        except:
            have_preview = False
        chooser.set_preview_widget_active(have_preview)

    def setup_bool(self, group, key, check):
        """sets up checkboxes"""
        self.load_bool(group, key, check)
        check.connect("toggled", self.bool_changed, (group, key))
        self.client.notify_add(group, key, self.reload_bool, check)

    def load_bool(self, group, key, check):
        check.set_active(self.client.get_bool(group, key))

    def reload_bool(self, entry, check):
        self.load_bool(entry['group'], entry['key'], check)

    def bool_changed(self, check, groupkey):
        group, key = groupkey
        self.client.set_bool(group, key, check.get_active())
#        if key == defs.KEEP_BELOW:
#            self.wTree.get_widget('autohide').set_active(check.get_active())
#        elif key == defs.AUTO_HIDE:
#            if not check.get_active() and self.wTree.get_widget("keepbelow").get_active():
#                self.wTree.get_widget("keepbelow").set_active(False)

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

    def reload_font(self, entry, font_btn):
        self.load_font(entry['group'], entry['key'], font_btn)

    def font_changed(self, font_btn, groupkey):
        group, key = groupkey
        self.client.set_string(group, key, font_btn.get_font_name())

    def setup_effect(self, group, key, dropdown):
        model = gtk.ListStore(str)
        model.append([_("Simple")])
        model.append([_("Classic")])
        model.append([_("Fade")])
        model.append([_("Spotlight")])
        model.append([_("Zoom")])
        model.append([_("Squish")])
        model.append([_("3D turn")])
        model.append([_("3D turn with spotlight")])
        model.append([_("Glow")])
        dropdown.set_model(model)
        cell = gtk.CellRendererText()
        dropdown.pack_start(cell)
        dropdown.add_attribute(cell,'text',0)

        self.load_effect(group, key, dropdown)
        dropdown.connect("changed", self.effect_changed, (group, key))

        self.client.notify_add(group, key, self.reload_effect, dropdown)

    def load_effect(self, group, key, dropdown):
        dropdown.set_active(self.client.get_int(group, key))

    def reload_effect(self, entry, dropdown):
        self.load_effect(entry['group'], entry['key'], dropdown)

    def effect_changed(self, dropdown, groupkey):
        group, key = groupkey
        self.client.set_int(group, key, dropdown.get_active())

    def setup_look(self, group, key, dropdown):
        dropdown.append_text(_("Flat bar"))
        dropdown.append_text(_("3D look"))
        self.changed_look(dropdown)
        dropdown.connect("changed", self.changed_look)
        self.client.notify_add(group, key, self.reload_look, dropdown)

    def reload_look(self, entry, dropdown):
        if self.client.get_int(defs.BAR, defs.BAR_ANGLE) == 0:
            dropdown.set_active(0)
            self.wTree.get_widget("barangle_holder").hide_all()
        else:
            dropdown.set_active(1)
            self.wTree.get_widget("barangle_holder").show_all()

    def changed_look(self, dropdown):
        if dropdown.get_active() == -1: #init
            self.reload_look(0, dropdown)
        elif dropdown.get_active() == 0:
            self.wTree.get_widget("barangle").set_value(0)
            self.wTree.get_widget("barangle_holder").hide_all()
        else:
            self.wTree.get_widget("barangle").set_value(45)
            self.wTree.get_widget("barangle_holder").show_all()

    def setup_effect_custom(self, group, key):
        self.effect_drop = []
        for drop in ['hover', 'open', 'close', 'launch', 'attention']:
            d = self.wTree.get_widget('effect_'+drop)
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
        for drop in ['hover', 'open', 'close', 'launch', 'attention']:
            d = self.wTree.get_widget('effect_'+drop)
            d.connect("changed", self.effect_custom_changed, (group, key))

    def load_effect_custom(self, group, key):
        effect_settings = self.client.get_int(group, key)
        cnt = 0
        for drop in ['hover', 'open', 'close', 'launch', 'attention']:
            d = self.wTree.get_widget('effect_'+drop)
            current_effect = (effect_settings & (15 << (cnt*4))) >> (cnt*4)
            if(current_effect == 15):
                d.set_active(0)
            else:
                d.set_active(current_effect+1)
            cnt = cnt+1

    def reload_effect_custom(self, entry, nothing):
        self.load_effect_custom(entry['group'], entry['key'])

    def effect_custom_changed(self, dropdown, groupkey):
        group, key = groupkey
        if (dropdown.get_active() != self.effects_dd.get_active()):
            self.effects_dd.set_active(10) #Custom
            new_effects = self.get_custom_effects()
            self.client.set_int(group, key, new_effects)
            print "effects set to: ", "%0.8X" % new_effects

    def get_custom_effects(self):
        effects = 0
        for drop in ['attention', 'launch', 'close', 'open', 'hover']:
            d = self.wTree.get_widget('effect_'+drop)
            if(d.get_active() == 0):
                effects = effects << 4 | 15
            else:
                effects = effects << 4 | int(d.get_active())-1
        return effects

    def setup_effect(self, group, key, dropdown):
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
        #self.setup_effect_custom(group, key)
        self.client.notify_add(group, key, self.reload_effect, dropdown)

    def load_effect(self, group, key, dropdown):
        effect_settings = self.client.get_int(group, key)
        hover_effect = effect_settings & 15
        bundle = 0
        for i in range(5):
            bundle = bundle << 4 | hover_effect

        if (bundle == effect_settings):
        #    self.wTree.get_widget('customeffectsframe').hide()
        #    self.wTree.get_widget('customeffectsframe').set_no_show_all(True)
            if (hover_effect == 15):
                active = 0
            else:
                active = hover_effect+1
        else:
            active = 10 #Custom
        #    self.wTree.get_widget('customeffectsframe').show()

        dropdown.set_active(int(active))

    def reload_effect(self, entry, dropdown):
        self.load_effect(entry['group'], entry['key'], dropdown)

    def effect_changed(self, dropdown, groupkey):
        group, key = groupkey
        new_effects = 0
        effect = 0
        if(dropdown.get_active() != 10): # not Custom
#            self.wTree.get_widget('customeffectsframe').hide()
            if(dropdown.get_active() == 0):
                effect = 15
            else:
                effect = dropdown.get_active() - 1
            for i in range(5):
                new_effects = new_effects << 4 | effect
            self.client.set_int(group, key, new_effects)
            print "effects set to: ", "%0.8X" % new_effects
#            self.load_effect_custom(group, key)
        else:
#            self.wTree.get_widget('customeffectsframe').show()
            pass

    def setup_autostart(self, group, key, check):
        """sets up checkboxes"""
        self.load_autostart(group, key, check)
        check.connect("toggled", self.autostart_changed, (group, key))
        self.client.notify_add(group, key, self.reload_autostart, check)

    def load_autostart(self, group, key, check):
        check.set_active(self.client.get_bool(group, key))

    def reload_autostart(self, entry, check):
        self.load_autostart(entry['group'], entry['key'], check)

    def autostart_changed(self, check, groupkey):
        group, key = groupkey
        self.client.set_bool(group, key, check.get_active())
        if check.get_active():
            self.create_autostarter()
        else:
            self.delete_autostarter()

    # The following code is adapted from screenlets-manager.py
    def get_autostart_file_path(self):
        if os.environ['DESKTOP_SESSION'].startswith('kde'):
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
            parent = self.wTree.get_widget('awnManagerWindow')
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
            starter_item.set('Exec', 'avant-window-navigator')
            starter_item.set('X-GNOME-Autostart-enabled', 'true')
            starter_item.write()

    def delete_autostarter(self):
        '''Delete the autostart entry for the dock.'''
        os.remove(self.get_autostart_file_path())

    def setup_orientation(self, group, key, dropdown):
        self.effects_dd = dropdown
        model = gtk.ListStore(str)
        model.append([_("Top")])
        model.append([_("Right")])
        model.append([_("Bottom")])
        model.append([_("Left")])
        dropdown.set_model(model)
        cell = gtk.CellRendererText()
        dropdown.pack_start(cell)
        dropdown.add_attribute(cell,'text',0)

        self.load_orientation(group,key,dropdown)

        dropdown.connect("changed", self.orientation_changed, (group, key))
        #self.setup_effect_custom(group, key)
        self.client.notify_add(group, key, self.reload_orientation, dropdown)

    def load_orientation(self, group, key, dropdown):
        orientation_settings = self.client.get_int(group, key)
        dropdown.set_active(int(orientation_settings))

    def reload_orientation(self, entry, dropdown):
        self.load_orientation(entry['group'], entry['key'], dropdown)

    def orientation_changed(self, dropdown, groupkey):
	group, key = groupkey
	self.client.set_int(group, key, dropdown.get_active())

    def setup_freeze(self, toggle, freezed, group, key, parameter, data=None):
	'''	Setup a "linked" toggle button
		toggle : the gtk.ToggleButton
		freezed : the gtkWidget to freeze
	'''
	toggle.connect("toggled", self.freeze_changed, freezed, group, key, parameter)
	
    def freeze_changed(self, widget, freezed, group, key, parameter):
	'''	Callback for the setup_freeze
	'''
	if widget.get_active() == True:
		freezed.set_sensitive(False)
		self.load_element_from_desktop(self.theme_desktop, parameter, group, key)
	else:
		freezed.set_sensitive(True)

    def test_bzr_themes(self, widget, data=None):
		if widget.get_active() == True:
			self.create_sources_list()
			self.update_sources_list()
			print self.catalog_from_sources_list()
		else:
			pass

class awnManager:

    def __init__(self):

        if not os.path.exists(defs.HOME_CONFIG_DIR):
            os.makedirs(defs.HOME_CONFIG_DIR)
        self.GLADE_PATH = os.path.join(AWN_MANAGER_DIR, 'awn-manager-mini.glade')
        gtk.glade.bindtextdomain(defs.I18N_DOMAIN, defs.LOCALEDIR)
        gtk.glade.textdomain(defs.I18N_DOMAIN)

        self.wTree = gtk.glade.XML(self.GLADE_PATH, domain=defs.I18N_DOMAIN)

        self.window = self.wTree.get_widget("awnManagerWindow")
        self.theme = gtk.icon_theme_get_default()
        icon_search_path = os.path.join('@DATADIR@', 'icons')
        if icon_search_path not in self.theme.get_search_path():
            self.theme.append_search_path(icon_search_path)
        icon_list = []
        icon_sizes = self.theme.get_icon_sizes('awn-manager')
        for size in icon_sizes:
            if size == -1: # scalable
                if 128 not in icon_sizes:
                    icon = self.safe_load_icon('awn-manager', 128, gtk.ICON_LOOKUP_USE_BUILTIN)
                else:
                    continue
            else:
                icon = self.safe_load_icon('awn-manager', size, gtk.ICON_LOOKUP_USE_BUILTIN)
            icon_list.append(icon)
        if len(icon_list) > 0:
            gtk.window_set_default_icon_list(*icon_list)
        self.window.connect("delete-event", gtk.main_quit)

        # Enable RGBA colormap
        self.gtk_screen = self.window.get_screen()
        colormap = self.gtk_screen.get_rgba_colormap()
        if colormap == None:
            colormap = self.gtk_screen.get_rgb_colormap()
        gtk.widget_set_default_colormap(colormap)

        #self.notebook = self.wTree.get_widget("panelCategory")

        refresh = self.wTree.get_widget("buttonRefresh")
        refresh.connect("clicked", self.refresh)

        about = self.wTree.get_widget("buttonAbout")
        about.connect("clicked", self.about)

        close = self.wTree.get_widget("buttonClose")
        close.connect("clicked", gtk.main_quit)

        #self.make_menu_model()

        #icon_view = gtk.IconView(self.menu_model)
        #icon_view.set_text_column(0)
        #icon_view.set_pixbuf_column(1)
        #icon_view.set_orientation(gtk.ORIENTATION_VERTICAL)
        #icon_view.set_selection_mode(gtk.SELECTION_SINGLE)
        #icon_view.set_columns(1)
        #icon_view.set_item_width(-1)
        #icon_view.set_size_request(icon_view.get_item_width(), -1)
        #icon_view.connect("selection-changed", self.changeTab)

        #iconViewFrame = self.wTree.get_widget('CategoryMenuFrame')
        #iconViewFrame.add(icon_view)
        #iconViewFrame.show_all()

        #applet
        #self.appletManager = awnApplet(self.wTree)

        #launcher
        #self.launchManager = awnLauncher(self.wTree)

        #preferences
        #self.prefManager = awnPreferencesMini(self.wTree)

        #theme
        #self.themeManager = AwnThemeManager(self.wTree)

        self.window.show_all()

    def safe_load_icon(self, name, size, flags = 0):
        try:
            icon = self.theme.load_icon(name, size, flags)
        except gobject.GError:
            msg = _('Could not load the "' + name + '" icon.  Make sure that ' + \
                    'the SVG loader for Gtk+ is installed. It usually comes ' + \
                    'with librsvg, or a package similarly named.')
            dialog = gtk.MessageDialog(self.window, 0, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, msg)
            dialog.run()
            dialog.destroy()
            sys.exit(1)
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

    def refresh(self, button):
        dialog = gtk.MessageDialog(self.window, 0, gtk.MESSAGE_INFO,
                                   gtk.BUTTONS_OK,
                                   _('AWN has been successfully refreshed'))
        dialog.run()
        dialog.hide()

    def about(self, button):
        self.about = gtk.AboutDialog()
        self.about.set_name(_("Avant Window Navigator"))
        self.about.set_version('@VERSION@')
        self.about.set_copyright("Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>")
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
        self.about.run()
        self.about.destroy()

    def win_destroy(self, button, w):
        w.destroy()

    def main(self):
        gtk.main()

class awnLauncher:

    def __init__(self, glade):
        self.wTree = glade
        if not os.path.exists(defs.HOME_LAUNCHERS_DIR):
            os.makedirs(defs.HOME_LAUNCHERS_DIR)

        self.load_finished = False

        self.client = awn.Config()
        self.client.ensure_group(defs.LAUNCHERS)

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

        if not None in launchers and self.load_finished:
            self.client.set_list(defs.LAUNCHERS, defs.LAUNCHERS_LIST, awn.CONFIG_LIST_STRING, launchers)

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
        if self.client.exists(defs.LAUNCHERS, defs.LAUNCHERS_LIST):
            uris = self.client.get_list(defs.LAUNCHERS, defs.LAUNCHERS_LIST, awn.CONFIG_LIST_STRING)

        self.refresh_tree(uris)

        self.load_finished = True

    def refresh_tree (self, uris):
        self.model.clear()
        for i in uris:
		if os.path.isfile(i):
            		text = self.make_row (i)
            		if len(text) > 2:
                		row = self.model.append ()
                		self.model.set_value (row, 0, self.make_icon (i))
                		self.model.set_value (row, 1, text)
                		self.model.set_value (row, 2, i)

    def make_row (self, uri):
        try:
            item = DesktopEntry (uri)
            text = "<b>%s</b>\n%s" % (item.getName(), item.getComment())
        except:
            return ""
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
        if os.path.exists(uri):
            uris = self.client.get_list(defs.LAUNCHERS, defs.LAUNCHERS_LIST, awn.CONFIG_LIST_STRING)
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

        self.scrollwindow = self.wTree.get_widget("appletScrollActive")
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

    def activate_applet (self, button):
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

        self.client.set_list(defs.PANEL, defs.APPLET_LIST, awn.CONFIG_LIST_STRING, l)

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

        applets = self.client.get_list(defs.PANEL, defs.APPLET_LIST, awn.CONFIG_LIST_STRING)

        self.refresh_icon_list (applets, self.active_model)

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
            icon, text = self.make_row(path)
            if len (text) < 2:
                continue;

            model.append([icon, path, uid, text])

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
        self.appmodel.set_sort_column_id(1, gtk.SORT_ASCENDING)
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
            self.client.set_list(defs.PANEL, defs.APPLET_LIST, awn.CONFIG_LIST_STRING, applets)
