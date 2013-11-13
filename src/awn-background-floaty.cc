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
 *
 */

#include "config.h"

#include <math.h>
#include <libawn/awn-cairo-utils.h>

#include "awn-background-floaty.h"

G_DEFINE_TYPE (AwnBackgroundFloaty, awn_background_floaty, AWN_TYPE_BACKGROUND)

static void awn_background_floaty_draw (AwnBackground  *bg,
                                        cairo_t        *cr,
                                        GtkPositionType  position,
                                        GdkRectangle   *area);

static void awn_background_floaty_get_shape_mask (AwnBackground  *bg,
                                                  cairo_t        *cr,
                                                  GtkPositionType  position,
                                                  GdkRectangle   *area);

static void awn_background_floaty_padding_request (AwnBackground *bg,
                                                   GtkPositionType position,
                                                   guint *padding_top,
                                                   guint *padding_bottom,
                                                   guint *padding_left,
                                                   guint *padding_right);

static void
awn_background_floaty_expand_changed (AwnBackground *bg) // has more params...
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_floaty_constructed (GObject *object)
{
  AwnBackground *bg = AWN_BACKGROUND (object);

  G_OBJECT_CLASS (awn_background_floaty_parent_class)->constructed (object);

  g_signal_connect (object, "notify::corner-radius",
                    G_CALLBACK (awn_background_emit_padding_changed), NULL);

  g_signal_connect (object, "notify::floaty-offset",
                    G_CALLBACK (awn_background_emit_padding_changed), NULL);

  g_signal_connect_swapped (bg->panel, "notify::expand",
                            G_CALLBACK (awn_background_floaty_expand_changed),
                            object);
}

static void
awn_background_floaty_dispose (GObject *object)
{
  g_signal_handlers_disconnect_by_func (AWN_BACKGROUND (object)->panel,
      G_CALLBACK (awn_background_floaty_expand_changed), object);

  G_OBJECT_CLASS (awn_background_floaty_parent_class)->dispose (object);
}

static void
awn_background_floaty_class_init (AwnBackgroundFloatyClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);
  GObjectClass       *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed  = awn_background_floaty_constructed;
  obj_class->dispose = awn_background_floaty_dispose;

  bg_class->draw = awn_background_floaty_draw;
  bg_class->padding_request = awn_background_floaty_padding_request;
  bg_class->get_shape_mask = awn_background_floaty_get_shape_mask;
  bg_class->get_input_shape_mask = awn_background_floaty_get_shape_mask;
}


static void
awn_background_floaty_init (AwnBackgroundFloaty *bg)
{
  ;
}

AwnBackground * 
awn_background_floaty_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_FLOATY,
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
           GtkPositionType position,
           gdouble         x,
           gdouble         y,
           gint            width,
           gint            height,
           gboolean        round_all)
{
  AwnCairoRoundCorners state = round_all ? ROUND_ALL : ROUND_TOP;

  awn_cairo_rounded_rect (cr, x, y, width, height, bg->corner_radius, state);
}

static void
draw_top_bottom_background (AwnBackground  *bg,
                            GtkPositionType position,
                            cairo_t        *cr,
                            gint            width,
                            gint            height)
{
  cairo_pattern_t *pat;
  gint bg_size;
  gboolean expand = FALSE;

  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  cairo_translate (cr, 0.5, 0.5);

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  // eyecandy in expand mode
  g_object_get (bg->panel, "expand", &expand, NULL);
  if (expand)
  {
    gint extra_space = bg->floaty_offset * 3 / 4;
    cairo_translate (cr, extra_space, 0.0);
    width -= 2*extra_space;
  }

  bg_size = height - bg->floaty_offset + 1;

  if (awn_panel_get_composited (bg->panel) == FALSE)
  {
    goto paint_lines;
  }

  /* Draw the background */
  if (bg->enable_pattern && bg->pattern)
  {
    pat = cairo_pattern_create_for_surface (bg->pattern);
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
  }
  else
  {
    pat = cairo_pattern_create_linear (0, 0, 0, bg_size);
    awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_step_1);
    awn_cairo_pattern_add_color_stop_color (pat, 1.0, bg->g_step_2);
  }
  
  cairo_save (cr);

  draw_rect (bg, cr, position, 1, 1, width-3, bg_size-2, TRUE);
  cairo_clip_preserve (cr);
  cairo_set_source (cr, pat);
  cairo_paint (cr);

  cairo_restore (cr);

  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_linear (0., 0., 0., height);
  awn_cairo_pattern_add_color_stop_color (pat, 0.0, bg->g_histep_1);
  awn_cairo_pattern_add_color_stop_color (pat, 0.3, bg->g_histep_2);
  double red, green, blue, alpha;
  desktop_agnostic_color_get_cairo_color (bg->g_histep_2, &red, &green, &blue, &alpha);
  cairo_pattern_add_color_stop_rgba (pat, 0.36, red, green, blue, 0.);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  paint_lines:

  /* Internal border */
  awn_cairo_set_source_color (cr, bg->hilight_color);
  draw_rect (bg, cr, position, 1, 1, width-3, bg_size-2, TRUE);
  cairo_stroke (cr);

  /* External border */
  awn_cairo_set_source_color (cr, bg->border_color);
  draw_rect (bg, cr, position, 0, 0, width-1, bg_size, TRUE);
  cairo_stroke (cr);
}

static
void awn_background_floaty_padding_request (AwnBackground *bg,
                                            GtkPositionType position,
                                            guint *padding_top,
                                            guint *padding_bottom,
                                            guint *padding_left,
                                            guint *padding_right)
{
  #define TOP_PADDING 2
  #define MIN_SIDE_PADDING 6
  gboolean expand = FALSE;

  gint side_padding = bg->corner_radius * 3 / 4;
  if (side_padding < MIN_SIDE_PADDING) side_padding = MIN_SIDE_PADDING;

  g_object_get (bg->panel, "expand", &expand, NULL);
  if (expand)
  {
    side_padding += bg->floaty_offset * 3 / 4;
  }
  
  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = bg->floaty_offset; *padding_bottom = TOP_PADDING;
      *padding_left = side_padding; *padding_right = side_padding;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = TOP_PADDING; *padding_bottom = bg->floaty_offset;
      *padding_left = side_padding; *padding_right = side_padding;
      break;
    case GTK_POS_LEFT:
      *padding_top  = side_padding; *padding_bottom = side_padding;
      *padding_left = bg->floaty_offset; *padding_right = TOP_PADDING;
      break;
    case GTK_POS_RIGHT:
      *padding_top  = side_padding; *padding_bottom = side_padding;
      *padding_left = TOP_PADDING; *padding_right = bg->floaty_offset;
      break;
    default:
      break;
  }
}

static void 
awn_background_floaty_draw (AwnBackground  *bg,
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

  draw_top_bottom_background (bg, position, cr, width, height);

  cairo_restore (cr);
}

static void 
awn_background_floaty_get_shape_mask (AwnBackground  *bg,
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
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 1., 1., 1., 1.);

  // eyecandy in expand mode
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  if (expand)
  {
    gint extra_space = bg->floaty_offset * 3 / 4;
    cairo_translate (cr, extra_space, 0.0);
    width -= 2*extra_space;
  }
  draw_rect (bg, cr, position, 0, 0, width, height - bg->floaty_offset + 2, TRUE);
  cairo_fill (cr);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
