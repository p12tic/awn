/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *  Copyright (C) 1999 The Free Software Foundation
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
 */

#ifndef _AWN_EFFECTS_OPS_HELPERS_H
#define _AWN_EFFECTS_OPS_HELPERS_H

#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>
#include "awn-effects.h"

#include <math.h>
#include <string.h>

void
paint_arrow_triangle (cairo_t *cr, double size, gint count);

void
paint_arrow_dot (cairo_t *cr, double size, gint count,
                 double r, double g, double b);

guchar
lighten_component (const guchar cur_value, const gfloat amount,
                   gboolean absolute);

void
lighten_surface (cairo_surface_t *src, 
                 gint surface_width, gint surface_height,
                 const gfloat amount);

void
darken_surface (cairo_t *cr, gint surface_width, gint surface_height);

void
blur_surface_shadow (cairo_surface_t *src,
                     gint surface_width, gint surface_height,
                     const int radius);

void
blur_surface_shadow_rgba (cairo_surface_t *src,
                          gint surface_width, gint surface_height, const int radius,
                          guchar r, guchar g, guchar b, gfloat alpha_intensity);

void
surface_saturate(cairo_surface_t * icon_srfc, const gfloat saturation);

#endif

