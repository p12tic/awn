/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Michal Hruby <michal.mhr@gmail.com>
 *  Adjusted by: Alberto Aldegheri <albyrock87+dev@gmail.com>
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-cairo-utils.h>
#include <math.h>

#include "awn-background-edgy.h"
#include "awn-applet-manager.h"

G_DEFINE_TYPE (AwnBackgroundEdgy, awn_background_edgy, AWN_TYPE_BACKGROUND_FLAT)

#define AWN_BACKGROUND_EDGY_GET_PRIVATE(obj) ( \
  G_TYPE_INSTANCE_GET_PRIVATE (obj, AWN_TYPE_BACKGROUND_EDGY, \
  AwnBackgroundEdgyPrivate))

struct _AwnBackgroundEdgyPrivate
{
  gint radius;
  gint top_pad;

  gboolean in_corner;

  GtkWidget *separator;
};

static void awn_background_edgy_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      GtkPositionType  position,
                                      GdkRectangle   *area);

static void awn_background_edgy_get_shape_mask (AwnBackground  *bg,
                                                cairo_t        *cr,
                                                GtkPositionType  position,
                                                GdkRectangle   *area);

static void awn_background_edgy_padding_request (AwnBackground *bg,
                                                 GtkPositionType position,
                                                 guint *padding_top,
                                                 guint *padding_bottom,
                                                 guint *padding_left,
                                                 guint *padding_right);

static void awn_background_edgy_get_strut_offsets (AwnBackground *bg,
                                                   GtkPositionType position,
                                                   GdkRectangle *area,
                                                   gint *strut,
                                                   gint *strut_start,
                                                   gint *strut_end);

static void
awn_background_edgy_update_panel_size (AwnBackgroundEdgy *bg)
{
  gint size, offset;
  g_object_get (AWN_BACKGROUND (bg)->panel,
                "size", &size, "offset", &offset, NULL);

  const gint size_offset = size + offset;
  bg->priv->radius = sqrt (size_offset * size_offset * 2);
  bg->priv->top_pad = bg->priv->radius - size_offset + 1;
  const gint side_pad = bg->priv->top_pad + offset - 2;

  gtk_misc_set_padding (GTK_MISC (bg->priv->separator), side_pad/2, side_pad/2);
}

static void
awn_background_edgy_radius_changed (AwnBackgroundEdgy *bg)
{
  awn_background_edgy_update_panel_size (bg);
  awn_background_emit_padding_changed (AWN_BACKGROUND (bg));
}

static gint
awn_background_edgy_separator_pos (AwnBackgroundEdgy *bg, gfloat align)
{
  if (awn_background_do_rtl_swap ((AwnBackground*)bg))
  {
    return align == 0.0 ? -1 :1;
  }
  else
  {
    return align == 0.0 ? 1: -1;
  }
}

static void
awn_background_edgy_align_changed (AwnBackgroundEdgy *bg) // has more params...
{
  AwnBackground *abg = AWN_BACKGROUND (bg);
  gfloat align = awn_background_get_panel_alignment (abg);
  gboolean in_corner = align == 0.0 || align == 1.0;
  if (bg->priv->in_corner != in_corner)
  {
    /* change from in_corner to in_the_middle or vice versa */
    bg->priv->in_corner = in_corner;
    if (in_corner)
    {
      gtk_widget_show (bg->priv->separator);
      /* we'll emit padding_changed after updating position in manager */
    }
    else
    {
      gtk_widget_hide (bg->priv->separator);
      awn_background_emit_padding_changed (abg);
    }
  }

  if (in_corner)
  {
    AwnAppletManager *manager;
    g_object_get (abg->panel, "applet-manager", &manager, NULL);
    awn_applet_manager_add_widget (manager, bg->priv->separator,
                                   awn_background_edgy_separator_pos (bg, align));
    awn_background_emit_padding_changed (abg);
  }
}

static void
awn_background_edgy_widgets_changed (AwnBackground *bg)
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_edgy_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_edgy_parent_class)->constructed (object);

  AwnBackgroundEdgy *bg = AWN_BACKGROUND_EDGY (object);
  gpointer panel = AWN_BACKGROUND (object)->panel;
  g_return_if_fail (panel);

  g_signal_connect_swapped (panel, "notify::size",
                            G_CALLBACK (awn_background_edgy_radius_changed),
                            object);
  g_signal_connect_swapped (panel, "notify::offset",
                            G_CALLBACK (awn_background_edgy_radius_changed),
                            object);

  AwnAppletManager *manager;
  g_object_get (panel, "applet-manager", &manager, NULL);
  g_return_if_fail (manager);
  awn_applet_manager_add_widget (manager, bg->priv->separator, 1);
  g_signal_connect_swapped (manager, "applets-refreshed",
                      G_CALLBACK (awn_background_edgy_widgets_changed), bg);


  gpointer monitor = NULL;
  g_object_get (panel, "monitor", &monitor, NULL);

  g_return_if_fail (monitor);

  g_signal_connect_swapped (monitor, "notify::monitor-align",
                            G_CALLBACK (awn_background_edgy_align_changed),
                            object);

  gfloat align = awn_background_get_panel_alignment (AWN_BACKGROUND (object));
  bg->priv->in_corner = align == 0.0 || align == 1.0;

  if (bg->priv->in_corner)
  {
    awn_applet_manager_add_widget (manager, bg->priv->separator,
                                   awn_background_edgy_separator_pos (bg, align));
    gtk_widget_show (bg->priv->separator);
  }

  awn_background_edgy_update_panel_size (AWN_BACKGROUND_EDGY (object));
}

