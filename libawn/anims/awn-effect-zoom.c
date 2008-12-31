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

#include "awn-effect-zoom.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

// FIXME: because of orientation support the effect shouldn't use window_width

gboolean
zoom_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
  }

  gint max;
  switch (anim->effects->orientation)
  {
    case AWN_EFFECT_ORIENT_LEFT:
    case AWN_EFFECT_ORIENT_RIGHT:
      max = priv->window_height;
      break;
    default:
      max = priv->window_width;
      break;
  }

  switch (priv->direction)
  {

    case AWN_EFFECT_DIR_UP:

      if (priv->delta_width + priv->icon_width < max)
      {
        priv->delta_width += priv->icon_width/8;
        priv->delta_height += priv->icon_width/8;
      }

      gboolean top = awn_effect_check_top_effect(anim, NULL);

      if (top)
      {
        awn_effects_redraw(anim->effects);
        if (priv->delta_width + priv->icon_width < max)
          return TRUE;
        else
          return awn_effect_suspend_animation(anim, (GSourceFunc)zoom_effect);
      }
      else
        priv->direction = AWN_EFFECT_DIR_DOWN;

      break;

    case AWN_EFFECT_DIR_DOWN:
      priv->delta_width -= priv->icon_width/8;
      priv->delta_height -= priv->icon_width/8;

      if (priv->delta_width <= 0)
      {
        priv->direction = AWN_EFFECT_DIR_UP;
        priv->delta_width = 0;
        priv->delta_height = 0;
        priv->top_offset = 0;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_UP;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_UP && !priv->delta_width
      && !priv->delta_height)
  {
    priv->top_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
zoom_attention_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
  }

  gint max;
  switch (anim->effects->orientation)
  {
    case AWN_EFFECT_ORIENT_LEFT:
    case AWN_EFFECT_ORIENT_RIGHT:
      max = priv->window_height;
      break;
    default:
      max = priv->window_width;
      break;
  }

  switch (priv->direction)
  {

    case AWN_EFFECT_DIR_UP:

      if (priv->delta_width + priv->icon_width < max)
      {
        priv->delta_width += 2;
        priv->delta_height += 2;
        priv->top_offset += 1;
      }
      else
      {
        priv->direction = AWN_EFFECT_DIR_DOWN;
      }

      break;

    case AWN_EFFECT_DIR_DOWN:
      priv->delta_width -= 2;
      priv->delta_height -= 2;
      priv->top_offset -= 1;

      if (priv->delta_width <= 0)
      {
        priv->direction = AWN_EFFECT_DIR_UP;
        priv->delta_width = 0;
        priv->delta_height = 0;
        priv->top_offset = 0;
      }

      break;

    default:
      priv->direction = AWN_EFFECT_DIR_UP;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_DIR_UP && !priv->delta_width
      && !priv->delta_height)
  {
    priv->top_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
zoom_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->delta_width = -priv->icon_width;
    priv->delta_height = -priv->icon_width;
    priv->alpha = 0.0;
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
  }

  const gint PERIOD = 20;

  priv->delta_width += (priv->icon_width) / PERIOD;

  priv->delta_height += (priv->icon_width) / PERIOD;

  priv->alpha += 1.0 / PERIOD;

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->delta_width > 0)
  {
    priv->top_offset = 0;
    priv->alpha = 1.0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
zoom_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->delta_width = 0;
    priv->delta_height = 0;
    priv->alpha = 1.0;
    priv->top_offset = 0;
    priv->direction = AWN_EFFECT_DIR_UP;
  }

  const gint PERIOD = 20;

  priv->delta_width -= (priv->icon_width) / PERIOD;

  priv->delta_height -= (priv->icon_width) / PERIOD;

  priv->alpha -= 1.0 / PERIOD;

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->alpha < 0.0)
  {
    priv->top_offset = 0;
    priv->alpha = 0.0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
zoom_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("bounce_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}


