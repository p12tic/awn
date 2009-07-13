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
#include <libdesktop-agnostic/desktop-agnostic.h>
#include <math.h>

#include "awn-applet.h"
#include "awn-dialog.h"
#include "awn-cairo-utils.h"
#include "awn-config.h"
#include "awn-defines.h"
#include "awn-utils.h"
#include "awn-overlayable.h"

#include "gseal-transition.h"

G_DEFINE_TYPE(AwnDialog, awn_dialog, GTK_TYPE_WINDOW)

#define AWN_DIALOG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_DIALOG, \
    AwnDialogPrivate))

struct _AwnDialogPrivate
{
  GtkWidget *hbox;
  GtkWidget *title;
  GtkWidget *vbox;
  GtkWidget *align;

  /* Properties */
  GtkWidget *anchor;
  AwnApplet *anchor_applet;

  AwnOrientation orient;
  gboolean anchored;
  gboolean esc_hide;
  gboolean effects_activate;

  gint window_offset;
  gint window_offset_pub;
  gint window_padding;

  /* Standard box drawing colours */
  DesktopAgnosticColor *g_step_1;
  DesktopAgnosticColor *g_step_2;
  DesktopAgnosticColor *g_histep_1;
  DesktopAgnosticColor *g_histep_2;
  DesktopAgnosticColor *border_color;
  DesktopAgnosticColor *hilight_color;

  gulong anchor_configure_id;
  gulong applet_configure_id;
  gulong applet_comp_id;
  gulong applet_size_id;
  gulong orient_changed_id;

  guint inhibit_cookie;

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
  PROP_EFFECTS_HILIGHT,

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

static void awn_dialog_set_anchor_applet (AwnDialog *dialog,
                                          AwnApplet *applet);

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
    g_return_if_fail (win);

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
    if (BORDER*2 + ROUND_RADIUS > width - (BORDER*2 + ROUND_RADIUS))
    {
      arrow.x = width / 2;
    }
    else
    {
      arrow.x = CLAMP (arrow.x, BORDER*2 + ROUND_RADIUS,
                       width - (BORDER*2 + ROUND_RADIUS));
    }
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

    cairo_arc_negative (cr, arrow.x + BORDER, arrow.y + BORDER,
                        BORDER, M_PI * 1.5, M_PI);
    cairo_arc_negative (cr, arrow.x - BORDER, arrow.y + BORDER,
                        BORDER, 0.0, M_PI * 1.5);

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

static void
_real_show (GtkWidget *widget)
{
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (widget);

  awn_dialog_refresh_position (AWN_DIALOG (widget), 0, 0);

  gtk_widget_show (priv->vbox);
  gtk_widget_show (priv->align);

  /* in Vala terms: base.show(); */
  GTK_WIDGET_CLASS (awn_dialog_parent_class)->show (widget);

  if (priv->anchor_applet && priv->inhibit_cookie == 0)
  {
    priv->inhibit_cookie = 
      awn_applet_inhibit_autohide (priv->anchor_applet,
                                   "AwnDialog being displayed");
  }
}

static void
_real_hide (GtkWidget *widget)
{
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (widget);

  /* in Vala terms: base.hide(); */
  GTK_WIDGET_CLASS (awn_dialog_parent_class)->hide (widget);

  if (priv->anchor_applet && priv->inhibit_cookie)
  {
    awn_applet_uninhibit_autohide (priv->anchor_applet, priv->inhibit_cookie);
    priv->inhibit_cookie = 0;
  }
}

static gboolean
_expose_event (GtkWidget *widget, GdkEventExpose *expose)
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

  /* inner border, which is 2 pixels smaller */
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

  // make sure the constants here equal the ones in paint_border_path method!
  const int BORDER = priv->window_padding * 3/4;
  const int ROUND_RADIUS = priv->window_padding / 2;

  /* fill for the titlebar */
  if (GTK_WIDGET_VISIBLE (priv->title))
  {
    cairo_save (cr);

    cairo_identity_matrix (cr);
    cairo_translate (cr, 0.5, 0.5);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    awn_cairo_set_source_color (cr, priv->g_histep_1);

    const int start_x = BORDER + 2;
    const int start_y = BORDER + 2;
    const int inner_w = width - 2*BORDER - 4;
    const int inner_h = priv->title->allocation.height
                        + (priv->window_padding - BORDER);
    const int round_radius = priv->window_padding / 2;

    awn_cairo_rounded_rect (cr, start_x, start_y,
                            inner_w, inner_h,
                            round_radius, ROUND_TOP);
    cairo_fill_preserve (cr);
    cairo_stroke (cr);
#if 0
    const int SHADOW_RADIUS = 4;
    cairo_rectangle (cr, BORDER + 2,
                     BORDER + 2 + inner_h - priv->window_padding / 2,
                     inner_w, priv->window_padding / 2 + SHADOW_RADIUS);
    cairo_clip (cr);
    awn_cairo_rounded_rect_shadow (cr, BORDER + 2, BORDER + 2,
                                   inner_w, inner_h,
                                   priv->window_padding / 2, ROUND_TOP,
                                   SHADOW_RADIUS, 0.4);
#endif
    const GdkColor *color = &priv->title->style->fg[GTK_STATE_PRELIGHT];
    double r = color->red / 65535.0;
    double g = color->green / 65535.0;
    double b = color->blue / 65535.0;

    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear (start_x, start_y + inner_h,
                                       start_x + inner_w, start_y + inner_h);
    cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.2, r, g, b, 0.625);
    cairo_pattern_add_color_stop_rgba (pat, 0.8, r, g, b, 0.625);
    cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

