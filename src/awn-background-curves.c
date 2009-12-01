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
 *  Author : h4writer <hv1989@gmail.com>
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-cairo-utils.h>

#include "awn-background-curves.h"

#include <math.h>

G_DEFINE_TYPE (AwnBackgroundCurves, awn_background_curves, AWN_TYPE_BACKGROUND)

/* FORWARDS */
static void awn_background_curves_padding_request (AwnBackground *bg,
                                                   GtkPositionType position,
                                                   guint *padding_top,
                                                   guint *padding_bottom,
                                                   guint *padding_left,
                                                   guint *padding_right);

static AwnPathType awn_background_curves_get_path_type (AwnBackground *bg,
                                                        gfloat *offset_mod);

static void awn_background_curves_draw (AwnBackground  *bg,
                                        cairo_t        *cr,
                                        GtkPositionType  position,
                                        GdkRectangle   *area);

static void
awn_background_curves_constructed (GObject *object)
{
    G_OBJECT_CLASS (awn_background_curves_parent_class)->constructed (object);

    g_signal_connect (object, "notify::curves-symmetry",
                      G_CALLBACK (awn_background_emit_padding_changed), NULL);
}

static void
awn_background_curves_class_init (AwnBackgroundCurvesClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  obj_class->constructed = awn_background_curves_constructed;

  bg_class->draw = awn_background_curves_draw;
  bg_class->padding_request = awn_background_curves_padding_request;
  bg_class->get_input_shape_mask = awn_background_curves_draw;
  bg_class->get_path_type = awn_background_curves_get_path_type;
}


static void
awn_background_curves_init (AwnBackgroundCurves *bg)
{

}

AwnBackground * 
awn_background_curves_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_CURVES,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}

/*
 * Drawing functions
 */

/**
 * draw_rect_path:
 * @param bg: AwnBackground
 * @param cr: a cairo context 
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 * @param padding: makes the path X amount of pixels smaller on every side
 *
 * This function draws the curved path of the bar
 */
static void 
draw_rect_path (AwnBackground  *bg,
                cairo_t        *cr, 
                gdouble         x,
                gdouble         y,
                gint            width,
                gint            height)
{
  cairo_save (cr);

  cairo_translate (cr, x+width/2.0, y);
  cairo_scale (cr, width/2.0, height);

  cairo_move_to (cr, 0.0, 1.0);
  cairo_arc_negative (cr, 0.0, 1.2, 1.0, 0.0, M_PI);

  cairo_restore (cr);
}

/**
 * draw_top_bottom_background:
 * @param bg: AwnBackground
 * @param cr: a cairo context
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Draws the bar in the bottom position on the cairo context &cr given 
 * the &x position, &y position, &width and &height.
 */
static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            gdouble         x,
                            gdouble         y,
                            gint            width,
                            gint            height)
{
  cairo_pattern_t *pat;
  cairo_matrix_t matrix;
  gdouble curves_height;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.5);
  width -= 1;
  curves_height = height;
  if (bg->curviness < 1.0)
  {
    curves_height = height*bg->curviness;
  }

  /* Drawing outer ellips */
  if (bg->enable_pattern && bg->pattern)
  {
    pat = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
  }
  else
  {
    // we need pat_xscale for scaling the spherical gradient to ellipse...
    // why * 1.5 ? because it looks better :)
    const gdouble pat_xscale = curves_height / width * 1.5;

    pat = cairo_pattern_create_radial (width / 2.0 * pat_xscale, height,
                                       0.01,
                                       width / 2.0 * pat_xscale, height,
                                       curves_height);
    awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_2);
    awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_1);

    // scale to ellipse
    cairo_matrix_init_scale (&matrix, pat_xscale, 1.0);
    cairo_pattern_set_matrix (pat, &matrix);
  }

  cairo_save (cr);

  draw_rect_path (bg, cr, 0, height-curves_height, width, curves_height);
  cairo_clip (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);

  cairo_restore (cr);

  cairo_pattern_destroy (pat);

  /* Internal border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  draw_rect_path (bg, cr, 1.0, height-curves_height + 2.0,
                  width-2.0, curves_height-2.0);
  cairo_stroke (cr);

  /* External border */
  awn_cairo_set_source_color (cr, bg->border_color);
  draw_rect_path (bg, cr, 0.0, height-curves_height, width, curves_height);
  cairo_stroke (cr);

  /* Drawing inner ellips */
  gint width_inner = width * 3 / 4;
  const gdouble inner_pat_xscale = curves_height/2.0 / width_inner * 1.5;
  
  gdouble x_pos = (width-width_inner);
  gdouble min = x_pos / 6;
  x_pos = x_pos - min * 2;
  x_pos = x_pos * (bg->curves_symmetry) + min;

  pat = cairo_pattern_create_radial (
    (x_pos + width_inner/2.0) * inner_pat_xscale, height, 0.01,
    (x_pos + width_inner/2.0) * inner_pat_xscale, height, curves_height/2.0
  );
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_histep_2);

  // scale to ellipse
  cairo_matrix_init_scale (&matrix, inner_pat_xscale, 1.0);
  cairo_pattern_set_matrix (pat, &matrix);

  draw_rect_path (bg, cr, x_pos, height-curves_height/2.0,  width_inner, curves_height/2.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr); 
  cairo_pattern_destroy (pat);
}

