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

/* The pixels to draw the side of the panel*/
#define SIDE_SPACE 4

/* Some defines for debugging */
#define DEBUG_DRAW_INTERNAL_BORDER_BOTTOM     TRUE
#define DEBUG_DRAW_EXTERNAL_BORDER_BOTTOM     TRUE
#define DEBUG_DRAW_SIDE                       TRUE
#define DEBUG_DRAW_INTERNAL_BORDER_TOP        TRUE
#define DEBUG_DRAW_EXTERNAL_BORDER_TOP        TRUE
#define DEBUG_DRAW_HIGHLIGHT                  TRUE

G_DEFINE_TYPE (AwnBackground3d, awn_background_3d, AWN_TYPE_BACKGROUND)

/* FORWARDS */
static void awn_background_3d_padding_request (AwnBackground *bg,
                                               AwnOrientation orient,
                                               guint *padding_top,
                                               guint *padding_bottom,
                                               guint *padding_left,
                                               guint *padding_right);

static void awn_background_3d_draw (AwnBackground  *bg,
                                    cairo_t        *cr,
                                    AwnOrientation  orient,
                                    GdkRectangle   *area);

static void awn_background_3d_bar_angle_changed (AwnBackground *bg);


static void
awn_background_3d_class_init (AwnBackground3dClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  bg_class->draw = awn_background_3d_draw;
  bg_class->padding_request = awn_background_3d_padding_request;
  bg_class->get_input_shape_mask = awn_background_3d_draw;
}


static void
awn_background_3d_init (AwnBackground3d *bg)
{
  g_signal_connect(bg, "notify::panel-angle", G_CALLBACK(awn_background_3d_bar_angle_changed), NULL);
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
 * awn_background_3d_bar_angle_changed:
 * @param bg: AwnBackground
 * 
 * This function get's called when the bar_angle has changed.
 * It asks the awn_background to emit a signal that the padding has changed.
 */
static void 
awn_background_3d_bar_angle_changed (AwnBackground *bg)
{
  awn_background_emit_padding_changed(bg);
}

/**
 * apply_perspective_x:
 * @param width: the current width of the bar
 * @param angle: the angle the bar will have
 * @param x: the x pos in the flat dimension
 * @param y: the y pos in the perspective
 * 
 * Calculates the x position of the point (x,y) in the flat dimension brought back in the perspective
 * note: - the bottom left corner is (0,0)
 *       - the bottom right corner should be (width, 0)
 *
 * Returns: gives the x position back of the point (x,y) in the flat dimension brought back in the perspective
 */
static double
apply_perspective_x( double width, double angle, double x, double y )
{
		return (width/2-x)/(width/2*tan((90-angle)*M_PI/180))*y+x;
}

/**
 * get_width_on_height:
 * @param width: the current width of the bar
 * @param angle: the angle the bar will have
 * @param y: the y pos in the perspective
 *
 * Calculates the width of the bar on a given y-pos (in the perspective)
 *
 * Returns: a double containing the width
 */
static double
get_width_on_height( double width, double angle, double y )
{
		return width-2*y/tan((90-angle)*M_PI/180);
}

/**
 * cubic_bezier_curve:
 * @param point1: control point 1
 * @param point2: control point 2
 * @param point3: control point 3
 * @param point4: control point 4
 * @param t: the position between O and 1 (0 gives back point1, 1 gives back point4)
 *
 * This function is used to get back the left most position of the rounded corner,
 * to let the side of the bar begin there.
 *
 * - Note: Not used atm. Can be I need it later on, but for now it's commented out.
 *
 * Returns: gives back the position of the cubic bezier curve constructed with these control points 
 */
/*static double cubic_bezier_curve(double point1, double point2, double point3, double point4, double t)
{
	return (1-t)*(1-t)*(1-t)*point1 + 3*t*(1-t)*(1-t)*point2 + 3*t*t*(1-t)*point3 + t*t*t*point4;
}*/


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
 * This function draws the path of the bar in perspective.
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
  double x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11,
         y0, y1, y2, y3;
  double radius = bg->corner_radius;

  /* Carefull: here (0,0) is in the top left corner of the screen
   *     (x3,y3)  (x4,y3)        (x5,y3) (x6,y3)
   *           '-.,  |              | ,.-'
   *     (x2,y2)___.-''''''''''''''''-.___(x7,y2)
   *              /                    \
   *   (x1,y1)___/                      \___(x8,y1)
   *             '.____________________.'
   *  (x0,y0)-'´  |                   | `'-- (x9,y0)
   *           (x11,y0)            (x10,y0)
   */

  x0 = x + apply_perspective_x(width, bg->panel_angle, padding, padding);
  x1 = x + apply_perspective_x(width, bg->panel_angle, padding, radius+padding);
  x2 = x + apply_perspective_x(width, bg->panel_angle, padding, height/2-radius-padding);
  x3 = x + apply_perspective_x(width, bg->panel_angle, padding, height/2-padding);
  x4 = x + apply_perspective_x(width, bg->panel_angle, radius+padding, height/2-padding);
  x5 = x + apply_perspective_x(width, bg->panel_angle, width-radius-padding, height/2-padding);
  x6 = x + apply_perspective_x(width, bg->panel_angle, width-padding, height/2-padding);
  x7 = x + apply_perspective_x(width, bg->panel_angle, width-padding, height/2-radius-padding);
  x8 = x + apply_perspective_x(width, bg->panel_angle, width-padding, radius+padding);
  x9 = x + apply_perspective_x(width, bg->panel_angle, width-padding, padding);
  x10 = x + apply_perspective_x(width, bg->panel_angle, width-radius-padding, padding);
  x11 = x + apply_perspective_x(width, bg->panel_angle, radius+padding, padding);

  y0 = y + height - padding;
  y1 = y + height - radius - padding;
  y2 = y + height/2 + radius + padding;
  y3 = y + height/2 + padding;

  cairo_move_to(cr, x2, y2);
  cairo_curve_to(cr, x3, y3, x3, y3, x4, y3);
  cairo_line_to(cr, x5, y3);
  cairo_curve_to(cr, x6, y3, x6, y3, x7, y2);
  if( x8 > x7 )
  {
    /* draw the rounded corners on the bottom too */
    cairo_line_to(cr, x8, y1);
    cairo_curve_to(cr, x9, y0, x9, y0, x10, y0);
    cairo_line_to(cr, x11, y0);
    cairo_curve_to(cr, x0, y0, x0, y0, x1, y1);
    cairo_line_to(cr, x2, y2);
  }
  else
  {
    /* the radius is to big, so only draw the rounded corners on the top. On the bottom just ordinary corners. */
    cairo_line_to(cr, x9, y0);
    cairo_line_to(cr, x0, y0);
    cairo_line_to(cr, x2, y2);
  }

  cairo_close_path(cr);
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
#if DEBUG_DRAW_SIDE
  int i;
#endif
  cairo_pattern_t *pat;

  height -= SIDE_SPACE;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.5);

