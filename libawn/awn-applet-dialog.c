/*
 * Copyright (c) 2007 Anthony Arobone <aarobone@gmail.com>
 *                    Neil Jagdish Patel <njpatel@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "awn-applet.h"
#include "awn-applet-dialog.h"
#include "awn-cairo-utils.h"
#include "awn-config-client.h"
#include "awn-defines.h"

G_DEFINE_TYPE(AwnAppletDialog, awn_applet_dialog, GTK_TYPE_WINDOW)

#define AWN_APPLET_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_APPLET_DIALOG, \
    AwnAppletDialogPrivate))

struct _AwnAppletDialogPrivate
{
  AwnApplet *applet;
  GtkWidget *title;
  GtkWidget *title_label;
  GtkWidget *vbox;
  GtkWidget *align;

  gint offset;
};

/* PRIVATE CLASS METHODS */
static void awn_applet_dialog_class_init(AwnAppletDialogClass *klass);
static void awn_applet_dialog_init(AwnAppletDialog *dialog);
/*static void awn_applet_dialog_finalize(GObject *obj); */

static void
_on_alpha_screen_changed(GtkWidget* pWidget,
                         GdkScreen* pOldScreen,
                         GtkWidget* pLabel)
{
  GdkScreen* pScreen = gtk_widget_get_screen(pWidget);
  GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);

  if (!pColormap)
    pColormap = gdk_screen_get_rgb_colormap(pScreen);

  gtk_widget_set_colormap(pWidget, pColormap);
}


/**
 * awn_applet_dialog_position_reset:
 * @dialog: The dialog to reposition.
 *
 * Resets the position of the dialog so it is centered over its associated
 * applet window.  Should "reposition" dialog-arrow if the dialog does not fit
 * (fall-off-screen) on the desired place.
 */
void
awn_applet_dialog_position_reset(AwnAppletDialog *dialog)
{
  gint ax, ay, aw, ah;
  gint x, y, w, h;

  g_return_if_fail(AWN_IS_APPLET_DIALOG(dialog));

  gdk_window_get_origin(GTK_WIDGET(dialog->priv->applet)->window,
                        &ax, &ay);
  gtk_widget_get_size_request(GTK_WIDGET(dialog->priv->applet),
                              &aw, &ah);
  gtk_window_get_size(GTK_WINDOW(dialog), &w, &h);

  x = ax - w / 2 + aw / 2;
  y = ay - h + dialog->priv->offset;

  if (x < 0)
    x = 2;

  if ((x + w) > gdk_screen_get_width(gdk_screen_get_default()))
    x = gdk_screen_get_width(gdk_screen_get_default()) - w - 20;

  gtk_window_move(GTK_WINDOW(dialog), x, y);

}

static gboolean
_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
#define BOR 4
  AwnAppletDialog *dialog;
  cairo_t *cr = NULL;
  GtkWidget *child = NULL;
  gint width, height;
  gint gap = 20;
  gint x;
  GtkStyle *style;
  GdkColor bg;
  gfloat alpha;
  GdkColor border;

  dialog = AWN_APPLET_DIALOG(widget);

  cr = gdk_cairo_create(widget->window);

  if (!cr)
    return FALSE;

  gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

  gtk_widget_style_get(widget, "bg_alpha", &alpha, NULL);

  style = gtk_widget_get_style(widget);

  bg = style->bg[GTK_STATE_NORMAL];

  border = style->bg[GTK_STATE_SELECTED];

  // Clear the background to transparent
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.0f);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);

  cairo_paint(cr);

  // draw everything else over transparent background
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width(cr, 3.0);

  // background shading
  cairo_set_source_rgba(cr, bg.red / 65535.0,
                        bg.green / 65535.0,
                        bg.blue / 65535.0,
                        alpha);

  awn_cairo_rounded_rect(cr, BOR, BOR, width - (BOR*2),
                         height - (BOR*2) - gap,
                         15, ROUND_ALL);

  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, border.red / 65535.0,
                        border.green / 65535.0,
                        border.blue / 65535.0,
                        alpha);

  cairo_stroke(cr);

  // do some maths
  x = width / 2;

  // draw arrow
  cairo_set_source_rgba(cr, bg.red / 65535.0,
                        bg.green / 65535.0,
                        bg.blue / 65535.0,
                        alpha);

  cairo_move_to(cr, x - 15, height - gap - BOR);

  cairo_line_to(cr, x, height);

  cairo_line_to(cr, x + 15, height - gap - BOR);

  //cairo_line_to (cr, x-15, height - gap - BOR);
  cairo_close_path(cr);

  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, border.red / 65535.0,
                        border.green / 65535.0,
                        border.blue / 65535.0,
                        alpha);

  cairo_stroke(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

  cairo_set_source_rgba(cr, bg.red / 65535.0,
                        bg.green / 65535.0,
                        bg.blue / 65535.0,
                        alpha);

  cairo_move_to(cr, x - 14, height - gap - (BOR*2));

  cairo_line_to(cr, x, height - (BOR*2));

  cairo_line_to(cr, x + 14, height - gap - (BOR*2));

  cairo_close_path(cr);

  cairo_fill_preserve(cr);

  cairo_stroke(cr);

  // Clean up
  cairo_destroy(cr);

  awn_applet_dialog_position_reset(AWN_APPLET_DIALOG(widget));

  /* Propagate the signal */
  child = gtk_bin_get_child(GTK_BIN(widget));

  if (child)
    gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                   child, expose);

  return FALSE;
}

