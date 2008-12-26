/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
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


#ifndef __AWN_EFFECT_SHARED_H__
#define __AWN_EFFECT_SHARED_H__

#include "../awn-effects.h"

typedef struct _AwnEffectsAnimation AwnEffectsAnimation;

struct _AwnEffectsAnimation
{
  AwnEffects *effects;
  AwnEffect this_effect;
  gint max_loops;
  AwnEventNotify start, stop;
};

struct _AwnEffectsPrivate
{
  GtkWidget *self;
  GList *effect_queue;

  GSourceFunc sleeping_func;

  gint icon_width, icon_height;
  gint window_width, window_height;

  /* EFFECT VARIABLES */
  gboolean effect_lock;
  AwnEffect current_effect;
  gint direction;
  gint count;

  gdouble side_offset;
  gdouble top_offset;
  gdouble curve_offset;

  gint delta_width;
  gint delta_height;

  GtkAllocation clip_region;

  gdouble rotate_degrees;
  gfloat alpha;
  gfloat spotlight_alpha;
  gfloat saturation;
  gfloat glow_amount;

  gint icon_depth;
  gint icon_depth_direction;

  /* State variables */
  gboolean clip;
  gboolean flip;
  gboolean spotlight;

  guint timer_id;
};

typedef enum
{
  AWN_EFFECT_DIR_NONE,
  AWN_EFFECT_DIR_STOP,
  AWN_EFFECT_DIR_DOWN,
  AWN_EFFECT_DIR_UP,
  AWN_EFFECT_DIR_LEFT,
  AWN_EFFECT_DIR_RIGHT,
  AWN_EFFECT_SQUISH_DOWN,
  AWN_EFFECT_SQUISH_DOWN2,
  AWN_EFFECT_SQUISH_UP,
  AWN_EFFECT_SQUISH_UP2,
  AWN_EFFECT_TURN_1,
  AWN_EFFECT_TURN_2,
  AWN_EFFECT_TURN_3,
  AWN_EFFECT_TURN_4,
  AWN_EFFECT_SPOTLIGHT_ON,
  AWN_EFFECT_SPOTLIGHT_TREMBLE_UP,
  AWN_EFFECT_SPOTLIGHT_TREMBLE_DOWN,
  AWN_EFFECT_SPOTLIGHT_OFF
} AwnEffectSequence;

gboolean awn_effect_check_top_effect   (AwnEffectsAnimation * anim,
                                        gboolean * stopped);

gboolean awn_effect_handle_repeating   (AwnEffectsAnimation * anim);

gboolean awn_effect_check_max_loops    (AwnEffectsAnimation * anim);

gboolean awn_effect_suspend_animation  (AwnEffectsAnimation * anim,
                                        GSourceFunc func);

#endif
