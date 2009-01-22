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
except Exception, e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)
from xdg.DesktopEntry import DesktopEntry

import awn
import awnDefs as defs

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
        self.client.ensure_group(defs.BAR)
        self.client.ensure_group(defs.WINMAN)
        self.client.ensure_group(defs.APP)
        self.client.ensure_group(defs.TITLE)

        self.setup_autostart (self.wTree.get_widget("autostart"))

        self.setup_bool (defs.AWN, defs.AUTO_HIDE, self.wTree.get_widget("autohide"))
        self.setup_bool (defs.AWN, defs.KEEP_BELOW, self.wTree.get_widget("keepbelow"))
        self.setup_bool (defs.AWN, defs.PANEL_MODE, self.wTree.get_widget("panelmode"))
        self.setup_bool (defs.APP, defs.NAME_CHANGE_NOTIFY, self.wTree.get_widget("namechangenotify"))
        self.setup_bool (defs.BAR, defs.RENDER_PATTERN, self.wTree.get_widget("patterncheck"))
        self.setup_bool (defs.BAR, defs.ROUNDED_CORNERS, self.wTree.get_widget("roundedcornerscheck"))
	self.setup_bool (defs.BAR, defs.EXPAND_BAR, self.wTree.get_widget("expandbarcheck"))
        self.setup_bool (defs.WINMAN, defs.SHOW_ALL_WINS, self.wTree.get_widget("allwindowscheck"))

        self.setup_bool (defs.BAR, defs.SHOW_SEPARATOR, self.wTree.get_widget("separatorcheck"))
        self.setup_bool (defs.APP, defs.TASKS_H_ARROWS, self.wTree.get_widget("arrowcheck"))
        self.setup_effect (defs.APP, defs.ICON_EFFECT, self.wTree.get_widget("iconeffects"))

        self.setup_chooser(defs.APP, defs.ACTIVE_PNG, self.wTree.get_widget("activefilechooser"))
        self.setup_chooser(defs.BAR, defs.PATTERN_URI, self.wTree.get_widget("patternchooserbutton"))

        self.setup_font(defs.TITLE, defs.FONT_FACE, self.wTree.get_widget("selectfontface"))

        self.setup_spin(defs.BAR, defs.ICON_OFFSET, self.wTree.get_widget("bariconoffset"))
        self.setup_spin(defs.BAR, defs.BAR_HEIGHT, self.wTree.get_widget("barheight"))
        self.setup_spin(defs.BAR, defs.BAR_ANGLE, self.wTree.get_widget("barangle"))
        self.setup_spin(defs.APP, defs.ARROW_OFFSET, self.wTree.get_widget("arrowoffset"))
        self.setup_look(defs.BAR, defs.BAR_ANGLE, self.wTree.get_widget("look"))

        self.setup_scale(defs.BAR, defs.PATTERN_ALPHA, self.wTree.get_widget("patternscale"))

        self.setup_color(defs.TITLE, defs.TEXT_COLOR, self.wTree.get_widget("textcolor"), False)
        self.setup_color(defs.TITLE, defs.SHADOW_COLOR, self.wTree.get_widget("shadowcolor"))
        self.setup_color(defs.TITLE, defs.BACKGROUND, self.wTree.get_widget("backgroundcolor"))

        self.setup_color(defs.BAR, defs.BORDER_COLOR, self.wTree.get_widget("mainbordercolor"))
        self.setup_color(defs.BAR, defs.HILIGHT_COLOR, self.wTree.get_widget("internalbordercolor"))

        self.setup_color(defs.BAR, defs.GLASS_STEP_1, self.wTree.get_widget("gradientcolor1"))
        self.setup_color(defs.BAR, defs.GLASS_STEP_2, self.wTree.get_widget("gradientcolor2"))

        self.setup_color(defs.BAR, defs.GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor1"))
        self.setup_color(defs.BAR, defs.GLASS_HISTEP_2, self.wTree.get_widget("highlightcolor2"))

        self.setup_color(defs.BAR, defs.SEP_COLOR, self.wTree.get_widget("sepcolor"))
        self.setup_color(defs.APP, defs.ARROW_COLOR, self.wTree.get_widget("arrowcolor"))

    def reload(self):
        self.load_autostart (self.wTree.get_widget("autostart"))

        self.load_bool (defs.AWN, defs.AUTO_HIDE, self.wTree.get_widget("autohide"))
        self.load_bool (defs.AWN, defs.KEEP_BELOW, self.wTree.get_widget("keepbelow"))
        self.load_bool (defs.AWN, defs.PANEL_MODE, self.wTree.get_widget("panelmode"))
        self.load_bool (defs.APP, defs.NAME_CHANGE_NOTIFY, self.wTree.get_widget("namechangenotify"))
        self.load_bool (defs.BAR, defs.RENDER_PATTERN, self.wTree.get_widget("patterncheck"))
        self.load_bool (defs.BAR, defs.ROUNDED_CORNERS, self.wTree.get_widget("roundedcornerscheck"))
        self.load_bool (defs.WINMAN, defs.SHOW_ALL_WINS, self.wTree.get_widget("allwindowscheck"))

        self.load_bool (defs.BAR, defs.SHOW_SEPARATOR, self.wTree.get_widget("separatorcheck"))
        self.load_bool (defs.APP, defs.TASKS_H_ARROWS, self.wTree.get_widget("arrowcheck"))
        self.load_effect (defs.APP, defs.ICON_EFFECT, self.wTree.get_widget("iconeffects"))

        self.load_chooser(defs.APP, defs.ACTIVE_PNG, self.wTree.get_widget("activefilechooser"))
        self.load_chooser(defs.BAR, defs.PATTERN_URI, self.wTree.get_widget("patternchooserbutton"))

        self.load_font(defs.TITLE, defs.FONT_FACE, self.wTree.get_widget("selectfontface"))

        self.load_spin(defs.BAR, defs.ICON_OFFSET, self.wTree.get_widget("bariconoffset"))
        self.load_spin(defs.BAR, defs.BAR_HEIGHT, self.wTree.get_widget("barheight"))
        self.load_spin(defs.BAR, defs.BAR_ANGLE, self.wTree.get_widget("barangle"))
        self.load_spin(defs.APP, defs.ARROW_OFFSET, self.wTree.get_widget("arrowoffset"))

        self.load_scale(defs.BAR, defs.PATTERN_ALPHA, self.wTree.get_widget("patternscale"))

        self.load_color(defs.TITLE, defs.TEXT_COLOR, self.wTree.get_widget("textcolor"), False)
        self.load_color(defs.TITLE, defs.SHADOW_COLOR, self.wTree.get_widget("shadowcolor"))
        self.load_color(defs.TITLE, defs.BACKGROUND, self.wTree.get_widget("backgroundcolor"))

        self.load_color(defs.BAR, defs.BORDER_COLOR, self.wTree.get_widget("mainbordercolor"))
        self.load_color(defs.BAR, defs.HILIGHT_COLOR, self.wTree.get_widget("internalbordercolor"))

        self.load_color(defs.BAR, defs.GLASS_STEP_1, self.wTree.get_widget("gradientcolor1"))
        self.load_color(defs.BAR, defs.GLASS_STEP_2, self.wTree.get_widget("gradientcolor2"))

        self.load_color(defs.BAR, defs.GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor1"))
        self.load_color(defs.BAR, defs.GLASS_HISTEP_2, self.wTree.get_widget("highlightcolor2"))

        self.load_color(defs.BAR, defs.SEP_COLOR, self.wTree.get_widget("sepcolor"))
        self.load_color(defs.APP, defs.ARROW_COLOR, self.wTree.get_widget("arrowcolor"))       

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
        if key == defs.KEEP_BELOW:
            self.wTree.get_widget('autohide').set_active(check.get_active())
        elif key == defs.AUTO_HIDE:
            if not check.get_active() and self.wTree.get_widget("keepbelow").get_active():
                self.wTree.get_widget("keepbelow").set_active(False)

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
            self.effects_dd.set_active(9) #Custom
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
        self.setup_effect_custom(group, key)
        self.client.notify_add(group, key, self.reload_effect, dropdown)

    def load_effect(self, group, key, dropdown):
        effect_settings = self.client.get_int(group, key)
        hover_effect = effect_settings & 15
        bundle = 0
        for i in range(5):
            bundle = bundle << 4 | hover_effect

        if (bundle == effect_settings):
            self.wTree.get_widget('customeffectsframe').hide()
            self.wTree.get_widget('customeffectsframe').set_no_show_all(True)
            if (hover_effect == 15):
                active = 0
            else:
                active = hover_effect+1
        else:
            active = 9 #Custom
            self.wTree.get_widget('customeffectsframe').show()

        dropdown.set_active(int(active))

    def reload_effect(self, entry, dropdown):
        self.load_effect(entry['group'], entry['key'], dropdown)

    def effect_changed(self, dropdown, groupkey):
        group, key = groupkey
        new_effects = 0
        effect = 0
        if(dropdown.get_active() != 9): # not Custom
            self.wTree.get_widget('customeffectsframe').hide()
            if(dropdown.get_active() == 0):
                effect = 15
            else:
                effect = dropdown.get_active() - 1
            for i in range(5):
                new_effects = new_effects << 4 | effect
            self.client.set_int(group, key, new_effects)
            print "effects set to: ", "%0.8X" % new_effects
            self.load_effect_custom(group, key)
        else:
            self.wTree.get_widget('customeffectsframe').show()

    def setup_autostart(self, check):
        """sets up checkboxes"""
        self.load_autostart(check)
        check.connect("toggled", self.autostart_changed)

    def load_autostart(self, check):
        autostart_file = self.get_autostart_file_path()
        check.set_active(os.path.isfile(autostart_file))

    def reload_autostart(self, entry, check):
        self.load_autostart(entry['group'], entry['key'], check)

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
            starter_item.set('Exec', 'awn-autostart')
            starter_item.set('X-GNOME-Autostart-enabled', 'true')
            starter_item.write()

    def delete_autostarter(self):
        '''Delete the autostart entry for the dock.'''
        os.remove(self.get_autostart_file_path())