    cairo_set_source (cr, pat);
    cairo_move_to (cr, start_x, start_y + inner_h);
    cairo_rel_line_to (cr, inner_w, 0.0);
    cairo_stroke (cr);

    cairo_pattern_destroy (pat);

    cairo_restore (cr);
  }

  awn_cairo_set_source_color (cr, priv->border_color);
  cairo_append_path (cr, path);
  cairo_stroke (cr);

  /* draw shadow */
  // FIXME: add property to disable it? (setting padding to <= 1 will do it now)
  if (gtk_widget_is_composited (widget) && priv->window_padding > 1)
  {
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
                                   SHADOW_RADIUS, 0.6);

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
                      "<span size='medium' weight='bold'>%s</span>", title);
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
_on_active_changed(GObject *dialog, GParamSpec *spec, gpointer null)
{
  // FIXME: add property to disable this
  AwnDialogPrivate *priv;

  priv = AWN_DIALOG(dialog)->priv;

  if (!AWN_IS_OVERLAYABLE (priv->anchor) || !priv->effects_activate) return;

  AwnOverlayable *icon = AWN_OVERLAYABLE (priv->anchor);

  g_object_set (awn_overlayable_get_effects (icon),
                "active", gtk_window_is_active (GTK_WINDOW (dialog)), NULL);
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

  g_return_if_fail (AWN_IS_DIALOG (dialog));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  priv = AWN_DIALOG (dialog)->priv;

  gtk_box_pack_start (GTK_BOX (priv->vbox), widget, TRUE, TRUE, 0);
}

static void
awn_dialog_remove(GtkContainer *dialog, GtkWidget *widget)
{
  AwnDialogPrivate *priv;

  g_return_if_fail (AWN_IS_DIALOG (dialog));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  priv = AWN_DIALOG (dialog)->priv;

  if (widget == priv->align)
  {
    /* the alignment was added using the base method,
         so it also has to be removed using that way */
    GtkContainerClass *klass = GTK_CONTAINER_CLASS (awn_dialog_parent_class);
    klass->remove (GTK_CONTAINER (dialog), widget);
  }
  else
  {
    gtk_container_remove (GTK_CONTAINER (priv->vbox), widget);
  }
}

