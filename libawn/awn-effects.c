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
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "awn-effect-spotlight.h"
#include "awn-effect-bounce.h"
#include "awn-effect-glow.h"
#include "awn-effect-zoom.h"
#include "awn-effect-fade.h"
#include "awn-effect-squish.h"
#include "awn-effect-turn.h"


#define  M_PI	3.14159265358979323846

#define  AWN_FRAME_RATE 			40

/* FORWARD DECLARATIONS */

extern GdkPixbuf *SPOTLIGHT_PIXBUF;


// effect functions
static gboolean awn_on_enter_event (GtkWidget * widget,
				    GdkEventCrossing * event, gpointer data);
static gboolean awn_on_leave_event (GtkWidget * widget,
				    GdkEventCrossing * event, gpointer data);

static gdouble calc_curve_position (gdouble cx, gdouble a, gdouble b);

void
awn_effects_init (GObject * self, AwnEffects * fx)
{
  fx->self = self;
  fx->settings = awn_get_settings ();
  fx->focus_window = NULL;
  fx->title = NULL;
  fx->get_title = NULL;
  fx->effect_queue = NULL;

  fx->window_width = 0;
  fx->window_height = 0;
  fx->icon_width = 48;
  fx->icon_height = 48;
  fx->delta_width = 0;
  fx->delta_height = 0;

  /* EFFECT VARIABLES */
  fx->effect_lock = FALSE;
  fx->direction = AWN_EFFECT_DIR_NONE;
  fx->count = 0;

  fx->x_offset = 0;
  fx->y_offset = 0;
  fx->rotate_degrees = 0.0;
  fx->alpha = 1.0;
  fx->spotlight_alpha = 0.0;
  fx->saturation = 1.0;
  fx->glow_amount = 0.0;

  fx->hover = FALSE;
  fx->clip = FALSE;
  fx->flip = FALSE;
  fx->spotlight = FALSE;

  fx->enter_notify = 0;
  fx->leave_notify = 0;
  fx->timer_id = 0;
}

void
awn_effect_dispose_queue (AwnEffects * fx)
{
  if (fx->timer_id)
  {
    GSource *s = g_main_context_find_source_by_id (NULL, fx->timer_id);
    if (s)
      g_source_destroy (s);
  }
  GList *queue = fx->effect_queue;
  while (queue)
  {
    g_free (queue->data);
    queue->data = NULL;
    queue = g_list_next (queue);
  }
  g_list_free (fx->effect_queue);
  fx->effect_queue = NULL;
}

void
awn_effects_finalize (AwnEffects * fx)
{
  awn_unregister_effects (fx);
  awn_effect_dispose_queue (fx);
  fx->self = NULL;
}



void
awn_effects_set_title (AwnEffects * fx, AwnTitle * title,
		       AwnTitleCallback title_func)
{
  fx->title = title;
  fx->get_title = title_func;
}


gboolean
spotlight3D_hover_effect (AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;
  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->y_offset = 0;
    fx->spotlight_alpha = 1.0;
    fx->spotlight = TRUE;
    fx->glow_amount = fx->spotlight_alpha;
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    if (priv->start)
      priv->start (fx->self);
    priv->start = NULL;
  }

  const gint PERIOD = 44;
  const gdouble ALPHA_STEP = 0.04;

  if (awn_effect_check_top_effect (priv, NULL))
  {
    fx->spotlight_alpha = 1.0;
  }
  else
  {
    fx->spotlight_alpha -= ALPHA_STEP;
    if (fx->spotlight_alpha < 0)
    {
      fx->spotlight_alpha = 0;
    }
  }
  fx->glow_amount = fx->spotlight_alpha;

  gint prev_count = fx->count;
  if (prev_count > PERIOD)
    prev_count = --fx->count;

  fx->count = sin (fx->count * M_PI / 2 / PERIOD) * PERIOD;

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
  if (abs (fx->delta_width) >= fx->icon_width - MIN_WIDTH)
  {
    if (fx->delta_width > 0)
      fx->delta_width = fx->icon_width - MIN_WIDTH;
    else
      fx->delta_width = -fx->icon_width + MIN_WIDTH;
  }

  // repaint widget
  gtk_widget_queue_draw (GTK_WIDGET (fx->self));

  gboolean repeat = TRUE;
  if (fx->count >= PERIOD
      && (fx->spotlight_alpha >= 1 || fx->spotlight_alpha <= 0))
  {
    fx->count = 0;
    fx->y_offset = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    fx->delta_width = 0;
    fx->flip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating (priv);
    if (!repeat)
      fx->spotlight = FALSE;
  }
  return repeat;
}

