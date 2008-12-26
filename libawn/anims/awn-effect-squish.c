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

#include "awn-effect-squish.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
bounce_squish_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 28;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:

    case AWN_EFFECT_SQUISH_DOWN2:
      priv->delta_width += (priv->icon_width * 3 / 4) / (PERIOD / 4);
      priv->delta_height -= (priv->icon_height * 3 / 4) / (PERIOD / 4);

      if (priv->delta_height <= priv->icon_height * -1 / 4)
        priv->direction =
          priv->direction ==
          AWN_EFFECT_SQUISH_DOWN ? AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:

    case AWN_EFFECT_SQUISH_UP2:
      priv->delta_width -= (priv->icon_width * 3 / 4) / (PERIOD / 4);

      priv->delta_height += (priv->icon_height * 3 / 4) / (PERIOD / 4);

      if (priv->delta_height >= 0 && priv->direction == AWN_EFFECT_SQUISH_UP)
        priv->direction = AWN_EFFECT_DIR_NONE;

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      if (priv->count >= PERIOD / 2)
        priv->direction = AWN_EFFECT_SQUISH_DOWN2;

      break;

    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SQUISH_UP2 && priv->delta_height >= 0)
  {
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_attention_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    priv->direction = AWN_EFFECT_SQUISH_DOWN;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 28;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:

    case AWN_EFFECT_SQUISH_DOWN2:
      priv->delta_width += (priv->icon_width * 3 / 4) / (PERIOD / 4);
      priv->delta_height -= (priv->icon_height * 3 / 4) / (PERIOD / 4);

      if (priv->delta_height <= priv->icon_height * -1 / 4)
        priv->direction =
          priv->direction ==
          AWN_EFFECT_SQUISH_DOWN ? AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:

    case AWN_EFFECT_SQUISH_UP2:
      priv->delta_width -= (priv->icon_width * 3 / 4) / (PERIOD / 4);

      priv->delta_height += (priv->icon_height * 3 / 4) / (PERIOD / 4);

      if (priv->delta_height >= 0 && priv->direction == AWN_EFFECT_SQUISH_UP)
        priv->direction = AWN_EFFECT_DIR_NONE;

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      priv->delta_width =
        sin(priv->count * M_PI * 2 / PERIOD) * (priv->icon_width * 1 / 6);

      priv->delta_height =
        sin(priv->count * M_PI * 2 / PERIOD) * (priv->icon_width * 1 / 6);

      if (priv->count >= PERIOD / 2)
      {
        priv->direction = AWN_EFFECT_SQUISH_DOWN2;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SQUISH_UP2 && priv->delta_height >= 0)
  {
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_squish_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->direction = AWN_EFFECT_DIR_NONE;
    priv->delta_width = -priv->icon_width;
    priv->delta_height = -priv->icon_height;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 20;

  const gint PERIOD2 = 28;

  switch (priv->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
      priv->delta_width += (priv->icon_width * 3 / 4) / (PERIOD2 / 4);
      priv->delta_height -= (priv->icon_height * 3 / 4) / (PERIOD2 / 4);

      if (priv->delta_height <= priv->icon_height * -1 / 4)
        priv->direction = AWN_EFFECT_SQUISH_UP;

      break;

    case AWN_EFFECT_SQUISH_UP:
      priv->delta_width -= (priv->icon_width * 3 / 4) / (PERIOD2 / 4);

      priv->delta_height += (priv->icon_height * 3 / 4) / (PERIOD2 / 4);

      if (priv->delta_height >= 0)
      {
        priv->direction = AWN_EFFECT_DIR_NONE;
        priv->count = 0;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

      if (priv->delta_width < 0)
        priv->delta_width += priv->icon_width * 2 / PERIOD;

      if (priv->delta_height < 0)
        priv->delta_height += priv->icon_height * 2 / PERIOD;

      if (priv->count == PERIOD)
      {
        priv->direction = AWN_EFFECT_SQUISH_DOWN;
        priv->top_offset = 0;
        priv->delta_width = 0;
        priv->delta_height = 0;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_NONE;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_NONE && priv->count <= 0)
  {
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}


gboolean
bounce_squish_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  if (!priv->effect_lock)
  {
    priv->effect_lock = TRUE;
    // effect start initialize values
    priv->count = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
    priv->delta_width = -priv->icon_width;
    priv->delta_height = -priv->icon_height;

    if (anim->start)
      anim->start(priv->self);

    anim->start = NULL;
  }

  const gdouble MAX_OFFSET = 50.0;

  const gint PERIOD = 20;

  priv->top_offset = ++priv->count * (MAX_OFFSET / PERIOD);

  priv->alpha = priv->count * (-1.0 / PERIOD) + 1;

  priv->delta_width = -priv->count * (priv->icon_width / PERIOD);

  priv->delta_height = -priv->count * (priv->icon_height / PERIOD);

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (MAX_OFFSET == priv->top_offset)
  {
    priv->count = 0;
    // check for repeating
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
