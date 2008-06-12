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
fade_out_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->alpha = 1.0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_OFFSET = 50.0;

  const gint PERIOD = 20;

  fx->y_offset = ++fx->count * (MAX_OFFSET / PERIOD);

  fx->alpha = fx->count * (-1.0 / PERIOD) + 1;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->count >= PERIOD)
  {
    fx->count = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
fading_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->alpha = 1.0;
    fx->direction = AWN_EFFECT_DIR_DOWN;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MIN_ALPHA = 0.35;

  const gdouble ALPHA_STEP = 0.05;

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_DIR_DOWN)
  {
    fx->alpha -= ALPHA_STEP;

    if (fx->alpha <= MIN_ALPHA)
      fx->direction = AWN_EFFECT_DIR_UP;

    // repaint widget
    gtk_widget_queue_draw(GTK_WIDGET(fx->self));
  }
  else
  {
    fx->alpha += ALPHA_STEP * 1.5;
    // repaint widget
    gtk_widget_queue_draw(GTK_WIDGET(fx->self));

    if (fx->alpha >= 1)
    {
      fx->alpha = 1.0;
      fx->direction = AWN_EFFECT_DIR_DOWN;
      repeat = awn_effect_handle_repeating(priv);
    }
  }

  return repeat;
}

gboolean
fading_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("fading_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}
