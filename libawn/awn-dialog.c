/*
 * Copyright (c) 2007-2009 Anthony Arobone <aarobone@gmail.com>
 *                         Neil Jagdish Patel <njpatel@gmail.com>
 *                         Michal Hruby <michal.mhr@gmail.com>
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

#include "awn-dialog.h"
#include "awn-cairo-utils.h"
#include "awn-config-bridge.h"
#include "awn-config-client.h"
#include "awn-defines.h"

G_DEFINE_TYPE(AwnDialog, awn_dialog, GTK_TYPE_WINDOW)

#define AWN_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_DIALOG, \
    AwnDialogPrivate))

struct _AwnDialogPrivate
{
  GtkWidget *title;
  GtkWidget *vbox;
  GtkWidget *align;

  /* Properties */
  GtkWidget *anchor;

  AwnOrientation orient;
  gboolean anchored;
  gboolean esc_hide;

  /* Standard box drawing colours */
  AwnColor g_step_1;
  AwnColor g_step_2;
  AwnColor g_histep_1;
  AwnColor g_histep_2;
  AwnColor border_color;
  AwnColor hilight_color;
};

enum
{
  PROP_0,

  PROP_ANCHOR,
  PROP_ANCHORED,
  PROP_ORIENT,
  PROP_HIDE_ON_ESC,

  PROP_GSTEP1,
  PROP_GSTEP2,
  PROP_GHISTEP1,
  PROP_GHISTEP2,
  PROP_BORDER,
  PROP_HILIGHT,
};

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
 * awn_dialog_position_reset:
 * @dialog: The dialog to reposition.
 *
 * Resets the position of the dialog so it is centered over its associated
 * applet window.  Should "reposition" dialog-arrow if the dialog does not fit
 * (fall-off-screen) on the desired place.
 */
void
awn_dialog_position_reset(AwnDialog *dialog)
{
  gint ax = 200, ay = 200, aw = 0, ah = 0;
  gint x, y, w, h;

  g_return_if_fail(AWN_IS_DIALOG(dialog));

  if (dialog->priv->anchor)
  {
    gdk_window_get_origin(GTK_WIDGET(dialog->priv->anchor)->window,
                          &ax, &ay);
    gtk_widget_get_size_request(GTK_WIDGET(dialog->priv->anchor),
                                &aw, &ah);
  }
  gtk_window_get_size(GTK_WINDOW(dialog), &w, &h);

  x = ax - w / 2 + aw / 2;
  y = ay - h;

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
  AwnDialog *dialog;
  AwnDialogPrivate *priv;
  GtkWidget *child;
  cairo_t *cr = NULL;
  gint width, height;
  gint gap = 20;

  dialog = AWN_DIALOG (widget);
  priv = dialog->priv;

  cr = gdk_cairo_create(widget->window);

  if (!cr)
    return FALSE;

  width = widget->allocation.width;
  height = widget->allocation.height;

  /* Clear the background to transparent */
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  /* draw everything else over transparent background */
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width(cr, 2.0);

  /* background shading */
  cairo_set_source_rgba (cr, priv->g_step_2.red,
                         priv->g_step_2.green,
                         priv->g_step_2.blue,
                         priv->g_step_2.alpha);

  awn_cairo_rounded_rect(cr, BOR, BOR, width - (BOR*2),
                         height - (BOR*2) - gap,
                         15, ROUND_ALL);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba (cr, priv->border_color.red,
                         priv->border_color.green,
                         priv->border_color.blue,
                         priv->border_color.alpha);
  cairo_stroke(cr);

  /* Clean up */
  cairo_destroy(cr);

  /* Propagate the signal */
  child = gtk_bin_get_child(GTK_BIN(widget));

  if (child)
    gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                    child, expose);

  return TRUE;
}

static gboolean
on_title_expose(GtkWidget       *widget,
                GdkEventExpose  *expose,
                AwnDialog *dialog)
{
  cairo_t *cr = NULL;
  cairo_pattern_t *pat = NULL;
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (dialog);
  gint width, height;

  cr = gdk_cairo_create(widget->window);

  if (!cr)
    return FALSE;

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_cairo_region (cr, expose->region);
  cairo_clip (cr);

  cairo_translate (cr, expose->area.x, expose->area.y);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width(cr, 1.0);

  /* Paint the background the border colour */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  cairo_pattern_add_color_stop_rgba (pat, 0.0,
                                     priv->g_histep_1.red,
                                     priv->g_histep_1.green,
                                     priv->g_histep_1.blue,
                                     priv->g_histep_1.alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     priv->g_histep_2.red,
                                     priv->g_histep_2.green,
                                     priv->g_histep_2.blue,
                                     priv->g_histep_2.alpha);

  awn_cairo_rounded_rect (cr, 0, 0, width, height, 15, ROUND_ALL);
  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  cairo_set_source_rgba (cr, priv->hilight_color.red,
                             priv->hilight_color.green,
                             priv->hilight_color.blue,
                             priv->hilight_color.alpha);
  cairo_stroke (cr);

  /* Highlight */
  /*
  pat = cairo_pattern_create_linear(0, 0, 0, (height / 5) * 2);

  cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, 0.3);

  cairo_pattern_add_color_stop_rgba(pat, 1, 1, 1, 1, 0.1);

  awn_cairo_rounded_rect(cr, 1, 1, width - 2, (height / 5)*2,
                         15, ROUND_TOP);

  cairo_set_source(cr, pat);

  cairo_fill(cr);

  cairo_pattern_destroy(pat);
  */

  /* Clean up */
  cairo_destroy(cr);

  return FALSE;
}

