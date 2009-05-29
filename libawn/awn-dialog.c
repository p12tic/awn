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
#include <libdesktop-agnostic/color.h>
#include <math.h>

#include "awn-applet.h"
#include "awn-dialog.h"
#include "awn-cairo-utils.h"
#include "awn-config-bridge.h"
#include "awn-config-client.h"
#include "awn-defines.h"

#include "gseal-transition.h"

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
  AwnApplet *anchor_owner;

  AwnOrientation orient;
  gboolean anchored;
  gboolean esc_hide;

  gint window_offset;
  gint window_padding;

  /* Standard box drawing colours */
  DesktopAgnosticColor *g_step_1;
  DesktopAgnosticColor *g_step_2;
  DesktopAgnosticColor *g_histep_1;
  DesktopAgnosticColor *g_histep_2;
  DesktopAgnosticColor *border_color;
  DesktopAgnosticColor *hilight_color;

  gulong anchor_configure_id;
  gulong orient_changed_id;
  guint  idle_id;

  gint old_x, old_y, old_w, old_h;
  gint a_old_x, a_old_y, a_old_w, a_old_h;

  gint last_x, last_y;
};

enum
{
  PROP_0,

  PROP_ANCHOR,
  PROP_ANCHOR_OWNER,
  PROP_ANCHORED,
  PROP_ORIENT,
  PROP_WINDOW_OFFSET,
  PROP_WINDOW_PADDING,
  PROP_HIDE_ON_ESC,

  PROP_GSTEP1,
  PROP_GSTEP2,
  PROP_GHISTEP1,
  PROP_GHISTEP2,
  PROP_BORDER,
  PROP_HILIGHT,
};

#define AWN_DIALOG_DEFAULT_OFFSET 15

/* FORWARDS */

static void awn_dialog_set_anchor_widget (AwnDialog *dialog,
                                          GtkWidget *anchor);

static void awn_dialog_set_orientation   (AwnDialog *dialog,
                                          AwnOrientation orient);

static void awn_dialog_set_offset        (AwnDialog *dialog, gint offset);

static void awn_dialog_refresh_position  (AwnDialog *dialog,
                                          gint width, gint height);

static void awn_dialog_set_masks         (GtkWidget *widget,
                                          gint width, gint height);

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

static void
_on_composited_changed (GtkWidget *widget, gpointer data)
{
  if (gtk_widget_is_composited (widget))
  {
    gtk_widget_shape_combine_mask (widget, NULL, 0, 0);
  }
  else
  {
    gtk_widget_input_shape_combine_mask (widget, NULL, 0, 0);
  }

  awn_dialog_set_masks (widget,
                        widget->allocation.width, widget->allocation.height);
}