static void
awn_dialog_constructed (GObject *object)
{
  GError *error = NULL;
  DesktopAgnosticConfigClient *client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_critical ("An error occurred when retrieving the config client: %s", error->message);
    g_error_free (error);
  }
  else
  {
#define AWN_GROUP_THEME "theme"

    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "gstep1",
                                         object, "gstep1", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "gstep2",
                                         object, "gstep2", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "ghistep1",
                                         object, "ghistep1", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "ghistep2",
                                         object, "ghistep2", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "border",
                                         object, "border", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
    desktop_agnostic_config_client_bind (client, AWN_GROUP_THEME, "hilight",
                                         object, "hilight", TRUE,
                                         DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                         NULL);
  }
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
      g_value_set_int (value, priv->window_offset_pub);
      break;
    case PROP_WINDOW_PADDING:
      g_value_set_int (value, priv->window_padding);
      break;
    case PROP_EFFECTS_HILIGHT:
      g_value_set_boolean (value, priv->effects_activate);
      break;

    case PROP_GSTEP1:
    case PROP_GSTEP2:
    case PROP_GHISTEP1:
    case PROP_GHISTEP2:
    case PROP_BORDER:
    case PROP_HILIGHT:
    {
      DesktopAgnosticColor *color;

      g_warning ("Property get unimplemented!");
      color = desktop_agnostic_color_new_from_string ("white", NULL);
      g_value_take_object (value, color);
      break;
    }
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
    case PROP_ANCHOR_OWNER:
      awn_dialog_set_anchor_applet (AWN_DIALOG (object),
                                    AWN_APPLET (g_value_get_object (value)));
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
    case PROP_EFFECTS_HILIGHT:
      priv->effects_activate = g_value_get_boolean (value);
      break;

    case PROP_GSTEP1:
      priv->g_step_1 = (DesktopAgnosticColor*)g_value_dup_object (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GSTEP2:
      priv->g_step_2 = (DesktopAgnosticColor*)g_value_dup_object (value);;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP1:
      priv->g_histep_1 = (DesktopAgnosticColor*)g_value_dup_object (value);;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_GHISTEP2:
      priv->g_histep_2 = (DesktopAgnosticColor*)g_value_dup_object (value);;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_BORDER:
      priv->border_color = (DesktopAgnosticColor*)g_value_dup_object (value);;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_HILIGHT:
      priv->hilight_color = (DesktopAgnosticColor*)g_value_dup_object (value);;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_dialog_finalize (GObject *object)
{
  AwnDialogPrivate *priv = AWN_DIALOG_GET_PRIVATE (object);
  GError *error = NULL;
  DesktopAgnosticConfigClient *client;
  
  client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_warning ("An error occurred when retrieving the config client: %s",
               error->message);
    g_error_free (error);
  }
  else
  {
    desktop_agnostic_config_client_unbind_all_for_object (client,
                                                          object, NULL);
  }

  if (priv->anchor_configure_id)
  {
    g_signal_handler_disconnect (priv->anchor, priv->anchor_configure_id);
    priv->anchor_configure_id = 0;
  }

  if (priv->applet_configure_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_configure_id);
    priv->applet_configure_id = 0;
  }

  if (priv->orient_changed_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->orient_changed_id);
    priv->orient_changed_id = 0;
  }

  if (priv->applet_comp_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_comp_id);
    priv->applet_comp_id = 0;
  }

  if (priv->applet_size_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_size_id);
    priv->applet_size_id = 0;
  }

  G_OBJECT_CLASS (awn_dialog_parent_class)->finalize (object);
}

/*
 * class init
 */