static gboolean
spotlight3D_opening_effect (AwnEffectsPrivate * priv)
{
  AwnEffects *fx = priv->effects;
  if (!fx->effect_lock)
  {
    fx->effect_lock = TRUE;
    // effect start initialize values
    fx->count = 0;
    fx->y_offset = 0;
    fx->spotlight_alpha = 1.0;
    fx->spotlight = TRUE;
    fx->glow_amount = fx->spotlight_alpha;
    fx->clip = TRUE;
    fx->clip_region.x = 0;
    fx->clip_region.y = 0;
    fx->clip_region.width = fx->icon_width;
    fx->clip_region.height = 0;
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    if (priv->start)
      priv->start (fx->self);
    priv->start = NULL;
  }

  const gint PERIOD = 44;
  const gint MAX_OFFSET = fx->icon_height / 2;

  gint prev_count = fx->count;

  fx->count = sin (fx->count * M_PI / 2 / PERIOD) * PERIOD;

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
    fx->spotlight_alpha =
      -(fx->count - PERIOD * 3 / 4) * 1.0 / (PERIOD / 4) + 1.0;
  }
  fx->icon_depth = 10.00 * -fx->delta_width / fx->icon_width;
  fx->glow_amount = fx->spotlight_alpha;

  fx->count = ++prev_count;

  // fix icon flickering
  const gint MIN_WIDTH = 4;
  if (abs (fx->delta_width) >= fx->icon_width - MIN_WIDTH)
  {
    if (fx->delta_width > 0)
      fx->delta_width = fx->icon_width - MIN_WIDTH;
    else
      fx->delta_width = -fx->icon_width + MIN_WIDTH;
  }

  // repaint widget
  gtk_widget_queue_draw (GTK_WIDGET (fx->self));

  gboolean repeat = TRUE;
  if (fx->count >= PERIOD)
  {
    fx->count = 0;
    fx->y_offset = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
    fx->delta_width = 0;
    fx->flip = FALSE;
    fx->spotlight = FALSE;
    fx->spotlight_alpha = 0.0;
    fx->glow_amount = 0.0;
    fx->clip = FALSE;
    // check for repeating
    repeat = awn_effect_handle_repeating (priv);
  }
  return repeat;
}

