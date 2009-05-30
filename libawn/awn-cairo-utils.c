/*
 *  Copyright (C) 2007 Anthony Arobone <aarobone@gmail.com>
 *  Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  Author : Anthony Arobone <aarobone@gmail.com>
 *           (awn_cairo_rounded_rect)
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 *           (awn_cairo_set_source_color,
 *            awn_cairo_set_source_color_with_alpha_multiplier,
 *            awn_cairo_pattern_add_color_stop_color)
*/

#include "awn-cairo-utils.h"
#include <math.h>

/**
 * awn_cairo_rounded_rect:
 * draws a rounded rectangle via cairo.
 */
void
awn_cairo_rounded_rect(cairo_t *cr, double rx0, double ry0,
                       double width, double height,
                       double radius, AwnCairoRoundCorners state)
{
  const double rx1 = rx0 + width;
  const double ry1 = ry0 + height;

  /* arc with radius == 0.0 doesn't paint anything */
  if (radius == 0.0) state = ROUND_NONE;

  cairo_move_to (cr, rx0, ry1 - radius);

  /* top left corner */

  if (state & ROUND_TOP_LEFT)
  {
    cairo_arc (cr, rx0 + radius, ry0 + radius, radius, M_PI, M_PI * 1.5);
  }
  else
  {
    cairo_line_to (cr, rx0, ry0);
  }

  /* top right */
  if (state & ROUND_TOP_RIGHT)
  {
    cairo_arc (cr, rx1 - radius, ry0 + radius, radius, M_PI * 1.5, M_PI * 2);
  }
  else
  {
    cairo_line_to (cr, rx1, ry0);
  }

  /* bottom right */
  if (state & ROUND_BOTTOM_RIGHT)
  {
    cairo_arc (cr, rx1 - radius, ry1 - radius, radius, 0, M_PI * 0.5);
  }
  else
  {
    cairo_line_to (cr, rx1, ry1);
  }

  /* bottom left */
  if (state & ROUND_BOTTOM_LEFT)
  {
    cairo_arc (cr, rx0 + radius, ry1 - radius, radius, M_PI * 0.5, M_PI);
  }
  else
  {
    cairo_line_to (cr, rx0, ry1);
  }

  cairo_close_path (cr);
}

/**
 * awn_cairo_rounded_rect:
 * draws a rounded rectangle via cairo.
 */
