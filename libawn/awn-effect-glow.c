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
glow_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->glow_amount = 1.0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gfloat GLOW_STEP = 0.08;

  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  // check for repeating
  gboolean top = awn_effect_check_top_effect(priv, NULL);

  if (top)
  {
    fx->glow_amount = 1.0;
    return top;   // == TRUE
  }
  else
  {
    fx->glow_amount -= GLOW_STEP;

    if (fx->glow_amount <= 0)
    {
      fx->glow_amount = 0.0;
      gboolean repeat = awn_effect_handle_repeating(priv);
      return repeat;  // == FALSE
    }
  }

  return TRUE;
}

gboolean
glow_opening_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->direction = AWN_EFFECT_DIR_UP;
    fx->alpha = 0.0;
    fx->glow_amount = 1.95;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble ALPHA_STEP = 0.04;

  const gdouble GLOW_STEP = 0.05;

  switch (fx->direction)
  {

    case AWN_EFFECT_DIR_UP:
      fx->alpha += ALPHA_STEP;

      if (fx->alpha > 1)
      {
        fx->alpha = 1.0;
        fx->direction = AWN_EFFECT_DIR_DOWN;
      }

      break;

    case AWN_EFFECT_DIR_DOWN:
      fx->glow_amount -= GLOW_STEP;

      if (fx->glow_amount < 0)
      {
        fx->glow_amount = 0.0;
        fx->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_DIR_DOWN;
  }

  // repaint widget
  if (fx->self && GTK_IS_WIDGET(fx->self))
  {
    gtk_widget_queue_draw(GTK_WIDGET(fx->self));
  }

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_NONE)
  {
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
glow_closing_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->direction = AWN_EFFECT_DIR_DOWN;
    fx->glow_amount = 0.8;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble ALPHA_STEP = 0.03;

  const gdouble GLOW_STEP = 0.085;

  switch (fx->direction)
  {

    case AWN_EFFECT_DIR_DOWN:
      fx->alpha -= ALPHA_STEP;
      fx->glow_amount += GLOW_STEP;

      if (fx->alpha < 0)
      {
        fx->alpha = 0.0;
        fx->direction = AWN_EFFECT_DIR_NONE;
      }

      break;

    default:
      fx->direction = AWN_EFFECT_DIR_DOWN;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_NONE)
  {
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
glow_attention_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->direction = AWN_EFFECT_DIR_UP;
    fx->glow_amount = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 20;

  const gfloat MAX_GLOW = 1.5;

  if (fx->direction == AWN_EFFECT_DIR_UP)
  {
    fx->glow_amount += MAX_GLOW / PERIOD;
  }
  else
  {
    fx->glow_amount -= MAX_GLOW / PERIOD;
  }

  if (fx->glow_amount >= MAX_GLOW)
    fx->direction = AWN_EFFECT_DIR_DOWN;
  else if (fx->glow_amount <= 0.0)
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->glow_amount <= 0)
  {
    fx->count = 0;
    fx->glow_amount = 0;
    fx->direction = AWN_EFFECT_DIR_UP;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
glow_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("glow_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}
