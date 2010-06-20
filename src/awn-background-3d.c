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
 *  Original Author : h4writer <hv1989@gmail.com>
 *  Rewrited by: Alberto Aldegheri <albyrock87+dev@gmail.com>
 *
 */

#include "config.h"

#include <libawn/awn-cairo-utils.h>

#include "awn-background-3d.h"
#include "awn-x.h"

#include <math.h>
#include "awn-applet-manager.h"

/* The pixels to draw the side of the panel INTEGER*/
#define SIDE_SPACE 8
#define PADDING_BOTTOM 1

/* Some defines for debugging */
#define DRAW_INTERNAL_BORDER            TRUE
#define DRAW_EXTERNAL_BORDER            TRUE
#define DRAW_HIGHLIGHT                  TRUE

#define DEBUG_DRAW_INPUT_SHAPE_MASK           FALSE

#define TRANSFORM_RADIUS(x) (x / 90. * 75)

G_DEFINE_TYPE (AwnBackground3d, awn_background_3d, AWN_TYPE_BACKGROUND)

struct _Point3
{
  float x;
  float y;
  float z;
};
typedef struct _Point3 Point3;
/* FORWARDS */
static void awn_background_3d_padding_request (AwnBackground *bg,
                                               GtkPositionType position,
                                               guint *padding_top,
                                               guint *padding_bottom,
                                               guint *padding_left,
                                               guint *padding_right);

static void awn_background_3d_draw (AwnBackground  *bg,
                                    cairo_t        *cr,
                                    GtkPositionType  position,
                                    GdkRectangle   *area);

static void awn_background_3d_input_shape_mask (AwnBackground  *bg,
                                                cairo_t        *cr,
                                                GtkPositionType  position,
                                                GdkRectangle   *area);

static void awn_background_3d_update_padding (AwnBackground *bg);

static void
awn_background_3d_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_3d_parent_class)->constructed (object);

  AwnBackground3d *bg = AWN_BACKGROUND_3D (object);
  gpointer panel = AWN_BACKGROUND (object)->panel;

  g_return_if_fail (panel);

  g_signal_connect_swapped (panel, "notify::size",
                            G_CALLBACK (awn_background_3d_update_padding),
                            object);
  g_signal_connect_swapped (panel, "notify::offset",
                            G_CALLBACK (awn_background_3d_update_padding),
                            object);

  g_signal_connect (bg, "notify::panel-angle", 
                    G_CALLBACK (awn_background_3d_update_padding), 
                    NULL);

  g_signal_connect (bg, "notify::corner-radius", 
                    G_CALLBACK (awn_background_3d_update_padding), 
                    NULL);

  g_signal_connect (bg, "notify::floaty-offset", 
                    G_CALLBACK (awn_background_3d_update_padding), 
                    NULL);
}

static void
awn_background_3d_dispose (GObject *object)
{
  gpointer panel = AWN_BACKGROUND (object)->panel;
  AwnBackground3d *bg = AWN_BACKGROUND_3D (object);

  g_signal_handlers_disconnect_by_func (bg,
      G_CALLBACK (awn_background_3d_update_padding), NULL);

  if (panel)
  {
    g_signal_handlers_disconnect_by_func (panel,
        G_CALLBACK (awn_background_3d_update_padding), object);
  }

  G_OBJECT_CLASS (awn_background_3d_parent_class)->dispose (object);
}

static void
awn_background_3d_class_init (AwnBackground3dClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);

  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  obj_class->constructed  = awn_background_3d_constructed;
  obj_class->dispose = awn_background_3d_dispose;

  bg_class->draw = awn_background_3d_draw;
#if DEBUG_DRAW_INPUT_SHAPE_MASK
  bg_class->draw = awn_background_3d_input_shape_mask;
#endif

  bg_class->padding_request = awn_background_3d_padding_request;
  bg_class->get_input_shape_mask = awn_background_3d_input_shape_mask;
}


static void
awn_background_3d_init (AwnBackground3d *bg)
{

}