void
awn_cairo_rounded_rect_shadow(cairo_t *cr, double rx0, double ry0,
                              double width, double height,
                              double radius, AwnCairoRoundCorners state,
                              double shadow_radius, double shadow_alpha)
{
  cairo_pattern_t *pat;

  const double rx1 = rx0 + width;
  const double ry1 = ry0 + height;

  const double xr = rx0 + radius;
  const double yr = ry0 + radius;

  cairo_save (cr);

  cairo_rectangle (cr, rx0 - shadow_radius, ry0 - shadow_radius,
                   width + shadow_radius * 2, height + shadow_radius * 2);

  awn_cairo_rounded_rect (cr, rx0, ry0, width, height, radius, state);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

  cairo_clip (cr);

  /* top left */
  pat = cairo_pattern_create_radial (xr, yr, radius,
                                     xr, yr, radius + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, ry0 - shadow_radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* top */
  pat = cairo_pattern_create_linear (0.0, ry0 - shadow_radius, 0.0, ry0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, shadow_alpha);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, xr, ry0 - shadow_radius,
                   width - radius * 2, shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* top right */
  pat = cairo_pattern_create_radial (rx1 - radius, yr, radius,
                                     rx1 - radius, yr, radius + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx1 - radius, ry0 - shadow_radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* right */
  pat = cairo_pattern_create_linear (rx1, 0.0, rx1 + shadow_radius, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx1, ry0 + radius,
                   shadow_radius, height - radius * 2);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* bottom right */
  pat = cairo_pattern_create_radial (rx1 - radius, ry1 - radius, radius,
                                     rx1 - radius, ry1 - radius,
                                     radius + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx1 - radius, ry1 - radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* bottom */
  pat = cairo_pattern_create_linear (0.0, ry1, 0.0, ry1 + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, xr, ry1,
                   width - radius * 2, shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* bottom left */
  pat = cairo_pattern_create_radial (xr, ry1 - radius, radius,
                                     xr, ry1 - radius, radius + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, ry1 - radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* left */
  pat = cairo_pattern_create_linear (rx0 - shadow_radius, 0.0, rx0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, shadow_alpha);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, yr,
                   shadow_radius, height - radius * 2);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  cairo_restore (cr);
}

void
awn_cairo_set_source_color (cairo_t              *cr,
                            DesktopAgnosticColor *color)
{
  g_return_if_fail (color);

  cairo_set_source_rgba (cr,
                         desktop_agnostic_color_get_red (color) / AWN_RGBA_SCALE_FACTOR,
                         desktop_agnostic_color_get_green (color) / AWN_RGBA_SCALE_FACTOR,
                         desktop_agnostic_color_get_blue (color) / AWN_RGBA_SCALE_FACTOR,
                         color->alpha / AWN_RGBA_SCALE_FACTOR);
}

void
awn_cairo_set_source_color_with_alpha_multiplier (cairo_t              *cr,
                                                  DesktopAgnosticColor *color,
                                                  gdouble               multiplier)
{
  g_return_if_fail (color);
  g_return_if_fail (multiplier >= 0 && multiplier <= 1.0);

  cairo_set_source_rgba (cr,
                         desktop_agnostic_color_get_red (color) / AWN_RGBA_SCALE_FACTOR,
                         desktop_agnostic_color_get_green (color) / AWN_RGBA_SCALE_FACTOR,
                         desktop_agnostic_color_get_blue (color) / AWN_RGBA_SCALE_FACTOR,
                         color->alpha / AWN_RGBA_SCALE_FACTOR * multiplier);
}

void
awn_cairo_set_source_color_with_multipliers (cairo_t              *cr,
                                             DesktopAgnosticColor *color,
                                             gdouble               color_multiplier,
                                             gdouble               alpha_multiplier)
{
  g_return_if_fail (color);

  cairo_set_source_rgba (cr,
                         desktop_agnostic_color_get_red (color) / AWN_RGBA_SCALE_FACTOR * color_multiplier,
                         desktop_agnostic_color_get_green (color) / AWN_RGBA_SCALE_FACTOR * color_multiplier,
                         desktop_agnostic_color_get_blue (color) / AWN_RGBA_SCALE_FACTOR * color_multiplier,
                         color->alpha / AWN_RGBA_SCALE_FACTOR * alpha_multiplier);
}

void
awn_cairo_pattern_add_color_stop_color (cairo_pattern_t      *pattern,
                                        double                offset,
                                        DesktopAgnosticColor *color)
{
  g_return_if_fail (color);

  cairo_pattern_add_color_stop_rgba (pattern, offset,
                                     desktop_agnostic_color_get_red (color) / AWN_RGBA_SCALE_FACTOR,
                                     desktop_agnostic_color_get_green (color) / AWN_RGBA_SCALE_FACTOR,
                                     desktop_agnostic_color_get_blue (color) / AWN_RGBA_SCALE_FACTOR,
                                     color->alpha / AWN_RGBA_SCALE_FACTOR);
}

void
awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier (cairo_pattern_t      *pattern,
                                                              double                offset,
                                                              DesktopAgnosticColor *color,
                                                              gdouble               multiplier)
{
  g_return_if_fail (color);
  g_return_if_fail (multiplier >= 0 && multiplier <= 1.0);

  cairo_pattern_add_color_stop_rgba (pattern, offset,
                                     desktop_agnostic_color_get_red (color) / AWN_RGBA_SCALE_FACTOR,
                                     desktop_agnostic_color_get_green (color) / AWN_RGBA_SCALE_FACTOR,
                                     desktop_agnostic_color_get_blue (color) / AWN_RGBA_SCALE_FACTOR,
                                     color->alpha / AWN_RGBA_SCALE_FACTOR * multiplier);
}

/* vim: set et ts=2 sts=2 sw=2 ai cindent : */