static gboolean
_on_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer *data)
{
  g_return_val_if_fail (AWN_IS_DIALOG (widget), FALSE);

  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (widget);

  if (priv->esc_hide && event->keyval == GDK_Escape)
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
_on_title_notify(GObject *dialog, GParamSpec *spec, gpointer null)
{
  AwnDialogPrivate *priv;
  const gchar *title;

  priv = AWN_DIALOG(dialog)->priv;

  title = gtk_window_get_title(GTK_WINDOW(dialog));

  if (title)
  {
    gchar *markup = g_strdup_printf(
                      "<span size='x-large' weight='bold'>%s</span>", title);
    gtk_label_set_markup(GTK_LABEL(priv->title), markup);
    g_free(markup);
    gtk_widget_show(priv->title);
  }
  else
  {
    gtk_widget_hide(priv->title);
  }
}

static void
awn_dialog_add(GtkContainer *dialog, GtkWidget *widget)
{
  AwnDialogPrivate *priv;

  g_return_if_fail(AWN_IS_DIALOG(dialog));
  g_return_if_fail(GTK_IS_WIDGET(widget));
  priv = AWN_DIALOG(dialog)->priv;

  gtk_box_pack_start(GTK_BOX(priv->vbox), widget, TRUE, TRUE, 0);
}

static void
awn_dialog_remove(GtkContainer *dialog, GtkWidget *widget)
{
  AwnDialogPrivate *priv;

  g_return_if_fail(AWN_IS_DIALOG(dialog));
  g_return_if_fail(GTK_IS_WIDGET(widget));
  priv = AWN_DIALOG(dialog)->priv;
  gtk_container_remove(GTK_CONTAINER(priv->vbox), widget);
}

static void
awn_dialog_constructed (GObject *object)
{
  AwnConfigClient *client = awn_config_client_new ();
  AwnConfigBridge *bridge = awn_config_bridge_get_default ();

#ifndef AWN_GROUP_THEME
 #define AWN_GROUP_THEME "theme"
#endif

  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "gstep1",
                          object, "gstep1");
  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "gstep2",
                          object, "gstep2");
  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "ghistep1",
                          object, "ghistep1");
  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "ghistep2",
                          object, "ghistep2");
  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "border",
                          object, "border");
  awn_config_bridge_bind (bridge, client,
                          AWN_GROUP_THEME, "hilight",
                          object, "hilight");
}

static void
awn_dialog_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  g_return_if_fail (AWN_IS_DIALOG (object));

  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_ANCHORED:
      g_value_set_boolean (value, priv->anchored);
      break;
    case PROP_ORIENT:
      // TODO
      break;
    case PROP_HIDE_ON_ESC:
      g_value_set_boolean (value, priv->esc_hide);
      break;

    case PROP_GSTEP1:
    case PROP_GSTEP2:
    case PROP_GHISTEP1:
    case PROP_GHISTEP2:
    case PROP_BORDER:
    case PROP_HILIGHT:
      g_warning ("Property get unimplemented!");
      g_value_set_string (value, "FFFFFFFF");
      break;
  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_dialog_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  g_return_if_fail (AWN_IS_DIALOG (object));

  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_ANCHOR:
      priv->anchor = g_value_get_object (value);
      break;
    case PROP_ANCHORED:
      priv->anchored = g_value_get_boolean (value);
      break;
    case PROP_ORIENT:
      // TODO: priv->orient = g_value_get_enum (value);
      break;
    case PROP_HIDE_ON_ESC:
      priv->esc_hide = g_value_get_boolean (value);
      break;

    case PROP_GSTEP1:
      awn_cairo_string_to_color (g_value_get_string (value), &priv->g_step_1);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GSTEP2:
      awn_cairo_string_to_color (g_value_get_string (value), &priv->g_step_2);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP1:
      awn_cairo_string_to_color (g_value_get_string (value), &priv->g_histep_1);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP2:
      awn_cairo_string_to_color (g_value_get_string (value), &priv->g_histep_2);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_BORDER:
      awn_cairo_string_to_color (g_value_get_string (value), &priv->border_color);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_HILIGHT:
      awn_cairo_string_to_color (g_value_get_string (value),&priv->hilight_color);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}
