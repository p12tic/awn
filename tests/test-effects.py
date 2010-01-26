#!/usr/bin/python
# -*- coding: utf-8 -*-

#  Copyright © 2009 Michal Hruby <michal.mhr@gmail.com>
#  Copyright © 2009 Mark Lee <avant-wn@lazymalevolence.com>
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

import gtk
import awn
import rsvg


class EffectedDA(gtk.DrawingArea):

    def __init__(self):
        gtk.DrawingArea.__init__(self)
        self.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color(65535, 0, 32000))
        self.useSVG = True
        self.effects = awn.Effects(self)
        # two ways to do the same set_property(name, value) or
        # .props.name = value
        self.effects.set_property("effects", 0)
        self.effects.props.orientation = 0
        self.effects.props.no_clear = True
        self.add_events(gtk.gdk.ALL_EVENTS_MASK)
        self.connect("expose-event", self.expose)
        self.svg = rsvg.Handle("../awn-manager/awn-manager.svg")
        svg_pixbuf = self.svg.get_pixbuf()
        self.pixbuf = svg_pixbuf.scale_simple(48, 48, gtk.gdk.INTERP_BILINEAR)
        self.pixbuf_large = svg_pixbuf.scale_simple(64, 64,
                                                    gtk.gdk.INTERP_BILINEAR)

    def toggle_svg_usage(self, widget, extra = None):
        self.useSVG = not self.useSVG
        self.queue_draw()
        if self.useSVG:
            widget.set_label("SVG")
        else:
            widget.set_label(u"χρ") # Chi Rho == Cairo ;)

    def expose(self, widget, event):

        self.effects.set_icon_size(48, 48, False)

        if not self.useSVG:
            self.effects.set_icon_size(64, 48, False)
        cr = self.effects.cairo_create_clipped(event)

        if not self.useSVG:
            cr.rectangle(0, 0, 64, 48)
            cr.set_source_rgb(0, 0, 0.9)
            cr.fill_preserve()
            cr.set_source_rgb(0, 0, 0)
            cr.stroke()
            cr.move_to(25, 35)
            cr.show_text("123")
        else:
            cr = gtk.gdk.CairoContext(cr)
            cr.scale(0.75, 0.75)
            cr.set_source_pixbuf(self.pixbuf_large, 0, 0)
            cr.paint()

        self.effects.cairo_destroy()
        return True


class Main:

    def __init__(self):
        # Create GUI objects
        rgbaColormap = gtk.gdk.screen_get_default().get_rgba_colormap()
        if rgbaColormap != None:
            gtk.gdk.screen_get_default().set_default_colormap(rgbaColormap)
        self.testedEffect = "hover"
        self.window = gtk.Window()
        self.window.connect("destroy", gtk.main_quit)

        self.vbox = gtk.VBox(False)
        self.effectsHBox = gtk.HBox()

        self.align1 = gtk.Alignment()
        self.eda = EffectedDA()
        ICON_SIZE = 48
        self.eda.set_size_request(ICON_SIZE*5/4, ICON_SIZE*2)
        self.align1.add(self.eda)
        self.align1.set(0.5, 0.5, 0.95, 0.95)
        self.eda.connect("button-release-event", self.OnButton)
        self.eda.connect("enter-notify-event", self.OnMouseOver)
        self.eda.connect("leave-notify-event", self.OnMouseOut)
        self.effectsHBox.add(self.align1)

        self.vbox.pack_start(self.effectsHBox)

        self.table = gtk.Table(2, 3)
        self.quitButton = gtk.Button(stock='gtk-quit')
        self.quitButton.connect("clicked", self.OnQuit)
        self.svgButton = gtk.Button("SVG")
        self.svgButton.connect("clicked", self.eda.toggle_svg_usage)
        self.offsetButton = gtk.Button("Offset++")
        self.offsetButton.connect("clicked", self.ChangeOffsets)
        self.orientButton = gtk.Button("Orientation")
        self.orientButton.connect("clicked", self.ChangeOrient)
        self.effButton = gtk.Button("Effects")
        self.effButton.connect("clicked", self.ChangeEffects)
        self.clearButton = gtk.Button("No-clear")
        self.clearButton.connect("clicked", self.ChangeNoClear)
        self.indButton = gtk.Button("Indirect")
        self.indButton.connect("clicked", self.ChangeIndirect)
        self.table.attach(self.svgButton, 0, 1, 0, 1)
        self.table.attach(self.effButton, 1, 2, 0, 1)
        self.table.attach(self.offsetButton, 2, 3, 0, 1)
        self.table.attach(self.orientButton, 3, 4, 0, 1)
        self.table.attach(self.clearButton, 1, 2, 1, 2)
        self.table.attach(self.indButton, 2, 3, 1, 2)
        self.table.attach(self.quitButton, 3, 4, 1, 2)
        self.vbox.pack_start(self.table, False)
        self.window.add(self.vbox)

        self.window.show_all()

    def ChangeEffects(self, widget, *args, **kwargs):
        new_fx = self.eda.effects.props.effects+0x11111
        if new_fx > 0x88888:
            new_fx = 0
        self.eda.effects.props.effects = new_fx

    def ChangeOffsets(self, widget, *args, **kwargs):
        new_offset = self.eda.effects.get_property("icon-offset")+1
        self.eda.effects.set_property("icon-offset", new_offset)

    def ChangeOrient(self, widget, *args, **kwargs):
        new_offset = (self.eda.effects.get_property("orientation")+1) % 4
        self.eda.effects.set_property("orientation", new_offset)

    def ChangeNoClear(self, widget, *args, **kwargs):
        self.eda.effects.props.no_clear = not self.eda.effects.props.no_clear
        if self.eda.effects.props.no_clear:
            widget.set_label("No clear")
        else:
            widget.set_label("Clearing")

    def ChangeIndirect(self, widget, *args, **kwargs):
        fx = self.eda.effects
        fx.props.indirect_paint = not fx.props.indirect_paint
        if self.eda.effects.props.indirect_paint:
            widget.set_label("Indirect")
        else:
            widget.set_label("Direct")

    def OnMouseOver(self, widget, *args, **kwargs):
        self.eda.effects.start(self.testedEffect)

    def OnMouseOut(self, widget, *args, **kwargs):
        self.eda.effects.stop(self.testedEffect)

    def OnButton(self, widget, event):
        self.eda.effects.start_ex("attention", max_loops=1)

    def OnQuit(self, widget):
        gtk.main_quit()

start = Main()
gtk.main()