static void
awn_background_edgy_dispose (GObject *object)
{
  gpointer panel = AWN_BACKGROUND (object)->panel;
  gpointer monitor = NULL;
  gpointer manager = NULL;

  if (panel)
  {
    g_signal_handlers_disconnect_by_func (panel,
        G_CALLBACK (awn_background_edgy_radius_changed), object);
    g_object_get (panel, "monitor", &monitor, "applet-manager", &manager, NULL);
  }

  if (monitor)
  {
    g_signal_handlers_disconnect_by_func (monitor, 
        G_CALLBACK (awn_background_edgy_align_changed), object);
  }

  GtkWidget *widget = AWN_BACKGROUND_EDGY_GET_PRIVATE (object)->separator;

  if (manager)
  {
    g_signal_handlers_disconnect_by_func (manager, 
        G_CALLBACK (awn_background_edgy_widgets_changed), object);
  }

  if (manager && widget)
  {
    awn_applet_manager_remove_widget (manager, widget);
    /* removing the widget from it's container will destroy it
     *  - no need to call gtk_widget_destroy
     */
    AWN_BACKGROUND_EDGY_GET_PRIVATE (object)->separator = NULL;
  }

  G_OBJECT_CLASS (awn_background_edgy_parent_class)->dispose (object);
}

static void
awn_background_edgy_class_init (AwnBackgroundEdgyClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_background_edgy_constructed;
  obj_class->dispose = awn_background_edgy_dispose;

  bg_class->draw = awn_background_edgy_draw;
  bg_class->padding_request = awn_background_edgy_padding_request;
  bg_class->get_shape_mask = awn_background_edgy_get_shape_mask;
  bg_class->get_input_shape_mask = awn_background_edgy_get_shape_mask;
  bg_class->get_strut_offsets = awn_background_edgy_get_strut_offsets;

  g_type_class_add_private (obj_class, sizeof (AwnBackgroundEdgyPrivate));
}


static void
awn_background_edgy_init (AwnBackgroundEdgy *bg)
{
  bg->priv = AWN_BACKGROUND_EDGY_GET_PRIVATE (bg);

  GtkWidget *image = gtk_image_new ();
  bg->priv->separator = image;
}

AwnBackground * 
awn_background_edgy_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_EDGY,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}

/*
 * Drawing functions
 */
static void
draw_path (cairo_t *cr, gint x, gdouble radius, gint width, gint height,
           gboolean bottom_left)
{
  if (bottom_left)
    cairo_arc (cr, x, height, radius, -M_PI / 2.0, 0.0);
  else
    cairo_arc_negative (cr, width, height, radius, -M_PI / 2.0, -M_PI);
}

static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            gint            width,
                            gint            height)
{
  cairo_pattern_t *pat;

  gfloat align = awn_background_get_panel_alignment (bg);
  if (align != 0.0 && align != 1.0) return;

  gboolean bottom_left = align == 0.0;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);

  if (awn_panel_get_composited (bg->panel) == FALSE)
  {
    goto paint_lines;
  }

  /*
   * [0,0]
   *  __
   * |  '-.
   * |     '.
   * |       \
   * |        ;
   * '--------'  [width, height]
   * [0,height]
   */

  /* Draw the background */
  if (bg->enable_pattern && bg->pattern)
  {
    pat = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
  }
  else
  {
    pat = cairo_pattern_create_radial (bottom_left ? 0 : width, height, 1,
        bottom_left ? 0 : width, height, height);
    awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_2);
    awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_1);
  }

  cairo_save (cr);

  draw_path(cr, 0, height - 1.0, width, height, bottom_left);
  cairo_line_to (cr, bottom_left ? 0.0 : width, height);
  cairo_clip (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source (cr, pat);
  cairo_paint (cr);

  cairo_restore (cr);

  cairo_pattern_destroy (pat);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  /* Draw the hi-light */

  pat = cairo_pattern_create_radial (bottom_left ? 0 : width, height,
                                     height * 3/4,
                                     bottom_left ? 0 : width, height,
                                     height);
  double red, green, blue, alpha;
  desktop_agnostic_color_get_cairo_color (bg->g_histep_2, &red, &green, &blue, &alpha);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, red, green, blue, 0.);
  awn_cairo_pattern_add_color_stop_color (pat, 0.2, bg->g_histep_2);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_histep_1);

  draw_path (cr, 0, height * 3/4, width, height, bottom_left);
  if (bottom_left)
    cairo_arc_negative (cr, 0, height, height-2.0, 0.0, -M_PI / 2.0);
  else
    cairo_arc (cr, width, height, height-2.0, -M_PI, -M_PI / 2.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  paint_lines:

  /* Internal border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  draw_path(cr, 0, height - 2.0, width, height, bottom_left);
  cairo_stroke (cr);

  /* External border */
  awn_cairo_set_source_color (cr, bg->border_color);
  draw_path(cr, 0, height - 1.0, width, height, bottom_left);
  cairo_stroke (cr);
}

