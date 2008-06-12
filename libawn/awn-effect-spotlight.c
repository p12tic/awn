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


GdkPixbuf *SPOTLIGHT_PIXBUF = NULL;
#include "../data/active/spotlight_png_inline.c"


gboolean
spotlight_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->spotlight_alpha = 0;
    fx->spotlight = TRUE;
    fx->glow_amount = 0;
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 15;

  const gint TREMBLE_PERIOD = 5;

  const gfloat TREMBLE_HEIGHT = 0.4;

  gboolean busy = awn_effect_check_top_effect(priv, NULL);

  if (fx->spotlight_alpha < 1.0 && fx->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    fx->spotlight_alpha += 1.0 / PERIOD;
  }
  else if (busy && fx->direction != AWN_EFFECT_SPOTLIGHT_OFF)
  {
    if (fx->spotlight_alpha >= 1.0)
      fx->direction = AWN_EFFECT_SPOTLIGHT_TREMBLE_DOWN;
    else if (fx->spotlight_alpha < 1.0 - TREMBLE_HEIGHT)
      fx->direction = AWN_EFFECT_SPOTLIGHT_TREMBLE_UP;

    if (fx->direction == AWN_EFFECT_SPOTLIGHT_TREMBLE_UP)
      fx->spotlight_alpha += TREMBLE_HEIGHT / TREMBLE_PERIOD;
    else
      fx->spotlight_alpha -= TREMBLE_HEIGHT / TREMBLE_PERIOD;
  }
  else
  {
    fx->direction = AWN_EFFECT_SPOTLIGHT_OFF;
    fx->spotlight_alpha -= 1.0 / PERIOD;
  }

  fx->glow_amount = fx->spotlight_alpha;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_SPOTLIGHT_OFF && fx->spotlight_alpha <= 0.0)
  {
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;
    fx->spotlight_alpha = 0;
    fx->glow_amount = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);

    if (!repeat)
      fx->spotlight = FALSE;
  }

  return repeat;
}


gboolean
spotlight_half_fade_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;
    fx->spotlight = TRUE;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 20;

  if (fx->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    fx->spotlight_alpha += 0.75 / PERIOD;
  }
  else
  {
    fx->spotlight_alpha -= 0.75 / PERIOD;
  }

  fx->glow_amount = fx->spotlight_alpha;

  if (fx->spotlight_alpha > 0.75)
    fx->direction = AWN_EFFECT_SPOTLIGHT_OFF;
  else if (fx->spotlight_alpha <= 0.0)
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->spotlight_alpha <= 0)
  {
    fx->count = 0;
    fx->spotlight_alpha = 0;
    fx->glow_amount = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);

    if (!repeat)
      fx->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight_opening_effect2(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->spotlight_alpha = 1.0;
    fx->spotlight = TRUE;
    fx->glow_amount = fx->spotlight_alpha;
    fx->delta_width = -fx->icon_width / 2;
    fx->clip = TRUE;
    fx->clip_region.x = 0;
    fx->clip_region.y = 0;
    fx->clip_region.height = 0;
    fx->clip_region.width = fx->icon_width;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 20;

  if (fx->delta_width < 0)
  {
    fx->clip_region.height += (3 / 2) * fx->icon_height / PERIOD;
    fx->delta_width += (3 / 1) * (fx->icon_width / 2) * 1 / PERIOD;
  }
  else if (fx->clip_region.height < fx->icon_height)
  {
    fx->clip_region.height += (3 / 2) * fx->icon_height / PERIOD;

    if (fx->clip_region.height > fx->icon_height)
      fx->clip_region.height = fx->icon_height;
  }
  else
  {
    fx->clip = FALSE;
    fx->spotlight_alpha -= (3 / 1) * 1.0 / PERIOD;
    fx->glow_amount = fx->spotlight_alpha;
  }

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->spotlight_alpha <= 0)
  {
    fx->count = 0;
    fx->spotlight_alpha = 0;
    fx->glow_amount = 0;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);

    if (!repeat)
      fx->spotlight = FALSE;
  }

  return repeat;
}

gboolean
spotlight_closing_effect(AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;

  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->spotlight_alpha = 0.0;
    fx->spotlight = TRUE;
    fx->glow_amount = fx->spotlight_alpha;
    fx->clip = TRUE;
    fx->clip_region.x = 0;
    fx->clip_region.y = 0;
    fx->clip_region.height = fx->icon_height;
    fx->clip_region.width = fx->icon_width;
    fx->direction = AWN_EFFECT_SPOTLIGHT_ON;

    if (priv->start)
      priv->start(fx->self);

    priv->start = NULL;
  }

  const gint PERIOD = 40;

  if (fx->direction == AWN_EFFECT_SPOTLIGHT_ON)
  {
    fx->spotlight_alpha += 4.0 / PERIOD;

    if (fx->spotlight_alpha >= 1)
    {
      fx->spotlight_alpha = 1;
      fx->direction = AWN_EFFECT_DIR_NONE;
    }
  }
  else if (fx->direction == AWN_EFFECT_DIR_NONE)
  {
    fx->clip_region.height -= 2 * fx->icon_height / PERIOD;
    fx->delta_width -= 2 * fx->icon_width / PERIOD;
    fx->alpha -= 2.0 / PERIOD;

    if (fx->alpha <= 0)
    {
      fx->alpha = 0;
      fx->direction = AWN_EFFECT_SPOTLIGHT_OFF;
    }
    else if (fx->alpha <= 0.5)
    {
      fx->spotlight_alpha -= 2.0 / PERIOD;
    }
  }
  else
  {
    fx->clip = FALSE;
    fx->spotlight_alpha -= 2.0 / PERIOD;
  }

  fx->glow_amount = fx->spotlight_alpha;

  // repaint widget
  gtk_widget_queue_draw(GTK_WIDGET(fx->self));

  gboolean repeat = TRUE;

  if (fx->direction == AWN_EFFECT_SPOTLIGHT_OFF && fx->spotlight_alpha <= 0)
  {
    fx->spotlight_alpha = 0;
    fx->glow_amount = 0;
    fx->direction = AWN_EFFECT_DIR_NONE;
    // check for repeating
    repeat = awn_effect_handle_repeating(priv);

    if (!repeat)
      fx->spotlight = FALSE;
  }

  return repeat;
}

void
spotlight_init()
{
  GError *error = NULL;

  if (!SPOTLIGHT_PIXBUF)
    SPOTLIGHT_PIXBUF =
      gdk_pixbuf_new_from_inline(-1, spotlight1_png_inline, FALSE, NULL);

  g_return_if_fail(error == NULL);
}

gboolean
spotlight_effect_finalize(AwnEffectsPrivate * priv)
{
  printf("spotlight_effect_finalize(AwnEffectsPrivate * priv)\n");
  return TRUE;
}

