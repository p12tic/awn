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

#include "awn-effect-turn.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
turn_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->top_offset = 0;
    priv->width_mod = 1.0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
  }

  const gint PERIOD = 36;

  gint prev_count = priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - priv->count / (PERIOD / 4.);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD / 4) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - (priv->count - PERIOD / 2) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD * 3 / 4) / (PERIOD / 4.);
    priv->flip = FALSE;
  }

  priv->icon_depth = 10.00 * (1 - priv->width_mod);

  priv->count = ++prev_count;

  /* fix icon flickering */
  const gfloat MIN_WIDTH = 0.1;

  if (priv->width_mod < MIN_WIDTH)
  {
    priv->width_mod = MIN_WIDTH;
  }
  else if (priv->width_mod > 1.0)
  {
    priv->width_mod = 1.0;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->width_mod = 1.0;
    priv->flip = FALSE;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
turn_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->top_offset = 0;
    priv->width_mod = 1.0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
  }

  const gint PERIOD = 36;

  gint prev_count = priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - priv->count / (PERIOD / 4.);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD / 4) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - (priv->count - PERIOD / 2) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD * 3 / 4) / (PERIOD / 4.);
    priv->flip = FALSE;
  }

  priv->icon_depth = 10.00 * (1 - priv->width_mod);

  priv->count = ++prev_count;

  /* fix icon flickering */
  const gfloat MIN_WIDTH = 0.1;

  if (priv->width_mod < MIN_WIDTH)
  {
    priv->width_mod = MIN_WIDTH;
  }
  else if (priv->width_mod > 1.0)
  {
    priv->width_mod = 1.0;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->width_mod = 1.0;
    priv->flip = FALSE;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
turn_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->top_offset = 0;
    priv->clip = TRUE;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.width = priv->icon_width;
    priv->clip_region.height = 0;
    priv->width_mod = 1.0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
  }

  const gint PERIOD = 36;
  const gint MAX_OFFSET = priv->icon_height / 2;

  gint prev_count = priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->clip_region.height = priv->count * (priv->icon_height) / (PERIOD / 2);
    priv->width_mod = 1 - priv->count / (PERIOD / 4.);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->clip_region.height = (priv->count) * (priv->icon_height) / (PERIOD / 2);
    priv->width_mod = (priv->count - PERIOD / 4) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->clip = FALSE;
    priv->top_offset = (priv->count - PERIOD / 2) * MAX_OFFSET / (PERIOD / 4);
    priv->width_mod = 1 - (priv->count - PERIOD / 2) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->top_offset =
      MAX_OFFSET - (priv->count - PERIOD * 3 / 4) * MAX_OFFSET / (PERIOD / 4);
    priv->width_mod = (priv->count - PERIOD * 3 / 4) / (PERIOD / 4.);
    priv->flip = FALSE;
  }

  priv->icon_depth = 10.00 * (1 - priv->width_mod);

  priv->count = ++prev_count;

  /* fix icon flickering */
  const gfloat MIN_WIDTH = 0.1;

  if (priv->width_mod < MIN_WIDTH)
  {
    priv->width_mod = MIN_WIDTH;
  }
  else if (priv->width_mod > 1.0)
  {
    priv->width_mod = 1.0;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->width_mod = 1.0;
    priv->flip = FALSE;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
turn_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->top_offset = 0;
    priv->width_mod = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
  }

  const gint PERIOD = 36;

  const gint MAX_OFFSET = priv->icon_height;

  gint prev_count = priv->count;

  priv->count = sin(priv->count * M_PI / 2 / PERIOD) * PERIOD;
  priv->top_offset = priv->count * MAX_OFFSET / PERIOD;
  priv->alpha = 1.0 - priv->count * 1.0 / PERIOD;

  if (priv->count < PERIOD / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - priv->count / (PERIOD / 4.);
    priv->flip = FALSE;
  }
  else if (priv->count < PERIOD / 2)
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD / 4) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else if (priv->count < PERIOD * 3 / 4)
  {
    priv->icon_depth_direction = 0;
    priv->width_mod = 1 - (priv->count - PERIOD / 2) / (PERIOD / 4.);
    priv->flip = TRUE;
  }
  else
  {
    priv->icon_depth_direction = 1;
    priv->width_mod = (priv->count - PERIOD * 3 / 4) / (PERIOD / 4.);
    priv->flip = FALSE;
  }

  priv->icon_depth = 10.00 * (1 - priv->width_mod);

  priv->count = ++prev_count;

  /* fix icon flickering */
  const gfloat MIN_WIDTH = 0.1;

  if (priv->width_mod < MIN_WIDTH)
  {
    priv->width_mod = MIN_WIDTH;
  }
  else if (priv->width_mod > 1.0)
  {
    priv->width_mod = 1.0;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    priv->icon_depth = 0;
    priv->icon_depth_direction = 0;
    priv->width_mod = 1.0;
    priv->alpha = 1.0;
    priv->flip = FALSE;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
turn_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("turn_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
