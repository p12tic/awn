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
#include <libawn/awn-applet-simple.h>

gint
main (gint argc, gchar **argv)
{
  GtkWidget   *window, *socket, *icon;
  GdkScreen   *screen;
  GdkColormap *map;
  
  gtk_init (&argc, &argv);

  screen = gdk_screen_get_default ();
  map = gdk_screen_get_rgba_colormap (screen);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (map)
    gtk_widget_set_colormap (window, map);
  gtk_window_resize (GTK_WINDOW (window), 50, 100);
  gtk_widget_show (window);
  
  socket = gtk_socket_new ();
  gtk_container_add (GTK_CONTAINER (window), socket);
  gtk_widget_show (socket);

  icon = awn_applet_simple_new ("test-applet", "1234567890", 0);
  awn_applet_simple_set_icon_name (AWN_APPLET_SIMPLE (icon), GTK_STOCK_APPLY);
  gtk_plug_construct (GTK_PLUG (icon), gtk_socket_get_id (GTK_SOCKET (socket)));

  gtk_main ();
  return 0;
}
