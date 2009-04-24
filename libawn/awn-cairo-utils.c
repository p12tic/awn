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

/**
 * awn_cairo_rounded_rect:
 * draws a rounded rectangle via cairo.
 */
void
awn_cairo_rounded_rect(cairo_t *cr, double rx0, double ry0,
                       double width, double height,
                       double radius, AwnCairoRoundCorners state)
{
  double rx1, ry1;

  rx1 = rx0 + width;
  ry1 = ry0 + height;

  /* top left corner */

  if ((state & ROUND_TOP_LEFT) == ROUND_TOP_LEFT)
  {
    cairo_move_to(cr, rx0, ry0 + radius);
    cairo_curve_to(cr, rx0 , ry0, rx0 , ry0, rx0 + radius, ry0);
  }
  else
  {
    cairo_move_to(cr, rx0, ry0);
  }

  /* top right */
  if ((state & ROUND_TOP_RIGHT) == ROUND_TOP_RIGHT)
  {
    cairo_line_to(cr, rx1 - radius, ry0);
    cairo_curve_to(cr, rx1, ry0, rx1, ry0, rx1, ry0 + radius);
  }
  else
  {
    cairo_line_to(cr, rx1, ry0);
  }

  /* bottom right */
  if ((state & ROUND_BOTTOM_RIGHT) == ROUND_BOTTOM_RIGHT)
  {
    cairo_line_to(cr, rx1 , ry1 - radius);
    cairo_curve_to(cr, rx1, ry1, rx1, ry1, rx1 - radius, ry1);
  }
  else
  {
    cairo_line_to(cr, rx1, ry1);
  }

  /* bottom left */
  if ((state & ROUND_BOTTOM_LEFT) == ROUND_BOTTOM_LEFT)
  {
    cairo_line_to(cr, rx0 + radius, ry1);
    cairo_curve_to(cr, rx0, ry1, rx0, ry1, rx0, ry1 - radius);
  }
  else
  {
    cairo_line_to(cr, rx0, ry1);
  }

  cairo_close_path(cr);
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
