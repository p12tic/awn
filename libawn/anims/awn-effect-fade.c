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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-effect-fade.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
fade_out_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->alpha = 1.0;
  }

  const gdouble MAX_OFFSET = 50.0;

  const gint PERIOD = 20;

  priv->top_offset = ++priv->count * (MAX_OFFSET / PERIOD);

  priv->alpha = priv->count * (-1.0 / PERIOD) + 1;

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
fading_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->alpha = 1.0;
    priv->direction = AWN_EFFECT_DIR_DOWN;
  }

  const gdouble MIN_ALPHA = 0.35;
  const gdouble ALPHA_STEP = 0.05;

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_DOWN)
  {
    priv->alpha -= ALPHA_STEP;

    if (priv->alpha <= MIN_ALPHA)
      priv->direction = AWN_EFFECT_DIR_UP;

    // repaint widget
    awn_effects_redraw(anim->effects);
  }
  else
  {
    priv->alpha += ALPHA_STEP * 1.5;
    // repaint widget
    awn_effects_redraw(anim->effects);

    if (priv->alpha >= 1)
    {
      priv->alpha = 1.0;
      priv->direction = AWN_EFFECT_DIR_DOWN;
      repeat = awn_effect_handle_repeating(anim);
    }
  }

  return repeat;
}

gboolean
fading_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("fading_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
