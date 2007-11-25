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


#ifndef __AWN_CAIRO_UTILS_H__
#define __AWN_CAIRO_UTILS_H__

#include <gtk/gtk.h>


typedef enum
{
	ROUND_NONE		= 0,
	ROUND_TOP_LEFT		= 1,
	ROUND_TOP_RIGHT		= 2,
	ROUND_BOTTOM_RIGHT	= 4,
	ROUND_BOTTOM_LEFT	= 8,
	ROUND_TOP		= ROUND_TOP_LEFT | ROUND_TOP_RIGHT,
	ROUND_BOTTOM		= ROUND_BOTTOM_LEFT | ROUND_BOTTOM_RIGHT,
	ROUND_LEFT		= ROUND_TOP_LEFT | ROUND_BOTTOM_LEFT,
	ROUND_RIGHT		= ROUND_TOP_RIGHT | ROUND_BOTTOM_RIGHT,
	ROUND_ALL		= ROUND_LEFT | ROUND_RIGHT
} AwnCairoRoundCorners;

typedef struct
{
        gfloat red;
        gfloat green;
        gfloat blue;
        gfloat alpha;
} AwnColor;


void 
awn_cairo_rounded_rect (cairo_t *cr, 
                        int x0, int y0, 
                        int width, int height, 
                        double radius, AwnCairoRoundCorners state);


/* takes a string of RRGGBBAA and converts to AwnColor */
void
awn_cairo_string_to_color (const gchar *string, AwnColor *color);

#endif

