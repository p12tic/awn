#!/usr/bin/env python

#  Copyright © 2009 haytjes <hv1989@gmail.com>
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

count_windows = 1


def destroy_window(button, window):
    window.destroy()


def button_clicked(button, combobox):
    type_hint = {0: gtk.gdk.WINDOW_TYPE_HINT_NORMAL,
                 1: gtk.gdk.WINDOW_TYPE_HINT_DIALOG,
                 2: gtk.gdk.WINDOW_TYPE_HINT_UTILITY}[combobox.get_active()]

    create_window(type_hint)


def create_window(type_hint):
    global count_windows

    win = gtk.Window()
    win.set_type_hint(type_hint)
    win.set_title("Window "+str(count_windows))

    vbox = gtk.VBox()
    win.add(vbox)

    type_hint_txt = {0: "WINDOW_TYPE_HINT_NORMAL",
                     1: "WINDOW_TYPE_HINT_DIALOG",
                     2: "WINDOW_TYPE_HINT_UTILITY"}[combobox.get_active()]
    label = gtk.Label("type hint: "+type_hint_txt)
    vbox.add(label)


    button = gtk.Button("close")
    button.connect('clicked', lambda w: win.destroy())
    vbox.add(button)

    win.show_all()

    count_windows += 1

win = gtk.Window()
win.connect('destroy', lambda w: gtk.main_quit())

vbox = gtk.VBox()
win.add(vbox)

combobox = gtk.combo_box_new_text()
combobox.append_text('GDK_WINDOW_TYPE_HINT_NORMAL')
combobox.append_text('GDK_WINDOW_TYPE_HINT_DIALOG')
combobox.append_text('GDK_WINDOW_TYPE_HINT_UTILITY')
combobox.set_active(0)
vbox.add(combobox)

button = gtk.Button("Create window")
button.connect("clicked", button_clicked, combobox)
vbox.add(button)

win.show_all()

gtk.main()


#  WNCK_WINDOW_NORMAL,       /* document/app window */
#  WNCK_WINDOW_DIALOG,       /* dialog */
#  WNCK_WINDOW_UTILITY,      /* palette/toolbox window */
