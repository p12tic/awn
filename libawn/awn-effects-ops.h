/*
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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


#ifndef __AWN_EFFECTS_OPS_

#define __AWN_EFFECTS_OPS_
#include "awn-effects.h"

gboolean awn_effect_op_scaling(AwnEffects * fx,
                               DrawIconState * ds,
                               cairo_surface_t * icon,
                               cairo_surface_t ** picon_srfc,
                               cairo_t ** picon_ctx,
                               cairo_surface_t ** preflect_srfc,
                               cairo_t ** preflect_ctx
                              )  ;

gboolean awn_effect_op_depth(AwnEffects * fx,
                             cairo_t * cr,
                             DrawIconState * ds,
                             cairo_surface_t * icon_srfc,
                             cairo_t * icon_ctx,
                             gpointer null
                            );

//--------------------------------------------------------

gboolean awn_effect_op_saturate(AwnEffects * fx,
                                DrawIconState * ds,
                                cairo_surface_t * icon_srfc,
                                cairo_t * icon_ctx,
                                gpointer null
                               ) ;

gboolean awn_effect_op_hflip(AwnEffects * fx,
                             DrawIconState * ds,
                             cairo_surface_t * icon_srfc,
                             cairo_t * icon_ctx,
                             gpointer null
                            ) ;

gboolean awn_effect_op_glow(AwnEffects * fx,
                            DrawIconState * ds,
                            cairo_surface_t * icon_srfc,
                            cairo_t * icon_ctx,
                            gpointer null
                           ) ;

gboolean awn_effect_move_x(AwnEffects * fx,
                           DrawIconState * ds,
                           cairo_surface_t * icon_srfc,
                           cairo_t * icon_ctx,
                           gpointer null
                          ) ;

GdkPixbuf * get_pixbuf_from_surface(cairo_surface_t * surface);



#endif
