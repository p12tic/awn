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
zoom_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    fx->y_offset = 0;
    fx->direction = AWN_EFFECT_DIR_UP;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  switch (fx->direction)
  {

    case AWN_EFFECT_DIR_UP:

      if (fx->delta_width + fx->icon_width < fx->window_width)
      {
        fx->delta_width += 2;
        fx->delta_height += 2;
        fx->y_offset += 1;
      }

      gboolean top = awn_effect_check_top_effect(priv, NULL);

      if (top)
      {
        gtk_widget_queue_draw(GTK_WIDGET(fx->self));
        return top;
      }
      else
        fx->direction = AWN_EFFECT_DIR_DOWN;

      break;

    case AWN_EFFECT_DIR_DOWN:
      fx->delta_width -= 2;

      fx->delta_height -= 2;

      fx->y_offset -= 1;

      if (fx->delta_width <= 0)
      {
        fx->direction = AWN_EFFECT_DIR_UP;
        fx->delta_width = 0;
        fx->delta_height = 0;
        fx->y_offset = 0;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_DIR_UP;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_UP && !fx->delta_width
      && !fx->delta_height)
  {
    fx->y_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
zoom_attention_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    fx->y_offset = 0;
    fx->direction = AWN_EFFECT_DIR_UP;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  switch (fx->direction)
  {

    case AWN_EFFECT_DIR_UP:

      if (fx->delta_width + fx->icon_width < fx->window_width)
      {
        fx->delta_width += 2;
        fx->delta_height += 2;
        fx->y_offset += 1;
      }
      else
      {
        fx->direction = AWN_EFFECT_DIR_DOWN;
      }

      break;

    case AWN_EFFECT_DIR_DOWN:
      fx->delta_width -= 2;
      fx->delta_height -= 2;
      fx->y_offset -= 1;

      if (fx->delta_width <= 0)
      {
        fx->direction = AWN_EFFECT_DIR_UP;
        fx->delta_width = 0;
        fx->delta_height = 0;
        fx->y_offset = 0;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_DIR_UP;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_UP && !fx->delta_width
      && !fx->delta_height)
  {
    fx->y_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
zoom_opening_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = -fx->icon_width;
    fx->delta_height = -fx->icon_width;
    fx->alpha = 0.0;
    fx->y_offset = 0;
    fx->direction = AWN_EFFECT_DIR_UP;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 20;

  fx->delta_width += (fx->icon_width) / PERIOD;

  fx->delta_height += (fx->icon_width) / PERIOD;

  fx->alpha += 1.0 / PERIOD;

  // repaint widget
  if (fx->self && GTK_IS_WIDGET(fx->self))
  {
    gtk_widget_queue_draw(GTK_WIDGET(fx->self));
  }

  gboolean repeat = TRUE;

  if (fx->delta_width > 0)
  {
    fx->y_offset = 0;
    fx->alpha = 1.0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
zoom_closing_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->delta_width = 0;
    fx->delta_height = 0;
    fx->alpha = 1.0;
    fx->y_offset = 0;
    fx->direction = AWN_EFFECT_DIR_UP;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 20;

  fx->delta_width -= (fx->icon_width) / PERIOD;

  fx->delta_height -= (fx->icon_width) / PERIOD;

  fx->alpha -= 1.0 / PERIOD;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->alpha < 0.0)
  {
    fx->y_offset = 0;
    fx->alpha = 0.0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
zoom_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("bounce_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}


