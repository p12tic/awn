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

import awn
import gtk


def change_orient(widget, dialog):
    i = dialog.props.position
    i = (i + 1) % 4
    dialog.props.position = i


def change_attach(widget, dialog):
    dialog.props.anchored = not dialog.props.anchored

d = awn.Dialog()
d.set_title("Hello")
d.show_all()

win = gtk.Window()
win.connect('delete-event', lambda w, e: gtk.main_quit())
button = gtk.Label("One of the dialogs points to this window")
win.add(button)
win.show_all()

d2 = awn.Dialog(win)
d2.set_title("Another title")
d2.set_size_request(-1, 200)
button = gtk.Button("Change orient")
button.connect("clicked", change_orient, d2)
d2.add(button)
button = gtk.Button("Toggle attach")
button.connect("clicked", change_attach, d2)
d2.add(button)
d2.show_all()

gtk.main()
