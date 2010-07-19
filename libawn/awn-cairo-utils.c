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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Author : Anthony Arobone <aarobone@gmail.com>
 *           Alberto Aldegheri <albyrock87+dev@gmail.com>
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
 *
 * Draws a rounded rectangle via cairo.
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

  /* fix to radius to avoid wrong draws */
  if (radius > height / 2. && (
        ((state & ROUND_TOP_LEFT) && (state & ROUND_BOTTOM_LEFT)) ||
        ((state & ROUND_TOP_RIGHT) && (state & ROUND_BOTTOM_RIGHT))
      ))
  {
    radius = height / 2.;
  }
  else if (radius > height)
  {
    radius = height;
  }
  if (radius > width / 2. && (
        ((state & ROUND_TOP_LEFT) && (state & ROUND_TOP_RIGHT)) ||
        ((state & ROUND_BOTTOM_LEFT) && (state & ROUND_BOTTOM_RIGHT))
      ))
  {
    radius = width / 2.;
  }
  else if (radius > width)
  {
    radius = width;
  }

  /* top left corner */
  if (state & ROUND_TOP_LEFT)
  {
    if (state & ROUND_BOTTOM_LEFT)
    {
      cairo_move_to (cr, rx0, ry1 - radius);
    }
    else
    {
      cairo_move_to (cr, rx0, ry1);
    }
    cairo_arc (cr, rx0 + radius, ry0 + radius, radius, M_PI, M_PI * 1.5);
  }
  else
  {
    cairo_move_to (cr, rx0, ry1 - radius);
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
 * awn_cairo_rounded_rect_shadow:
 *
 * Draws a shadow for rounded rectangle via cairo.
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
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, ry0 - shadow_radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* top */
  pat = cairo_pattern_create_linear (0.0, ry0 - shadow_radius, 0.0, ry0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 2./3, 0.0, 0.0, 0.0, shadow_alpha/2);
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
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx1 - radius, ry0 - shadow_radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* right */
  pat = cairo_pattern_create_linear (rx1, 0.0, rx1 + shadow_radius, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
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
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx1 - radius, ry1 - radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* bottom */
  pat = cairo_pattern_create_linear (0.0, ry1, 0.0, ry1 + shadow_radius);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, shadow_alpha);
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
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
  cairo_pattern_add_color_stop_rgba (pat, 1./3, 0.0, 0.0, 0.0, shadow_alpha/2);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, 0.0);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, ry1 - radius,
                   radius + shadow_radius, radius + shadow_radius);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  /* left */
  pat = cairo_pattern_create_linear (rx0 - shadow_radius, 0.0, rx0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.0, 0.0, 0.0, 0.0);
  cairo_pattern_add_color_stop_rgba (pat, 2./3, 0.0, 0.0, 0.0, shadow_alpha/2);
  cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.0, 0.0, 0.0, shadow_alpha);

  cairo_set_source (cr, pat);
  cairo_rectangle (cr, rx0 - shadow_radius, yr,
                   shadow_radius, height - radius * 2);
  cairo_fill (cr);

  cairo_pattern_destroy (pat);

  cairo_restore (cr);
}

/**
 * awn_cairo_set_source_color:
 * @cr: A cairo context.
 * @color: The source color.
 *
 * A convenience function which wraps #cairo_set_source_rgba by using the
 * values from the @color parameter.
 */
void
awn_cairo_set_source_color (cairo_t              *cr,
                            DesktopAgnosticColor *color)
{
  g_return_if_fail (color);

  double red, green, blue, alpha;

  desktop_agnostic_color_get_cairo_color (color, &red, &green, &blue, &alpha);

  cairo_set_source_rgba (cr, red, green, blue, alpha);
}

/**
 * awn_cairo_set_source_color_with_alpha_multiplier:
 * @cr: A cairo context.
 * @color: The source color.
 * @multiplier: The number (between 0 and 1.0 inclusive) that the alpha value
 * is multiplied by.
 *
 * Similar to #awn_cairo_set_source_color, except for the @multiplier
 * parameter.
 */
void
awn_cairo_set_source_color_with_alpha_multiplier (cairo_t              *cr,
                                                  DesktopAgnosticColor *color,
                                                  gdouble               multiplier)
{
  g_return_if_fail (color);
  g_return_if_fail (multiplier >= 0 && multiplier <= 1.0);

  double red, green, blue, alpha;

  desktop_agnostic_color_get_cairo_color (color, &red, &green, &blue, &alpha);

  cairo_set_source_rgba (cr, red, green, blue, alpha * multiplier);
}

/**
 * awn_cairo_set_source_color_with_multipliers:
 * @cr: A cairo context.
 * @color: The source color.
 * @color_multiplier: The number that the color values are multiplied by.
 * @alpha_multiplier: The number that the alpha value is multiplied by.
 *
 * Similar to #awn_cairo_set_source_color_with_alpha_multiplier, except that
 * there is an additional @color_multiplier parameter.
 */
void
awn_cairo_set_source_color_with_multipliers (cairo_t              *cr,
                                             DesktopAgnosticColor *color,
                                             gdouble               color_multiplier,
                                             gdouble               alpha_multiplier)
{
  g_return_if_fail (color);

  double red, green, blue, alpha;

  desktop_agnostic_color_get_cairo_color (color, &red, &green, &blue, &alpha);

  cairo_set_source_rgba (cr, red * color_multiplier, green * color_multiplier,
                         blue * color_multiplier, alpha * alpha_multiplier);
}

/**
 * awn_cairo_pattern_add_color_stop_color:
 * @pattern: A cairo pattern.
 * @offset: An offset in the range [0.0 .. 1.0].
 * @color: The source color.
 *
 * A convenience function which wraps #cairo_pattern_add_color_stop_rgba by
 * using the values from the @color parameter.
 */
void
awn_cairo_pattern_add_color_stop_color (cairo_pattern_t      *pattern,
                                        double                offset,
                                        DesktopAgnosticColor *color)
{
  g_return_if_fail (color);

  double red, green, blue, alpha;

  desktop_agnostic_color_get_cairo_color (color, &red, &green, &blue, &alpha);

  cairo_pattern_add_color_stop_rgba (pattern, offset, red, green, blue, alpha);
}

/**
 * awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier:
 * @pattern: A cairo pattern.
 * @offset: An offset in the range [0.0 .. 1.0].
 * @color: The source color.
 * @multiplier: The number (between 0 and 1.0 inclusive) that the alpha value
 * is multiplied by.
 *
 * Similar to #awn_cairo_pattern_add_color_stop_color, except for the
 * @multiplier parameter.
 */
void
awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier (cairo_pattern_t      *pattern,
                                                              double                offset,
                                                              DesktopAgnosticColor *color,
                                                              gdouble               multiplier)
{
  g_return_if_fail (color);
  g_return_if_fail (multiplier >= 0 && multiplier <= 1.0);

  double red, green, blue, alpha;

  desktop_agnostic_color_get_cairo_color (color, &red, &green, &blue, &alpha);

  cairo_pattern_add_color_stop_rgba (pattern, offset, red, green, blue,
                                     alpha * multiplier);
}

/* vim: set et ts=2 sts=2 sw=2 ai cindent : */
