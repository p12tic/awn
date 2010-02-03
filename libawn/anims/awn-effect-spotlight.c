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

#include "awn-effect-spotlight.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

gboolean
spotlight_hover_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->spotlight_alpha = 0;
    priv->spotlight = TRUE;
    priv->glow_amount = 0;
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;
  }

  const gint PERIOD = 15;
  const gint TREMBLE_PERIOD = 5;
  const gfloat TREMBLE_HEIGHT = 0.4;

  gboolean busy = awn_effect_check_top_effect(anim, NULL);

  if (priv->spotlight_alpha < 1.0 && priv->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    priv->spotlight_alpha += 1.0 / PERIOD;
  }
  else if (busy && priv->direction != AWN_EFFECT_SPOTLIGHT_OFF)
  {
    if (priv->spotlight_alpha >= 1.0)
      priv->direction = AWN_EFFECT_SPOTLIGHT_TREMBLE_DOWN;
    else if (priv->spotlight_alpha < 1.0 - TREMBLE_HEIGHT)
      priv->direction = AWN_EFFECT_SPOTLIGHT_TREMBLE_UP;

    if (priv->direction == AWN_EFFECT_SPOTLIGHT_TREMBLE_UP)
      priv->spotlight_alpha += TREMBLE_HEIGHT / TREMBLE_PERIOD;
    else
      priv->spotlight_alpha -= TREMBLE_HEIGHT / TREMBLE_PERIOD;
  }
  else
  {
    priv->direction = AWN_EFFECT_SPOTLIGHT_OFF;
    priv->spotlight_alpha -= 1.0 / PERIOD;
  }

  priv->glow_amount = priv->spotlight_alpha;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_OFF && priv->spotlight_alpha <= 0.0)
  {
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;
    priv->spotlight_alpha = 0;
    priv->glow_amount = 0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);

    if (!repeat)
      priv->spotlight = FALSE;
  }

  return repeat;
}


gboolean
spotlight_half_fade_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;
    priv->spotlight = TRUE;
  }

  const gint PERIOD = 20;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    priv->spotlight_alpha += 0.75 / PERIOD;
  }
  else
  {
    priv->spotlight_alpha -= 0.75 / PERIOD;
  }

  priv->glow_amount = priv->spotlight_alpha;

  if (priv->spotlight_alpha > 0.75)
    priv->direction = AWN_EFFECT_SPOTLIGHT_OFF;
  else if (priv->spotlight_alpha <= 0.0)
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->spotlight_alpha <= 0)
  {
    priv->count = 0;
    priv->spotlight_alpha = 0;
    priv->glow_amount = 0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);

    if (!repeat)
      priv->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight_opening_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->count = 0;
    priv->spotlight_alpha = 1.0;
    priv->spotlight = TRUE;
    priv->glow_amount = priv->spotlight_alpha;
    priv->width_mod = 0.5;
    priv->clip = TRUE;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.height = 0;
    priv->clip_region.width = priv->icon_width;
  }

  const gint PERIOD = 20;

  if (priv->width_mod < 1.0)
  {
    priv->clip_region.height += (3 / 2) * priv->icon_height / PERIOD;
    priv->width_mod += 1. / PERIOD * 1.5;
  }
  else if (priv->clip_region.height < priv->icon_height)
  {
    priv->width_mod = 1.0;
    priv->clip_region.height += (3 / 2) * priv->icon_height / PERIOD;

    if (priv->clip_region.height > priv->icon_height)
      priv->clip_region.height = priv->icon_height;
  }
  else
  {
    priv->width_mod = 1.0;
    priv->clip = FALSE;
    priv->spotlight_alpha -= 3.0 / PERIOD;
    priv->glow_amount = priv->spotlight_alpha;
  }

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->spotlight_alpha <= 0)
  {
    priv->count = 0;
    priv->spotlight_alpha = 0;
    priv->glow_amount = 0;
    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);

    if (!repeat)
      priv->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight_closing_effect(AwnEffectsAnimation * anim)
{
  AwnEffectsPrivate *priv = anim->effects->priv;

  AWN_ANIMATION_INIT(anim) {
    priv->spotlight_alpha = 0.0;
    priv->glow_amount = priv->spotlight_alpha;
    priv->clip_region.x = 0;
    priv->clip_region.y = 0;
    priv->clip_region.height = priv->icon_height;
    priv->clip_region.width = priv->icon_width;
    priv->direction = AWN_EFFECT_SPOTLIGHT_ON;
  }

  priv->spotlight = TRUE;
  priv->clip = TRUE;
  
  const gint PERIOD = 40;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    priv->spotlight_alpha += 4.0 / PERIOD;

    if (priv->spotlight_alpha >= 1)
    {
      priv->spotlight_alpha = 1;
      priv->direction = AWN_EFFECT_DIR_NONE;
    }
  }
  else if (priv->direction == AWN_EFFECT_DIR_NONE)
  {
    priv->clip_region.height -= 2 * priv->icon_height / PERIOD;
    priv->width_mod -= 2.0 / PERIOD;
    priv->alpha -= 2.0 / PERIOD;

    if (priv->alpha <= 0)
    {
      priv->width_mod = 1.0;
      priv->alpha = 0;
      priv->direction = AWN_EFFECT_SPOTLIGHT_OFF;
    }
    else if (priv->alpha <= 0.5)
    {
      priv->spotlight_alpha -= 2.0 / PERIOD;
    }
  }
  else
  {
    priv->clip = FALSE;
    priv->spotlight_alpha -= 2.0 / PERIOD;
  }

  priv->glow_amount = priv->spotlight_alpha;

  /* repaint widget */
  awn_effects_redraw(anim->effects);

  gboolean repeat = TRUE;

  if (priv->direction == AWN_EFFECT_SPOTLIGHT_OFF && priv->spotlight_alpha <= 0)
  {
    priv->alpha = 1.0;
    priv->spotlight_alpha = 0.0;
    priv->glow_amount = 0.0;
    priv->clip = FALSE;
    priv->spotlight = FALSE;
    priv->direction = AWN_EFFECT_DIR_NONE;

    /* check for repeating */
    repeat = awn_effect_handle_repeating(anim);
  }

  return repeat;
}

void
spotlight_init()
{
/*
  GError *error = NULL;

  if (!SPOTLIGHT_PIXBUF)
    SPOTLIGHT_PIXBUF =
      gdk_pixbuf_new_from_inline(-1, spotlight1_png_inline, FALSE, NULL);

  g_return_if_fail(error == NULL);
*/
}

gboolean
spotlight_effect_finalize(AwnEffectsAnimation * anim)
{
  printf("spotlight_effect_finalize(AwnEffectsAnimation * anim)\n");
  return TRUE;
}

