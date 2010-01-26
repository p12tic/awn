#!/usr/bin/env python

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
import gobject


def show_hide_tooltip(icon):
    tooltip = icon.get_tooltip()
    if (tooltip.flags() & gtk.VISIBLE) == 0:
        tooltip.update_position()
        tooltip.show()
        icon.set_state("icon1")
    else:
        tooltip.hide()
        icon.set_state("icon2")

    return True # timer repeats

win = gtk.Window()
box = gtk.HBox()
icon = awn.ThemedIcon()
icon.set_info_simple("media-player", "12345", "media-player")
icon.set_tooltip_text("Icon with smart-behavior & toggle-on-click")

icon2 = awn.ThemedIcon()
icon2.set_info("media-player", "1234", ["icon1", "icon2"],
               ["gnome-main-menu", "media-player"])
icon2.set_state("icon2")
icon2.set_tooltip_text("Icon with manual tooltip control")
icon2.get_tooltip().set_position_hint("top", 35)
icon2.get_tooltip().props.smart_behavior = False
icon2.get_tooltip().props.toggle_on_click = False
gobject.timeout_add(1500, show_hide_tooltip, icon2)

icon3 = awn.ThemedIcon()
icon3.set_info_simple("media-player", "123456", "media-player")
icon3.set_tooltip_text("Tooltip toggled only with mouse clicks")
icon3.get_tooltip().props.smart_behavior = False
icon3.get_tooltip().show()
# we'll leave the toggle-on-click property to True

box.add(icon)
box.add(icon2)
box.add(icon3)

win.add(box)
win.show_all()

gtk.main()
