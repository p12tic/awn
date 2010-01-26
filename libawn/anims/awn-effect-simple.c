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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-effect-simple.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
simple_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->glow_amount = 1.0;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  if (awn_effect_check_top_effect(anim, NULL))
    return awn_effect_suspend_animation(anim,
                                        (GSourceFunc)simple_hover_effect);
  else
    priv->glow_amount = 0.0;

  /* check for repeating, but it'll return FALSE anyway if we're here */
  gboolean repeat = awn_effect_handle_repeating(anim);

  return repeat;
}

gboolean
simple_attention_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->simple_rect = TRUE;
    awn_effects_redraw(anim->effects);
    return awn_effect_force_timeout(anim, 750,
             (GSourceFunc)simple_attention_effect);
  }

  /* blink */
  priv->simple_rect = !priv->simple_rect;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  /* check for repeating */
  gboolean repeat = awn_effect_handle_repeating(anim);
  if (!repeat) {
    priv->simple_rect = FALSE;
  }

  return repeat;
}

gboolean
simple_opening_effect (AwnEffectsAnimation *anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT (anim)
  {
    priv->count = 0;
  }

  const gint PERIOD = 10;

  gdouble sinus = sin(priv->count++ * M_PI/2 / PERIOD);
  priv->alpha = sinus * sinus;

  /* repaint widget */
  awn_effects_redraw (anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->alpha = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating (anim);
  }

  return repeat;
}

gboolean
simple_closing_effect (AwnEffectsAnimation *anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT (anim)
  {
    priv->count = 0;
  }

  const gint PERIOD = 10;

  gdouble cosin = cos(priv->count++ * M_PI/2 / PERIOD);
  priv->alpha = cosin * cosin;

  /* repaint widget */
  awn_effects_redraw (anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->alpha = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating (anim);
  }

  return repeat;
}