/**
 * awn_background_curves_padding_request:
 * @param bg: AwnBackground
 * @param position: the position of the bar
 * @param padding_top: the top padding 
 * @param padding_bottom: the bottom padding
 * @param padding_left: the left padding
 * @param padding_right: the right padding
 *
 * Gives back the padding the background needs for drawing.
 * The values get returned through &padding_top, &padding_bottom,
 * &padding_left and &padding_right.
 */
static void 
awn_background_curves_padding_request (AwnBackground *bg,
                                   GtkPositionType position,
                                   guint *padding_top,
                                   guint *padding_bottom,
                                   guint *padding_left,
                                   guint *padding_right)
{
  gfloat left = bg->curves_symmetry;
  gfloat right = 1.0 - bg->curves_symmetry;
  const gint BASE_PADDING = 20;
  const gint INC_PADDING = 75;

  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = 0; *padding_bottom = 0;
      *padding_left = BASE_PADDING + INC_PADDING * right;
      *padding_right = BASE_PADDING + INC_PADDING * left;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = 0; *padding_bottom = 0;
      *padding_left = BASE_PADDING + INC_PADDING * left;
      *padding_right = BASE_PADDING + INC_PADDING * right;
      break;
    case GTK_POS_LEFT:
      *padding_top  = BASE_PADDING + INC_PADDING * left;
      *padding_bottom = BASE_PADDING + INC_PADDING * right;
      *padding_left = 0; *padding_right = 0;
      break;
    case GTK_POS_RIGHT:
      *padding_top  = BASE_PADDING + INC_PADDING * right;
      *padding_bottom = BASE_PADDING + INC_PADDING * left;
      *padding_left = 0; *padding_right = 0;
      break;
    default:
      break;
  }
}

static
AwnPathType awn_background_curves_get_path_type (AwnBackground *bg,
                                                 gfloat *offset_mod)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND (bg) && offset_mod, AWN_PATH_LINEAR);

  *offset_mod = 20.0;
  return AWN_PATH_ELLIPSE;
}

/**
 * awn_background_curves_draw:
 * @param bg: AwnBackground
 * @param cr: a cairo context 
 * @param position: position of the bar
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Draws the bar in the in the cairo context &cr given the position &position,
 * the &x and &y position and given &width and &height
 */
static void 
awn_background_curves_draw (AwnBackground  *bg,
                        cairo_t        *cr, 
                        GtkPositionType  position,
                        GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  cairo_save (cr);

  switch (position)
  {
    case GTK_POS_RIGHT:
      cairo_translate (cr, x, y+height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height; height = temp;
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x+width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height; height = temp;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x+width, y+height);
      cairo_rotate (cr, M_PI);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }

  draw_top_bottom_background (bg, cr, 0, 0, width, height);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
