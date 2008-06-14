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

#include "awn-effects.h"
#include "awn-effects-shared.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
bounce_squish_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    fx->direction = AWN_EFFECT_SQUISH_DOWN;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 28;

  switch (fx->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:

    case AWN_EFFECT_SQUISH_DOWN2:
      fx->delta_width += (fx->icon_width * 3 / 4) / (PERIOD / 4);
      fx->delta_height -= (fx->icon_height * 3 / 4) / (PERIOD / 4);

      if (fx->delta_height <= fx->icon_height * -1 / 4)
        fx->direction =
          fx->direction ==
          AWN_EFFECT_SQUISH_DOWN ? AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:

    case AWN_EFFECT_SQUISH_UP2:
      fx->delta_width -= (fx->icon_width * 3 / 4) / (PERIOD / 4);

      fx->delta_height += (fx->icon_height * 3 / 4) / (PERIOD / 4);

      if (fx->delta_height >= 0 && fx->direction == AWN_EFFECT_SQUISH_UP)
        fx->direction = AWN_EFFECT_DIR_NONE;

      break;

    case AWN_EFFECT_DIR_NONE:
      fx->y_offset = sin(++fx->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      if (fx->count >= PERIOD / 2)
        fx->direction = AWN_EFFECT_SQUISH_DOWN2;

      break;

    default:
      fx->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_SQUISH_UP2 && fx->delta_height >= 0)
  {
    fx->direction = AWN_EFFECT_DIR_NONE;
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
bounce_squish_attention_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    fx->direction = AWN_EFFECT_SQUISH_DOWN;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 28;

  switch (fx->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:

    case AWN_EFFECT_SQUISH_DOWN2:
      fx->delta_width += (fx->icon_width * 3 / 4) / (PERIOD / 4);
      fx->delta_height -= (fx->icon_height * 3 / 4) / (PERIOD / 4);

      if (fx->delta_height <= fx->icon_height * -1 / 4)
        fx->direction =
          fx->direction ==
          AWN_EFFECT_SQUISH_DOWN ? AWN_EFFECT_SQUISH_UP : AWN_EFFECT_SQUISH_UP2;

      break;

    case AWN_EFFECT_SQUISH_UP:

    case AWN_EFFECT_SQUISH_UP2:
      fx->delta_width -= (fx->icon_width * 3 / 4) / (PERIOD / 4);

      fx->delta_height += (fx->icon_height * 3 / 4) / (PERIOD / 4);

      if (fx->delta_height >= 0 && fx->direction == AWN_EFFECT_SQUISH_UP)
        fx->direction = AWN_EFFECT_DIR_NONE;

      break;

    case AWN_EFFECT_DIR_NONE:
      fx->y_offset = sin(++fx->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;

      fx->delta_width =
        sin(fx->count * M_PI * 2 / PERIOD) * (fx->icon_width * 1 / 6);

      fx->delta_height =
        sin(fx->count * M_PI * 2 / PERIOD) * (fx->icon_width * 1 / 6);

      if (fx->count >= PERIOD / 2)
      {
        fx->direction = AWN_EFFECT_SQUISH_DOWN2;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_SQUISH_DOWN;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_SQUISH_UP2 && fx->delta_height >= 0)
  {
    fx->direction = AWN_EFFECT_DIR_NONE;
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
bounce_squish_opening_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->direction = AWN_EFFECT_DIR_NONE;
    fx->delta_width = -fx->icon_width;
    fx->delta_height = -fx->icon_height;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 20;

  const gint PERIOD2 = 28;

  switch (fx->direction)
  {

    case AWN_EFFECT_SQUISH_DOWN:
      fx->delta_width += (fx->icon_width * 3 / 4) / (PERIOD2 / 4);
      fx->delta_height -= (fx->icon_height * 3 / 4) / (PERIOD2 / 4);

      if (fx->delta_height <= fx->icon_height * -1 / 4)
        fx->direction = AWN_EFFECT_SQUISH_UP;

      break;

    case AWN_EFFECT_SQUISH_UP:
      fx->delta_width -= (fx->icon_width * 3 / 4) / (PERIOD2 / 4);

      fx->delta_height += (fx->icon_height * 3 / 4) / (PERIOD2 / 4);

      if (fx->delta_height >= 0)
      {
        fx->direction = AWN_EFFECT_DIR_NONE;
        fx->count = 0;
      }

      break;

    case AWN_EFFECT_DIR_NONE:
      fx->y_offset = sin(++fx->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

      if (fx->delta_width < 0)
        fx->delta_width += fx->icon_width * 2 / PERIOD;

      if (fx->delta_height < 0)
        fx->delta_height += fx->icon_height * 2 / PERIOD;

      if (fx->count == PERIOD)
      {
        fx->direction = AWN_EFFECT_SQUISH_DOWN;
        fx->y_offset = 0;
        fx->delta_width = 0;
        fx->delta_height = 0;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_DIR_NONE;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_NONE && fx->count <= 0)
  {
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}


gboolean
bounce_squish_closing_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->direction = AWN_EFFECT_DIR_UP;
    fx->delta_width = -fx->icon_width;
    fx->delta_height = -fx->icon_height;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_OFFSET = 50.0;

  const gint PERIOD = 20;

  fx->y_offset = ++fx->count * (MAX_OFFSET / PERIOD);

  fx->alpha = fx->count * (-1.0 / PERIOD) + 1;

  fx->delta_width = -fx->count * (fx->icon_width / PERIOD);

  fx->delta_height = -fx->count * (fx->icon_height / PERIOD);

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (MAX_OFFSET == fx->y_offset)
  {
    fx->count = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
bounce_squish_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("bounce_squish_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}
