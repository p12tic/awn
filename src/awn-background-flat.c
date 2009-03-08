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

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif

G_DEFINE_TYPE (AwnBackgroundFlat, awn_background_flat, AWN_TYPE_BACKGROUND)

static void awn_background_flat_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      AwnOrientation  orient,
                                      GdkRectangle   *area);

static void awn_background_flat_get_shape_mask (AwnBackground  *bg,
                                                cairo_t        *cr,
                                                AwnOrientation  orient,
                                                GdkRectangle   *area);

static void awn_background_flat_padding_request (AwnBackground *bg,
                                                 AwnOrientation orient,
                                                 guint *padding_top,
                                                 guint *padding_bottom,
                                                 guint *padding_left,
                                                 guint *padding_right);

static void
awn_background_flat_expand_changed (AwnBackground *bg) // has more params...
{
  awn_background_emit_changed(bg);
}

static void
awn_background_flat_align_changed (AwnBackground *bg) // has more params...
{
  awn_background_emit_changed(bg);
}


static void
awn_background_flat_constructed (GObject *object)
{
  AwnBackground *bg = AWN_BACKGROUND (object);
  gpointer monitor = NULL;

  G_OBJECT_CLASS (awn_background_flat_parent_class)->constructed (object);

  g_return_if_fail (bg->panel);

  g_signal_connect_swapped (bg->panel, "notify::expand",
                            G_CALLBACK (awn_background_flat_expand_changed),
                            object);

  g_object_get (bg->panel, "monitor", &monitor, NULL);

  g_return_if_fail (monitor);

  g_signal_connect_swapped (monitor, "notify::monitor-align",
                            G_CALLBACK (awn_background_flat_align_changed),
                            object);
}

static void
awn_background_flat_dispose (GObject *object)
{
  gpointer monitor = NULL;
  if (AWN_BACKGROUND (object)->panel)
    g_object_get (AWN_BACKGROUND (object)->panel, "monitor", &monitor, NULL);

  if (monitor)
    g_signal_handlers_disconnect_by_func (monitor, 
        G_CALLBACK (awn_background_flat_align_changed), object);

  g_signal_handlers_disconnect_by_func (AWN_BACKGROUND (object)->panel, 
        G_CALLBACK (awn_background_flat_expand_changed), object);

  G_OBJECT_CLASS (awn_background_flat_parent_class)->dispose (object);
}

static void
awn_background_flat_class_init (AwnBackgroundFlatClass *klass)
{
  AwnBackgroundClass *bg_class = AWN_BACKGROUND_CLASS (klass);
  GObjectClass       *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed  = awn_background_flat_constructed;
  obj_class->dispose = awn_background_flat_dispose;

  bg_class->draw = awn_background_flat_draw;
  bg_class->padding_request = awn_background_flat_padding_request;
  bg_class->get_shape_mask = awn_background_flat_get_shape_mask;
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
           gdouble         x,
           gdouble         y,
           gint            width,
           gint            height)
{
  AwnCairoRoundCorners state = ROUND_TOP;
  gfloat align = awn_background_get_panel_alignment (bg);

  gboolean expand = FALSE;

  g_object_get (bg->panel, "expand", &expand, NULL);

  if (expand) state = ROUND_NONE;
  else if (align == 0.0f) state = ROUND_TOP_RIGHT;
  else if(align == 1.0f) state = ROUND_TOP_LEFT;

  awn_cairo_rounded_rect (cr, x, y, width, height, bg->corner_radius, state);
}

static void
draw_top_bottom_background (AwnBackground  *bg,
                            cairo_t        *cr,
                            gint            width,
                            gint            height)
{
  cairo_pattern_t *pat;

  /* Make sure the bar gets drawn on the 0.5 pixels (for sharp edges) */
  cairo_translate (cr, 0.5, 0.5);

  /* Basic set-up */
  cairo_set_line_width (cr, 1.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if (gtk_widget_is_composited (GTK_WIDGET (bg->panel)) == FALSE)
  {
    goto paint_lines;
  }

  /* Draw the background */
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
  draw_rect (bg, cr, 1, 1, width-2, height-1);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  /* Draw the hi-light */
  pat = cairo_pattern_create_linear (0, 0, 0, (height/3.0));
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
  draw_rect (bg, cr, 1, 1, width-2, height/3.0);

  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  paint_lines:

  /* Internal border */
  cairo_set_source_rgba (cr, bg->hilight_color.red,
                             bg->hilight_color.green,
                             bg->hilight_color.blue,
                             bg->hilight_color.alpha);
  draw_rect (bg, cr, 1, 1, width-3, height+3);
  cairo_stroke (cr);

  /* External border */
  cairo_set_source_rgba (cr, bg->border_color.red,
                             bg->border_color.green,
                             bg->border_color.blue,
                             bg->border_color.alpha);
  draw_rect (bg, cr, 0, 0, width-1, height+3);
  cairo_stroke (cr);
}

static
void awn_background_flat_padding_request (AwnBackground *bg,
                                          AwnOrientation orient,
                                          guint *padding_top,
                                          guint *padding_bottom,
                                          guint *padding_left,
                                          guint *padding_right)
{
  #define SIDE_PADDING 6
  #define TOP_PADDING 2
  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
      *padding_top  = 0; *padding_bottom = TOP_PADDING;
      *padding_left = SIDE_PADDING; *padding_right = SIDE_PADDING;
      break;
    case AWN_ORIENTATION_BOTTOM:
      *padding_top  = TOP_PADDING; *padding_bottom = 0;
      *padding_left = SIDE_PADDING; *padding_right = SIDE_PADDING;
      break;
    case AWN_ORIENTATION_LEFT:
      *padding_top  = SIDE_PADDING; *padding_bottom = SIDE_PADDING;
      *padding_left = 0; *padding_right = TOP_PADDING;
      break;
    case AWN_ORIENTATION_RIGHT:
      *padding_top  = SIDE_PADDING; *padding_bottom = SIDE_PADDING;
      *padding_left = TOP_PADDING; *padding_right = 0;
      break;
    default:
      break;
  }
}

static void 
awn_background_flat_draw (AwnBackground  *bg,
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

  draw_top_bottom_background (bg, cr, width, height);

  cairo_restore (cr);
}

static void 
awn_background_flat_get_shape_mask (AwnBackground  *bg,
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

  draw_rect (bg, cr, 0, 0, width, height+3);
  cairo_fill (cr);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
