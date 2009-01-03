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

import sys, os
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

#from bzrlib import bzrdir

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

class awnPreferences:
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