static void
awn_dialog_paint_border_path(AwnDialog *dialog, cairo_t *cr,
                             gint width, gint height)
{
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (dialog);

  const int BORDER = priv->window_padding * 3/4;
  const int ROUND_RADIUS = priv->window_padding / 2;

  /* FIXME: mhr3: I couldn't get the shape mask to work in non-composited env,
   *  so I disabled the arrow painting there, anyone feel free to fix it :)
   */
  if (priv->anchor && priv->anchored &&
      gtk_widget_get_window (priv->anchor) &&
      gtk_widget_is_composited (GTK_WIDGET (dialog)))
  {
    GdkPoint arrow;
    GdkPoint a_center_point = { .x = 0, .y = 0 };
    GdkPoint o_center_point = { .x = 0, .y = 0 };
    gint temp, aw = 0, ah = 0;
    GdkWindow *win;

    /* Calculate position of the arrow point
     *   1) get anchored window center point in root window coordinates
     *   2) get our origin in root window coordinates
     *   3) calc the difference (which is different for each orient)
     */
    win = gtk_widget_get_window (priv->anchor);

    gdk_window_get_origin (win, &a_center_point.x, &a_center_point.y);
    gdk_drawable_get_size (GDK_DRAWABLE (win), &aw, &ah);

    a_center_point.x += aw/2;
    a_center_point.y += ah/2;

    if (GTK_WIDGET_REALIZED (dialog))
    {
      gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (dialog)),
                             &o_center_point.x, &o_center_point.y);
    }

    switch (priv->orient)
    {
      case AWN_ORIENTATION_LEFT:
        cairo_translate (cr, width, 0.0);
        cairo_rotate (cr, M_PI * 0.5);
        temp = width;
        width = height; height = temp;

        arrow.x = a_center_point.y - o_center_point.y;
        break;
      case AWN_ORIENTATION_RIGHT:
        cairo_translate (cr, 0.0, height);
        cairo_rotate (cr, M_PI * 1.5);
        temp = width;
        width = height; height = temp;

        arrow.x = width - (a_center_point.y - o_center_point.y);
        break;
      case AWN_ORIENTATION_TOP:
        cairo_translate (cr, width, height);
        cairo_rotate (cr, M_PI);

        arrow.x = width - (a_center_point.x - o_center_point.x);
        break;
      case AWN_ORIENTATION_BOTTOM:
      default:
        arrow.x = a_center_point.x - o_center_point.x;
        break;
    }
    /* Make sure we paint the arrow in our window */
    arrow.x = CLAMP (arrow.x,
                     BORDER + ROUND_RADIUS,
                     width - (BORDER + ROUND_RADIUS));
    arrow.y = height - BORDER;

    GdkPoint top_left  = { .x = BORDER, .y = BORDER };
    GdkPoint top_right = { .x = width - BORDER, .y = BORDER };
    GdkPoint bot_left  = { .x = BORDER, .y = height - BORDER };
    GdkPoint bot_right = { .x = width - BORDER, .y = height - BORDER };

    /* start @ top-left curve */
    cairo_move_to (cr, bot_left.x, bot_left.y - ROUND_RADIUS);
    cairo_arc (cr, top_left.x + ROUND_RADIUS, top_left.y + ROUND_RADIUS,
               ROUND_RADIUS, M_PI, M_PI * 1.5);

    /* line to top-right corner + curve */
    cairo_arc (cr, top_right.x - ROUND_RADIUS, top_right.y + ROUND_RADIUS,
               ROUND_RADIUS, M_PI * 1.5, M_PI * 2);

    /* line to bottom-right corner + curve */
    cairo_arc (cr, bot_right.x - ROUND_RADIUS, bot_right.y - ROUND_RADIUS,
               ROUND_RADIUS, 0.0, M_PI * 0.5);

    /* Painting the actual "arrow"
     *   now we'll use BORDER for ROUND_RADIUS, because there's only BORDER
     *   pixels between the line and window edge.
     */
    cairo_line_to (cr, arrow.x + BORDER, arrow.y);
    cairo_curve_to (cr, arrow.x + BORDER, arrow.y, arrow.x, arrow.y,
                    arrow.x, arrow.y + BORDER);
    cairo_curve_to (cr, arrow.x, arrow.y + BORDER, arrow.x, arrow.y,
                    arrow.x - BORDER, arrow.y);

    /* line to bottom-left corner + curve */
    cairo_arc (cr, bot_left.x + ROUND_RADIUS, bot_left.y - ROUND_RADIUS,
               ROUND_RADIUS, M_PI * 0.5, M_PI);

    /* close the path */
    cairo_close_path (cr);
  }
  else
  {
    /* If we're not anchored use a simple rounded rect */
    awn_cairo_rounded_rect(cr, BORDER, BORDER,
                           width - (BORDER*2), height - (BORDER*2),
                           ROUND_RADIUS, ROUND_ALL);
  }
}