AwnBackground * 
awn_background_3d_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_3D,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}

/**
 * awn_background_3d_update_padding:
 * @param bg: AwnBackground
 * 
 * This function get's called when the padding should get recalculated.
 * It asks the awn_background to emit a signal that the padding has changed.
 */
static void 
awn_background_3d_update_padding (AwnBackground *bg)
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
 * Calculates the x position of the point (x,y) in the flat dimension 
 * brought back in the perspective
 * note: - the bottom left corner is (0,0)
 *       - the bottom right corner should be (width, 0)
 *
 * Returns: gives the x position back of the point (x,y) in the flat dimension
 * brought back in the perspective
 */
static double
apply_perspective_x( double width, double angle, double x, double y )
{
		return (width/2.0-x)/(width/2.0*tan((90-angle)*M_PI/180))*y+x;
}

/**
 * cubic_bezier_curve:
 * @param point1: control point 1
 * @param point2: control point 2
 * @param point3: control point 3
 * @param point4: control point 4
 * @param t: the position between O and 1 
 *          (0 gives back point1, 1 gives back point4)
 *
 * This function is used to get back the left most position of the rounded
 * corner to let the side of the bar begin there.
 *
 * Note: Not used atm. Can be I need it later on, but for now it's commented out
 *
 * Returns: gives back the position of the cubic bezier curve constructed with
 *          these control points 
 */
/*static double cubic_bezier_curve(double point1, double point2, double point3,
                                   double point4, double t)
{
	return (1-t)*(1-t)*(1-t)*point1 + 3*t*(1-t)*(1-t)*point2 + 
	                        3*t*t*(1-t)*point3 + t*t*t*point4;
}*/

/**
 * calc_points
 * @param bg: AwnBackground
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Calculates vertices to draw the path
 */
static Point3*
calc_points (AwnBackground  *bg,
             float           x,
             float           y,
             float           width,
             float           height)
{
  float xp0, xp1, xp2, xp3, yp0, yp1, yp2, yp3;
  float radius = bg->corner_radius;
  /**
   * rb = decrease the radius on bottom vertices
   * ex = increase the x-radius on top vertices
   */
  float rb = 0., ex = 0.;
  /* calc sin and cos for current angle */
  float s = sin (TRANSFORM_RADIUS (bg->panel_angle) * M_PI / 180.);
  float c = cos (TRANSFORM_RADIUS (bg->panel_angle) * M_PI / 180.);

  /* Adjust radius */
  if (radius > height)
  {
    ex = radius - height;
    radius = height;
  }
  if (radius > height / 2.)
  {
    rb = 2. * radius - height;
  }
  if (radius == 0.)
  {
    /* for nice edges */
    radius = 0.5;
  }

  /* Carefull: here (0,0) is in the top left corner of the screen
   */
  /*        0   1                   2    3
   *    A   x0  x1                  x2  x3     B
   *        |___|___________________|___|__y0
   *   11   |                           |__y1     4
   *        |                           |
   *   10   |                           |__y2     5
   *        |___________________________|__y3     6
   *    D   9   8                   7          C
   *
   * WARNING!! When draw the path, remember that ->
   *                 A becomes D
   *                 B becomes C
   */
  xp0 = 0.;
  xp1 = radius;
  xp2 = width - radius;
  xp3 = width;

  yp0 = 0.;
  yp1 = radius;
  yp2 = height - radius;
  yp3 = height;

  Point3 *vertices = (Point3 *) malloc (sizeof (Point3) * 12); 
  float z = 2; 
  vertices[0] = (Point3){xp0, yp0, z};
  vertices[1] = (Point3){xp1 - rb, yp0, z};
  vertices[2] = (Point3){xp2 + rb, yp0, z};
  vertices[3] = (Point3){xp3, yp0, z};
  vertices[4] = (Point3){xp3, yp1 - rb, z};
  vertices[5] = (Point3){xp3, yp2, z};
  vertices[6] = (Point3){xp3, yp3, z};
  vertices[7] = (Point3){xp2 - ex, yp3, z};
  vertices[8] = (Point3){xp1 + ex, yp3, z};
  vertices[9] = (Point3){xp0, yp3, z};
  vertices[10] = (Point3){xp0, yp2, z};
  vertices[11] = (Point3){xp0, yp1 - rb, z};

  int i = 0;

  for (; i < 12; ++i)
  {
    /* 3D ROTATION OVER X AXIS */
    vertices[i].y = c * vertices[i].y - s * vertices[i].z;
    vertices[i].z = s * vertices[i].y + c * vertices[i].z;

    /* 3D to 2D - For now, use apply_perspective_x
     * maybe in the future we use projection matrix
     */
    vertices[i].x = apply_perspective_x (width, 
                                         TRANSFORM_RADIUS(bg->panel_angle),
                                         vertices[i].x,
                                         vertices[i].y ) + x;
    /* Invert coordinates for our Y coordinate system */
    vertices[i].y = height - vertices[i].y + y;
  }
  /* use vertices[8]->y to find the top coordinate of the panel */
  return vertices;
} 
/**
 * draw_rect_path:
 * @param cr: a cairo context 
 * @param vertices: vertices to use for drawing 
 * @param padding: padding from top
 * 
 * This function draws the path of the bar in perspective.
 */
