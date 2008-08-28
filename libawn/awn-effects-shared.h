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

#include "awn-effects.h"

typedef enum
{
  AWN_EFFECT_PRIORITY_HIGHEST,
  AWN_EFFECT_PRIORITY_HIGH,
  AWN_EFFECT_PRIORITY_ABOVE_NORMAL,
  AWN_EFFECT_PRIORITY_NORMAL,
  AWN_EFFECT_PRIORITY_BELOW_NORMAL,
  AWN_EFFECT_PRIORITY_LOW,
  AWN_EFFECT_PRIORITY_LOWEST
} AwnEffectPriority;

typedef struct _AwnEffectsPrivate AwnEffectsPrivate;

struct _AwnEffectsPrivate
{
  AwnEffects *effects;
  AwnEffect this_effect;
  gint max_loops;
  AwnEffectPriority priority;
  AwnEventNotify start, stop;
};


gboolean awn_effect_check_top_effect(AwnEffectsPrivate * priv, gboolean * stopped);
gboolean awn_effect_handle_repeating(AwnEffectsPrivate * priv);
gboolean awn_effect_check_max_loops(AwnEffectsPrivate * priv);
void awn_effects_main_effect_loop(AwnEffects * fx);


#endif