static gboolean
_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
  AwnDialog *dialog;
  AwnDialogPrivate *priv;
  GtkWidget *child;
  cairo_t *cr = NULL;
  cairo_path_t *path = NULL;
  gint width, height;

  dialog = AWN_DIALOG (widget);
  priv = dialog->priv;

  cr = gdk_cairo_create(gtk_widget_get_window (widget));

  g_return_val_if_fail (cr, FALSE);

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_cairo_region (cr, expose->region);
  cairo_clip (cr);

  /* Clear the background to transparent */
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);

  /* draw everything else over transparent background */
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_line_width (cr, 1.0);
  cairo_translate (cr, 0.5, 0.5);

  /* background shading */
  awn_cairo_set_source_color (cr, priv->g_step_2);

  awn_dialog_paint_border_path (dialog, cr, width, height);
  path = cairo_copy_path (cr);
  cairo_fill (cr);

  cairo_save (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  awn_cairo_set_source_color (cr, priv->hilight_color);
  cairo_translate (cr, 1.0, 1.0);
  switch (priv->orient)
  {
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
      cairo_scale (cr, (height-2) / (double)height, (width-2) / (double)width);
      break;
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
    default:
      cairo_scale (cr, (width-2) / (double)width, (height-2) / (double)height);
      break;
  }
  cairo_append_path (cr, path);
  cairo_stroke (cr);

  cairo_restore (cr);

  awn_cairo_set_source_color (cr, priv->border_color);
  cairo_append_path (cr, path);
  cairo_stroke (cr);

  /* draw shadow */
  // FIXME: add property to disable it? (setting padding to <= 1 will do it now)
  if (gtk_widget_is_composited (widget) && priv->window_padding > 1)
  {
    // make sure the constants here equal the ones in paint_border_path method!
    const int BORDER = priv->window_padding * 3/4;
    const int ROUND_RADIUS = priv->window_padding / 2;

    const double SHADOW_RADIUS = MIN (priv->window_padding / 2, 15);

    int w, h;

    switch (priv->orient)
    {
      case AWN_ORIENTATION_TOP:
      case AWN_ORIENTATION_BOTTOM:
        w = width;
        h = height;
        break;
      case AWN_ORIENTATION_LEFT:
      default:
        w = height;
        h = width;
        break;
    }
    // clip, so the shadow doesn't get drawn over the arrow
    cairo_save (cr);

    cairo_rectangle (cr, BORDER - SHADOW_RADIUS, BORDER - SHADOW_RADIUS,
                     w + SHADOW_RADIUS * 2, h + SHADOW_RADIUS * 2);
    cairo_append_path (cr, path);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip (cr);

    awn_cairo_rounded_rect_shadow (cr, BORDER, BORDER,
                                   w - BORDER*2, h - BORDER*2,
                                   ROUND_RADIUS, ROUND_ALL,
                                   SHADOW_RADIUS, 0.5);

    cairo_restore (cr);
  }

  cairo_path_destroy (path);

  /* Clean up */
  cairo_destroy(cr);

  /* Propagate the signal */
  child = gtk_bin_get_child (GTK_BIN (widget));

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

  cr = gdk_cairo_create(gtk_widget_get_window (widget));

  g_return_val_if_fail (cr, FALSE);

  width = widget->allocation.width;
  height = widget->allocation.height;

  gdk_cairo_region (cr, expose->region);
  cairo_clip (cr);

  cairo_translate (cr, expose->area.x, expose->area.y);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_set_line_width (cr, 1.0);
  cairo_translate (cr, 0.5, 0.5);

  /* Paint the background the border colour */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, priv->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, priv->g_histep_2);

  awn_cairo_rounded_rect (cr, 0, 0, width-1, height-0.5, 15, ROUND_ALL);
  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);

  /* border */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, priv->border_color);
  awn_cairo_pattern_add_color_stop_color (pat, 0.75, priv->border_color);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.25);

  cairo_set_source (cr, pat);
  cairo_stroke (cr);

  cairo_pattern_destroy (pat);

  /* hilight */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, priv->hilight_color);
  awn_cairo_pattern_add_color_stop_color (pat, 0.75, priv->hilight_color);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
  awn_cairo_rounded_rect (cr, 1, 1, width-3, height-1.5, 15, ROUND_ALL);
  cairo_set_source (cr, pat);
  cairo_stroke (cr);

  cairo_pattern_destroy (pat);

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
awn_dialog_set_masks (GtkWidget *widget, gint width, gint height)
{
  GdkBitmap *shaped_bitmap;
  shaped_bitmap = (GdkBitmap*) gdk_pixmap_new (NULL, width, height, 1);

  if (shaped_bitmap)
  {
    cairo_t *cr = gdk_cairo_create (shaped_bitmap);

    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    cairo_translate (cr, 0.5, 0.5);
    awn_dialog_paint_border_path (AWN_DIALOG (widget), cr, width, height);

    cairo_fill_preserve (cr);
    cairo_set_line_width (cr, 1.0);
    cairo_stroke (cr);

    cairo_destroy (cr);

    if (gtk_widget_is_composited (widget))
    {
      gtk_widget_input_shape_combine_mask (widget, NULL, 0, 0);
      gtk_widget_input_shape_combine_mask (widget, shaped_bitmap, 0, 0);
    }
    else
    {
      gtk_widget_shape_combine_mask (widget, NULL, 0, 0);
      gtk_widget_shape_combine_mask (widget, shaped_bitmap, 0, 0);
    }

    g_object_unref (shaped_bitmap);
  }
}

