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
#include <libawn/awn-themed-icon.h>

gint
main (gint argc, gchar **argv)
{
  GtkWidget   *window, *hbox, *icon;
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
  
  hbox = gtk_hbox_new (TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), hbox);
  gtk_widget_show (hbox);

  icon = awn_themed_icon_new ();
  awn_themed_icon_set_info_simple (AWN_THEMED_ICON (icon),
                                   "test-applet", "test-uid", 
                                   GTK_STOCK_DIALOG_INFO);
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_widget_show (icon);

  icon = awn_themed_icon_new ();
  awn_themed_icon_set_info_simple (AWN_THEMED_ICON (icon),
                                   "test-applet", "test-uid",
                                   "../data/avant-window-navigator.svg");
  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
  gtk_widget_show (icon);

  gtk_main ();
  return 0;
}
