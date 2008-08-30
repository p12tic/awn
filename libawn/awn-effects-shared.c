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

#include "awn-effects-shared.h"

gboolean
awn_effect_check_top_effect(AwnEffectsPrivate * priv, gboolean * stopped)
{
  if (stopped)
    *stopped = TRUE;

  AwnEffects *fx = priv->effects;

  GList *queue = fx->effect_queue;

  AwnEffectsPrivate *item;

  while (queue)
  {
    item = queue->data;

    if (item->this_effect == priv->this_effect)
    {
      if (stopped)
        *stopped = FALSE;

      break;
    }

    queue = g_list_next(queue);
  }

  if (!fx->effect_queue)
    return FALSE;

  item = fx->effect_queue->data;

  return item->this_effect == priv->this_effect;
}


gboolean
awn_effect_handle_repeating(AwnEffectsPrivate * priv)
{
  gboolean effect_stopped = TRUE;
  gboolean max_reached = awn_effect_check_max_loops(priv);
  gboolean repeat = !max_reached
                    && awn_effect_check_top_effect(priv, &effect_stopped);

  if (!repeat)
  {
    gboolean unregistered = FALSE;
    AwnEffects *fx = priv->effects;
    fx->current_effect = AWN_EFFECT_NONE;
    fx->effect_lock = FALSE;
    fx->timer_id = 0;

    if (effect_stopped)
    {
      if (priv->stop)
        priv->stop(fx->self);

      unregistered = fx->self == NULL;

      g_free(priv);
    }

    if (!unregistered)
      awn_effects_main_effect_loop(fx);
  }

  return repeat;
}


gboolean
awn_effect_check_max_loops(AwnEffectsPrivate * priv)
{
  gboolean max_reached = FALSE;

  if (priv->max_loops > 0)
  {
    priv->max_loops--;
    max_reached = priv->max_loops <= 0;
  }

  if (max_reached)
    awn_effects_stop(priv->effects, priv->this_effect);

  return max_reached;
}
