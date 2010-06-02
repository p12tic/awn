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

#include <math.h>
#include <libawn/awn-cairo-utils.h>

#include "awn-background-flat.h"

G_DEFINE_TYPE (AwnBackgroundFlat, awn_background_flat, AWN_TYPE_BACKGROUND)
static int timer = 0;

static gboolean
awn_background_redraw (AwnBackground *bg)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND (bg), FALSE);

  timer++;
  awn_background_invalidate (bg);
  gtk_widget_queue_draw (bg->panel);

  return TRUE;
}
static void awn_background_flat_draw (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      GtkPositionType  position,
                                      GdkRectangle   *area);

static void awn_background_flat_get_shape_mask (AwnBackground  *bg,
                                                cairo_t        *cr,
                                                GtkPositionType  position,
                                                GdkRectangle   *area);

static void awn_background_flat_padding_request (AwnBackground *bg,
                                                 GtkPositionType position,
                                                 guint *padding_top,
                                                 guint *padding_bottom,
                                                 guint *padding_left,
                                                 guint *padding_right);

static void
awn_background_flat_expand_changed (AwnBackground *bg) // has more params...
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_flat_align_changed (AwnBackground *bg) // has more params...
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_flat_radius_changed (AwnBackground *bg)
{
  awn_background_emit_padding_changed (bg);
}

static void
awn_background_flat_constructed (GObject *object)
{
  AwnBackground *bg = AWN_BACKGROUND (object);
  gpointer monitor = NULL;

  G_OBJECT_CLASS (awn_background_flat_parent_class)->constructed (object);

  g_signal_connect (bg, "notify::corner-radius",
                    G_CALLBACK (awn_background_flat_radius_changed), NULL);

  g_return_if_fail (bg->panel);

  g_signal_connect_swapped (bg->panel, "notify::expand",
                            G_CALLBACK (awn_background_flat_expand_changed),
                            object);

  g_object_get (bg->panel, "monitor", &monitor, NULL);

  g_return_if_fail (monitor);

  g_signal_connect_swapped (monitor, "notify::monitor-align",
                            G_CALLBACK (awn_background_flat_align_changed),
                            object);
  g_timeout_add (80, (GSourceFunc)awn_background_redraw, object);
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
awn_background_flat_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
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
           GtkPositionType position,
           gdouble         x,
           gdouble         y,
           gint            width,
           gint            height,
           gfloat          align,
           gboolean        expand)
{
  AwnCairoRoundCorners state = ROUND_TOP;

  if (expand){ state = ROUND_NONE; x-=2; width+=4; }
  else
  {
    switch (position)
    {
      case GTK_POS_BOTTOM:
      case GTK_POS_LEFT:
        if (align == 0.0f){ state = ROUND_TOP_RIGHT; x-=2; width+=2; }
        else if(align == 1.0f){ state = ROUND_TOP_LEFT; width+=2; }
        break;
      case GTK_POS_TOP:
      case GTK_POS_RIGHT:
        if (align == 0.0f){ state = ROUND_TOP_LEFT; width+=2; }
        else if(align == 1.0f){ state = ROUND_TOP_RIGHT; x-=2; width+=2; }
        break;
      default:
        break;
    }

  }

  awn_cairo_rounded_rect (cr, x, y, width, height, bg->corner_radius, state);
}
static void
draw_sin_line (cairo_t *cr, float t, double r, double g, double b, double fLen)
{
  const float YMAX = 0.35f;
  const int STEPS = 10;

  cairo_set_line_width (cr, fLen);

  cairo_pattern_t *pat = cairo_pattern_create_linear (0.0, 0.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, r, g, b, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.5, r, g, b, 1.0);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, r, g, b, 0.0);

  cairo_set_source (cr, pat);

  const double offset = 2*M_PI*t;
  const double step_mult = 1./STEPS;

  for (int i=0; i<STEPS; i++)
  {
    double y = sin(M_PI/180. * i * 180./STEPS + offset) * YMAX;
    cairo_line_to (cr, i*step_mult, y);
  }
  cairo_stroke (cr);

  cairo_pattern_destroy (pat);
}
static void
draw_top_bottom_background (AwnBackground  *bg,
                            GtkPositionType position,
                            cairo_t        *cr,
                            gint            width,
                            gint            height)
{
  float t = (timer % 25) / 25.;
  double fLen = 1. / height * 2;

  cairo_scale (cr, width, height);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  cairo_translate (cr, 0.0, 0.5);

  cairo_save (cr);
  draw_sin_line (cr, t, 1.0, 0.1, 0.1, fLen);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_rotate (cr, -0.2);
  cairo_translate (cr, 0.03, 0.1);
  draw_sin_line (cr, t+0.3, 0.3, 0.8, 0.3, fLen * 3);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_rotate (cr, 0.15);
  cairo_translate (cr, 0.02, 0.05);
  draw_sin_line (cr, t+0.7, 0.1, 0.4, 0.9, fLen * 2);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_rotate (cr, 0.03);
  cairo_translate (cr, 0.0, 0.15);
  draw_sin_line (cr, t+0.4, 0.9, 0.1, 0.9, fLen * 1.5);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_rotate (cr, 0.09);
  cairo_translate (cr, 0.0, -0.15);
  draw_sin_line (cr, t-0.25, 0.9, 0.9, 0.2, fLen * 2);
  cairo_restore (cr);
#if 0
   cairo_pattern_t *pat;
   gfloat   align = 0.5;
   gboolean expand = FALSE;
@@ -229,6 +307,7 @@
   awn_cairo_set_source_color (cr, bg->border_color);
   draw_rect (bg, cr, position, 0, 0, width-1, height+3, align, expand);
   cairo_stroke (cr);
#endif
}

