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
 *           (awn_cairo_set_source_color)
*/

#include "awn-cairo-utils.h"

//
// awn_cairo_rounded_rect - draws a rounded rectangle via cairo
//
void
awn_cairo_rounded_rect(cairo_t *cr, double x0, double y0,
                       double width, double height,
                       double radius, AwnCairoRoundCorners state)
{
  double x1, y1;

  x1 = x0 + width;
  y1 = y0 + height;

  // top left corner

  if ((state & ROUND_TOP_LEFT) == ROUND_TOP_LEFT)
  {
    cairo_move_to(cr, x0, y0 + radius);
    cairo_curve_to(cr, x0 , y0, x0 , y0, x0 + radius, y0);
  }
  else
  {
    cairo_move_to(cr, x0, y0);
  }

  // top right
  if ((state & ROUND_TOP_RIGHT) == ROUND_TOP_RIGHT)
  {
    cairo_line_to(cr, x1 - radius, y0);
    cairo_curve_to(cr, x1, y0, x1, y0, x1, y0 + radius);
  }
  else
  {
    cairo_line_to(cr, x1, y0);
  }

  // bottom right
  if ((state & ROUND_BOTTOM_RIGHT) == ROUND_BOTTOM_RIGHT)
  {
    cairo_line_to(cr, x1 , y1 - radius);
    cairo_curve_to(cr, x1, y1, x1, y1, x1 - radius, y1);
  }
  else
  {
    cairo_line_to(cr, x1, y1);
  }

  // bottom left
  if ((state & ROUND_BOTTOM_LEFT) == ROUND_BOTTOM_LEFT)
  {
    cairo_line_to(cr, x0 + radius, y1);
    cairo_curve_to(cr, x0, y1, x0, y1, x0, y1 - radius);
  }
  else
  {
    cairo_line_to(cr, x0, y1);
  }

  cairo_close_path(cr);
}


void
awn_cairo_set_source_color (cairo_t              *cr,
                            DesktopAgnosticColor *color)
{
  cairo_set_source_rgba (cr,
                         desktop_agnostic_color_get_red (color),
                         desktop_agnostic_color_get_green (color),
                         desktop_agnostic_color_get_blue (color),
                         color->alpha);
}

void
awn_cairo_pattern_add_color_stop_color (cairo_pattern_t      *pattern,
                                        double                offset,
                                        DesktopAgnosticColor *color)
{
  cairo_pattern_add_color_stop_rgba (pattern, offset,
                                     desktop_agnostic_color_get_red (color),
                                     desktop_agnostic_color_get_green (color),
                                     desktop_agnostic_color_get_blue (color),
                                     color->alpha);
}

/* vim: set et ts=2 sts=2 sw=2 ai cindent : */
