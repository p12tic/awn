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

// simple bounce effect based on sin function
gboolean
bounce_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gdouble MAX_BOUNCE_OFFSET = 15.0;

  const gint PERIOD = 20;

  fx->y_offset = sin(++fx->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

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
bounce_opening_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->y_offset = 0;
    fx->clip = TRUE;
    fx->clip_region.x = 0;
    fx->clip_region.y = 0;
    fx->clip_region.width = fx->icon_width;
    fx->clip_region.height = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD1 = 15;

  const gint PERIOD2 = 20;

  const gint MAX_BOUNCE_OFFSET = 15;

  if (fx->count < PERIOD1)
    fx->clip_region.height = fx->icon_height * ++fx->count / PERIOD1;
  else if (fx->count < PERIOD1 + PERIOD2)
  {
    fx->clip = FALSE;
    fx->y_offset =
      sin((++fx->count - PERIOD1) * M_PI / PERIOD2) * MAX_BOUNCE_OFFSET;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->count >= PERIOD1 + PERIOD2)
  {
    fx->count = 0;
    fx->y_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
bounce_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("bounce_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}
