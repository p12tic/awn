/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 */
#include <string.h>
#include <gtk/gtk.h>
#include <libawn/libawn.h>

#define PICTURE_FILE "../data/avant-window-navigator.png"

static AwnOrientation orient = AWN_ORIENTATION_BOTTOM;

static gboolean
on_click (GtkWidget *widget, GdkEventButton *event, AwnIconBox *box)
{
  switch (event->button) {
    // left click > test progress pie
    case 1: {
      AwnIcon *icon = AWN_ICON(widget);
      float progress = awn_icon_get_progress(icon);
      if (progress < 1.0) progress += 0.1; else progress = 0.0;
      if (progress > 1.0) progress = 1.0;
      awn_icon_set_progress(icon, progress);
      break;
    }
    // middle click > destroy AwnIcon
    case 2:
      gtk_container_remove(GTK_CONTAINER(box), widget);
      break;
    // right click > change orientation
    case 3:
      orient++;
      if (orient > AWN_ORIENTATION_LEFT) orient = AWN_ORIENTATION_TOP;
      awn_icon_box_set_orientation (box, orient);
      break;
  }
  return TRUE;
}

static GtkWidget *
pixbuf_icon (GtkWidget *parent)
{
  GdkPixbuf *pixbuf;
  GtkWidget *icon;

  pixbuf = gdk_pixbuf_new_from_file_at_size (PICTURE_FILE, 50, 50, NULL);

  icon = awn_icon_new ();
  awn_icon_set_from_pixbuf (AWN_ICON (icon), pixbuf);
  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Pixbuf Icon");
  gtk_container_add (GTK_CONTAINER (parent), icon);
  gtk_widget_show (icon);

  g_signal_connect (icon, "button-release-event", 
                    G_CALLBACK (on_click), parent); 
  g_object_unref (pixbuf);
  return icon;
}

/*
 * Make a image surface cairo drawing to test out the conversions
 */
static GtkWidget *
cairo_icon (GtkWidget *parent)
{
  GtkWidget *icon;
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 50, 50);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);
  cairo_rectangle (cr, 0, 0, 50, 50);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);
  cairo_stroke (cr);

  icon = awn_icon_new ();
  awn_icon_set_from_context (AWN_ICON (icon), cr);
  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Cairo Icon");
  
  gtk_container_add (GTK_CONTAINER (parent), icon);
  gtk_widget_show (icon);

  g_signal_connect (icon, "button-release-event", 
                    G_CALLBACK (on_click), parent);

  cairo_destroy (cr);

  return icon;
}

gint
main (gint argc, gchar **argv)
{
  GtkWidget   *window, *box;
  GdkScreen   *screen;
  GdkColormap *map;
  
  gtk_init (&argc, &argv);

  screen = gdk_screen_get_default ();
  map = gdk_screen_get_rgba_colormap (screen);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  if (map)
    gtk_widget_set_colormap (window, map);
  gtk_window_resize (GTK_WINDOW (window), 50, 100);
  gtk_widget_show (window);
  
  box = awn_icon_box_new ();
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_widget_show (box);

  pixbuf_icon (box);
  cairo_icon (box);
  pixbuf_icon (box);
  cairo_icon (box);

  gtk_main ();
  return 0;
}