static gboolean
spotlight3D_closing_effect (AwnEffectsPrivate * priv)
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
      priv->start (fx->self);
    priv->start = NULL;
    fx->count = 0;
    fx->delta_width = 0;
    fx->icon_depth = 0;
    fx->icon_depth_direction = 0;
  }

  const gint PERIOD = 80;
  const gint TURN_PERIOD = 20;

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
    fx->clip_region.height -= 2.0 * fx->icon_height / PERIOD;
    fx->alpha -= 2.0 / PERIOD;

    if (fx->count < TURN_PERIOD / 4)
    {
      fx->icon_depth_direction = 0;
      fx->delta_width = -fx->count * (fx->icon_width) / (TURN_PERIOD / 4);
      fx->flip = FALSE;
    }
    else if (fx->count < TURN_PERIOD / 2)
    {
      fx->icon_depth_direction = 1;
      fx->delta_width =
	(fx->count - TURN_PERIOD / 4) * (fx->icon_width) / (TURN_PERIOD / 4) -
	fx->icon_width;
      fx->flip = TRUE;
    }
    else if (fx->count < TURN_PERIOD * 3 / 4)
    {
      fx->icon_depth_direction = 0;
      fx->delta_width =
	-(fx->count - TURN_PERIOD / 2) * (fx->icon_width) / (TURN_PERIOD / 4);
      fx->flip = TRUE;
    }
    else
    {
      fx->icon_depth_direction = 1;
      fx->delta_width =
	(fx->count -
	 TURN_PERIOD * 3 / 4) * (fx->icon_width) / (TURN_PERIOD / 4) -
	fx->icon_width;
      fx->flip = FALSE;
    }

    fx->icon_depth = 10.00 * -fx->delta_width / fx->icon_width;

    // fix icon flickering
    const gint MIN_WIDTH = 4;
    if (abs (fx->delta_width) >= fx->icon_width - MIN_WIDTH)
    {
      if (fx->delta_width > 0)
	fx->delta_width = fx->icon_width - MIN_WIDTH;
      else
	fx->delta_width = -fx->icon_width + MIN_WIDTH;
    }
    if (fx->count++ > TURN_PERIOD)
      fx->count = 0;

    if (fx->alpha <= 0 || fx->clip_region.height <= 0)
    {
      fx->alpha = 0;
      fx->direction = AWN_EFFECT_SPOTLIGHT_OFF;
      fx->clip = FALSE;
    }
    else if (fx->alpha <= 0.5)
    {
      fx->spotlight_alpha -= 2.0 / PERIOD;
    }
  }
  else
  {
    fx->spotlight_alpha -= 2.0 / PERIOD;
  }
  fx->glow_amount = fx->spotlight_alpha;

  // repaint widget
  gtk_widget_queue_draw (GTK_WIDGET (fx->self));

  gboolean repeat = TRUE;
  if (fx->direction == AWN_EFFECT_SPOTLIGHT_OFF && fx->spotlight_alpha <= 0)
  {
    fx->spotlight_alpha = 0;
    fx->glow_amount = 0;
    fx->direction = AWN_EFFECT_DIR_NONE;
    // check for repeating
    repeat = awn_effect_handle_repeating (priv);
    if (!repeat)
      fx->spotlight = FALSE;
  }
  return repeat;
}