static gboolean
_on_configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  g_return_val_if_fail (AWN_IS_DIALOG (widget), FALSE);

  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (widget);

  if (event->x == priv->old_x && event->y == priv->old_y &&
      event->width == priv->old_w && event->height == priv->old_h)
  {
    return FALSE;
  }

  gboolean dimensions_changed =
    event->width != priv->old_w || event->height != priv->old_h;

  priv->old_x = event->x;     priv->old_y = event->y;
  priv->old_w = event->width; priv->old_h = event->height;

  awn_dialog_refresh_position (AWN_DIALOG (widget),
                               event->width, event->height);

  if (dimensions_changed)
  {
    awn_dialog_set_masks (widget, event->width, event->height);

    gtk_widget_queue_draw (widget);
  }

  return FALSE;
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
      g_value_set_int (value, priv->orient);
      break;
    case PROP_HIDE_ON_ESC:
      g_value_set_boolean (value, priv->esc_hide);
      break;
    case PROP_WINDOW_OFFSET:
      g_value_set_int (value, priv->window_offset);
      break;
    case PROP_WINDOW_PADDING:
      g_value_set_int (value, priv->window_padding);
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
      awn_dialog_set_anchor_widget (AWN_DIALOG (object),
                                    GTK_WIDGET (g_value_get_object (value)));
      break;
    case PROP_ANCHORED:
      priv->anchored = g_value_get_boolean (value);
      if (priv->anchored)
      {
        awn_dialog_refresh_position (AWN_DIALOG (object), 0, 0);
      }
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_ORIENT:
      awn_dialog_set_orientation (AWN_DIALOG (object),
                                  g_value_get_int (value));
      break;
    case PROP_WINDOW_OFFSET:
      awn_dialog_set_offset (AWN_DIALOG (object), g_value_get_int (value));
      break;
    case PROP_WINDOW_PADDING:
      awn_dialog_set_padding (AWN_DIALOG (object), g_value_get_int (value));
      break;
    case PROP_HIDE_ON_ESC:
      priv->esc_hide = g_value_get_boolean (value);
      break;

    case PROP_GSTEP1:
      priv->g_step_1 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GSTEP2:
      priv->g_step_2 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP1:
      priv->g_histep_1 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP2:
      priv->g_histep_2 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_BORDER:
      priv->border_color = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_HILIGHT:
      priv->hilight_color = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
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
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The orientation of the window",
                      0, 3, AWN_ORIENTATION_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_WINDOW_OFFSET,
    g_param_spec_int ("window-offset",
                      "Window offset",
                      "The offset from window border",
                      G_MININT, G_MAXINT, AWN_DIALOG_DEFAULT_OFFSET,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_WINDOW_PADDING,
    g_param_spec_int ("window-padding",
                      "Window padding",
                      "The padding from window border",
                      0, G_MAXINT, 15,
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
awn_dialog_init (AwnDialog *dialog)
{
  AwnDialogPrivate *priv;

  priv = dialog->priv = AWN_DIALOG_GET_PRIVATE (dialog);

  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_decorated (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_stick (GTK_WINDOW (dialog));

  _on_alpha_screen_changed (GTK_WIDGET (dialog), NULL, NULL);
  gtk_widget_set_app_paintable (GTK_WIDGET (dialog), TRUE);

  /*  events */
  gtk_widget_add_events (GTK_WIDGET (dialog), GDK_ALL_EVENTS_MASK);
  g_signal_connect (dialog, "key-press-event",
                    G_CALLBACK (_on_key_press_event), NULL);
  g_signal_connect (dialog, "delete-event", 
                    G_CALLBACK (_on_delete_event), NULL);
  g_signal_connect (dialog, "configure-event",
                    G_CALLBACK (_on_configure_event), NULL);
  g_signal_connect (dialog, "composited-changed",
                    G_CALLBACK (_on_composited_changed), NULL);

  priv->align = gtk_alignment_new (0.5, 0.5, 1, 1);

  GTK_CONTAINER_CLASS (awn_dialog_parent_class)->add (GTK_CONTAINER (dialog),
                                                      priv->align);

  priv->vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (priv->align), priv->vbox);

  priv->title = gtk_label_new ("");
  gtk_widget_set_no_show_all (priv->title, TRUE);
  gtk_box_pack_start (GTK_BOX (priv->vbox), priv->title, TRUE, TRUE, 0);
  g_signal_connect (priv->title, "expose-event",
                    G_CALLBACK (on_title_expose), dialog);

  gtk_widget_set_state (priv->title, GTK_STATE_PRELIGHT);
  gtk_misc_set_alignment (GTK_MISC (priv->title), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (priv->title), 4, 4);

  /* See if the title has been set */
  g_signal_connect (dialog, "notify::title",
                    G_CALLBACK (_on_title_notify), NULL);

  g_object_notify (G_OBJECT (dialog), "title");
}

// FIXME: we're still missing dispose method

static void
awn_dialog_refresh_position (AwnDialog *dialog, gint width, gint height)
{
  gint ax, ay, aw, ah;
  gint x = 0, y = 0;
  GdkWindow *win, *dialog_win;

  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  if (!priv->anchor || !priv->anchored) return;

  if (!width)  width  = GTK_WIDGET (dialog)->allocation.width;
  if (!height) height = GTK_WIDGET (dialog)->allocation.height;

    win = gtk_widget_get_window (priv->anchor);

  if (!win) return; // widget might not be realized yet

  gdk_window_get_origin (win, &ax, &ay);
  gdk_drawable_get_size (GDK_DRAWABLE (win), &aw, &ah);

  const int OFFSET = priv->window_offset;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_LEFT:
      x = ax + aw + OFFSET;
      y = ay - height / 2 + ah / 2;
      break;
    case AWN_ORIENTATION_RIGHT:
      x = ax - width - OFFSET;
      y = ay - height / 2 + ah / 2;
      break;
    case AWN_ORIENTATION_TOP:
      x = ax - width / 2 + aw / 2;
      y = ay + ah + OFFSET;
      break;
    case AWN_ORIENTATION_BOTTOM:
    default:
      x = ax - width / 2 + aw / 2;
      y = ay - height - OFFSET;
      break;
  }

  /* fits to screen? */
  if (gtk_widget_has_screen (GTK_WIDGET (dialog)))
  {
    GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (dialog));
    gint screen_w = gdk_screen_get_width (screen);
    gint screen_h = gdk_screen_get_height (screen);
    if (x + width > screen_w) x -= x + width - screen_w;
    if (x < 0) x = 0;
    if (y + height > screen_h) y -= y + height -screen_h;
    if (y < 0) y = 0;
  }

  dialog_win = gtk_widget_get_window (GTK_WIDGET (dialog));

  if (dialog_win)
  {
    gdk_window_get_position (dialog_win,
                             &priv->last_x, &priv->last_y);
  }

  if (priv->last_x != x || priv->last_y != y)
  {
    /* 
     * some optimization, we dont want to move the window on every small
     *  change 
     */
    gboolean both_axes = priv->last_x != x && priv->last_y != y;
    gboolean huge_delta = FALSE;
    if (!both_axes)
    {
      gint delta = abs(priv->last_x - x) + abs(priv->last_y - y);
      switch (priv->orient)
      {
        case AWN_ORIENTATION_LEFT:
        case AWN_ORIENTATION_RIGHT:
          huge_delta = delta >= height / 10;
          break;
        default:
          huge_delta = delta >= width / 10;
          break;
      }
    }
    if (both_axes || huge_delta)
    {
      priv->last_x = x;
      priv->last_y = y;
      gtk_window_move (GTK_WINDOW (dialog), x, y);
    }
  }

  /* Invalidate part of the window where the arrow is */
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      gtk_widget_queue_draw_area (GTK_WIDGET (dialog), 0, 0,
                                  width, priv->window_padding);
      break;
    case AWN_ORIENTATION_LEFT:
      gtk_widget_queue_draw_area (GTK_WIDGET (dialog), 0, 0,
                                  priv->window_padding, height);
      break;
    case AWN_ORIENTATION_RIGHT:
      gtk_widget_queue_draw_area (GTK_WIDGET (dialog), 
                                  width - priv->window_padding, 0,
                                  priv->window_padding, height);
      break;
    case AWN_ORIENTATION_BOTTOM:
    default:
      gtk_widget_queue_draw_area (GTK_WIDGET (dialog),
                                  0, height - priv->window_padding,
                                  width, priv->window_padding);
      break;
  }
}

