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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __AWN_EFFECTS_OPS_NEW_
#define __AWN_EFFECTS_OPS_NEW_

#include "awn-effects.h"

#define AWN_EFFECT_ORIENT_TOP 0
#define AWN_EFFECT_ORIENT_RIGHT 1
#define AWN_EFFECT_ORIENT_BOTTOM 2
#define AWN_EFFECT_ORIENT_LEFT 3

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

gboolean awn_effects_pre_op_active(AwnEffects * fx,
                                   cairo_t * cr,
                                   GtkAllocation * ds,
                                   gpointer user_data
                                   );
gboolean awn_effects_pre_op_running(AwnEffects * fx,
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

gboolean awn_effects_post_op_progress(AwnEffects * fx,
                               cairo_t * cr,
                               GtkAllocation * ds,
                               gpointer user_data
                              );

#endif
