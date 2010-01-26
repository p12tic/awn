/*
 *  Copyright (C) 2008 Michal Hruby <michal.mhr@gmail.com>
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


#ifndef __AWN_EFFECTS_OPS_NEW_
#define __AWN_EFFECTS_OPS_NEW_

#include "awn-effects.h"

void awn_effects_get_base_coords (AwnEffects *fx, double *x, double *y);

gboolean awn_effects_pre_op_clear(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_pre_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_pre_op_translate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_pre_op_scale(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_pre_op_rotate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_pre_op_flip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_clip(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_saturate(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_glow(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_depth(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_shadow(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_spotlight(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_alpha(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_reflection(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

gboolean awn_effects_post_op_active(AwnEffects * fx,
                                    cairo_t * cr,
                                    GtkAllocation * ds,
                                    gpointer user_data
                                   );

gboolean awn_effects_post_op_arrow(AwnEffects * fx,
                                   cairo_t * cr,
                                   GtkAllocation * ds,
                                   gpointer user_data
                                  );

gboolean awn_effects_post_op_progress(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

#endif