static void
_on_origin_changed (AwnApplet *applet, GdkRectangle *rect, AwnDialog *dialog)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  if (!GTK_WIDGET_VISIBLE (dialog)) return;

  awn_dialog_refresh_position (dialog, 0, 0);
}

static gboolean
_on_anchor_configure_event (GtkWidget *widget, GdkEventConfigure *event,
                            AwnDialog *dialog)
{
  g_return_val_if_fail (AWN_IS_DIALOG (dialog), FALSE);

  AwnDialogPrivate *priv = dialog->priv;

  if (!GTK_WIDGET_VISIBLE (widget)) return FALSE;

  if (event->x == priv->a_old_x && event->y == priv->a_old_y
      && event->width == priv->a_old_w && event->height == priv->a_old_h)
  {
    return FALSE;
  }

  priv->a_old_x = event->x;     priv->a_old_y = event->y;
  priv->a_old_w = event->width; priv->a_old_h = event->height;
  awn_dialog_refresh_position (dialog, 0, 0);

  return FALSE;
}

static void
awn_dialog_set_anchor_widget (AwnDialog *dialog, GtkWidget *anchor)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  if (priv->anchor_configure_id)
  {
    g_signal_handler_disconnect (priv->anchor, priv->anchor_configure_id);
    priv->anchor_configure_id = 0;
  }
  if (priv->orient_changed_id)
  {
    g_signal_handler_disconnect (priv->anchor, priv->orient_changed_id);
    priv->orient_changed_id = 0;
  }

  // FIXME: perhaps we should ref the object and unref it in our dispose
  priv->anchor = anchor;

  if (anchor)
  {
    if (AWN_IS_APPLET (anchor))
    {
      /* special behaviour if we're anchoring to an AwnApplet */
      AwnApplet *applet = AWN_APPLET (anchor);

      /* connect to the special configure-event */
      priv->anchor_configure_id =
        g_signal_connect (applet, "origin-changed",
                          G_CALLBACK (_on_origin_changed), dialog);

      /* get orientation from the applet and connect to its changed signal */
      priv->orient = awn_applet_get_orientation (applet);
      priv->orient_changed_id =
        g_signal_connect_swapped (applet, "orientation-changed",
                                  G_CALLBACK (awn_dialog_set_orientation),
                                  dialog);

      if (gtk_widget_is_composited (anchor))
      {
        // there's an extra space above AwnApplet, lets compensate it
        priv->window_offset = awn_applet_get_size (applet) * -1;
        priv->window_offset += AWN_DIALOG_DEFAULT_OFFSET;
      }
    }
    else
    {
      // use normal configure event
      priv->anchor_configure_id =
        g_signal_connect (anchor, "configure-event",
                          G_CALLBACK (_on_anchor_configure_event), dialog);
    }
  }

  awn_dialog_refresh_position (dialog, 0, 0);

  gtk_widget_queue_draw (GTK_WIDGET (dialog));
}