static gboolean
on_title_expose(GtkWidget       *widget,
                GdkEventExpose  *expose,
                AwnAppletDialog *dialog)
{
  cairo_t *cr = NULL;
  cairo_pattern_t *pat = NULL;
  GtkWidget *child = NULL;
  gint width, height;
  GtkStyle *style;
  GdkColor bg;
  gfloat alpha;
  GdkColor border;

  cr = gdk_cairo_create(widget->window);

  if (!cr)
    return FALSE;

  width = widget->allocation.width;

  height = widget->allocation.height;

  gtk_widget_style_get(GTK_WIDGET(dialog), "bg_alpha", &alpha, NULL);

  style = gtk_widget_get_style(GTK_WIDGET(dialog));

  bg = style->bg[GTK_STATE_NORMAL];

  border = style->bg[GTK_STATE_SELECTED];

  // Clear the background to transparent
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.0f);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);

  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width(cr, 1.0);

  /* Paint the dialog background */
  cairo_set_source_rgba(cr, bg.red / 65535.0,
                        bg.green / 65535.0,
                        bg.blue / 65535.0,
                        alpha);

  cairo_rectangle(cr, 0, 0, width, height);

  cairo_fill(cr);

  /* Paint the background the border colour */
  cairo_set_source_rgba(cr, border.red / 65535.0,
                        border.green / 65535.0,
                        border.blue / 65535.0,
                        alpha);

  awn_cairo_rounded_rect(cr, 0, 0, width, height, 15, ROUND_ALL);

  cairo_fill(cr);

  /* Make the background appear 'rounded' */
  pat = cairo_pattern_create_linear(0, 0, 0, height);

  cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 0.0);

  cairo_pattern_add_color_stop_rgba(pat, 1, 0, 0, 0, 0.3);

  awn_cairo_rounded_rect(cr, 0, 0, width, height, 15, ROUND_ALL);

  cairo_set_source(cr, pat);

  cairo_fill_preserve(cr);

  cairo_pattern_destroy(pat);

  cairo_set_source_rgba(cr, 0, 0, 0, 0.2);

  cairo_stroke(cr);

  /* Highlight */
  pat = cairo_pattern_create_linear(0, 0, 0, (height / 5) * 2);

  cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 0.3);

  cairo_pattern_add_color_stop_rgba(pat, 1, 1, 1, 1, 0.1);

  awn_cairo_rounded_rect(cr, 1, 1, width - 2, (height / 5)*2,
                         15, ROUND_TOP);

  cairo_set_source(cr, pat);

  cairo_fill(cr);

  cairo_pattern_destroy(pat);

  // Clean up
  cairo_destroy(cr);

  /* Propagate the signal */
  child = gtk_bin_get_child(GTK_BIN(widget));

  if (child)
    gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                   child, expose);

  return TRUE;
}

static void
_on_size_request(GtkWidget *widget, GtkRequisition *req, gpointer *data)
{
  awn_applet_dialog_position_reset(AWN_APPLET_DIALOG(widget));
  gtk_widget_queue_draw(widget);
}



static gboolean
_on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer *data)
{
  if (event->keyval == GDK_Escape && AWN_IS_APPLET_DIALOG(widget))
  {
    gtk_widget_hide(GTK_WIDGET(widget));
  }

  return FALSE;
}

static gboolean
_on_delete_event(GtkWidget *widget, GdkEventKey *event, gpointer *data)
{
  gtk_widget_hide(GTK_WIDGET(widget));
  return TRUE;
}

static void
_on_notify(GObject *dialog, GParamSpec *spec, gpointer null)
{
  AwnAppletDialogPrivate *priv;
  const gchar *title;

  priv = AWN_APPLET_DIALOG(dialog)->priv;

  if (strcmp("title", g_param_spec_get_name(spec)) != 0)
    return;

  title = gtk_window_get_title(GTK_WINDOW(dialog));

  if (title)
  {
    gchar *markup = g_strdup_printf(
                      "<span size='x-large' weight='bold'>%s</span>", title);
    gtk_label_set_markup(GTK_LABEL(priv->title_label), markup);
    g_free(markup);
    gtk_widget_show(priv->title_label);
    gtk_widget_show(priv->title);
  }
  else
  {
    gtk_widget_hide(priv->title);
  }
}

static void
on_realize(GtkWidget *dialog, gpointer null)
{
  _on_size_request(dialog, NULL, NULL);
}

