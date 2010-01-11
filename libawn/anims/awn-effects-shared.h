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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __AWN_EFFECT_SHARED_H__
#define __AWN_EFFECT_SHARED_H__

#include "../awn-effects.h"

typedef enum
{
  AWN_ARROW_TYPE_CUSTOM = 0,
  AWN_ARROW_TYPE_1,
  AWN_ARROW_TYPE_2,

  AWN_ARROW_TYPE_LAST
} AwnArrowType;

typedef struct _AwnEffectsAnimation AwnEffectsAnimation;

struct _AwnEffectsAnimation
{
  AwnEffects *effects;
  AwnEffect this_effect;
  gint max_loops;
  gboolean signal_start, signal_end;
};

struct _AwnEffectsPrivate
{
  GList *effect_queue;
  GList *overlays;

  GSourceFunc sleeping_func;

  gint icon_width, icon_height;
  gint window_width, window_height;
  gint last_redraw_size;

  DesktopAgnosticColor *active_rect_color;
  DesktopAgnosticColor *active_rect_outline;
  DesktopAgnosticColor *dot_color;

  /* EFFECT VARIABLES */
  gboolean effect_lock;
  AwnEffect current_effect;
  gint direction;
  gint count;

  gdouble side_offset;
  gdouble top_offset;
  gdouble curve_offset;

  gfloat width_mod;
  gfloat height_mod;

  GtkAllocation clip_region;

  gdouble rotate_degrees;
  gfloat alpha;
  gfloat spotlight_alpha;
  gfloat saturation;
  gfloat glow_amount;

  gint icon_depth;
  gint icon_depth_direction;

  AwnArrowType arrow_type;

  /* State variables */
  gboolean clip;
  gboolean flip;
  gboolean spotlight;
  gboolean simple_rect;

  guint timer_id;
  gboolean already_exposed;
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

/* Emits "animation-start" signal and initializes animation, extra 
 * initialization can follow in a single expression or block of code.
 */
#define AWN_ANIMATION_INIT(anim)                     \
          gboolean __done_lock = FALSE;              \
          if (!anim->effects->priv->effect_lock) {   \
            anim->effects->priv->effect_lock = TRUE; \
            __done_lock = TRUE;                      \
            awn_effect_emit_anim_start(anim);        \
          }                                          \
          if (__done_lock)

gboolean awn_effect_check_top_effect   (AwnEffectsAnimation * anim,
                                        gboolean * stopped);

gboolean awn_effect_handle_repeating   (AwnEffectsAnimation * anim);

gboolean awn_effect_check_max_loops    (AwnEffectsAnimation * anim);

gboolean awn_effect_suspend_animation  (AwnEffectsAnimation * anim,
                                        GSourceFunc func);

gboolean awn_effect_force_timeout      (AwnEffectsAnimation * anim,
                                        const gint timeout,
                                        GSourceFunc func);

void awn_effect_emit_anim_start        (AwnEffectsAnimation *anim);
void awn_effect_emit_anim_end          (AwnEffectsAnimation *anim);

#endif
