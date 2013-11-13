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

static void 
destroy( GtkWidget *widget )
{
    gtk_main_quit ();
}

static void
needs_attention (GtkToggleButton *button, GtkWindow *window)
{
  gtk_window_set_urgency_hint (window, gtk_toggle_button_get_active (button));
}

static void
skip_tasklist (GtkToggleButton *button, GtkWindow *window)
{
  gtk_window_set_skip_taskbar_hint (window, 
                                    gtk_toggle_button_get_active (button));
}

static void
change_icon (GtkToggleButton *button, GtkWindow *window)
{
  gboolean res;

  res = gtk_toggle_button_get_active (button);
  
  gtk_window_set_icon_name (window, res ? "gtk-cancel" : "gtk-apply");

}

static void
change_title (GtkToggleButton *button, GtkWindow *window)
{
  static gint i = 1;
  gchar *title;

  title = g_strdup_printf ("Window Title - Changed %d times", i);
  gtk_window_set_title (window, title);
  g_free (title);
  i++;
}

gint
main (gint argc, gchar **argv)
{
  GtkWidget   *window, *hbox, *button;
  
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "Window Title - Changed 0 times");
  gtk_window_set_icon_name (GTK_WINDOW (window), "gtk-apply");
  gtk_container_set_border_width (GTK_CONTAINER (window), 12);
  gtk_widget_show (window);
  
  hbox = gtk_vbox_new (FALSE, 8);
  gtk_container_add (GTK_CONTAINER (window), hbox);
  gtk_widget_show (hbox);

  button = gtk_toggle_button_new_with_label ("Needs-attention");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "toggled", G_CALLBACK (needs_attention), window);

  button = gtk_toggle_button_new_with_label ("Skip-tasklist");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "toggled", G_CALLBACK (skip_tasklist), window);

  button = gtk_toggle_button_new_with_label ("Change-icon");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "toggled", G_CALLBACK (change_icon), window);

  button = gtk_button_new_with_label ("Change-title");
  gtk_container_add (GTK_CONTAINER (hbox), button);
  g_signal_connect (button, "clicked", G_CALLBACK (change_title), window);
  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}