static void 
draw_rect_path (cairo_t *cr, Point3 *vertices, float padding)
{

  /* Let's make the path '*/
  cairo_new_path (cr);
  cairo_move_to (cr,  vertices[1].x, vertices[1].y + padding);

  cairo_line_to (cr,  vertices[2].x, vertices[2].y + padding);
  cairo_curve_to (cr, vertices[2].x, vertices[2].y + padding,
                      vertices[3].x, vertices[3].y + padding,
                      vertices[4].x, vertices[4].y + padding);

  cairo_line_to (cr,  vertices[5].x, vertices[5].y + padding);
  cairo_curve_to (cr, vertices[6].x, vertices[6].y + padding,
                      vertices[7].x, vertices[7].y + padding,
                      vertices[7].x, vertices[7].y + padding);

  cairo_line_to (cr,  vertices[8].x, vertices[8].y + padding);
  cairo_curve_to (cr, vertices[8].x, vertices[8].y + padding,
                      vertices[9].x, vertices[9].y + padding,
                      vertices[10].x, vertices[10].y + padding);

  cairo_line_to (cr,  vertices[11].x, vertices[11].y + padding);
  cairo_curve_to (cr, vertices[0].x, vertices[0].y + padding,
                      vertices[1].x, vertices[1].y + padding,
                      vertices[1].x, vertices[1].y + padding);
  cairo_close_path(cr);
}

/**
 * draw_top_bottom_background:
 * @param bg: AwnBackground
 * @param cr: a cairo context
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Draws the bar in the bottom position on the cairo context &cr
 * on position x:0, y:0 with the specified &width and &height.
 */
