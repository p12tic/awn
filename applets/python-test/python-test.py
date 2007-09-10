#!/usr/bin/python
import sys, os
import gobject
import pygtk
import gtk
from gtk import gdk
import awn

class App (awn.AppletSimple):
  def __init__ (self, uid, orient, height):
    awn.AppletSimple.__init__ (self, uid, orient, height)
    self.height = height
    self.theme = gtk.IconTheme ()
    icon = self.theme.load_icon ("gtk-apply", height, 0)
    #icon = gdk.pixbuf_new_from_file ("/home/njp/Projects/test.png")
    self.set_temp_icon (icon)
    self.title = awn.awn_title_get_default ()
    self.dialog = awn.AppletDialog (self)
    button = gtk.Button (stock="gtk-apply")
    self.dialog.add (button)
    button.show_all ()
    self.connect ("button-press-event", self.button_press)
    self.connect ("enter-notify-event", self.enter_notify)
    self.connect ("leave-notify-event", self.leave_notify)
    self.dialog.connect ("focus-out-event", self.dialog_focus_out)

  def button_press (self, widget, event):
    self.dialog.show_all ()
    self.title.hide (self)
    print "show dialog"

  def dialog_focus_out (self, widget, event):
    self.dialog.hide ()
    print "hide dialog"

  def enter_notify (self, widget, event):
    self.title.show (self, "Test python applet")
    icon = self.theme.load_icon ("gtk-apply", self.height, 0)
    self.set_temp_icon (icon)
    print "show title"

  def leave_notify (self, widget, event):
    self.title.hide (self)
    icon = self.theme.load_icon ("gtk-cancel", self.height, 0)
    self.set_temp_icon (icon)
    print "hide title"

if __name__ == "__main__":
  awn.init (sys.argv[1:])
  #print "%s %d %d" % (awn.uid, awn.orient, awn.height)
  applet = App (awn.uid, awn.orient, awn.height)
  awn.init_applet (applet)
  applet.show_all ()
  gtk.main ()
