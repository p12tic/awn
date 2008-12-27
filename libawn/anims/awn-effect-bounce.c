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

#include "awn-effect-bounce.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

// simple bounce effect based on sin function
gboolean
bounce_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) priv->count = 0;

  const gdouble MAX_BOUNCE_OFFSET = 15.0;
  const gint PERIOD = 20;

  priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}



gboolean
bounce_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->top_offset = 0;
    priv->clip = TRUE;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.width = priv->icon_width;
    priv->clip_region.height = 0;
  }

  const gint PERIOD1 = 15;
  const gint PERIOD2 = 20;
  const gint MAX_BOUNCE_OFFSET = 15;

  if (priv->count < PERIOD1)
    priv->clip_region.height = priv->icon_height * ++priv->count / PERIOD1;
  else if (priv->count < PERIOD1 + PERIOD2)
  {
    priv->clip = FALSE;
    priv->top_offset =
      sin((++priv->count - PERIOD1) * M_PI / PERIOD2) * MAX_BOUNCE_OFFSET;
  }

  // repaint widget
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD1 + PERIOD2)
  {
    priv->count = 0;
    priv->top_offset = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("bounce_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}