static
void awn_background_flat_padding_request (AwnBackground *bg,
                                          GtkPositionType position,
                                          guint *padding_top,
                                          guint *padding_bottom,
                                          guint *padding_left,
                                          guint *padding_right)
{
  #define TOP_PADDING 2
  gboolean expand = FALSE;
  g_object_get (bg->panel, "expand", &expand, NULL);
  gint side_padding = expand ? 0 : MAX (6, bg->corner_radius * 3 / 4);
  gint zero_padding = 0;

  gfloat align = awn_background_get_panel_alignment (bg);
  if (awn_background_do_rtl_swap (bg))
  {
    if (align <= 0.0 || align >= 1.0)
    {
      zero_padding = side_padding;
      side_padding = 0;
    }
  }

  switch (position)
  {
    case GTK_POS_TOP:
      *padding_top  = 0;
      *padding_bottom = TOP_PADDING;
      *padding_left = align == 0.0 ? zero_padding : side_padding;
      *padding_right = align == 1.0 ? zero_padding : side_padding;
      break;
    case GTK_POS_BOTTOM:
      *padding_top  = TOP_PADDING;
      *padding_bottom = 0;
      *padding_left = align == 0.0 ? zero_padding : side_padding;
      *padding_right = align == 1.0 ? zero_padding : side_padding;
      break;
    case GTK_POS_LEFT:
      *padding_top  = align == 0.0 ? zero_padding : side_padding;
      *padding_bottom = align == 1.0 ? zero_padding : side_padding;
      *padding_left = 0;
      *padding_right = TOP_PADDING;
      break;
    case GTK_POS_RIGHT:
      *padding_top  = align == 0.0 ? zero_padding : side_padding;
      *padding_bottom = align == 1.0 ? zero_padding : side_padding;
      *padding_left = TOP_PADDING;
      *padding_right = 0;
      break;
    default:
      break;
  }
}

static void 
awn_background_flat_draw (AwnBackground  *bg,
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

  draw_top_bottom_background (bg, position, cr, width, height);

  cairo_restore (cr);
}

static void 
awn_background_flat_get_shape_mask (AwnBackground  *bg,
                                    cairo_t        *cr, 
                                    GtkPositionType  position,
                                    GdkRectangle   *area)
{
  gint temp;
  gint x = area->x, y = area->y;
  gint width = area->width, height = area->height;
  gfloat   align = 0.5;
  gboolean expand = FALSE;

  cairo_save (cr);

  align = awn_background_get_panel_alignment (bg);
  g_object_get (bg->panel, "expand", &expand, NULL);

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

  draw_rect (bg, cr, position, 0, 0, width, height+3, align, expand);
  cairo_fill (cr);

  cairo_restore (cr);
}

/* vim: set et ts=2 sts=2 sw=2 : */