static gboolean
awn_on_enter_event (GtkWidget * widget, GdkEventCrossing * event,
		    gpointer data)
{

  AwnEffects *fx = (AwnEffects *) data;

  if (fx->settings)
    fx->settings->hiding = FALSE;

  if (fx->title && fx->get_title)
  {
    awn_title_show (fx->title, fx->focus_window, fx->get_title (fx->self));
  }

  fx->hover = TRUE;
  awn_effect_start (fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gboolean
awn_on_leave_event (GtkWidget * widget, GdkEventCrossing * event,
		    gpointer data)
{
  AwnEffects *fx = (AwnEffects *) data;

  if (fx->title)
  {
    awn_title_hide (fx->title, fx->focus_window);
  }

  fx->hover = FALSE;
  awn_effect_stop (fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gint
awn_effect_sort (gconstpointer a, gconstpointer b)
{
  const AwnEffectsPrivate *data1 = (AwnEffectsPrivate *) a;
  const AwnEffectsPrivate *data2 = (AwnEffectsPrivate *) b;
  return (gint) (data1->priority - data2->priority);
}

inline AwnEffectPriority
awn_effect_get_priority (const AwnEffect effect)
{
  switch (effect)
  {
  case AWN_EFFECT_OPENING:
    return AWN_EFFECT_PRIORITY_HIGH;
  case AWN_EFFECT_LAUNCHING:
    return AWN_EFFECT_PRIORITY_ABOVE_NORMAL;
  case AWN_EFFECT_HOVER:
    return AWN_EFFECT_PRIORITY_LOW;
  case AWN_EFFECT_ATTENTION:
    return AWN_EFFECT_PRIORITY_NORMAL;
  case AWN_EFFECT_CLOSING:
    return AWN_EFFECT_PRIORITY_HIGHEST;
  default:
    return AWN_EFFECT_PRIORITY_BELOW_NORMAL;
  }
}

void
awn_effect_start (AwnEffects * fx, const AwnEffect effect)
{
  awn_effect_start_ex (fx, effect, NULL, NULL, 0);
}

void
awn_effect_start_ex (AwnEffects * fx, const AwnEffect effect,
		     AwnEventNotify start, AwnEventNotify stop,
		     gint max_loops)
{
  if (effect == AWN_EFFECT_NONE || fx->self == NULL)
    return;

  AwnEffectsPrivate *queue_item;
  GList *queue = fx->effect_queue;
  // dont start the effect if already in queue
  while (queue)
  {
    queue_item = queue->data;
    if (queue_item->this_effect == effect)
      return;
    queue = g_list_next (queue);
  }
  AwnEffectsPrivate *priv = g_new (AwnEffectsPrivate, 1);
  priv->effects = fx;
  priv->this_effect = effect;
  priv->priority = awn_effect_get_priority (effect);
  priv->max_loops = max_loops;
  priv->start = start;
  priv->stop = stop;

  fx->effect_queue =
    g_list_insert_sorted (fx->effect_queue, priv, awn_effect_sort);
  main_effect_loop (fx);
}

void
awn_effect_stop (AwnEffects * fx, const AwnEffect effect)
{
  if (effect == AWN_EFFECT_NONE)
    return;

  AwnEffectsPrivate *queue_item;
  GList *queue = fx->effect_queue;
  // remove the effect if in queue
  while (queue)
  {
    queue_item = queue->data;
    if (queue_item->this_effect == effect)
      break;
    queue = g_list_next (queue);
  }
  if (queue)
  {
    gboolean dispose = queue_item->this_effect != fx->current_effect;
    fx->effect_queue = g_list_remove (fx->effect_queue, queue_item);
    if (dispose)
      g_free (queue_item);
  }
}

void
main_effect_loop (AwnEffects * fx)
{
  if (fx->current_effect != AWN_EFFECT_NONE || fx->effect_queue == NULL)
    return;

#define EFFECT_BOUNCE 0
#define EFFECT_FADE 1
#define EFFECT_SPOTLIGHT 2
#define EFFECT_ZOOM 3
#define EFFECT_SQUISH 4
#define EFFECT_TURN_3D 5
#define EFFECT_TURN_3D_SPOTLIGHT 6
#define EFFECT_GLOW 7

  // NOTE! Always make sure that all EFFECTS arrays have same number of elements
  static const GSourceFunc OPENING_EFFECTS[] = {
    NULL,
    (GSourceFunc) bounce_opening_effect,
    (GSourceFunc) zoom_opening_effect,
    (GSourceFunc) spotlight_opening_effect2,
    (GSourceFunc) zoom_opening_effect,
    (GSourceFunc) bounce_squish_opening_effect,
    (GSourceFunc) turn_opening_effect,
    (GSourceFunc) spotlight3D_opening_effect,
    (GSourceFunc) glow_opening_effect
  };
  static const GSourceFunc CLOSING_EFFECTS[] = {
    NULL,
    (GSourceFunc) fade_out_effect,
    (GSourceFunc) zoom_closing_effect,
    (GSourceFunc) spotlight_closing_effect,
    (GSourceFunc) zoom_closing_effect,
    (GSourceFunc) bounce_squish_closing_effect,
    (GSourceFunc) turn_closing_effect,
    (GSourceFunc) spotlight3D_closing_effect,
    (GSourceFunc) glow_closing_effect
  };
  static const GSourceFunc HOVER_EFFECTS[] = {
    NULL,
    (GSourceFunc) bounce_effect,
    (GSourceFunc) fading_effect,
    (GSourceFunc) spotlight_effect,
    (GSourceFunc) zoom_effect,
    (GSourceFunc) bounce_squish_effect,
    (GSourceFunc) turn_hover_effect,
    (GSourceFunc) spotlight3D_hover_effect,
    (GSourceFunc) glow_effect
  };
  static const GSourceFunc LAUNCHING_EFFECTS[] = {
    NULL,
    (GSourceFunc) bounce_effect,
    (GSourceFunc) fading_effect,
    (GSourceFunc) spotlight_half_fade_effect,
    (GSourceFunc) zoom_attention_effect,
    (GSourceFunc) bounce_squish_effect,
    (GSourceFunc) turn_hover_effect,
    (GSourceFunc) spotlight_half_fade_effect,
    (GSourceFunc) glow_attention_effect
  };
  static const GSourceFunc ATTENTION_EFFECTS[] = {
    NULL,
    (GSourceFunc) bounce_effect,
    (GSourceFunc) fading_effect,
    (GSourceFunc) spotlight_half_fade_effect,
    (GSourceFunc) zoom_attention_effect,
    (GSourceFunc) bounce_squish_attention_effect,
    (GSourceFunc) turn_hover_effect,
    (GSourceFunc) spotlight3D_hover_effect,
    (GSourceFunc) glow_attention_effect
  };

  GSourceFunc animation = NULL;
  AwnEffectsPrivate *topEffect =
    (AwnEffectsPrivate *) (fx->effect_queue->data);
  gint icon_effect = 0;
#define EFFECT_TYPES_COUNT 5
  gint effects[EFFECT_TYPES_COUNT] = { 0 };
  if (fx->settings)
  {
    icon_effect = fx->settings->icon_effect;
    gint i;
    for (i = 0; i < EFFECT_TYPES_COUNT; i++)
    {
      gint effect = icon_effect & (0xF << (i * 4));
      effect >>= i * 4;
      if (effect >= sizeof (HOVER_EFFECTS) / sizeof (GSourceFunc))
	effect = -1;
      // spotlight initialization
      if (effect == EFFECT_SPOTLIGHT || effect == EFFECT_TURN_3D_SPOTLIGHT)
	spotlight_init ();
      effects[i] = effect + 1;
    }
  }

  switch (topEffect->this_effect)
  {
  case AWN_EFFECT_HOVER:
    animation = HOVER_EFFECTS[effects[0]];
    break;
  case AWN_EFFECT_OPENING:
    animation = OPENING_EFFECTS[effects[1]];
    break;
  case AWN_EFFECT_CLOSING:
    animation = CLOSING_EFFECTS[effects[2]];
    break;
  case AWN_EFFECT_LAUNCHING:
    animation = LAUNCHING_EFFECTS[effects[3]];
    break;
  case AWN_EFFECT_ATTENTION:
    animation = ATTENTION_EFFECTS[effects[4]];
    break;
  default:
    return;
  }
  if (animation)
  {
    fx->timer_id = g_timeout_add (AWN_FRAME_RATE, animation, topEffect);
    fx->current_effect = topEffect->this_effect;
    fx->effect_lock = FALSE;
  }
  else
  {
    if (topEffect->start)
      topEffect->start (fx->self);
    if (topEffect->stop)
      topEffect->stop (fx->self);
    // dispose AwnEffectsPrivate
    awn_effect_stop (fx, topEffect->this_effect);
  }
}

void
awn_register_effects (GObject * obj, AwnEffects * fx)
{
  if (fx->focus_window)
  {
    awn_unregister_effects (fx);
  }
  fx->focus_window = GTK_WIDGET (obj);
  fx->enter_notify =
    g_signal_connect (obj, "enter-notify-event",
		      G_CALLBACK (awn_on_enter_event), fx);
  fx->leave_notify =
    g_signal_connect (obj, "leave-notify-event",
		      G_CALLBACK (awn_on_leave_event), fx);
}

void
awn_unregister_effects (AwnEffects * fx)
{
  if (fx->enter_notify)
    g_signal_handler_disconnect (G_OBJECT (fx->focus_window),
				 fx->enter_notify);
  if (fx->leave_notify)
    g_signal_handler_disconnect (G_OBJECT (fx->focus_window),
				 fx->leave_notify);
  fx->enter_notify = 0;
  fx->leave_notify = 0;
  fx->focus_window = NULL;
}

inline guchar
lighten_component (const guchar cur_value, const gfloat amount)
{
  int new_value = cur_value;
  new_value += (24 + (new_value >> 3)) * amount;
  if (new_value > 255)
  {
    new_value = 255;
  }
  return (guchar) new_value;
}

void
lighten_pixbuf (GdkPixbuf * src, const gfloat amount)
{
  int i, j;
  int width, height, row_stride, has_alpha;
  guchar *target_pixels;
  guchar *pixsrc;

  g_return_if_fail (gdk_pixbuf_get_colorspace (src) == GDK_COLORSPACE_RGB);
  g_return_if_fail ((!gdk_pixbuf_get_has_alpha (src)
		     && gdk_pixbuf_get_n_channels (src) == 3)
		    || (gdk_pixbuf_get_has_alpha (src)
			&& gdk_pixbuf_get_n_channels (src) == 4));
  g_return_if_fail (gdk_pixbuf_get_bits_per_sample (src) == 8);

  has_alpha = gdk_pixbuf_get_has_alpha (src);
  width = gdk_pixbuf_get_width (src);
  height = gdk_pixbuf_get_height (src);
  row_stride = gdk_pixbuf_get_rowstride (src);
  target_pixels = gdk_pixbuf_get_pixels (src);

  for (i = 0; i < height; i++)
  {
    pixsrc = target_pixels + i * row_stride;
    for (j = 0; j < width; j++)
    {
      *pixsrc = lighten_component (*pixsrc, amount);
      pixsrc++;
      *pixsrc = lighten_component (*pixsrc, amount);
      pixsrc++;
      *pixsrc = lighten_component (*pixsrc, amount);
      pixsrc++;
      if (has_alpha)
	pixsrc++;
    }
  }
}

inline void
apply_3d_illusion (AwnEffects * fx, cairo_t * cr, GdkPixbuf * icon,
		   const gint x, const gint y, const gdouble alpha)
{
  gint i;
  for (i = 1; i < fx->icon_depth; i++)
  {
    if (fx->icon_depth_direction == 0)
      gdk_cairo_set_source_pixbuf (cr, icon, x - fx->icon_depth + i, y);
    else
      gdk_cairo_set_source_pixbuf (cr, icon, x + fx->icon_depth - i, y);
    cairo_paint_with_alpha (cr, alpha);
  }
}

void
awn_draw_background (AwnEffects * fx, cairo_t * cr)
{
  gint x1 = 0;
  gint y1 = fx->window_height - fx->icon_height;
  if (fx->settings)
    y1 = fx->window_height - fx->settings->icon_offset - fx->icon_height;

  GdkPixbuf *spot = NULL;
  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    y1 += fx->icon_height / 12;
    spot =
      gdk_pixbuf_scale_simple (SPOTLIGHT_PIXBUF, fx->window_width,
			       fx->icon_height * 5 / 4, GDK_INTERP_BILINEAR);
    gdk_cairo_set_source_pixbuf (cr, spot, x1, y1);
    cairo_paint_with_alpha (cr, fx->spotlight_alpha);
    // TODO: use spot also for foreground, will save one allocation and scale
    g_object_unref (spot);
  }
}

void
awn_draw_icons (AwnEffects * fx, cairo_t * cr, GdkPixbuf * icon,
		GdkPixbuf * reflect)
{
  if (!icon || fx->window_width <= 0 || fx->window_height <= 0)
    return;

  /* Apply the curves */
  if (fx->settings->bar_angle < 0)
  {
    int awn_bar_width = fx->settings->bar_width;
    double awn_curve_curviness = fx->settings->curviness;
    int awn_monitor_width = fx->settings->monitor_width;

    gint curvex = GTK_WIDGET (fx->self)->allocation.x;
    if (curvex == 0)		// is applet?
    {
      gint curvex1 = 0;
      gdk_window_get_origin (fx->focus_window->window, &curvex1, NULL);
      curvex = curvex1 - (awn_monitor_width - awn_bar_width) / 2;
    }
    if (awn_curve_curviness)
      fx->settings->curviness = awn_curve_curviness;

    if (curvex > 0)
      fx->curve_offset =
	calc_curve_position (curvex + (fx->settings->task_width / 4),
			     awn_bar_width,
			     (fx->settings->bar_height *
			      fx->settings->curviness) / 2.);
    else
      fx->curve_offset = 0;

  }
  else if (fx->curve_offset)
    fx->curve_offset = 0;

  /* refresh icon info */
  fx->icon_width = gdk_pixbuf_get_width (icon);
  fx->icon_height = gdk_pixbuf_get_height (icon);

  gint current_width = fx->icon_width;
  gint current_height = fx->icon_height;

  gint x1 = (fx->window_width - current_width) / 2;
  gint y1 = (fx->window_height - current_height);	// sit on bottom by default
  if (fx->settings)
    y1 =
      fx->window_height - fx->settings->icon_offset - current_height -
      fx->y_offset;
  y1 -= fx->curve_offset;

  /* ICONS */
  gboolean free_reflect = FALSE;

  /* clipping */
  GdkPixbuf *clippedIcon = NULL;
  if (fx->clip)
  {
    gint x = fx->clip_region.x;
    gint y = fx->clip_region.y;
    gint w = fx->clip_region.width;
    gint h = fx->clip_region.height;

    if (x >= 0 && x < fx->icon_width &&
	w - x > 0 && w - x <= fx->icon_width &&
	y >= 0 && x < fx->icon_height &&
	h - y > 0 && h - y <= fx->icon_height)
    {

      // careful! new_subpixbuf shares original pixbuf, no copy!
      clippedIcon = gdk_pixbuf_new_subpixbuf (icon, x, y, w, h);
      // update current w&h
      current_width = w - x;
      current_height = h - y;
      // refresh reflection, icon was clipped
      if (!fx->delta_width && !fx->delta_height)
      {
	// don't create reflection if we're also scaling
	reflect = gdk_pixbuf_flip (clippedIcon, FALSE);
	free_reflect = TRUE;
      }
      // adjust offsets
      x1 = (fx->window_width - current_width) / 2;
      y1 += fx->icon_height - current_height;
      // override provided icon
      icon = clippedIcon;
    }
    else
    {
      /* it's pretty common on bar resize */
      //g_warning("Unable to scale icon!");
      return;
    }
  }

  /* scaling */
  GdkPixbuf *scaledIcon = NULL;
  if (fx->delta_width || fx->delta_height)
  {
    // sanity check
    if (fx->delta_width <= -current_width
	|| fx->delta_height <= -current_height)
    {
      // we would display blank icon
      if (clippedIcon)
	g_object_unref (clippedIcon);
      return;
    }
    scaledIcon =
      gdk_pixbuf_scale_simple (icon, current_width + fx->delta_width,
			       current_height + fx->delta_height,
			       GDK_INTERP_BILINEAR);
    // update current w&h
    current_width += fx->delta_width;
    current_height += fx->delta_height;
    // refresh reflection, we scaled icon
    reflect = gdk_pixbuf_flip (scaledIcon, FALSE);
    free_reflect = TRUE;
    // adjust offsets
    x1 = (fx->window_width - current_width) / 2;
    y1 -= fx->delta_height;
    // override provided icon
    icon = scaledIcon;
  }

  /* saturation */
  GdkPixbuf *saturatedIcon = NULL;
  if (fx->saturation < 1.0)
  {
    // do not change original pixbuf -> create new
    if (!scaledIcon)
    {
      saturatedIcon = gdk_pixbuf_copy (icon);
      icon = saturatedIcon;
    }
    gdk_pixbuf_saturate_and_pixelate (icon, icon, fx->saturation, FALSE);
    if (!free_reflect)
    {
      // refresh reflection, we saturated icon
      reflect = gdk_pixbuf_flip (icon, FALSE);
      free_reflect = TRUE;
    }
    else
    {
      gdk_pixbuf_saturate_and_pixelate (reflect, reflect, fx->saturation,
					FALSE);
    }
  }

  if (fx->x_offset)
    x1 += fx->x_offset;

  /* flipping */
  GdkPixbuf *flippedIcon = NULL;
  if (fx->flip)
  {
    flippedIcon = gdk_pixbuf_flip (icon, TRUE);
    icon = flippedIcon;
    if (free_reflect)
      g_object_unref (reflect);
    else
      free_reflect = TRUE;
    reflect = gdk_pixbuf_flip (icon, FALSE);
  }

  /* glow */
  GdkPixbuf *glowingIcon = NULL;
  if (fx->glow_amount > 0)
  {
    glowingIcon = gdk_pixbuf_copy (icon);
    icon = glowingIcon;
    lighten_pixbuf (icon, fx->glow_amount);
    if (free_reflect)
      g_object_unref (reflect);
    else
      free_reflect = TRUE;
    reflect = gdk_pixbuf_flip (icon, FALSE);
  }

  /* icon depth */
  if (fx->icon_depth)
  {
    if (!fx->icon_depth_direction)
      x1 += fx->icon_depth / 2;
    else
      x1 -= fx->icon_depth / 2;
    apply_3d_illusion (fx, cr, icon, x1, y1, fx->alpha);
  }


  gdk_cairo_set_source_pixbuf (cr, icon, x1, y1);
  cairo_paint_with_alpha (cr, fx->alpha);
  if (scaledIcon)
    g_object_unref (scaledIcon);
  if (clippedIcon)
    g_object_unref (clippedIcon);
  if (flippedIcon)
    g_object_unref (flippedIcon);
  if (saturatedIcon)
    g_object_unref (saturatedIcon);
  if (glowingIcon)
    g_object_unref (glowingIcon);

  /* reflection */
  if (fx->y_offset >= 0)
  {
    if (!reflect)
    {
      // create reflection automatically
      reflect = gdk_pixbuf_flip (icon, FALSE);
      free_reflect = TRUE;
    }
    y1 += current_height + fx->y_offset * 2;

    /* icon depth */
    if (fx->icon_depth)
    {
      apply_3d_illusion (fx, cr, reflect, x1, y1, fx->alpha / 6);
    }
    gdk_cairo_set_source_pixbuf (cr, reflect, x1, y1);
    cairo_paint_with_alpha (cr, fx->alpha / 3);
    if (free_reflect)
      g_object_unref (reflect);
  }

  /* 4px offset for 3D look */
  if (fx->settings && fx->settings->bar_angle > 0)
  {
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba (cr, 1, 1, 1, 0);
    cairo_rectangle (cr, 0,
		     ((fx->settings->bar_height * 2) - 4) +
		     fx->settings->icon_offset, fx->window_width, 4);
    cairo_fill (cr);
    cairo_restore (cr);
  }
}

void
awn_draw_foreground (AwnEffects * fx, cairo_t * cr)
{
  gint x1 = 0;
  gint y1 = fx->window_height - fx->icon_height;
  if (fx->settings)
    y1 = fx->window_height - fx->settings->icon_offset - fx->icon_height;

  GdkPixbuf *spot = NULL;
  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    y1 += fx->icon_height / 12;
    spot =
      gdk_pixbuf_scale_simple (SPOTLIGHT_PIXBUF, fx->window_width,
			       fx->icon_height * 5 / 4, GDK_INTERP_BILINEAR);
    gdk_cairo_set_source_pixbuf (cr, spot, x1, y1);
    cairo_paint_with_alpha (cr, fx->spotlight_alpha / 2);
    g_object_unref (spot);
  }
}

void
awn_draw_set_icon_size (AwnEffects * fx, const gint width, const gint height)
{
  fx->icon_width = width;
  fx->icon_height = height;
}

void
awn_draw_set_window_size (AwnEffects * fx, const gint width,
			  const gint height)
{
  fx->window_width = width;
  fx->window_height = height;
}

static gdouble
calc_curve_position (gdouble cx, gdouble a, gdouble b)	// pos, width, height
{
  return a <= 0 ? 0 : sin (cx / a * M_PI) * b;
}
