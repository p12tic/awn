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
    import gconf
except:
    sys.exit(1)

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

        self.client = gconf.client_get_default()
        self.client.add_dir(defs.BAR_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(defs.WINMAN_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(defs.APP_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(defs.TITLE_PATH, gconf.CLIENT_PRELOAD_NONE)

        self.setup_bool (defs.AWN_AUTO_HIDE, self.wTree.get_widget("autohide"))
        self.setup_bool (defs.AWN_KEEP_BELOW, self.wTree.get_widget("keepbelow"))
        self.setup_bool (defs.AWN_PANEL_MODE, self.wTree.get_widget("panelmode"))
        self.setup_bool (defs.APP_NAME_CHANGE_NOTIFY, self.wTree.get_widget("namechangenotify"))
        self.setup_bool (defs.BAR_RENDER_PATTERN, self.wTree.get_widget("patterncheck"))
        self.setup_bool (defs.BAR_ROUNDED_CORNERS, self.wTree.get_widget("roundedcornerscheck"))
        self.setup_bool (defs.WINMAN_SHOW_ALL_WINS, self.wTree.get_widget("allwindowscheck"))

        self.setup_bool (defs.BAR_SHOW_SEPARATOR, self.wTree.get_widget("separatorcheck"))
        self.setup_bool (defs.APP_TASKS_H_ARROWS, self.wTree.get_widget("arrowcheck"))
        self.setup_effect (defs.APP_ICON_EFFECT, self.wTree.get_widget("iconeffects"))

        self.setup_chooser(defs.APP_ACTIVE_PNG, self.wTree.get_widget("activefilechooser"))
        self.setup_chooser(defs.BAR_PATTERN_URI, self.wTree.get_widget("patternchooserbutton"))

        self.setup_font(defs.TITLE_FONT_FACE, self.wTree.get_widget("selectfontface"))

        self.setup_spin(defs.BAR_ICON_OFFSET, self.wTree.get_widget("bariconoffset"))
        self.setup_spin(defs.BAR_HEIGHT, self.wTree.get_widget("barheight"))
        self.setup_spin(defs.BAR_ANGLE, self.wTree.get_widget("barangle"))
        self.setup_spin(defs.APP_ARROW_OFFSET, self.wTree.get_widget("arrowoffset"))
        self.setup_look(self.wTree.get_widget("look"))

        self.setup_scale(defs.BAR_PATTERN_ALPHA, self.wTree.get_widget("patternscale"))

        self.setup_color(defs.TITLE_TEXT_COLOR, self.wTree.get_widget("textcolor"))
        self.setup_color(defs.TITLE_SHADOW_COLOR, self.wTree.get_widget("shadowcolor"))
        self.setup_color(defs.TITLE_BACKGROUND, self.wTree.get_widget("backgroundcolor"))

        self.setup_color(defs.BAR_BORDER_COLOR, self.wTree.get_widget("mainbordercolor"))
        self.setup_color(defs.BAR_HILIGHT_COLOR, self.wTree.get_widget("internalbordercolor"))

        self.setup_color(defs.BAR_GLASS_STEP_1, self.wTree.get_widget("gradientcolor1"))
        self.setup_color(defs.BAR_GLASS_STEP_2, self.wTree.get_widget("gradientcolor2"))

        self.setup_color(defs.BAR_GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor1"))
        self.setup_color(defs.BAR_GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor2"))

        self.setup_color(defs.BAR_SEP_COLOR, self.wTree.get_widget("sepcolor"))
        self.setup_color(defs.APP_ARROW_COLOR, self.wTree.get_widget("arrowcolor"))

    def setup_color(self, key, colorbut):
        try:
            color, alpha = make_color(self.client.get_string(key))
        except TypeError:
            raise "\nKey: "+key+" isn't set.\nRestarting AWN usually solves this issue\n"
        colorbut.set_color(color)
        colorbut.set_alpha(alpha)
        colorbut.connect("color-set", self.color_changed, key)

    def color_changed(self, colorbut, key):
        string =  make_color_string(colorbut.get_color(), colorbut.get_alpha())
        self.client.set_string(key, string)

    def setup_scale(self, key, scale):
        try:
            val = self.client.get_float(key)
        except TypeError:
            raise "\nKey: "+key+" isn't set.\nRestarting AWN usually solves this issue\n"
        val = 100 - (val * 100)
        scale.set_value(val)
        scale.connect("value-changed", self.scale_changed, key)

    def scale_changed(self, scale, key):
        val = scale.get_value()
        val = 100 - val
        if (val):
            val = val/100
        self.client.set_float(key, val)

    def setup_spin(self, key, spin):
        try:
            spin.set_value(    self.client.get_float(key) )
        except TypeError:
            raise "\nKey: "+key+" isn't set.\nRestarting AWN usually solves this issue\n"
        except gobject.GError, err:
            spin.set_value(    float(self.client.get_int(key)) )

        spin.connect("value-changed", self.spin_changed, key)

    def spin_changed(self, spin, key):
        try:
            self.client.get_float(key) #gives error if it is an int
            self.client.set_float(key, spin.get_value())
        except gobject.GError, err:
            self.client.set_int(key, int(spin.get_value()))

    def setup_chooser(self, key, chooser):
        """sets up png choosers"""
        fil = gtk.FileFilter()
        fil.set_name("PNG Files")
        fil.add_pattern("*.png")
        fil.add_pattern("*.PNG")
        chooser.add_filter(fil)
        preview = gtk.Image()
        chooser.set_preview_widget(preview)
        chooser.connect("update-preview", self.update_preview, preview)
        try:
            if os.path.exists(key):
                chooser.set_filename(self.client.get_string(key))
            else:
                self.client.set_string(defs.BAR_PATTERN_URI, "~")
        except TypeError:
            raise "\nKey: "+key+" isn't set.\nRestarting AWN usually solves this issue\n"
        chooser.connect("selection-changed", self.chooser_changed, key)

    def chooser_changed(self, chooser, key):
        f = chooser.get_filename()
        if f == None:
            return
        self.client.set_string(key, f)

    def update_preview(self, chooser, preview):
        f = chooser.get_preview_filename()
        try:
            pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(f, 128, 128)
            preview.set_from_pixbuf(pixbuf)
            have_preview = True
        except:
            have_preview = False
        chooser.set_preview_widget_active(have_preview)

    def setup_bool(self, key, check):
        """sets up checkboxes"""
        check.set_active(self.client.get_bool(key))
        check.connect("toggled", self.bool_changed, key)

    def bool_changed(self, check, key):
        self.client.set_bool(key, check.get_active())
        if key == defs.AWN_KEEP_BELOW:
            if check.get_active():
                self.wTree.get_widget("autohide").set_active(True)
            else:
                self.wTree.get_widget("autohide").set_active(False)
        elif key == defs.AWN_AUTO_HIDE:
            if not check.get_active():
                if self.wTree.get_widget("keepbelow").get_active():
                    self.wTree.get_widget("keepbelow").set_active(False)
        print "toggled"

    def setup_font(self, key, font_btn):
        """sets up font chooser"""
        font_btn.set_font_name(self.client.get_string(key))
        font_btn.connect("font-set", self.font_changed, key)

    def font_changed(self, font_btn, key):
        self.client.set_string(key, font_btn.get_font_name())

    def setup_effect(self, key, dropdown):
        model = gtk.ListStore(str)
        model.append(["Classic"])
        model.append(["Fade"])
        model.append(["Spotlight"])
        model.append(["Zoom"])
        model.append(["Squish"])
        model.append(["3D turn"])
        dropdown.set_model(model)
        cell = gtk.CellRendererText()
        dropdown.pack_start(cell)
        dropdown.add_attribute(cell,'text',0)

        dropdown.set_active(self.client.get_int(key))
        dropdown.connect("changed", self.effect_changed, key)

    def effect_changed(self, dropdown, key):
        self.client.set_int(key, dropdown.get_active())

    def setup_look(self, dropdown):
        dropdown.append_text("Flat bar")
        dropdown.append_text("3D look")
        self.look_changed(dropdown)
        dropdown.connect("changed", self.look_changed)

    def look_changed(self, dropdown):
        if dropdown.get_active() == -1: #init
            if self.client.get_int(defs.BAR_ANGLE) == 0:
                dropdown.set_active(0)
                self.wTree.get_widget("roundedcornerscheck_holder").show_all()
                self.wTree.get_widget("barangle_holder").hide_all()
            else:
                dropdown.set_active(1)
                self.wTree.get_widget("roundedcornerscheck_holder").hide_all()
                self.wTree.get_widget("barangle_holder").show_all()
        elif dropdown.get_active() == 0:
            self.wTree.get_widget("barangle").set_value(0)
            self.wTree.get_widget ("roundedcornerscheck_holder").show_all()
            self.wTree.get_widget("barangle_holder").hide_all()
        else:
            self.wTree.get_widget("barangle").set_value(45)
            self.wTree.get_widget("roundedcornerscheck_holder").hide_all()
            self.wTree.get_widget("barangle_holder").show_all()
