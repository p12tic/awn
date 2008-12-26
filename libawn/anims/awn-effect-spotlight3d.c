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

#include "awn-effect-spotlight3d.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
spotlight3D_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->top_offset = 0;
    priv->spotlight_alpha = 1.0;
    priv->spotlight = TRUE;
    priv->glow_amount = priv->spotlight_alpha;
    priv->delta_width = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gint PERIOD = 44;

  const gdouble ALPHA_STEP = 0.04;

  if (awn_effect_check_top_effect(anim, NULL))
  {
    priv->spotlight_alpha = 1.0;
  }
  else
  {
    priv->spotlight_alpha -= ALPHA_STEP;

    if (priv->spotlight_alpha < 0)
    {
      priv->spotlight_alpha = 0;
    }
  }

  priv->glow_amount = priv->spotlight_alpha;

  gint prev_count = priv->count;

  if (prev_count > PERIOD)
    prev_count = --priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->delta_width = -priv->count * (priv->icon_width) / (PERIOD / 4);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->delta_width =
      (priv->count - PERIOD / 4) * (priv->icon_width) / (PERIOD / 4) -
      priv->icon_width;
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->delta_width =
      -(priv->count - PERIOD / 2) * (priv->icon_width) / (PERIOD / 4);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->delta_width =
      (priv->count - PERIOD * 3 / 4) * (priv->icon_width) / (PERIOD / 4) -
      priv->icon_width;
    priv->flip = FALSE;
  }

  priv->icon_depth = 10.00 * -priv->delta_width / priv->icon_width;

  priv->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;

  if (abs(priv->delta_width) >= priv->icon_width - MIN_WIDTH)
  {
    if (priv->delta_width > 0)
      priv->delta_width = priv->icon_width - MIN_WIDTH;
    else
      priv->delta_width = -priv->icon_width + MIN_WIDTH;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD
      && (priv->spotlight_alpha >= 1 || priv->spotlight_alpha <= 0))
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->delta_width = 0;
    priv->flip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);

    if (!repeat)
      priv->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight3D_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->top_offset = 0;
    priv->spotlight_alpha = 1.0;
    priv->spotlight = TRUE;
    priv->glow_amount = priv->spotlight_alpha;
    priv->clip = TRUE;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.width = priv->icon_width;
    priv->clip_region.height = 0;
    priv->delta_width = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gint PERIOD = 44;

  const gint MAX_OFFSET = priv->icon_height / 2;

  gint prev_count = priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->clip_region.height = priv->count * (priv->icon_height) / (PERIOD / 2);
    priv->delta_width = -priv->count * (priv->icon_width) / (PERIOD / 4);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->clip_region.height = (priv->count) * (priv->icon_height) / (PERIOD / 2);
    priv->delta_width =
      (priv->count - PERIOD / 4) * (priv->icon_width) / (PERIOD / 4) -
      priv->icon_width;
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->clip = FALSE;
    priv->top_offset = (priv->count - PERIOD / 2) * MAX_OFFSET / (PERIOD / 4);
    priv->delta_width =
      -(priv->count - PERIOD / 2) * (priv->icon_width) / (PERIOD / 4);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->top_offset =
      MAX_OFFSET - (priv->count - PERIOD * 3 / 4) * MAX_OFFSET / (PERIOD / 4);
    priv->delta_width =
      (priv->count - PERIOD * 3 / 4) * (priv->icon_width) / (PERIOD / 4) -
      priv->icon_width;
    priv->flip = FALSE;
    priv->spotlight_alpha =
      -(priv->count - PERIOD * 3 / 4) * 1.0 / (PERIOD / 4) + 1.0;
  }

  priv->icon_depth = 10.00 * -priv->delta_width / priv->icon_width;

  priv->glow_amount = priv->spotlight_alpha;

  priv->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;

  if (abs(priv->delta_width) >= priv->icon_width - MIN_WIDTH)
  {
    if (priv->delta_width > 0)
      priv->delta_width = priv->icon_width - MIN_WIDTH;
    else
      priv->delta_width = -priv->icon_width + MIN_WIDTH;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->delta_width = 0;
    priv->flip = FALSE;
    priv->spotlight = FALSE;
    priv->spotlight_alpha = 0.0;
    priv->glow_amount = 0.0;
    priv->clip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
spotlight3D_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->spotlight_alpha = 0.0;
    priv->spotlight = TRUE;
    priv->glow_amount = priv->spotlight_alpha;
    priv->clip = TRUE;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.height = priv->icon_height;
    priv->clip_region.width = priv->icon_width;
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;

    priv->count = 0;

    priv->delta_width = 0;

    priv->icon_depth = 0;

    priv->icon_depth_direction = 0;
  }

  const gint PERIOD = 80;

  const gint TURN_PERIOD = 20;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    priv->spotlight_alpha += 4.0 / PERIOD;

    if (priv->spotlight_alpha >= 1)
    {
      priv->spotlight_alpha = 1;
      priv->direction = AWN_EFFECT_DIR_NONE;
    }
  }
  else if (priv->direction == AWN_EFFECT_DIR_NONE)
  {
    priv->clip_region.height -= 2.0 * priv->icon_height / PERIOD;
    priv->alpha -= 2.0 / PERIOD;

    if (priv->count < TURN_PERIOD / 4)
    {
      priv->icon_depth_direction = 0;
      priv->delta_width = -priv->count * (priv->icon_width) / (TURN_PERIOD / 4);
      priv->flip = FALSE;
    }
    else if (priv->count < TURN_PERIOD / 2)
    {
      priv->icon_depth_direction = 1;
      priv->delta_width =
        (priv->count - TURN_PERIOD / 4) * (priv->icon_width) / (TURN_PERIOD / 4) -
        priv->icon_width;
      priv->flip = TRUE;
    }
    else if (priv->count < TURN_PERIOD * 3 / 4)
    {
      priv->icon_depth_direction = 0;
      priv->delta_width =
        -(priv->count - TURN_PERIOD / 2) * (priv->icon_width) / (TURN_PERIOD / 4);
      priv->flip = TRUE;
    }
    else
    {
      priv->icon_depth_direction = 1;
      priv->delta_width =
        (priv->count -
         TURN_PERIOD * 3 / 4) * (priv->icon_width) / (TURN_PERIOD / 4) -
        priv->icon_width;
      priv->flip = FALSE;
    }

    priv->icon_depth = 10.00 * -priv->delta_width / priv->icon_width;

    // fix icon flickering
    const gint MIN_WIDTH = 4;

    if (abs(priv->delta_width) >= priv->icon_width - MIN_WIDTH)
    {
      if (priv->delta_width > 0)
        priv->delta_width = priv->icon_width - MIN_WIDTH;
      else
        priv->delta_width = -priv->icon_width + MIN_WIDTH;
    }

    if (priv->count++ > TURN_PERIOD)
      priv->count = 0;

    if (priv->alpha <= 0 || priv->clip_region.height <= 0)
    {
      priv->alpha = 0;
      priv->direction = AWN_EFFECT_SPOTLIGHT_OFF;
      priv->clip = FALSE;
    }
    else if (priv->alpha <= 0.5)
    {
      priv->spotlight_alpha -= 2.0 / PERIOD;
    }
  }
  else
  {
    priv->spotlight_alpha -= 2.0 / PERIOD;
  }

  priv->glow_amount = priv->spotlight_alpha;

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_OFF && priv->spotlight_alpha <= 0)
  {
    priv->spotlight_alpha = 0;
    priv->glow_amount = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);

    if (!repeat)
      priv->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight3D_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("spotlight3d_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
