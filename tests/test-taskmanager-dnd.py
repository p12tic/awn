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

import awn
import gtk

TARGET_TYPE_STRING = 0
TARGET_TYPE_TEXT_PLAIN = 1
TARGET_TYPE_URI_LIST = 2
TARGET_TYPE_SPECIAL = 3

drop_type_STRING = [("STRING", 0, TARGET_TYPE_STRING)]
drop_type_TEXT_PLAIN = [("text/plain", 0, TARGET_TYPE_TEXT_PLAIN)]
drop_type_URI_LIST = [("text/uri-list", 0, TARGET_TYPE_URI_LIST)]
drop_type_SPECIAL = [("5p3ci4l", 0, TARGET_TYPE_SPECIAL)]


def drag_data_get(widget, context, selection, targetType, eventTime):
    if targetType == TARGET_TYPE_STRING:
        selection.set(selection.target, 8, "string")
    elif targetType == TARGET_TYPE_TEXT_PLAIN:
        selection.set(selection.target, 8, "plain_text")
    elif targetType == TARGET_TYPE_URI_LIST:
        selection.set(selection.target, 8, "uri-list")
    elif targetType == TARGET_TYPE_SPECIAL:
        selection.set(selection.target, 8, "5p3ci4l")


win = gtk.Window()
win.connect('delete-event', lambda w, e: gtk.main_quit())
vbox = gtk.VBox()
win.add(vbox)

button = gtk.Button("DnD STRING")
button.connect("drag_data_get", drag_data_get)
button.drag_source_set(gtk.gdk.BUTTON1_MASK, drop_type_STRING,
                       gtk.gdk.ACTION_COPY)
vbox.add(button)


button = gtk.Button("DnD text/plain")
button.connect("drag_data_get", drag_data_get)
button.drag_source_set(gtk.gdk.BUTTON1_MASK, drop_type_TEXT_PLAIN,
                       gtk.gdk.ACTION_COPY)
vbox.add(button)

button = gtk.Button("DnD uri-list")
button.connect("drag_data_get", drag_data_get)
button.drag_source_set(gtk.gdk.BUTTON1_MASK, drop_type_URI_LIST,
                       gtk.gdk.ACTION_COPY)
vbox.add(button)

button = gtk.Button("DnD special")
button.connect("drag_data_get", drag_data_get)
button.drag_source_set(gtk.gdk.BUTTON1_MASK, drop_type_SPECIAL,
                       gtk.gdk.ACTION_COPY)
vbox.add(button)

win.show_all()

gtk.main()
