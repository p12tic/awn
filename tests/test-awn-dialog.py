#!/usr/bin/env python

import awn
import gtk

d = awn.Dialog()
d.set_title("Hello")
d.show_all()

win = gtk.Window()
button = gtk.Label("One of the dialogs points to this window")
win.add(button)
win.show_all()

d2 = awn.Dialog(win)
d2.set_title("Another title")
d2.show_all()

gtk.main()
