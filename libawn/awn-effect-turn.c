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
turn_hover_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->y_offset = 0;
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 44;

  gint prev_count = fx->count;

  fx->count = sin(fx->count * M_PI / 2 / PERIOD) * PERIOD;

  if (fx->count < PERIOD / 4)
  {
    fx->icon_depth_direction = 0;
    fx->delta_width = -fx->count * (fx->icon_width) / (PERIOD / 4);
    fx->flip = FALSE;
  }
  else if (fx->count < PERIOD / 2)
  {
    fx->icon_depth_direction = 1;
    fx->delta_width =
      (fx->count - PERIOD / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = TRUE;
  }
  else if (fx->count < PERIOD * 3 / 4)
  {
    fx->icon_depth_direction = 0;
    fx->delta_width =
      -(fx->count - PERIOD / 2) * (fx->icon_width) / (PERIOD / 4);
    fx->flip = TRUE;
  }
  else
  {
    fx->icon_depth_direction = 1;
    fx->delta_width =
      (fx->count - PERIOD * 3 / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = FALSE;
  }

  fx->icon_depth = 10.00 * -fx->delta_width / fx->icon_width;

  fx->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;

  if (abs(fx->delta_width) >= fx->icon_width - MIN_WIDTH)
  {
    if (fx->delta_width > 0)
      fx->delta_width = fx->icon_width - MIN_WIDTH;
    else
      fx->delta_width = -fx->icon_width + MIN_WIDTH;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->count >= PERIOD)
  {
    fx->count = 0;
    fx->y_offset = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    fx->delta_width = 0;
    fx->flip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
turn_opening_effect(AwnEffectsPrivate * priv)
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
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 44;

  const gint MAX_OFFSET = fx->icon_height / 2;

  gint prev_count = fx->count;

  fx->count = sin(fx->count * M_PI / 2 / PERIOD) * PERIOD;

  if (fx->count < PERIOD / 4)
  {
    fx->icon_depth_direction = 0;
    fx->clip_region.height = fx->count * (fx->icon_height) / (PERIOD / 2);
    fx->delta_width = -fx->count * (fx->icon_width) / (PERIOD / 4);
    fx->flip = FALSE;
  }
  else if (fx->count < PERIOD / 2)
  {
    fx->icon_depth_direction = 1;
    fx->clip_region.height = (fx->count) * (fx->icon_height) / (PERIOD / 2);
    fx->delta_width =
      (fx->count - PERIOD / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = TRUE;
  }
  else if (fx->count < PERIOD * 3 / 4)
  {
    fx->icon_depth_direction = 0;
    fx->clip = FALSE;
    fx->y_offset = (fx->count - PERIOD / 2) * MAX_OFFSET / (PERIOD / 4);
    fx->delta_width =
      -(fx->count - PERIOD / 2) * (fx->icon_width) / (PERIOD / 4);
    fx->flip = TRUE;
  }
  else
  {
    fx->icon_depth_direction = 1;
    fx->y_offset =
      MAX_OFFSET - (fx->count - PERIOD * 3 / 4) * MAX_OFFSET / (PERIOD / 4);
    fx->delta_width =
      (fx->count - PERIOD * 3 / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = FALSE;
  }

  fx->icon_depth = 10.00 * -fx->delta_width / fx->icon_width;

  fx->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;

  if (abs(fx->delta_width) >= fx->icon_width - MIN_WIDTH)
  {
    if (fx->delta_width > 0)
      fx->delta_width = fx->icon_width - MIN_WIDTH;
    else
      fx->delta_width = -fx->icon_width + MIN_WIDTH;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->count >= PERIOD)
  {
    fx->count = 0;
    fx->y_offset = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    fx->delta_width = 0;
    fx->flip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
turn_closing_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->y_offset = 0;
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 44;

  const gint MAX_OFFSET = fx->icon_height;

  gint prev_count = fx->count;

  fx->count = sin(fx->count * M_PI / 2 / PERIOD) * PERIOD;

  fx->y_offset = fx->count * MAX_OFFSET / PERIOD;

  fx->alpha = 1.0 - fx->count * 1.0 / PERIOD;

  if (fx->count < PERIOD / 4)
  {
    fx->icon_depth_direction = 0;
    fx->delta_width = -fx->count * (fx->icon_width) / (PERIOD / 4);
    fx->flip = FALSE;
  }
  else if (fx->count < PERIOD / 2)
  {
    fx->icon_depth_direction = 1;
    fx->delta_width =
      (fx->count - PERIOD / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = TRUE;
  }
  else if (fx->count < PERIOD * 3 / 4)
  {
    fx->icon_depth_direction = 0;
    fx->clip = FALSE;
    fx->delta_width =
      -(fx->count - PERIOD / 2) * (fx->icon_width) / (PERIOD / 4);
    fx->flip = TRUE;
  }
  else
  {
    fx->icon_depth_direction = 1;
    fx->delta_width =
      (fx->count - PERIOD * 3 / 4) * (fx->icon_width) / (PERIOD / 4) -
      fx->icon_width;
    fx->flip = FALSE;
  }

  fx->icon_depth = 10.00 * -fx->delta_width / fx->icon_width;

  fx->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;

  if (abs(fx->delta_width) >= fx->icon_width - MIN_WIDTH)
  {
    if (fx->delta_width > 0)
      fx->delta_width = fx->icon_width - MIN_WIDTH;
    else
      fx->delta_width = -fx->icon_width + MIN_WIDTH;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->count >= PERIOD)
  {
    fx->count = 0;
    fx->y_offset = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    fx->delta_width = 0;
    fx->flip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);
  }

  return repeat;
}

gboolean
turn_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("turn_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}
