/*
 * Copyright (c) 2007 Neil Jagdish Patel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This is a trash applet. the code has been adapted form the gnome-panel 
 * trash applet
 **/ 

#include "config.h"
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

#include <gtk/gtk.h>
#include <libawn/awn-applet.h>
#include <libawn/awn-applet-gconf.h>

#include "trashapplet.h"

#define PAGER_ROWS 2
#define BAR_HEIGHT 100
#define PADDING 2

static void
open_trash (GtkMenuItem *item, TrashApplet *applet)
{
	trash_applet_open_folder (applet);
}

static void
empty_trash (GtkMenuItem *item, TrashApplet *applet)
{
	trash_applet_do_empty (applet);
}

static void
show_help (GtkMenuItem *item, TrashApplet *applet)
{
	trash_applet_show_help (applet);
}

static void
show_about (GtkMenuItem *item, TrashApplet *applet)
{
	trash_applet_show_about (applet);
}

static gboolean
applet_button_release (GtkWidget      *widget,
			     GdkEventButton *event,
			     GtkMenu *menu)
{
	if (event->button == 3) {
                gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 3, event->time);
		return TRUE;
	}
	else
		return FALSE;
}

AwnApplet*
awn_applet_factory_initp ( gchar* uid, gint orient, gint height )
{
  AwnApplet *applet = awn_applet_new( uid, orient, height );
  GtkWidget     *trash;
  GtkWidget     *menu;
  GtkWidget     *item;
  
  gnome_vfs_init ();
  
  /* trash */
  trash = trash_applet_new (applet);

  /* menu */
  menu = awn_applet_create_default_menu (applet);

  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", 
                    G_CALLBACK (show_about), trash);


  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_HELP, NULL);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", 
                    G_CALLBACK (show_help), trash);


  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);

  item = gtk_menu_item_new_with_mnemonic ("_Empty the Wastebasket");
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", 
                    G_CALLBACK (empty_trash), trash);
  
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, NULL);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", 
                    G_CALLBACK (open_trash), trash);

  g_signal_connect (G_OBJECT (applet), "button-release-event", 
                    G_CALLBACK (applet_button_release), menu);
                         
  
  gtk_widget_set_size_request (GTK_WIDGET (applet),
                               awn_applet_get_height (applet), 
                               awn_applet_get_height (applet) * 2);
  
  gtk_container_add (GTK_CONTAINER (applet), trash);
  
  gtk_widget_show_all (GTK_WIDGET (menu));

  return applet;
}
  
