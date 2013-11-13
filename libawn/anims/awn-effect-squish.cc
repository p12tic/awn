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

#include "awn-effect-squish.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
bounce_squish_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  const gint PERIOD = 20;
  const gfloat MAX_SQUISH = 1.25;
  const gfloat SQUISH_STEP = 0.0834; // 3 frames to get to max (0.25 / 3)
  const gfloat SQUISH_STEP2 = 0.125;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
    case AWN_EFFECT_SQUISH_DOWN2:
      priv->width_mod += SQUISH_STEP;
      priv->height_mod -= SQUISH_STEP2;

      if (priv->width_mod >= MAX_SQUISH)
        priv->direction = priv->direction == AWN_EFFECT_SQUISH_DOWN ?
          AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:
    case AWN_EFFECT_SQUISH_UP2:
      priv->width_mod -= SQUISH_STEP;
      priv->height_mod += SQUISH_STEP2;

      if (priv->height_mod >= 1.0 && priv->direction == AWN_EFFECT_SQUISH_UP)
      {
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
        priv->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      if (priv->count == PERIOD / 4)
      {
        /* suspend in middle */
        if (awn_effect_check_top_effect(anim, NULL))
          return awn_effect_suspend_animation(anim,
                   (GSourceFunc)bounce_squish_hover_effect);
      }

      if (priv->count >= PERIOD / 2)
      {
        priv->top_offset = 0;
        priv->direction = AWN_EFFECT_SQUISH_DOWN2;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SQUISH_UP2 && priv->height_mod >= 1.0)
  {
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  const gint PERIOD = 20;
  const gfloat MAX_SQUISH = 1.25;
  const gfloat SQUISH_STEP = 0.0834; // 3 frames to get to max (0.25 / 3)
  const gfloat SQUISH_STEP2 = 0.125;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
    case AWN_EFFECT_SQUISH_DOWN2:
      priv->width_mod += SQUISH_STEP;
      priv->height_mod -= SQUISH_STEP2;

      if (priv->width_mod >= MAX_SQUISH)
        priv->direction = priv->direction == AWN_EFFECT_SQUISH_DOWN ?
          AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:
    case AWN_EFFECT_SQUISH_UP2:
      priv->width_mod -= SQUISH_STEP;
      priv->height_mod += SQUISH_STEP2;

      if (priv->height_mod >= 1.0 && priv->direction == AWN_EFFECT_SQUISH_UP)
      {
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
        priv->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      if (priv->count >= PERIOD / 2)
      {
        priv->top_offset = 0;
        priv->direction = AWN_EFFECT_SQUISH_DOWN2;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SQUISH_UP2 && priv->height_mod >= 1.0)
  {
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_attention_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  const gint PERIOD = 20;
  const gfloat MAX_SQUISH = 1.25;
  const gfloat SQUISH_STEP = 0.0834; // 3 frames to get to max (0.25 / 3)
  const gfloat SQUISH_STEP2 = 0.125;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
    case AWN_EFFECT_SQUISH_DOWN2:
      priv->width_mod += SQUISH_STEP;
      priv->height_mod -= SQUISH_STEP2;

      if (priv->width_mod >= MAX_SQUISH)
        priv->direction = priv->direction == AWN_EFFECT_SQUISH_DOWN ?
          AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:
    case AWN_EFFECT_SQUISH_UP2:
      priv->width_mod -= SQUISH_STEP;
      priv->height_mod += SQUISH_STEP2;

      if (priv->height_mod >= 1.0 && priv->direction == AWN_EFFECT_SQUISH_UP)
      {
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
        priv->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      priv->width_mod = 1 + sin(priv->count * M_PI * 2 / PERIOD) * (1. / 8);
      priv->height_mod = priv->width_mod;

      if (priv->count >= PERIOD / 2)
      {
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
        priv->top_offset = 0;
        priv->direction = AWN_EFFECT_SQUISH_DOWN2;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SQUISH_UP2 && priv->height_mod >= 1.0)
  {
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->width_mod = 0;
    priv->height_mod = 0;
  }

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  const gint PERIOD = 18;

  const gfloat MAX_SQUISH = 1.25;
  const gfloat SQUISH_STEP = 0.0834; // 3 frames to get to max (0.25 / 3)
  const gfloat SQUISH_STEP2 = 0.125;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
      priv->width_mod += SQUISH_STEP;
      priv->height_mod -= SQUISH_STEP2;

      if (priv->width_mod >= MAX_SQUISH)
        priv->direction = AWN_EFFECT_SQUISH_UP;

      break;

    case AWN_EFFECT_SQUISH_UP:
      priv->width_mod -= SQUISH_STEP;
      priv->height_mod += SQUISH_STEP2;

      if (priv->height_mod >= 1.0)
      {
        priv->direction = AWN_EFFECT_DIR_NONE;
        priv->count = 0;
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

      if (priv->width_mod < 1.0)
      {
        priv->width_mod += 1. / PERIOD;
        priv->height_mod = priv->width_mod;
      }

      if (priv->count == PERIOD)
      {
        priv->direction = AWN_EFFECT_SQUISH_DOWN;
        priv->top_offset = 0;
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_NONE;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_NONE && priv->count <= 0)
  {
    priv->top_offset = 0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}


gboolean
bounce_squish_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
  }

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  const gint PERIOD = 18;

  const gfloat MAX_SQUISH = 1.25;
  const gfloat SQUISH_STEP = 0.0834; // 3 frames to get to max (0.25 / 3)
  const gfloat SQUISH_STEP2 = 0.125;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
      priv->width_mod += SQUISH_STEP;
      priv->height_mod -= SQUISH_STEP2;

      if (priv->width_mod >= MAX_SQUISH)
        priv->direction = AWN_EFFECT_SQUISH_UP;
    break;

    case AWN_EFFECT_SQUISH_UP:
      priv->width_mod -= SQUISH_STEP;
      priv->height_mod += SQUISH_STEP2;

      if(priv->height_mod >= 1.0)
      {
        priv->direction = AWN_EFFECT_DIR_DOWN;
        priv->count = 0;
        priv->width_mod = 1.0;
        priv->height_mod = 1.0;
      }
      break;

    case AWN_EFFECT_DIR_DOWN:
      priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;
      if (priv->width_mod > 0.0)
      {
        priv->width_mod -= 1.0/PERIOD;
        priv->height_mod = priv->width_mod;
      }

      if (priv->count == PERIOD)
      {
        priv->direction = AWN_EFFECT_DIR_NONE;
        priv->top_offset = 0;
        priv->width_mod = 0.0;
        priv->height_mod = 0.0;
      }

      break;

    case AWN_EFFECT_DIR_NONE: /* reset cycle */
    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
      priv->count = 0;
      priv->width_mod = 1.0;
      priv->height_mod = 1.0;

      break;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_NONE && priv->count == PERIOD)
  {
    priv->top_offset = 0;
    priv->count = 0;
    priv->width_mod = 1.0;
    priv->height_mod = 1.0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("bounce_squish_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
