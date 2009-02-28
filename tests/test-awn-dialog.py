#!/usr/bin/env python

import awn
import gtk

def change_orient(widget, dialog):
  i = dialog.props.orient
  i = (i + 1) % 4
  dialog.props.orient = i

def change_attach(widget, dialog):
  dialog.props.anchored = not dialog.props.anchored

d = awn.Dialog()
d.set_title("Hello")
d.show_all()

win = gtk.Window()
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