/*
 * class init
 */
static void
awn_dialog_class_init(AwnDialogClass *klass)
{
  GObjectClass *obj_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *cont_class;

  obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_dialog_constructed;
  obj_class->get_property = awn_dialog_get_property;
  obj_class->set_property = awn_dialog_set_property;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = _expose_event;

  cont_class = GTK_CONTAINER_CLASS (klass);
  cont_class->add = awn_dialog_add;
  cont_class->remove = awn_dialog_remove;

  g_object_class_install_property (obj_class,
    PROP_ANCHOR,
    g_param_spec_object ("anchor",
                         "Anchor",
                         "Widget this window is attached to",
                         GTK_TYPE_WIDGET,
                         G_PARAM_WRITABLE));

  g_object_class_install_property (obj_class,
    PROP_ANCHORED,
    g_param_spec_boolean ("anchored",
                          "Anchored",
                          "Moves the window together with it's anchor widget",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_HIDE_ON_ESC,
    g_param_spec_boolean ("hide-on-esc",
                          "Hide on Escape",
                          "Hides the window when escape key is pressed",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP1,
    g_param_spec_string ("gstep1",
                         "GStep1",
                         "Gradient Step 1",
                         "FF0000FF",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP2,
    g_param_spec_string ("gstep2",
                         "GStep2",
                         "Gradient Step 2",
                         "00FF00FF",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP1,
    g_param_spec_string ("ghistep1",
                         "GHiStep1",
                         "Hilight Gradient Step 1",
                         "FFFFFF44",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP2,
    g_param_spec_string ("ghistep2",
                         "GHiStep2",
                         "Hilight Gradient Step 2",
                         "FFFFFF11",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_BORDER,
    g_param_spec_string ("border",
                         "Border",
                         "Border color",
                         "000000FF",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_HILIGHT,
    g_param_spec_string ("hilight",
                         "Hilight",
                         "Internal border color",
                         "FFFFFFff",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_type_class_add_private (G_OBJECT_CLASS (klass),
                            sizeof (AwnDialogPrivate));
}


/*
 *  init
 */
static void
awn_dialog_init(AwnDialog *dialog)
{
  AwnDialogPrivate *priv;

  priv = dialog->priv = AWN_DIALOG_GET_PRIVATE (dialog);

  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_decorated (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_stick (GTK_WINDOW (dialog));

  _on_alpha_screen_changed( GTK_WIDGET (dialog), NULL, NULL);
  gtk_widget_set_app_paintable( GTK_WIDGET (dialog), TRUE);

  /*  events */
  gtk_widget_add_events( GTK_WIDGET (dialog), GDK_ALL_EVENTS_MASK);
  g_signal_connect (G_OBJECT (dialog), "key-press-event",
                    G_CALLBACK (_on_key_press_event), NULL);
  g_signal_connect (G_OBJECT (dialog), "delete-event", 
                    G_CALLBACK (_on_delete_event), NULL);

  /* See if the title has been set */
  g_signal_connect (dialog, "notify::title",
                    G_CALLBACK (_on_title_notify), NULL);

  g_object_notify (G_OBJECT (dialog), "title");

  priv->align = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align),
                             15, 15, 15, 15);

  GTK_CONTAINER_CLASS (awn_dialog_parent_class)->add (GTK_CONTAINER (dialog),
                                                      priv->align);

  priv->vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (priv->align), priv->vbox);

  priv->title = gtk_label_new ("");
  gtk_widget_set_no_show_all (priv->title, TRUE);
  gtk_box_pack_start (GTK_BOX (priv->vbox), priv->title, TRUE, TRUE, 0);
  g_signal_connect (priv->title, "expose-event",
                    G_CALLBACK (on_title_expose), dialog);

  gtk_widget_set_state (priv->title, GTK_STATE_SELECTED);
  gtk_misc_set_alignment (GTK_MISC (priv->title), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (priv->title), 4, 4);
}

/**
 * awn_dialog_new:
 * @applet: The applet to which to associate the dialog
 *
 * Creates a new toplevel window that is "attached" to the @applet.
 * Returns: a new dialog.  Caller is responsible for freeing the memory when the
 * dialog is no longer being used.
 */
GtkWidget*
awn_dialog_new()
{
  AwnDialog *dialog;

  dialog = g_object_new(AWN_TYPE_DIALOG, NULL);

  return GTK_WIDGET(dialog);
}

GtkWidget*
awn_dialog_new_for_widget(GtkWidget *widget)
{
  AwnDialog *dialog;

  g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

  dialog = g_object_new(AWN_TYPE_DIALOG,
                        "anchor", widget,
                        NULL);

  return GTK_WIDGET(dialog);
}