#if DEBUG_DRAW_INTERNAL_BORDER_BOTTOM
  /* Internal border (on the bottom) */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  draw_rect_path (bg, cr, 0, SIDE_SPACE, width, height, 1);
  cairo_stroke (cr);
#endif

#if DEBUG_DRAW_EXTERNAL_BORDER_BOTTOM
  /* External border (on the bottom) */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  draw_rect_path (bg, cr, 0, SIDE_SPACE,  width, height, 0.5);
  cairo_stroke (cr);
#endif

#if DEBUG_DRAW_SIDE
  /* draw the side */
  /* TODO: if a side has no rounded corners, the border should be drawn. */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
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
  cairo_set_source (cr, pat);
  for(i=SIDE_SPACE-1; i>0; i--)
  {
    draw_rect_path (bg, cr, 0, i,  width, height, 0.5);
    cairo_stroke (cr);
  }

  cairo_pattern_destroy (pat);
#endif

  /* Draw the background (on the top) */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
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

  draw_rect_path (bg, cr, 0, 0, width, height, 0.5);
  cairo_set_source (cr, pat);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

#if DEBUG_DRAW_HIGHLIGHT
  /* Draw the hi-light (on the top) */
  pat = cairo_pattern_create_linear (0, height/3, 0, height*2/3);
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
  draw_rect_path (bg, cr, apply_perspective_x(width, bg->panel_angle, 0, height/3), height/3, get_width_on_height(width, bg->panel_angle, height/3), height/3, 1.5);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);
#endif

#if DEBUG_DRAW_INTERNAL_BORDER_TOP
  /* Internal border (on the top) */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  draw_rect_path (bg, cr, 0, 0, width, height, 1);
  cairo_stroke (cr);
#endif

#if DEBUG_DRAW_EXTERNAL_BORDER_BOTTOM
  /* External border (on the top) */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
   draw_rect_path (bg, cr, 0, 0,  width, height, 0.5);
   cairo_stroke (cr);
#endif

}

/**
 * awn_background_3d_padding_request:
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
awn_background_3d_padding_request (AwnBackground *bg,
                                   AwnOrientation orient,
                                   guint *padding_top,
                                   guint *padding_bottom,
                                   guint *padding_left,
                                   guint *padding_right)
{
  gint offset, size;
  guint padding;
  g_object_get (bg->panel, "offset", &offset, NULL);
  g_object_get (bg->panel, "size", &size, NULL);

  if(offset > size)
    padding = (size+offset)/2/tan((90-bg->panel_angle)*M_PI/180);
  else
    padding = offset/tan((90-bg->panel_angle)*M_PI/180);

  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      *padding_top  = SIDE_SPACE+2; *padding_bottom = 0;
      *padding_left = padding; *padding_right = padding;
      break;
    case AWN_ORIENTATION_BOTTOM:
      *padding_top  = 0; *padding_bottom = SIDE_SPACE+2;
      *padding_left = padding; *padding_right = padding;
      break;
    case AWN_ORIENTATION_LEFT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = SIDE_SPACE+2; *padding_right = 0;
      break;
    case AWN_ORIENTATION_RIGHT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = 0; *padding_right = SIDE_SPACE+2;
      break;
    default:
      break;
  }
}

/**
 * awn_background_3d_draw:
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
awn_background_3d_draw (AwnBackground  *bg,
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
      cairo_translate (cr, x-2, y+height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_LEFT:
      cairo_translate (cr, x+width+1, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height; height = temp;
      break;
    case AWN_ORIENTATION_TOP:
      cairo_translate (cr, x+width, y+height+1);
      cairo_rotate (cr, M_PI);
      break;
    default:
      cairo_translate (cr, x, y-2);
      break;
  }

  draw_top_bottom_background (bg, cr, 0, 0, width, height);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
