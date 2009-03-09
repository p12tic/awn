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
#include <libawn/awn-config-client.h>

#include "awn-background-curves.h"

#include <math.h>

G_DEFINE_TYPE (AwnBackgroundCurves, awn_background_curves, AWN_TYPE_BACKGROUND)

/* FORWARDS */
static void awn_background_curves_padding_request (AwnBackground *bg,
                                                   AwnOrientation orient,
                                                   guint *padding_top,
                                                   guint *padding_bottom,
                                                   guint *padding_left,
                                                   guint *padding_right);

static void awn_background_curves_draw (AwnBackground  *bg,
                                        cairo_t        *cr,
                                        AwnOrientation  orient,
                                        GdkRectangle   *area);

static void
awn_background_curves_class_init (AwnBackgroundCurvesClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  bg_class->draw = awn_background_curves_draw;
  bg_class->padding_request = awn_background_curves_padding_request;
  bg_class->get_input_shape_mask = awn_background_curves_draw;
}


static void
awn_background_curves_init (AwnBackgroundCurves *bg)
{

}

AwnBackground * 
awn_background_curves_new (AwnConfigClient *client, AwnPanel *panel)
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
                gint            height,
                gint            padding)
{
  cairo_save (cr);

  cairo_translate (cr, x+width/2.0, y);
  cairo_scale (cr, width/2.0, height);

  cairo_move_to (cr, 0.0, 1.0);
  cairo_arc_negative (cr, 0.0, 1.0, 1.0, 0.0, M_PI);

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
 * Draws the bar in the bottom orientation on the cairo context &cr given 
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
  gdouble curves_height;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.5);
  width -= 1;
  curves_height = height;
  if(bg->curviness < 1)
    curves_height = height*bg->curviness;

  /* Drawing outer ellips */
  pat = cairo_pattern_create_linear (0, height-curves_height, 0, height);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 
                                     bg->g_step_1.red,
                                     bg->g_step_1.green,
                                     bg->g_step_1.blue,
                                     bg->g_step_1.alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     bg->g_step_2.red, 
                                     bg->g_step_2.green,
                                     bg->g_step_2.blue, 
                                     bg->g_step_2.alpha);

  draw_rect_path (bg, cr, 0, height-curves_height, width, curves_height, 0.5);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* External border */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  draw_rect_path (bg, cr, 0, height-curves_height,  width, curves_height, 0.5);
  cairo_stroke (cr);

  /* Drawing inner ellips */
  gint width_inner = width*(1-bg->curves_symmetry*0.2);
  pat = cairo_pattern_create_linear (0, height-curves_height/2.0, 0, height);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 
                                     bg->g_histep_1.red,
                                     bg->g_histep_1.green,
                                     bg->g_histep_1.blue,
                                     bg->g_histep_1.alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0,
                                     bg->g_histep_2.red, 
                                     bg->g_histep_2.green,
                                     bg->g_histep_2.blue, 
                                     bg->g_histep_2.alpha);
  draw_rect_path (bg, cr, (width-width_inner)*bg->curves_symmetry*0.8, height-curves_height/2.0,  width_inner, curves_height/2.0, 0.5);
  cairo_set_source (cr, pat);
  cairo_fill (cr); 
  cairo_pattern_destroy (pat);
}

/**
 * awn_background_curves_padding_request:
 * @param bg: AwnBackground
 * @param orient: the orientation of the bar
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
                                   AwnOrientation orient,
                                   guint *padding_top,
                                   guint *padding_bottom,
                                   guint *padding_left,
                                   guint *padding_right)
{
  gint padding = 20;

  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      *padding_top  = 0; *padding_bottom = 0;
      *padding_left = padding; *padding_right = padding;
      break;
    case AWN_ORIENTATION_BOTTOM:
      *padding_top  = 0; *padding_bottom = 0;
      *padding_left = padding; *padding_right = padding;
      break;
    case AWN_ORIENTATION_LEFT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = 0; *padding_right = 0;
      break;
    case AWN_ORIENTATION_RIGHT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = 0; *padding_right = 0;
      break;
    default:
      break;
  }
}

/**
 * awn_background_curves_draw:
 * @param bg: AwnBackground
 * @param cr: a cairo context 
 * @param orient: orientation of the bar
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Draws the bar in the in the cairo context &cr given the orientation &orient,
 * the &x and &y position and given &width and &height
 */
static void 
awn_background_curves_draw (AwnBackground  *bg,
                        cairo_t        *cr, 
                        AwnOrientation  orient,
                        GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  cairo_save (cr);

  switch (orient)
  {
    case AWN_ORIENTATION_RIGHT:
      cairo_translate (cr, x, y+height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_LEFT:
      cairo_translate (cr, x+width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_TOP:
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
