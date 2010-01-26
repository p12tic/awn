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
 *           (awn_cairo_rounded_rect)
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 *           (awn_cairo_set_source_color,
 *            awn_cairo_set_source_color_with_alpha_multiplier,
 *            awn_cairo_pattern_add_color_stop_color)
*/


#ifndef __AWN_CAIRO_UTILS_H__
#define __AWN_CAIRO_UTILS_H__

#include <cairo.h>
#include <libdesktop-agnostic/desktop-agnostic.h>

typedef enum
{
	ROUND_NONE		= 0,
	ROUND_TOP_LEFT		= 1 << 0,
	ROUND_TOP_RIGHT		= 1 << 1,
	ROUND_BOTTOM_RIGHT	= 1 << 2,
	ROUND_BOTTOM_LEFT	= 1 << 3,
	ROUND_TOP		= ROUND_TOP_LEFT | ROUND_TOP_RIGHT,
	ROUND_BOTTOM		= ROUND_BOTTOM_LEFT | ROUND_BOTTOM_RIGHT,
	ROUND_LEFT		= ROUND_TOP_LEFT | ROUND_BOTTOM_LEFT,
	ROUND_RIGHT		= ROUND_TOP_RIGHT | ROUND_BOTTOM_RIGHT,
	ROUND_ALL		= ROUND_LEFT | ROUND_RIGHT
} AwnCairoRoundCorners;

void 
awn_cairo_rounded_rect (cairo_t *cr, 
                        double x0, double y0,
                        double width, double height,
                        double radius, AwnCairoRoundCorners state);

void
awn_cairo_rounded_rect_shadow(cairo_t *cr, double rx0, double ry0,
                              double width, double height,
                              double radius, AwnCairoRoundCorners state,
                              double shadow_radius, double shadow_alpha);

void
awn_cairo_set_source_color (cairo_t              *cr,
                            DesktopAgnosticColor *color);

void
awn_cairo_set_source_color_with_alpha_multiplier (cairo_t              *cr,
                                                  DesktopAgnosticColor *color,
                                                  gdouble               multiplier);

void
awn_cairo_set_source_color_with_multipliers (cairo_t              *cr,
                                             DesktopAgnosticColor *color,
                                             gdouble               color_multiplier,
                                             gdouble               alpha_multiplier);

void
awn_cairo_pattern_add_color_stop_color (cairo_pattern_t      *pattern,
                                        double                offset,
                                        DesktopAgnosticColor *color);

void
awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier (cairo_pattern_t      *pattern,
                                                              double                offset,
                                                              DesktopAgnosticColor *color,
                                                              gdouble               multiplier);

#endif

