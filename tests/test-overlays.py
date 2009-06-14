#!/usr/bin/python
#coding=utf-8
import pygtk
import gtk
import awn
import cairo
import rsvg

class Main:
 def __init__(self):
  # Create GUI objects
  rgbaColormap = gtk.gdk.screen_get_default().get_rgba_colormap()
  if rgbaColormap != None:
    gtk.gdk.screen_get_default().set_default_colormap(rgbaColormap)
  self.window = gtk.Window()
  self.window.connect("destroy", gtk.main_quit)
  #self.window.set_property("skip-taskbar-hint", True)
  #self.window.set_property("decorated", False)
  #self.window.set_property("resizable", False)
  
  self.vbox = gtk.VBox(False)
  self.effectsHBox = gtk.HBox()

  self.icon = awn.ThemedIcon()
  self.icon.set_info_simple("Applet", "123", "awn-manager")

  self.icon.connect("button-release-event", self.OnButton)
  self.icon.connect("enter-notify-event", self.OnMouseOver)
  self.icon.connect("leave-notify-event", self.OnMouseOut)
  self.effectsHBox.pack_start(self.icon, True, True)

  self.vbox.pack_start(self.effectsHBox)

  self.table = gtk.Table(2, 3)
  self.quitButton = gtk.Button(stock='gtk-quit')
  self.quitButton.connect("clicked", self.OnQuit)
  self.textOvrly = gtk.ToggleButton("Text overlay")
  self.textOvrly.connect("toggled", self.TextOverlay)
  self.throOvrly = gtk.ToggleButton("Throbber overlay")
  self.throOvrly.connect("toggled", self.ThrobberOverlay)
  self.thioOvrly = gtk.ToggleButton("ThemedIcon overlay")
  self.thioOvrly.connect("toggled", self.ThemedIconOverlay)

  self.table.attach(self.textOvrly, 0, 1, 0, 1)
  self.table.attach(self.throOvrly, 1, 2, 0, 1)
  self.table.attach(self.thioOvrly, 2, 3, 0, 1)

  self.table.attach(self.quitButton, 2, 3, 1, 2)

  self.vbox.pack_start(self.table, False)
  self.window.add(self.vbox)

  self.window.show_all()

 def TextOverlay(self, widget, *args, **kwargs):
  if widget.get_active():
    self.textOverlay = awn.OverlayText()
    self.textOverlay.props.text = "hello"
    self.icon.get_effects().add_overlay(self.textOverlay)
  elif self.textOverlay != None:
    self.icon.get_effects().remove_overlay(self.textOverlay)
    self.textOverlay = None

 def ThrobberOverlay(self, widget, *args, **kwargs):
  if widget.get_active():
    self.throOverlay = awn.OverlayThrobber(self.icon)
    self.icon.get_effects().add_overlay(self.throOverlay)
  elif self.throOverlay != None:
    self.icon.get_effects().remove_overlay(self.throOverlay)
    self.throOverlay = None

 def ThemedIconOverlay(self, widget, *args, **kwargs):
  if widget.get_active():
    self.thioOverlay = awn.OverlayThemedIcon(self.icon, "awn-manager", "something")
    self.icon.get_effects().add_overlay(self.thioOverlay)
  elif self.thioOverlay != None:
    self.icon.get_effects().remove_overlay(self.thioOverlay)
    self.thioOverlay = None

 def OnMouseOver(self, widget, *args, **kwargs):
  pass

 def OnMouseOut(self, widget, *args, **kwargs):
  pass

 def OnButton(self, widget, event):
  pass

 def OnQuit(self, widget):
  gtk.main_quit()

start=Main()
gtk.main()
