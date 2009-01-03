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
 *
 */

#include "config.h"

#include <gdk/gdk.h>
#include <libawn/awn-config-client.h>

#include "awn-background-flat.h"
#include "awn-x.h"

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif

G_DEFINE_TYPE (AwnBackgroundFlat, awn_background_flat, AWN_TYPE_BACKGROUND)

static void awn_background_flat_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      AwnOrientation  orient,
                                      gdouble         x,
                                      gdouble         y,
                                      gint            width,
                                      gint            height);


static void
awn_background_flat_class_init (AwnBackgroundFlatClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  bg_class->draw = awn_background_flat_draw;
}


static void
awn_background_flat_init (AwnBackgroundFlat *bg)
{
  ;
}

AwnBackground * 
awn_background_flat_new (AwnConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_FLAT,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}


/*
 * Drawing functions
 */
static void 
draw_rect (AwnBackground  *bg,
           cairo_t        *cr, 
           AwnOrientation  orient,
           gdouble         x,
           gdouble         y,
           gint            width,
           gint            height)
{
  AwnCairoRoundCorners state = ROUND_NONE;

  switch (orient)
  {/*
    case AWN_ORIENTATION_LEFT:
      state = ROUND_RIGHT;
      break;
    case AWN_ORIENTATION_RIGHT:
      state = ROUND_LEFT;
      break;
    case AWN_ORIENTATION_TOP:
      state = ROUND_BOTTOM;
      break;*/
    default:
      state = ROUND_TOP;
  }
  awn_cairo_rounded_rect (cr, x, y, width, height, bg->corner_radius, state);
}

static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            AwnOrientation  orient,
                            gdouble         x,
                            gdouble         y,
                            gint            width,
                            gint            height)
{
  cairo_matrix_t matrix;
  cairo_pattern_t *pat;

  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  // so look to the translation part of the matrix and add till it gets 0.5
  cairo_get_matrix (cr, &matrix);
  if(orient == AWN_ORIENTATION_RIGHT || orient == AWN_ORIENTATION_LEFT)
      cairo_translate (cr, 0.5 - matrix.y0 + (int)matrix.y0, 0.5 - matrix.x0 + (int)matrix.x0);
  else
      cairo_translate (cr, 0.5 - matrix.x0 + (int)matrix.x0, 0.5 - matrix.y0 + (int)matrix.y0);

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  /* Draw the background */
  pat = cairo_pattern_create_linear (0, -height, 0, 0);
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

  //draw_rect (bg, cr, orient, x+1, y+1, width-2, height-1);
  draw_rect (bg, cr, orient, -width/2.0+1, -height+1, width-2, height-1);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_linear (0, -height, 0, -height + (height/3.0));
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

  //draw_rect (bg, cr, orient, x+1, y+1, width-2, height/3);
  draw_rect (bg, cr, orient, -width/2.0+1, -height+1, width-2, height/3.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Internal border */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  //draw_rect (bg, cr, orient, x+1, y+1, width-3, height+3);
  draw_rect (bg, cr, orient, -width/2.0+1, -height+1, width-3, height+3);
  cairo_stroke (cr);

  /* External border */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  //draw_rect (bg, cr, orient, x, y,  width-1, height+3);
  draw_rect (bg, cr, orient, -width/2.0, -height, width-1, height+3);
  cairo_stroke (cr);
}

static void 
awn_background_flat_draw (AwnBackground  *bg,
                          cairo_t        *cr, 
                          AwnOrientation  orient,
                          gdouble         x,
                          gdouble         y,
                          gint            width,
                          gint            height)
{
  gint temp;
  cairo_save (cr);

  switch (orient)
  {
    case AWN_ORIENTATION_RIGHT:
      cairo_translate (cr, x + width, y + height/2.0);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_LEFT:
      cairo_translate (cr, x, y + height/2.0);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_TOP:
      cairo_translate (cr, x + width/2.0, y);
      cairo_rotate (cr, M_PI);
      break;
    default:
      cairo_translate (cr, x + width/2.0, y + height - 1);
      break;
  }

  height += 1;

  draw_top_bottom_background (bg, cr, orient, 0, 0, width, height);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
