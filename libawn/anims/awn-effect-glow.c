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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-effect-glow.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
glow_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->glow_amount = 1.0;
  }

  const gfloat GLOW_STEP = 0.08;

  awn_effects_redraw(anim->effects);

  /* check for repeating */
  gboolean top = awn_effect_check_top_effect(anim, NULL);

  if (top)
  {
    priv->glow_amount = 1.0;
    return awn_effect_suspend_animation(anim, (GSourceFunc)glow_effect);
  }
  else
  {
    priv->glow_amount -= GLOW_STEP;

    if (priv->glow_amount <= 0)
    {
      priv->glow_amount = 0.0;
      gboolean repeat = awn_effect_handle_repeating(anim);
      return repeat;  /* == FALSE */
    }
  }

  return TRUE;
}

gboolean
glow_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->direction = AWN_EFFECT_DIR_UP;
    priv->alpha = 0.0;
    priv->glow_amount = 1.95;
  }

  const gdouble ALPHA_STEP = 0.04;

  const gdouble GLOW_STEP = 0.05;

  switch (priv->direction)
  {

    case AWN_EFFECT_DIR_UP:
      priv->alpha += ALPHA_STEP;

      if (priv->alpha > 1)
      {
        priv->alpha = 1.0;
        priv->direction = AWN_EFFECT_DIR_DOWN;
      }

      break;

    case AWN_EFFECT_DIR_DOWN:
      priv->glow_amount -= GLOW_STEP;

      if (priv->glow_amount < 0)
      {
        priv->glow_amount = 0.0;
        priv->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_DOWN;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_NONE)
  {
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
glow_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->direction = AWN_EFFECT_DIR_DOWN;
    priv->glow_amount = 0.8;
  }

  const gdouble ALPHA_STEP = 0.03;

  const gdouble GLOW_STEP = 0.085;

  switch (priv->direction)
  {

    case AWN_EFFECT_DIR_DOWN:
      priv->alpha -= ALPHA_STEP;
      priv->glow_amount += GLOW_STEP;

      if (priv->alpha < 0)
      {
        priv->alpha = 0.0;
        priv->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_DOWN;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_NONE)
  {
    priv->alpha = 1.0;
    priv->glow_amount = 0.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
glow_attention_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
    priv->glow_amount = 0;
  }

  const gint PERIOD = 20;

  const gfloat MAX_GLOW = 1.5;

  if (priv->direction == AWN_EFFECT_DIR_UP)
  {
    priv->glow_amount += MAX_GLOW / PERIOD;
  }
  else
  {
    priv->glow_amount -= MAX_GLOW / PERIOD;
  }

  if (priv->glow_amount >= MAX_GLOW)
    priv->direction = AWN_EFFECT_DIR_DOWN;
  else if (priv->glow_amount <= 0.0)
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->glow_amount <= 0)
  {
    priv->count = 0;
    priv->glow_amount = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
glow_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("glow_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
