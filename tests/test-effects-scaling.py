#!/usr/bin/python
import pygtk
import gtk
import awn
import cairo
import rsvg

class EffectedDA(gtk.DrawingArea):
  def __init__(self):
    gtk.DrawingArea.__init__(self)
    self.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color(65535, 0, 32000))
    self.useSVG = True
    self.effects = awn.Effects(self)
    self.effects.props.effects = 0x44444
    self.add_events(gtk.gdk.ALL_EVENTS_MASK)
    self.connect("expose-event", self.expose)
    self.svg = rsvg.Handle("../awn-manager/awn-manager.svg")
  def toggle_svg_usage(self, widget, extra = None):
    self.useSVG = not self.useSVG
    self.queue_draw()
    if self.useSVG:
      widget.set_label("SVG")
    else:
      widget.set_label("PNG")
  def expose(self, widget, event):

    self.effects.draw_set_icon_size(48, 48, False)

    cr = self.effects.draw_cairo_create()

    if not self.useSVG:
      pixbuf = self.svg.get_pixbuf()
      pixbuf = pixbuf.scale_simple(48, 48, gtk.gdk.INTERP_BILINEAR)
      cr = gtk.gdk.CairoContext(cr)
      cr.set_source_pixbuf(pixbuf, 0, 0)
      cr.paint()
    else:
      svg_dimensions = self.svg.get_dimension_data()
      cr.scale(48 / float(svg_dimensions[0]), 48 / float(svg_dimensions[1]))
      self.svg.render_cairo(cr)

    self.effects.draw_cairo_destroy()
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
  #self.window.set_property("skip-taskbar-hint", True)
  #self.window.set_property("decorated", False)
  #self.window.set_property("resizable", False)
  
  self.vbox = gtk.VBox()
  self.effectsHBox = gtk.HBox()

  self.align1 = gtk.Alignment()
  self.eda = EffectedDA()
  ICON_SIZE = 48
  self.eda.set_size_request(ICON_SIZE*6/5, ICON_SIZE*2)
  self.align1.add(self.eda)
  self.align1.set(0.5, 0.5, 0.5, 0.75)
  self.eda.connect("button-release-event", self.OnButton)
  self.eda.connect("enter-notify-event", self.OnMouseOver)
  self.eda.connect("leave-notify-event", self.OnMouseOut)
  self.effectsHBox.add(self.align1)

  self.bb = gtk.VButtonBox()
  self.quitButton = gtk.Button(stock='gtk-quit')
  self.quitButton.connect("clicked", self.OnQuit)
  self.vbox.pack_start(self.effectsHBox)
  self.svgButton = gtk.Button("SVG")
  self.svgButton.connect("clicked", self.eda.toggle_svg_usage)
  self.bb.add(self.svgButton)
  self.bb.add(self.quitButton)
  self.vbox.pack_start(self.bb)
  self.window.add(self.vbox)

  self.window.show_all()

 def OnMouseOver(self, widget, *args, **kwargs):
  self.eda.effects.start(self.testedEffect)
 def OnMouseOut(self, widget, *args, **kwargs):
  self.eda.effects.stop(self.testedEffect)
 def OnButton(self, widget, event):
  self.eda.effects.start_ex("attention", max_loops=1)
 def OnQuit(self, widget):
  gtk.main_quit()

start=Main()
gtk.main()
