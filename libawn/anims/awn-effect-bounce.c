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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-effect-bounce.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

/* simple bounce effect based on sin function */
gboolean
bounce_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) priv->count = 0;

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 1.5 : priv->icon_height / 1.5;
  const gint PERIOD = 16;

  priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

gboolean
bounce_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) priv->count = 0;

  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;
  const gint PERIOD = 14;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  priv->top_offset = sin(++priv->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;
  if (priv->count == PERIOD/2)
  {
    /* suspend in middle */
    if (awn_effect_check_top_effect(anim, NULL))
      return awn_effect_suspend_animation(anim, 
                                          (GSourceFunc)bounce_hover_effect);
  }

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD)
  {
    priv->count = 0;
    priv->top_offset = 0;
    /* check for repeating */
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

  const gint PERIOD1 = 12;
  const gint PERIOD2 = 14;
  const gfloat MAX_BOUNCE_OFFSET =
    anim->effects->position == GTK_POS_LEFT ||
    anim->effects->position == GTK_POS_RIGHT ?
      priv->icon_width / 3. : priv->icon_height / 3.;

  if (priv->count < PERIOD1)
    priv->clip_region.height = priv->icon_height * ++priv->count / PERIOD1;
  else if (priv->count < PERIOD1 + PERIOD2)
  {
    priv->clip = FALSE;
    priv->top_offset =
      sin((++priv->count - PERIOD1) * M_PI / PERIOD2) * MAX_BOUNCE_OFFSET;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->count >= PERIOD1 + PERIOD2)
  {
    priv->count = 0;
    priv->top_offset = 0;
    /* check for repeating */
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