static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            gfloat          width,
                            gfloat          height)
{
  cairo_pattern_t *pat;
  /* adjust height */
  height -= PADDING_BOTTOM + bg->floaty_offset;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  /* Translate for sharp edges and for avoid drawing glitches */
  cairo_translate (cr, 1.5, 0.);
  width -= 2.5;

  /* calc vertices for draw the main path */
  Point3 *vertices = calc_points (bg, 0., 0., width, height);
  /* calc the y coord of the top panel, used for pattern painting */
  float top_y = vertices[8].y;

  /* Save general context */
  cairo_save (cr);
#if DRAW_INTERNAL_BORDER
  /* Internal border (The Side of the 3D panel) */
  /* Draw only if it will be visible */
  float s = sin ((bg->panel_angle - 1) * M_PI / 180.) * SIDE_SPACE;
  if (bg->floaty_offset > floor (s) && bg->panel_angle > 0)
  {
    float i;

    /* calc points on a wider space (for good looking) */
    Point3 *vertices_internal = calc_points (bg, -0.5, 0., width + 1., height);

    /* Draw bottom plane with its own border (Inner Border Color)  */
    draw_rect_path (cr, vertices_internal, ceil(s));
    cairo_save (cr);
    awn_cairo_set_source_color (cr, bg->g_step_2);
    cairo_clip_preserve (cr);
    cairo_paint (cr);
    cairo_set_line_width (cr, 3.);
    awn_cairo_set_source_color (cr, bg->hilight_color);
    cairo_stroke (cr);
    cairo_set_line_width (cr, 1.0);
    cairo_restore (cr);

    /* Pixel per Pixel draw each plane without border from bottom to top */
    for (i = floor (s); i > 0. ; i -= 1.)
    {
      draw_rect_path (cr, vertices_internal, i);
      cairo_save (cr);
      awn_cairo_set_source_color (cr, bg->g_step_2);
      cairo_clip (cr);
      cairo_paint (cr);
      cairo_restore (cr);
    }
    free (vertices_internal);

    /* Clear the area for the top plane*/
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    draw_rect_path (cr, vertices, 0.);
    cairo_save (cr);
    cairo_set_source_rgba (cr, 0., 0., 0., 1.);
    cairo_clip_preserve (cr);
    cairo_paint (cr);
    cairo_restore (cr);
  }
#endif
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  /* Draw the path of top plane */
  draw_rect_path (cr, vertices, 0.);
  /* obtain 0.0 - 1.0 relative height for pattern drawing */
  top_y = top_y / height;

  /* Paint the top plane gradient */
  pat = cairo_pattern_create_linear (0, 0, 0, height);
  awn_cairo_pattern_add_color_stop_color (pat, top_y, bg->g_step_1);
  awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_pattern_destroy (pat);
  cairo_restore (cr);

  if (bg->enable_pattern && bg->pattern)
  {
    /* Paint the top plane pattern */
    cairo_save (cr);
    pat = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
    cairo_clip_preserve (cr);
    cairo_set_source (cr, pat);
    cairo_paint (cr);
    cairo_pattern_destroy (pat);
    cairo_restore (cr);
  }

#if DRAW_HIGHLIGHT
  /* Prepare the hi-light */
  pat = cairo_pattern_create_linear (0., 0., 0., height);
  awn_cairo_pattern_add_color_stop_color
                        (pat, top_y + 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color
                        (pat, top_y + (1. - top_y) * 0.3, bg->g_histep_2);
  double red, green, blue, alpha;
  desktop_agnostic_color_get_cairo_color
                        (bg->g_histep_2, &red, &green, &blue, &alpha);
  cairo_pattern_add_color_stop_rgba
                        (pat, top_y + (1. - top_y) * 0.4, red, green, blue, 0.);
  /* Paint the hi-light gradient */
  cairo_save (cr);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);
  cairo_restore (cr);
  cairo_pattern_destroy (pat);
#endif
#if DRAW_EXTERNAL_BORDER
  /* External border (Thin) */
  cairo_set_line_width (cr, 1.);
  awn_cairo_set_source_color (cr, bg->border_color);
  cairo_stroke (cr);
#endif
  /* restore genereal context */
  cairo_restore (cr);
  /* free memory :) */
  free (vertices);
}

/**
 * _get_applet_manager_size
 * Obtain width and height from applet manager
 */
static void
_get_applet_manager_size (AwnBackground* bg, GtkPositionType position,
                          float *w, float *h)
{
  AwnAppletManager *manager = NULL;
  g_object_get (bg->panel, "applet-manager", &manager, NULL);

  switch (position)
  {
    case GTK_POS_BOTTOM:
    case GTK_POS_TOP:
      *w = GTK_WIDGET (manager)->allocation.width;
      break;
    default:
      *w = GTK_WIDGET (manager)->allocation.height;
      break;
  }
  gint size = 0;
  g_object_get (manager, "size", &size, NULL);
  *h = size;
}