static void
awn_dialog_class_init (AwnDialogClass *klass)
{
  GObjectClass *obj_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *cont_class;

  obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_dialog_constructed;
  obj_class->get_property = awn_dialog_get_property;
  obj_class->set_property = awn_dialog_set_property;

  obj_class->finalize = awn_dialog_finalize;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = _expose_event;
  widget_class->show = _real_show;
  widget_class->hide = _real_hide;

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
    PROP_ANCHOR_OWNER,
    g_param_spec_object ("anchor-applet",
                         "Anchor applet",
                         "AwnApplet this window is attached to",
                         AWN_TYPE_APPLET,
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
    PROP_EFFECTS_HILIGHT,
    g_param_spec_boolean ("effects-hilight",
                          "Effects Hilight",
                          "Sets the anchored widget active when dialog is "
                          "focused and the anchor implements "
                          "AwnOverlayable interface",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP1,
    g_param_spec_object ("gstep1",
                         "GStep1",
                         "Gradient Step 1",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP2,
    g_param_spec_object ("gstep2",
                         "GStep2",
                         "Gradient Step 2",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP1,
    g_param_spec_object ("ghistep1",
                         "GHiStep1",
                         "Hilight Gradient Step 1",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP2,
    g_param_spec_object ("ghistep2",
                         "GHiStep2",
                         "Hilight Gradient Step 2",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_BORDER,
    g_param_spec_object ("border",
                         "Border",
                         "Border color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_HILIGHT,
    g_param_spec_object ("hilight",
                         "Hilight",
                         "Internal border color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
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

  priv->anchor = NULL;
  priv->anchor_applet = NULL;

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

  awn_utils_ensure_transparent_bg (GTK_WIDGET (dialog));

  /* alignment for dialog's border */
  priv->align = gtk_alignment_new (0.5, 0.5, 1, 1);

  GTK_CONTAINER_CLASS (awn_dialog_parent_class)->add (GTK_CONTAINER (dialog),
                                                      priv->align);

  /* main container for widgets */
  priv->vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (priv->align), priv->vbox);

  /* titlebar */
  //priv->hbox = gtk_hbox_new (FALSE, 2);
  //gtk_box_pack_start (GTK_BOX(priv->vbox), priv->hbox, TRUE, TRUE, 0);

  /* title widget itself */
  priv->title = gtk_label_new ("");
  gtk_widget_set_no_show_all (priv->title, TRUE);
  gtk_widget_set_state (priv->title, GTK_STATE_PRELIGHT);
  gtk_misc_set_alignment (GTK_MISC (priv->title), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (priv->title), 4, 0);

  gtk_box_pack_start (GTK_BOX (priv->vbox), priv->title, TRUE, TRUE, 0);

  /* See if the title has been set */
  g_signal_connect (dialog, "notify::title",
                    G_CALLBACK (_on_title_notify), NULL);

  g_object_notify (G_OBJECT (dialog), "title");

  g_signal_connect (dialog, "notify::is-active",
                    G_CALLBACK (_on_active_changed), NULL);
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
    gboolean dominant_axis_changed = FALSE;
    switch (priv->orient)
    {
      case AWN_ORIENTATION_LEFT:
      case AWN_ORIENTATION_RIGHT:
        dominant_axis_changed = priv->last_x != x;
        break;
      default:
        dominant_axis_changed = priv->last_y != y;
        break;
    }

    /*g_debug ("last != current; last [%d, %d], now [%d, %d], (%d x %d)",
             priv->last_x, priv->last_y, x, y, width, height);*/

    gboolean huge_delta = FALSE;
    if (!dominant_axis_changed)
    {
      switch (priv->orient)
      {
        case AWN_ORIENTATION_LEFT:
        case AWN_ORIENTATION_RIGHT:
          huge_delta = !(y >= priv->last_y - height/3
                         && y <= priv->last_y + height/3);
          break;
        default:
          huge_delta = !(x >= priv->last_x - width/3
                         && x <= priv->last_x + width/3);
          break;
      }
    }
    if (dominant_axis_changed || huge_delta)
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

  if (!GTK_WIDGET_VISIBLE (dialog)) return;

  awn_dialog_refresh_position (dialog, 0, 0);
}

static void
_applet_on_size_changed (AwnDialog *dialog)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  awn_dialog_set_offset (dialog, priv->window_offset_pub);
}

static void
awn_dialog_set_anchor_applet (AwnDialog *dialog, AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_DIALOG (dialog));

  AwnDialogPrivate *priv = dialog->priv;

  if (priv->applet_configure_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_configure_id);
    priv->applet_configure_id = 0;
  }

  if (priv->orient_changed_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->orient_changed_id);
    priv->orient_changed_id = 0;
  }

  if (priv->applet_comp_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_comp_id);
    priv->applet_comp_id = 0;
  }

  if (priv->applet_size_id)
  {
    g_signal_handler_disconnect (priv->anchor_applet, priv->applet_size_id);
    priv->applet_size_id = 0;
  }

  if (applet) g_return_if_fail (AWN_IS_APPLET (applet));

  priv->anchor_applet = applet;

  if (applet)
  {
    /* get orientation from the applet and connect to its changed signal */
    priv->orient = awn_applet_get_orientation (applet);

    /* connect to the special configure-event and other relevant signals*/
    priv->applet_configure_id =
      g_signal_connect (applet, "origin-changed",
                        G_CALLBACK (_on_origin_changed), dialog);

    priv->orient_changed_id =
      g_signal_connect_swapped (applet, "orientation-changed",
                                G_CALLBACK (awn_dialog_set_orientation),
                                dialog);

    priv->applet_comp_id = 
      g_signal_connect_swapped (applet, "composited-changed",
                        G_CALLBACK (_applet_on_size_changed), dialog);

    priv->applet_size_id = 
      g_signal_connect_swapped (applet, "size-changed",
                        G_CALLBACK (_applet_on_size_changed), dialog);

    awn_dialog_set_offset (dialog, priv->window_offset_pub);
  }
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

  // FIXME: perhaps we should ref the object and unref it in our dispose
  priv->anchor = anchor;

  if (anchor)
  {
    if (AWN_IS_APPLET (anchor))
    {
      /* special behaviour if we're anchoring to an AwnApplet */
      awn_dialog_set_anchor_applet (dialog, AWN_APPLET (anchor));
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

  priv->window_offset_pub = offset;
  priv->window_offset = offset;

  if (priv->anchor_applet && gtk_widget_is_composited (
        GTK_WIDGET (priv->anchor_applet)))
  {
    // there's an extra space above AwnApplet, lets compensate it
    priv->window_offset += awn_applet_get_size (priv->anchor_applet) * -1;
  }

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
awn_dialog_new(void)
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

/**
 * awn_dialog_new_for_widget_with_applet:
 * @widget: The widget to which to associate the dialog.
 * @applet: AwnApplet associated with @widget.
 *
 * Creates a new toplevel window that is "attached" to the @widget.
 * Returns: a new dialog.  Caller is responsible for freeing the memory when
 * the dialog is no longer being used.
 */
GtkWidget*
awn_dialog_new_for_widget_with_applet (GtkWidget *widget, AwnApplet *applet)
{
  AwnDialog *dialog;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  dialog = g_object_new (AWN_TYPE_DIALOG,
                         "anchor", widget,
                         "anchor-applet", applet,
                         NULL);

  return GTK_WIDGET (dialog);
}

