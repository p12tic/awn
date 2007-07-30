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

/* This is one of the most basic applets you can make, it just embeds the 
   WnckPager into the applet container*/ 

#include "config.h"
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

#include <gtk/gtk.h>
#include <libawn/awn-applet.h>
#include <libawn/awn-applet-gconf.h>
#include <libgnome/libgnome.h>

#define PAGER_ROWS 2
#define BAR_HEIGHT 100
#define PADDING 4

typedef struct {

  AwnApplet *applet;
  GtkWidget *pager;
  GtkWidget *align;
  gint       n_rows;
  gint       width;
  GtkWidget *menu;
} Switcher;

static void
on_rows_changed (GtkSpinButton *button, Switcher *app)
{
  gint rows = gtk_spin_button_get_value (button);

  awn_applet_gconf_set_int (app->applet, "n_rows", rows, NULL);

  wnck_pager_set_n_rows (WNCK_PAGER (app->pager), rows);
}

static void
on_width_changed (GtkSpinButton *button, Switcher *app)
{
  gint width = gtk_spin_button_get_value (button);

  awn_applet_gconf_set_int (app->applet, "width", width, NULL);

  gtk_widget_set_size_request (GTK_WIDGET (app->pager), width, -1);
}

static void
on_height_changed (GtkSpinButton *button, Switcher *app)
{
  gint height = gtk_spin_button_get_value (button);
  gint padding;

  awn_applet_gconf_set_int (app->applet, "height", height, NULL);

  gtk_widget_set_size_request (GTK_WIDGET (app->applet), -1, height);

  padding = awn_applet_get_height (app->applet);
  padding -= height;
  padding /= 2;

  gtk_alignment_set_padding (GTK_ALIGNMENT (app->align), 
                             awn_applet_get_height (app->applet) +padding, 
                             padding, 0, 0);
                                
}

static void
on_close_clicked (GtkButton *button, GtkWidget *window)
{
  gtk_widget_destroy (window);
}

static void
show_prefs (GtkMenuItem *item, Switcher *app)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *mbox;
  GtkWidget *label;
  GtkWidget *butbox;
  GtkWidget *button;
  GtkWidget *hbox;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  gtk_window_set_title (GTK_WINDOW (window), 
                        "Workspace/Viewport Switcher Preferences");

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  mbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), mbox, TRUE, TRUE, 0);

  /* Number of rows */
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (mbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Rows:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  button = gtk_spin_button_new_with_range (1, 4, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button),
                             awn_applet_gconf_get_int (app->applet, "n_rows", NULL));
  g_signal_connect (G_OBJECT (button), "value-changed", 
                    G_CALLBACK (on_rows_changed), app);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (mbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Width:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  button = gtk_spin_button_new_with_range (1, 400, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button),
                             awn_applet_gconf_get_int (app->applet, "width", NULL));
  g_signal_connect (G_OBJECT (button), "value-changed", 
                    G_CALLBACK (on_width_changed), app);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (mbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Height:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  button = gtk_spin_button_new_with_range (1, 400, 1.0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button),
                             awn_applet_gconf_get_int (app->applet, "height", NULL));
  g_signal_connect (G_OBJECT (button), "value-changed", 
                    G_CALLBACK (on_height_changed), app);

  butbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (butbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), butbox, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  g_signal_connect (G_OBJECT (button), "clicked", 
                    G_CALLBACK (on_close_clicked), window);
  gtk_box_pack_start (GTK_BOX (butbox), button, FALSE, TRUE, 0);

  gtk_widget_show_all (window);
}

static gboolean
on_button_press_event (GtkWidget *applet, GdkEventButton *event, Switcher *app)
{
  if (event->button == 3)
  {
    gtk_menu_popup (GTK_MENU (app->menu), NULL, NULL, 
                    NULL, NULL, 3, event->time);
  }
  return FALSE;
}

static void
on_height_change (AwnApplet *applet, gint height, Switcher *app)
{
  gtk_widget_set_size_request (GTK_WIDGET (applet), -1, height);
}

AwnApplet*
awn_applet_factory_initp ( gchar* uid, gint orient, gint height )
{
  AwnApplet *applet = awn_applet_new( uid, orient, height );
  Switcher      *app = g_new0 (Switcher, 1);
  GtkWidget     *align;
  GtkWidget     *pager;
  GtkWidget     *menu, *item;

  app->applet = applet;
  g_signal_connect (G_OBJECT (applet), "button-press-event",
                    G_CALLBACK (on_button_press_event), app);

  awn_applet_add_preferences (applet, "/schemas/apps/awn-switcher/prefs", NULL);

  app->n_rows = awn_applet_gconf_get_int (applet, "n_rows", NULL);
  app->width = awn_applet_gconf_get_int (applet, "width", NULL);
  
  /* Set up menus */
  menu = awn_applet_create_default_menu (applet);
  app->menu = menu;
  
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
  gtk_widget_show_all (item);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_prefs), app);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), item);


  /* pager */
  pager = wnck_pager_new (wnck_screen_get_default ());
  app->pager = pager;
  wnck_pager_set_orientation (WNCK_PAGER (pager), GTK_ORIENTATION_HORIZONTAL);
  wnck_pager_set_n_rows (WNCK_PAGER (pager), app->n_rows);
  wnck_pager_set_display_mode (WNCK_PAGER (pager), WNCK_PAGER_DISPLAY_CONTENT);
  wnck_pager_set_show_all (WNCK_PAGER (pager), TRUE);
  wnck_pager_set_shadow_type (WNCK_PAGER (pager), GTK_SHADOW_NONE);
  
  gtk_widget_set_size_request (pager, app->width, height * 2);
  align = gtk_alignment_new (0, 0.5, 0, 0);
  app->align = align;
  gint padding = height;
  padding -= awn_applet_gconf_get_int (applet, "height", NULL);
  padding /= 2;
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), height+padding, 
                             padding, 0, 0); 
  
  gtk_container_add (GTK_CONTAINER (align), pager);
  gtk_container_add (GTK_CONTAINER (applet), align);
  
  gtk_widget_show_all (GTK_WIDGET (applet));
  
  g_signal_connect (G_OBJECT (applet), "height-changed",
                    G_CALLBACK (on_height_change), app);
  
  return applet;
}

