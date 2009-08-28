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

static GtkPositionType position = GTK_POS_BOTTOM;

#define NUM_ICONS 4
static AwnIcon *icons[NUM_ICONS];

const char* effect_names[] = {
  "AWN_EFFECT_NONE",
  "AWN_EFFECT_OPENING",
  "AWN_EFFECT_CLOSING",
  "AWN_EFFECT_HOVER",
  "AWN_EFFECT_LAUNCHING",
  "AWN_EFFECT_ATTENTION",
  "AWN_EFFECT_DESATURATE"};

static gboolean
on_active_click(GtkWidget *widget, gpointer user_data)
{
  int i;
  for (i=0; i<NUM_ICONS; i++) {
    awn_icon_set_is_active(icons[i], !awn_icon_get_is_active(icons[i]));
  }
  return FALSE;
}

static gboolean
on_running_click(GtkWidget *widget, gpointer user_data)
{
  int i;
  for (i=0; i<NUM_ICONS; i++) {
    awn_icon_set_indicator_count(icons[i], awn_icon_get_indicator_count(icons[i])+1);
    // reset
    if (awn_icon_get_indicator_count(icons[i]) > 3)
      awn_icon_set_indicator_count (icons[i], 0);
  }
  return FALSE;
}

static void
anim_started(AwnEffects *fx, AwnEffect effect, gpointer data)
{
  g_debug("Got animation-start signal (animation=%s).", effect_names[effect]);
}

static void
anim_ended(AwnEffects *fx, AwnEffect effect, gpointer data)
{
  g_debug("Got animation-end signal (animation=%s).", effect_names[effect]);
}

static gboolean
on_signal_click(GtkWidget *widget, gpointer user_data)
{
  AwnIcon *icon = AWN_ICON(icons[0]);

  AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (icon));
  awn_effects_start_ex(fx, AWN_EFFECT_OPENING, 1, TRUE, TRUE);

  g_object_set(fx, "arrow-png", "/usr/share/gimp/2.0/themes/Default/images/preferences/folders-gradients-22.png", NULL);

  return FALSE;
}


static gboolean
on_click (GtkWidget *widget, GdkEventButton *event, AwnIconBox *box)
{
  AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (widget));
  switch (event->button) {
    /* left click > test progress pie */
    case 1: {
      float progress;
      g_object_get (fx, "progress", &progress, NULL);
      if (progress < 1.0) progress += 0.1; else progress = 0.0;
      if (progress > 1.0) progress = 1.0;
      g_object_set (fx, "progress", progress, NULL);
      break;
    }
    /* middle click > destroy AwnIcon */
    case 2:
      gtk_container_remove(GTK_CONTAINER(box), widget);
      break;
    /* right click > change position */
    case 3:
      position++;
      if (position > GTK_POS_BOTTOM) position = GTK_POS_LEFT;
      awn_icon_box_set_pos_type (box, position);
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
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Pixbuf Icon");

  /* test the signals */
  AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (icon));
  g_signal_connect(fx, "animation-start", (GCallback)anim_started, NULL);
  g_signal_connect(fx, "animation-end", (GCallback)anim_ended, NULL);

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

  cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.8);
  cairo_rectangle (cr, 0, 0, 50, 50);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  icon = awn_icon_new ();
  awn_icon_set_from_context (AWN_ICON (icon), cr);
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Cairo Icon");
  
  awn_icon_set_offset (AWN_ICON (icon), 8);
  
  gtk_container_add (GTK_CONTAINER (parent), icon);
  gtk_widget_show (icon);

  g_signal_connect (icon, "button-release-event", 
                    G_CALLBACK (on_click), parent);

  cairo_destroy (cr);

  return icon;
}

static GtkWidget *
surface_icon (GtkWidget *parent)
{
  GtkWidget *icon;
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 75, 50);

  cr = cairo_create (surface);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 0.8);
  cairo_rectangle (cr, 0, 0, 75, 50);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  cairo_destroy (cr);

  icon = awn_icon_new ();
  awn_icon_set_from_surface (AWN_ICON (icon), surface);
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Surface Icon");
  
  gtk_container_add (GTK_CONTAINER (parent), icon);
  gtk_widget_show (icon);

  g_signal_connect (icon, "button-release-event", 
                    G_CALLBACK (on_click), parent);

  cairo_surface_destroy (surface);

  return icon;
}

static GtkWidget *
thin_surface_icon (GtkWidget *parent)
{
  GtkWidget *icon;
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 25, 75);

  cr = cairo_create (surface);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.8);
  cairo_rectangle (cr, 0, 0, 25, 75);
  cairo_fill_preserve (cr);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);

  cairo_destroy (cr);

  icon = awn_icon_new ();
  awn_icon_set_from_surface (AWN_ICON (icon), surface);
  awn_icon_set_tooltip_text (AWN_ICON (icon), "Thin Surface Icon");
  
  gtk_container_add (GTK_CONTAINER (parent), icon);
  gtk_widget_show (icon);

  g_signal_connect (icon, "button-release-event", 
                    G_CALLBACK (on_click), parent);

  cairo_surface_destroy (surface);

  return icon;
}
gint
main (gint argc, gchar **argv)
{
  GtkWidget   *window, *icon_box, *vbox;
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

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add( GTK_CONTAINER(window), vbox);

  icon_box = awn_icon_box_new ();
  gtk_container_add (GTK_CONTAINER (vbox), icon_box);

  icons[0] = AWN_ICON(pixbuf_icon (icon_box));
  icons[1] = AWN_ICON(cairo_icon (icon_box));
  icons[2] = AWN_ICON(surface_icon (icon_box));
  icons[3] = AWN_ICON(thin_surface_icon (icon_box));

  GtkWidget *button = gtk_button_new_with_label("Active");
  g_signal_connect(G_OBJECT(button), "clicked", 
                   G_CALLBACK(on_active_click), NULL);
  gtk_container_add (GTK_CONTAINER(vbox), button);

  GtkWidget *button2 = gtk_button_new_with_label("Running");
  g_signal_connect(G_OBJECT(button2), "clicked", 
                   G_CALLBACK(on_running_click), NULL);
  gtk_container_add (GTK_CONTAINER(vbox), button2);

  GtkWidget *button3 = gtk_button_new_with_label("Signal");
  g_signal_connect(G_OBJECT(button3), "clicked", 
                   G_CALLBACK(on_signal_click), NULL);
  gtk_container_add (GTK_CONTAINER(vbox), button3);

  gtk_widget_show_all (window);

  gtk_main ();
  return 0;
}