static
void awn_background_edgy_padding_request (AwnBackground *bg,
                                          GtkPositionType position,
                                          guint *padding_top,
                                          guint *padding_bottom,
                                          guint *padding_left,
                                          guint *padding_right)
{
  /* behave as standard flat bg */
  AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->padding_request (
    bg, position, padding_top, padding_bottom, padding_left, padding_right);

  const gint req = AWN_BACKGROUND_EDGY (bg)->priv->top_pad;

  /* set the top padding for background */
  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = 0; *padding_bottom = req;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = req; *padding_bottom = 0;
      break;
    case GTK_POS_LEFT:
      *padding_left = 0; *padding_right = req;
      break;
    case GTK_POS_RIGHT:
      *padding_left = req; *padding_right = 0;
      break;
    default:
      break;
  }
}

static void
awn_background_edgy_get_strut_offsets (AwnBackground *bg,
                                       GtkPositionType position,
                                       GdkRectangle *area,
                                       gint *strut,
                                       gint *strut_start,
                                       gint *strut_end)
{
  AwnBackgroundEdgy *edgy = AWN_BACKGROUND_EDGY (bg);

  /* FIXME: magic constant!
   *   it's flat bg top padding (for bottom position)
   */
  *strut = edgy->priv->radius - edgy->priv->top_pad +
    AWN_EFFECTS_ACTIVE_RECT_PADDING + 2;
}

static void
awn_background_edgy_translate_for_flat (AwnBackground *bg,
                                        GtkPositionType position,
                                        GdkRectangle *area)
{
  guint top, bot, left, right;

  AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)->padding_request (
    bg, position, &top, &bot, &left, &right);
  const gint modifier = AWN_BACKGROUND_EDGY (bg)->priv->top_pad;

  switch (position)
  {
    case GTK_POS_RIGHT:
      area->x += modifier - left;
      area->width -= modifier - left;
      break;
    case GTK_POS_LEFT:
      area->width -= modifier - right;
      break;
    case GTK_POS_TOP:
      area->height -= modifier - bot;
      break;
    case GTK_POS_BOTTOM:
    default:
      area->y += modifier - top;
      area->height -= modifier - top;
      break;
  }
}

static gboolean
awn_background_edgy_flat_needed (AwnBackground *bg, gint width)
{
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  if (expand || !AWN_BACKGROUND_EDGY (bg)->priv->in_corner)
  {
    return TRUE;
  }
  return width > AWN_BACKGROUND_EDGY (bg)->priv->radius * 4/3;
}

static void 
awn_background_edgy_get_shape_mask (AwnBackground  *bg,
                                    cairo_t        *cr, 
                                    GtkPositionType  position,
                                    GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y, start_x = area->x;
  gint width = area->width, height = area->height;
  const gboolean in_corner = AWN_BACKGROUND_EDGY (bg)->priv->in_corner;

  if (awn_background_edgy_flat_needed (bg, MAX (width, height)))
  {
    GdkRectangle areaf = {x, y, width, height};
    cairo_save (cr);
    awn_background_edgy_translate_for_flat (bg, position, &areaf);
    AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)-> get_shape_mask (
                                                    bg, cr, position, &areaf);
    cairo_restore (cr);
  }
  if (!in_corner)
  {
    return;
  }
  cairo_save (cr);

  switch (position)
  {
    case GTK_POS_RIGHT:
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      start_x = y;
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      start_x = y;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x, y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }
  
  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  gboolean bottom_left = awn_background_get_panel_alignment (bg) == 0.;
  draw_path(cr, start_x, height - 1.0, width, height, bottom_left);
  cairo_line_to (cr, bottom_left ? start_x : width, height);
  cairo_set_source_rgba (cr, 1., 1., 1., 1.);
  cairo_clip (cr);
  cairo_paint (cr);

  cairo_restore (cr);
}

static void 
awn_background_edgy_draw (AwnBackground  *bg,
                          cairo_t        *cr, 
                          GtkPositionType  position,
                          GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;

  if (awn_background_edgy_flat_needed (bg, MAX (width, height)))
  {
    GdkRectangle areaf = {x, y, width, height};
    cairo_save (cr);
    awn_background_edgy_translate_for_flat (bg, position, &areaf);
    AWN_BACKGROUND_CLASS (awn_background_edgy_parent_class)-> draw (
      bg, cr, position, &areaf);
    cairo_restore (cr);
  }

  cairo_save (cr);

  switch (position)
  {
    case GTK_POS_RIGHT:
      height += y;
      cairo_translate (cr, 0., height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      height += y;
      cairo_translate (cr, x + width, 0.);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      width += x;
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      width += x;
      cairo_translate (cr, 0., y);
      break;
  }
  
  draw_top_bottom_background (bg, cr, width, height);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