static void
awn_applet_dialog_add(GtkContainer *dialog, GtkWidget *widget)
{
  AwnAppletDialogPrivate *priv;

  g_return_if_fail(AWN_IS_APPLET_DIALOG(dialog));
  g_return_if_fail(GTK_IS_WIDGET(widget));
  priv = AWN_APPLET_DIALOG(dialog)->priv;

  gtk_box_pack_start(GTK_BOX(priv->vbox), widget, TRUE, TRUE, 0);
}

static void
awn_applet_dialog_remove(GtkContainer *dialog, GtkWidget *widget)
{
  AwnAppletDialogPrivate *priv;

  g_return_if_fail(AWN_IS_APPLET_DIALOG(dialog));
  g_return_if_fail(GTK_IS_WIDGET(widget));
  priv = AWN_APPLET_DIALOG(dialog)->priv;
  gtk_container_remove(GTK_CONTAINER(priv->vbox), widget);
}

/*
 * class init
 */
static void
awn_applet_dialog_class_init(AwnAppletDialogClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkContainerClass *cont_class;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = _expose_event;

  cont_class = GTK_CONTAINER_CLASS(klass);
  cont_class->add = awn_applet_dialog_add;
  cont_class->remove = awn_applet_dialog_remove;

  gtk_widget_class_install_style_property(widget_class,
                                          g_param_spec_float(
                                            "bg_alpha",
                                            "Alpha Value",
                                            "The alpha value of the window",
                                            0.0, 1.0, 0.9,
                                            G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_type_class_add_private(G_OBJECT_CLASS(klass),
                           sizeof(AwnAppletDialogPrivate));
}


/*
 *  init
 */
static void
awn_applet_dialog_init(AwnAppletDialog *dialog)
{
  AwnAppletDialogPrivate *priv;
  AwnConfigClient *client;

  priv = dialog->priv = AWN_APPLET_DIALOG_GET_PRIVATE(dialog);

  gtk_window_stick(GTK_WINDOW(dialog));

  _on_alpha_screen_changed(GTK_WIDGET(dialog), NULL, NULL);
  gtk_widget_set_app_paintable(GTK_WIDGET(dialog), TRUE);

  /* applet events */
  gtk_widget_add_events(GTK_WIDGET(dialog), GDK_ALL_EVENTS_MASK);
  g_signal_connect(G_OBJECT(dialog), "key-press-event",
                   G_CALLBACK(_on_key_press_event), NULL);
  g_signal_connect(G_OBJECT(dialog), "size-request",
                   G_CALLBACK(_on_size_request), NULL);
  g_signal_connect(G_OBJECT(dialog), "delete-event", 
                   G_CALLBACK(_on_delete_event), NULL);

  /* See if the title has been set */
  g_signal_connect(dialog, "notify",
                   G_CALLBACK(_on_notify), NULL);

  g_object_notify(G_OBJECT(dialog), "title");

  /* Position on realize */
  g_signal_connect(dialog, "realize",
                   G_CALLBACK(on_realize), NULL);

  priv->align = gtk_alignment_new(0.5, 0.5, 1, 1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(priv->align),
                            10, 30, 10, 10);

  GTK_CONTAINER_CLASS(awn_applet_dialog_parent_class)->add
  (GTK_CONTAINER(dialog), priv->align);

  priv->vbox = gtk_vbox_new(FALSE, 6);
  gtk_container_add(GTK_CONTAINER(priv->align), priv->vbox);

  priv->title = gtk_event_box_new();
  gtk_widget_set_no_show_all(priv->title, TRUE);
  gtk_box_pack_start(GTK_BOX(priv->vbox), priv->title, TRUE, TRUE, 0);
  g_signal_connect(priv->title, "expose-event",
                   G_CALLBACK(on_title_expose), dialog);

  priv->title_label = gtk_label_new("");
  gtk_widget_set_state(priv->title_label, GTK_STATE_SELECTED);
  gtk_misc_set_alignment(GTK_MISC(priv->title_label), 0.5, 0.5);
  gtk_misc_set_padding(GTK_MISC(priv->title_label), 0, 4);
  gtk_container_add(GTK_CONTAINER(priv->title), priv->title_label);

  client = awn_config_client_new();
  priv->offset = awn_config_client_get_int(client, "bar", "icon_offset", NULL);
}

/**
 * awn_applet_dialog_new:
 * @applet: The applet to which to associate the dialog
 *
 * Creates a new toplevel window that is "attached" to the @applet.
 * Returns: a new dialog.  Caller is responsible for freeing the memory when the
 * dialog is no longer being used.
 */
GtkWidget*
awn_applet_dialog_new(AwnApplet *applet)
{
  AwnAppletDialog *dialog;

  g_return_val_if_fail(AWN_IS_APPLET(applet), NULL);

  dialog = g_object_new(AWN_TYPE_APPLET_DIALOG,
                        "type", GTK_WINDOW_TOPLEVEL,
                        "skip-taskbar-hint", TRUE,
                        "decorated", FALSE,
                        "resizable", FALSE,
                        NULL);
  dialog->priv->applet = applet;

  return GTK_WIDGET(dialog);
}
