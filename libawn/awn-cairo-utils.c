/*
 *  Copyright (C) 2007 Anthony Arobone <aarobone@gmail.com>
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
*/


#include <gtk/gtk.h>

#include "awn-cairo-utils.h"

//
// awn_cairo_rounded_rect - draws a rounded rectangle via cairo
//
void
awn_cairo_rounded_rect(cairo_t *cr, int x0, int y0,
                       int width, int height,
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

static int
getdec(char hexchar)
{
  if ((hexchar >= '0') && (hexchar <= '9')) return hexchar - '0';

  if ((hexchar >= 'A') && (hexchar <= 'F')) return hexchar - 'A' + 10;

  if ((hexchar >= 'a') && (hexchar <= 'f')) return hexchar - 'a' + 10;

  return -1; // Wrong character

}

static void
hex2float(const char *HexColor, float *FloatColor)
{
  const char *HexColorPtr = HexColor;

  int i = 0;

  for (i = 0; i < 4; i++)
  {
    int IntColor = (getdec(HexColorPtr[0]) * 16) +
                   getdec(HexColorPtr[1]);

    FloatColor[i] = (float) IntColor / 255.0;
    HexColorPtr += 2;
  }

}

void
awn_cairo_string_to_color(const gchar *string, AwnColor *color)
{
  float colors[4];
  g_return_if_fail (string); 
  g_return_if_fail (color); 

  hex2float(string, colors);
  color->red = colors[0];
  color->green = colors[1];
  color->blue = colors[2];
  color->alpha = colors[3];
}

