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

#define M_PI 3.14159265

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
  {
    case AWN_ORIENTATION_TOP:
      state = ROUND_BOTTOM;
      break;
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
  cairo_pattern_t *pat;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.5);

  /* Draw the background */
  pat = cairo_pattern_create_linear (0, y, 0, y+height);
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

  if (orient == AWN_ORIENTATION_TOP)
    draw_rect (bg, cr, orient, x+1, y, width-2, height-1);
  else
    draw_rect (bg, cr, orient, x+1, y+1, width-2, height-1);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_linear (0, y, 0, y + (height/3));
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

  if (orient == AWN_ORIENTATION_TOP)
    draw_rect (bg, cr, orient, x+1, y, width-2, height/3);
  else
    draw_rect (bg, cr, orient, x+1, y+1, width-2, height/3);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Internal border */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  if (orient == AWN_ORIENTATION_TOP)
      draw_rect (bg, cr, orient, x+1, y-2, width-3, height+1);
  else
      draw_rect (bg, cr, orient, x+1, y+1, width-3, height+3);
  cairo_stroke (cr);

  /* External border */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  if (orient == AWN_ORIENTATION_TOP)
      draw_rect (bg, cr, orient, x, y-3, width-1, height+3);
  else
      draw_rect (bg, cr, orient, x, y,  width-1, height+3);
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
      cairo_translate (cr, width/2, height/2);
      cairo_rotate (cr, 270 * M_PI/180); 
      temp = y;
      y = width/2; x = -height/2;
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_LEFT:
      cairo_translate (cr, width/2, height/2);
      cairo_rotate (cr, 90 * M_PI/180);
      temp = y;
      y = -width/2; x = -height/2;
      temp = width;
      width = height; height = temp;
      break;
    default:
      break;
  }

  draw_top_bottom_background (bg, cr, orient, x, y, width, height);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