static void
awn_dialog_set_orientation (AwnDialog *dialog, AwnOrientation orient)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  if (orient == priv->orient) return;

  priv->orient = orient;

  awn_dialog_refresh_position (dialog, 0, 0);

  gtk_widget_queue_draw (GTK_WIDGET (dialog));
}

static void
awn_dialog_set_offset (AwnDialog *dialog, gint offset)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));
  
  AwnDialogPrivate *priv = dialog->priv;

  if (offset == priv->window_offset) return;

  priv->window_offset = offset;

  awn_dialog_refresh_position (dialog, 0, 0);
}

void
awn_dialog_set_padding (AwnDialog *dialog, gint padding)
{
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (dialog);

  priv->window_padding = padding;

  gtk_alignment_set_padding (GTK_ALIGNMENT (priv->align),
                             padding, padding, padding, padding);
}

/**
 * awn_dialog_new:
 *
 * Creates a new toplevel window.
 * Returns: a new dialog.  Caller is responsible for freeing the memory when
 * the dialog is no longer being used.
 */
GtkWidget*
awn_dialog_new()
{
  AwnDialog *dialog;

  dialog = g_object_new (AWN_TYPE_DIALOG, NULL);

  return GTK_WIDGET (dialog);
}

/**
 * awn_dialog_new_for_widget:
 * @widget: The widget to which to associate the dialog.
 *
 * Creates a new toplevel window that is "attached" to the @widget.
 * Returns: a new dialog.  Caller is responsible for freeing the memory when
 * the dialog is no longer being used.
 */
GtkWidget*
awn_dialog_new_for_widget (GtkWidget *widget)
{
  AwnDialog *dialog;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  dialog = g_object_new (AWN_TYPE_DIALOG,
                         "anchor", widget,
                         NULL);

  return GTK_WIDGET (dialog);
}

