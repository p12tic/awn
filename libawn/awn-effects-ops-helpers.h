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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _AWN_EFFECTS_OPS_HELPERS_H
#define _AWN_EFFECTS_OPS_HELPERS_H

#include <gtk/gtk.h>
#include <cairo/cairo-xlib.h>
#include "awn-effects.h"

#include <math.h>
#include <string.h>

void
lighten_surface(cairo_surface_t * src, const gfloat amount);

void
darken_surface(cairo_surface_t * src);

void
blur_surface_shadow(cairo_surface_t *src, const int radius);

void
surface_saturate(cairo_surface_t * icon_srfc, const gfloat saturation);

// FIXME: remove once using purely new drawing API
void
apply_3d_illusion(AwnEffects * fx, GtkAllocation * ds, const gdouble alpha);

#endif

