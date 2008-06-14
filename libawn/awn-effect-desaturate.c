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
desaturate_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->direction = AWN_EFFECT_DIR_DOWN;
    fx->saturation = 1.0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble DESATURATION_STEP = 0.04;

  switch (fx->direction)
  {

    case AWN_EFFECT_DIR_DOWN:
      fx->saturation -= DESATURATION_STEP;

      if (fx->saturation < 0)
        fx->saturation = 0;

      gboolean top = awn_effect_check_top_effect(priv, NULL);

      // TODO: implement sleep function, so effect will stop the timer, but will be paused in middle of the animation and will finish when awn_effect_stop is called or higher priority effect is started
      if (top)
      {
        gtk_widget_queue_draw(GTK_WIDGET(fx->self));
        return top;
      }
      else
        fx->direction = AWN_EFFECT_DIR_UP;

      break;

    case AWN_EFFECT_DIR_UP:

    default:
      fx->saturation += DESATURATION_STEP;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->saturation >= 1.0)
  {
    fx->saturation = 1.0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
desaturate_effect_finalize(AwnEffectsPrivate * priv)
{

  return TRUE;
}
