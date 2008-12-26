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

#include "awn-background-3d.h"
#include "awn-x.h"

#include <math.h>

G_DEFINE_TYPE (AwnBackground3d, awn_background_3d, AWN_TYPE_BACKGROUND)

static void awn_background_3d_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      AwnOrientation  orient,
                                      gdouble         x,
                                      gdouble         y,
                                      gint            width,
                                      gint            height);


static void
awn_background_3d_class_init (AwnBackground3dClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  bg_class->draw = awn_background_3d_draw;
}


static void
awn_background_3d_init (AwnBackground3d *bg)
{
  ;
}

AwnBackground * 
awn_background_3d_new (AwnConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_3D,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}


/**
 * apply_perspective_x()
 * @param width: the current width of the bar
 * @param angle: the angle the bar will have
 * @param x: the x pos in the flat dimension
 * @param y: the y pos in the flat dimension
 * @return: gives the x position back of the point (x,y) in the flat dimension brought back in the perspective
 * 
 * note: - the bottom left corner is (0,0)
 *       - the bottom right corner should be (width, 0)
 */
static double
apply_perspective_x( double width, double angle, double x, double y )
{
		return (width/2-x)/(width/2*tan((90-angle)*M_PI/180))*y+x;
}

/**
 * get_width_on_height()
 * @param width: the current width of the bar
 * @param angle: the angle the bar will have
 * @param y: the y pos in the flat dimension
 * @return: gives back the width of the bar on the given y-pos in pixels 
 */
static double
get_width_on_height( double width, double angle, double y )
{
		return width-2*y/tan((90-angle)*M_PI/180);
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
           gint            height,
           gint            offset)
{
  double x0, x1, x2, x3, y0, y1, y2, y3;

  /* Carefull: here (0,0) is in the top left corner of the screen
   *
   *     (x1,y1)  __________________  (x2,y2)
   *             /                  \
   *            /                    \
   *  (x0,y0)  /______________________\  (x3,y3)
   */

  x0 = x + apply_perspective_x(width, bg->panel_angle, offset, offset);
  x1 = x + apply_perspective_x(width, bg->panel_angle, offset, height/2-offset);
  x2 = x + apply_perspective_x(width, bg->panel_angle, width-offset, height/2-offset);
  x3 = x + apply_perspective_x(width, bg->panel_angle, width-offset, offset);

  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      y0 = y + offset;
      y1 = y + height/2 - offset;
      y2 = y + height/2 - offset;
      y3 = y + offset;
      break;
    default:
      y0 = y + height - offset;
      y1 = y + height/2 + offset;
      y2 = y + height/2 + offset;
      y3 = y + height - offset;
  }

  cairo_move_to(cr, x0, y0);
  cairo_line_to(cr, x1, y1);
  cairo_line_to(cr, x2, y2);
  cairo_line_to(cr, x3, y3);

  cairo_close_path(cr);
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
  //TODO: set padding in background properties
  gint padding = 3;

  if(orient!=AWN_ORIENTATION_TOP)
    y-= 2+padding;
  else
    y += padding;

  cairo_pattern_t *pat;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.5);

  /* Draw the side of the bar */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  if(orient==AWN_ORIENTATION_TOP)
  	cairo_rectangle (cr, x, y-padding-0.5, width, padding);
  else
  	cairo_rectangle (cr, x, y+height+0.5, width, padding);
  cairo_fill(cr);

  /* Draw the background */
  pat = cairo_pattern_create_linear (0, y, 0, y+height);
  cairo_pattern_add_color_stop_rgba (pat, (orient==AWN_ORIENTATION_TOP)?1.0:0.0, 
                                     bg->g_step_1.red,
                                     bg->g_step_1.green,
                                     bg->g_step_1.blue,
                                     bg->g_step_1.alpha);
  cairo_pattern_add_color_stop_rgba (pat, (orient==AWN_ORIENTATION_TOP)?0.0:1.0,
                                     bg->g_step_2.red, 
                                     bg->g_step_2.green,
                                     bg->g_step_2.blue, 
                                     bg->g_step_2.alpha);

  draw_rect (bg, cr, orient, x, y, width, height, 1);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_linear (0, y+height/3, 0, y+height*2/3);
  cairo_pattern_add_color_stop_rgba (pat, (orient==AWN_ORIENTATION_TOP)?1.0:0.0, 
                                     bg->g_histep_1.red,
                                     bg->g_histep_1.green,
                                     bg->g_histep_1.blue,
                                     bg->g_histep_1.alpha);
  cairo_pattern_add_color_stop_rgba (pat, (orient==AWN_ORIENTATION_TOP)?0.0:1.0,
                                     bg->g_histep_2.red, 
                                     bg->g_histep_2.green,
                                     bg->g_histep_2.blue, 
                                     bg->g_histep_2.alpha);

  if (orient == AWN_ORIENTATION_TOP)
    draw_rect (bg, cr, orient, x+apply_perspective_x(width, bg->panel_angle, 0, height/3), y+height/3+2, get_width_on_height(width, bg->panel_angle, height/3), height/3, 1.5);
  else
    draw_rect (bg, cr, orient, x+apply_perspective_x(width, bg->panel_angle, 0, height/3), y+height/3, get_width_on_height(width, bg->panel_angle, height/3), height/3, 1.5);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Internal border */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  draw_rect (bg, cr, orient, x, y, width, height, 1);
  cairo_stroke (cr);

  /* External border */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
   draw_rect (bg, cr, orient, x, y,  width, height, 0.5);
   cairo_stroke (cr);
}

static void 
awn_background_3d_draw (AwnBackground  *bg,
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