/**
 * awn_background_3d_padding_request:
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
awn_background_3d_padding_request (AwnBackground *bg,
                                   GtkPositionType position,
                                   guint *padding_top,
                                   guint *padding_bottom,
                                   guint *padding_left,
                                   guint *padding_right)
{
  gint offset, size;
  guint padding;
  g_object_get (bg->panel, "offset", &offset, "size", &size, NULL);

  /* Find the coordinate of the (0;0) point in the prospective */
  /* Its X coordinate is equal to the padding */
  float w,h;
  _get_applet_manager_size (bg, position, &w, &h);
  padding = apply_perspective_x
                      ( w, TRANSFORM_RADIUS(bg->panel_angle), 0., h ) / 2.;
  /* Padding > h/2 is not needed */
  if (padding > h / 2.)
  {
    padding = h / 2.;
  }

  /* Obtain the padding from corner radius */
  float padding_from_rad = bg->corner_radius;

  if (padding_from_rad > h)
  {
    padding_from_rad = h;
  }
  if (padding_from_rad > (h / 2.))
  {
    padding_from_rad = ( h - padding_from_rad );
  }
  /* Use biggest padding */
  padding += padding_from_rad;

  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = PADDING_BOTTOM + bg->floaty_offset; *padding_bottom = 0;
      *padding_left = padding; *padding_right = padding;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = 0; *padding_bottom = PADDING_BOTTOM + bg->floaty_offset;
      *padding_left = padding; *padding_right = padding;
      break;
    case GTK_POS_LEFT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = PADDING_BOTTOM + bg->floaty_offset; *padding_right = 0;
      break;
    case GTK_POS_RIGHT:
      *padding_top  = padding; *padding_bottom = padding;
      *padding_left = 0; *padding_right = PADDING_BOTTOM + bg->floaty_offset;
      break;
    default:
      break;
  }
}

/**
 * awn_background_3d_draw:
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
 *
 * Important: every change to this function should get 
 * adjusted in input_shape_mask!!!
 */
static void 
awn_background_3d_draw (AwnBackground  *bg,
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
      cairo_translate (cr, 0., y + height);
      cairo_scale (cr, 1., -1.);
      cairo_translate (cr, x, height);
      cairo_rotate (cr, M_PI * 1.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x, y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }

  draw_top_bottom_background (bg, cr, width, height);

  cairo_restore (cr);
}

/**
 * awn_background_3d_input_shape_mask:
 * @param bg: AwnBackground
 * @param cr: a cairo context 
 * @param position: position of the bar
 * @param x: the begin x position to draw 
 * @param y: the begin y position to draw
 * @param width: the width for the drawing
 * @param height: the height for the drawing
 *
 * Draws the bar in the in the cairo context &cr given the position &position,
 * the &x and &y position and given &width and &height.
 * The bar is only drawn in black for the input shape mask.
 */
static void 
awn_background_3d_input_shape_mask (AwnBackground  *bg,
                                    cairo_t        *cr, 
                                    GtkPositionType  position,
                                    GdkRectangle   *area)
{
  gfloat temp;
  gfloat x = area->x, y = area->y;
  gfloat width = area->width, height = area->height;
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
      break;
    case GTK_POS_LEFT:
      cairo_translate (cr, x + width, y);
      cairo_rotate (cr, M_PI * 0.5);
      temp = width;
      width = height;
      height = temp;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, x, y + height);
      cairo_scale (cr, 1., -1.);
      break;
    default:
      cairo_translate (cr, x, y);
      break;
  }

  height -= PADDING_BOTTOM + bg->floaty_offset;

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_translate (cr, 0.5, 0.);
  width -= 1.5;

  /* Draw the background (in black color) */
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
  /* for shape mask draw only top and bottom plane */
  Point3 *vertices = calc_points (bg, 0., 0., width, height);
  draw_rect_path (cr, vertices, 0.);
  cairo_fill (cr);
  float s = sin (bg->panel_angle * M_PI / 180.) * SIDE_SPACE;
  draw_rect_path (cr, vertices, ceil (s));
  cairo_fill (cr);
  free (vertices);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
