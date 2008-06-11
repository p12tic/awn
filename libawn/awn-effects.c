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
#include "awn-effects-ops.h"

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
#include "awn-effect-spotlight3d.h"
#include "awn-effect-desaturate.h"


#define  M_PI 3.14159265358979323846
#define  RADIANS_PER_DEGREE  0.0174532925

#define  AWN_FRAME_RATE    40

/* FORWARD DECLARATIONS */

extern GdkPixbuf *SPOTLIGHT_PIXBUF;


#define EFFECT_BOUNCE 0
#define EFFECT_FADE 1
#define EFFECT_SPOTLIGHT 2
#define EFFECT_ZOOM 3
#define EFFECT_SQUISH 4
#define EFFECT_TURN_3D 5
#define EFFECT_TURN_3D_SPOTLIGHT 6
#define EFFECT_GLOW 7

// NOTE! Always make sure that all EFFECTS arrays have same number of elements
static const GSourceFunc OPENING_EFFECTS[] =
{
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
static const GSourceFunc CLOSING_EFFECTS[] =
{
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
static const GSourceFunc HOVER_EFFECTS[] =
{
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
static const GSourceFunc LAUNCHING_EFFECTS[] =
{
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
static const GSourceFunc ATTENTION_EFFECTS[] =
{
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


// effect functions
static gboolean awn_on_enter_event(GtkWidget * widget,
                                   GdkEventCrossing * event, gpointer data);
static gboolean awn_on_leave_event(GtkWidget * widget,
                                   GdkEventCrossing * event, gpointer data);

static gdouble calc_curve_position(gdouble cx, gdouble a, gdouble b);




void
awn_effects_init(GObject * self, AwnEffects * fx)
{
  fx->self = self;
  fx->settings = awn_get_settings();
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
  fx->alpha = fx->settings->icon_alpha;
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

  fx->effect_frame_rate = AWN_FRAME_RATE;
}

void
awn_effect_dispose_queue(AwnEffects * fx)
{
  if (fx->timer_id)
  {
    GSource *s = g_main_context_find_source_by_id(NULL, fx->timer_id);

    if (s)
      g_source_destroy(s);
  }

  GList *queue = fx->effect_queue;

  while (queue)
  {
    g_free(queue->data);
    queue->data = NULL;
    queue = g_list_next(queue);
  }

  g_list_free(fx->effect_queue);

  fx->effect_queue = NULL;
}

void
awn_effects_finalize(AwnEffects * fx)
{
  awn_unregister_effects(fx);
  awn_effect_dispose_queue(fx);
  fx->self = NULL;
}



void
awn_effects_set_title(AwnEffects * fx, AwnTitle * title,
                      AwnTitleCallback title_func)
{
  fx->title = title;
  fx->get_title = title_func;
}

static gboolean
awn_on_enter_event(GtkWidget * widget, GdkEventCrossing * event,
                   gpointer data)
{

  AwnEffects *fx = (AwnEffects *) data;

  if (fx->settings)
    fx->settings->hiding = FALSE;

  if (fx->title && fx->get_title)
  {
    awn_title_show(fx->title, fx->focus_window, fx->get_title(fx->self));
  }

  fx->hover = TRUE;

  awn_effect_start(fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gboolean
awn_on_leave_event(GtkWidget * widget, GdkEventCrossing * event,
                   gpointer data)
{
  AwnEffects *fx = (AwnEffects *) data;

  if (fx->title)
  {
    awn_title_hide(fx->title, fx->focus_window);
  }

  fx->hover = FALSE;

  awn_effect_stop(fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gint
awn_effect_sort(gconstpointer a, gconstpointer b)
{
  const AwnEffectsPrivate *data1 = (AwnEffectsPrivate *) a;
  const AwnEffectsPrivate *data2 = (AwnEffectsPrivate *) b;
  return (gint)(data1->priority - data2->priority);
}

inline AwnEffectPriority
awn_effect_get_priority(const AwnEffect effect)
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
awn_effect_start(AwnEffects * fx, const AwnEffect effect)
{
  awn_effect_start_ex(fx, effect, NULL, NULL, 0);
}

void
awn_effect_start_ex(AwnEffects * fx, const AwnEffect effect,
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

    queue = g_list_next(queue);
  }

  AwnEffectsPrivate *priv = g_new(AwnEffectsPrivate, 1);

  priv->effects = fx;
  priv->this_effect = effect;
  priv->priority = awn_effect_get_priority(effect);
  priv->max_loops = max_loops;
  priv->start = start;
  priv->stop = stop;

  fx->effect_queue =
    g_list_insert_sorted(fx->effect_queue, priv, awn_effect_sort);
  main_effect_loop(fx);
}

void
awn_effect_stop(AwnEffects * fx, const AwnEffect effect)
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

    queue = g_list_next(queue);
  }

  if (queue)
  {
    gboolean dispose = queue_item->this_effect != fx->current_effect;
    fx->effect_queue = g_list_remove(fx->effect_queue, queue_item);

    if (dispose)
      g_free(queue_item);
  }
}

static GSourceFunc get_animation(AwnEffectsPrivate *topEffect, gint effects[])
{
  GSourceFunc animation = NULL;

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

    case AWN_EFFECT_DESATURATE:
      animation = (GSourceFunc) desaturate_effect;
      break;

    default:
      break;
  }

  return animation;
}

void
main_effect_loop(AwnEffects * fx)
{
  if (fx->current_effect != AWN_EFFECT_NONE || fx->effect_queue == NULL)
    return;

  AwnEffectsPrivate *topEffect =
    (AwnEffectsPrivate *)(fx->effect_queue->data);

  GSourceFunc animation = NULL;

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

      if (effect >= sizeof(HOVER_EFFECTS) / sizeof(GSourceFunc))
        effect = -1;

      // spotlight initialization
      if (effect == EFFECT_SPOTLIGHT || effect == EFFECT_TURN_3D_SPOTLIGHT)
        spotlight_init();

      effects[i] = effect + 1;
    }
  }

  animation = get_animation(topEffect, effects);

  if (animation)
  {
    fx->timer_id = g_timeout_add(fx->effect_frame_rate, animation, topEffect);
    fx->current_effect = topEffect->this_effect;
    fx->effect_lock = FALSE;
  }
  else
  {
    if (topEffect->start)
      topEffect->start(fx->self);

    if (topEffect->stop)
      topEffect->stop(fx->self);

    // dispose AwnEffectsPrivate
    awn_effect_stop(fx, topEffect->this_effect);
  }
}

void
awn_register_effects(GObject * obj, AwnEffects * fx)
{
  if (fx->focus_window)
  {
    awn_unregister_effects(fx);
  }

  fx->focus_window = GTK_WIDGET(obj);

  fx->enter_notify =
    g_signal_connect(obj, "enter-notify-event",
                     G_CALLBACK(awn_on_enter_event), fx);
  fx->leave_notify =
    g_signal_connect(obj, "leave-notify-event",
                     G_CALLBACK(awn_on_leave_event), fx);
}

void
awn_unregister_effects(AwnEffects * fx)
{
  if (fx->enter_notify)
    g_signal_handler_disconnect(G_OBJECT(fx->focus_window),
                                fx->enter_notify);

  if (fx->leave_notify)
    g_signal_handler_disconnect(G_OBJECT(fx->focus_window),
                                fx->leave_notify);

  fx->enter_notify = 0;

  fx->leave_notify = 0;

  fx->focus_window = NULL;
}

void
apply_spotlight(AwnEffects * fx, cairo_t * cr)
{
  static cairo_t * unscaled_spot_ctx = NULL; //FIXME ?
  static gint scaled_width = -1;
  static gint scaled_height = -1;
  cairo_t * spot_ctx = NULL; //FIXME ?
  cairo_surface_t * spot_srfc = NULL;
  gint x1 = 0;
  gint y1 = fx->window_height - fx->icon_height;

  if (fx->settings)
    y1 = fx->window_height - fx->settings->icon_offset - fx->icon_height;

  if (!spot_ctx)
  {
    cairo_surface_t * srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                             gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF),
                             gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF));
    unscaled_spot_ctx = cairo_create(srfc);
    gdk_cairo_set_source_pixbuf(unscaled_spot_ctx, SPOTLIGHT_PIXBUF, 0, 0);
    cairo_paint(unscaled_spot_ctx);
  }

  if ((scaled_width != fx->window_width) || (scaled_height != fx->icon_height*1.2))
  {
    if (spot_ctx)
    {
      cairo_surface_destroy(spot_srfc);
      cairo_destroy(spot_ctx);
    }

    scaled_width = fx->window_width;

    scaled_height = fx->icon_height * 1.2;
    spot_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                           fx->window_width,
                                           fx->icon_height * 1.2);
    spot_ctx = cairo_create(spot_srfc);
    cairo_save(spot_ctx);
    cairo_scale(spot_ctx,
                fx->icon_width / (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF)*1.2,
                fx->icon_height / (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF)*1.2
               );
    cairo_set_source_surface(spot_ctx, cairo_get_target(unscaled_spot_ctx), 0.0, 0.0);
    cairo_paint(spot_ctx);
    cairo_scale(spot_ctx,
                (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF) / fx->icon_width ,
                (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF) / fx->icon_height
               );
    cairo_restore(spot_ctx);
  }

  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    y1 = y1 + fx->icon_height / 12;
    cairo_save(cr);
    cairo_set_source_surface(cr, cairo_get_target(spot_ctx), x1, y1);
    cairo_paint_with_alpha(cr, fx->spotlight_alpha);
    cairo_restore(cr);
  }
}


void
awn_draw_background(AwnEffects * fx, cairo_t * cr)
{
  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    apply_spotlight(fx, cr);
  }
}

void apply_awn_curves(AwnEffects * fx)
{
  if (fx->settings->bar_angle < 0)
  {
    int awn_bar_width = fx->settings->bar_width;
    double awn_curve_curviness = fx->settings->curviness;
    int awn_monitor_width = fx->settings->monitor_width;

    gint curvex = GTK_WIDGET(fx->self)->allocation.x;

    if (curvex == 0)  // is applet?
    {
      gint curvex1 = 0;
      gdk_window_get_origin(fx->focus_window->window, &curvex1, NULL);
      curvex = curvex1 - (awn_monitor_width - awn_bar_width) / 2;
    }

    if (awn_curve_curviness)
      fx->settings->curviness = awn_curve_curviness;

    if (curvex > 0)
      fx->curve_offset =
        calc_curve_position(curvex + (fx->settings->task_width / 4),
                            awn_bar_width,
                            (fx->settings->bar_height *
                             fx->settings->curviness) / 2.);
    else
      fx->curve_offset = 0;

  }
  else if (fx->curve_offset)
  {
    fx->curve_offset = 0;
  }
}


//currently falls back to pixbuf code in more places than not.
void
awn_draw_icons_cairo(AwnEffects * fx, cairo_t * cr, cairo_t *  icon_context,
                     cairo_t * reflect_context)
{
  cairo_surface_t * icon;
  cairo_surface_t * reflect;
  static cairo_surface_t * icon_srfc = NULL;
  static cairo_t * icon_ctx = NULL;
  cairo_surface_t * reflect_srfc = NULL;
  cairo_t * reflect_ctx = NULL;
  DrawIconState  ds;

  icon = cairo_get_target(icon_context);

  if (reflect_context)
  {
    reflect = cairo_get_target(reflect_context);
  }

  fx->icon_width = cairo_image_surface_get_width(icon);

  fx->icon_height = cairo_image_surface_get_height(icon);

  ds.current_width = fx->icon_width;
  ds.current_height = fx->icon_height;
  ds.x1 = (fx->window_width - ds.current_width) / 2;
  ds.y1 = (fx->window_height - ds.current_height); // sit on bottom by default

  apply_awn_curves(fx);

  if (fx->settings)
  {
    ds.y1 = fx->window_height - fx->settings->icon_offset - ds.current_height -
            fx->y_offset;
  }

  ds.y1 -= fx->curve_offset;

  if (fx->clip)
  {
    g_warning("Clipping ~\n");
  }

  /* scaling */
//always first.
  awn_effect_op_scaling(fx, &ds, icon, &icon_srfc, &icon_ctx, &reflect_srfc, &reflect_ctx);

//These will move into the configurable list
  
  awn_effect_op_saturate(fx, &ds, icon_srfc, icon_ctx, NULL);
  awn_effect_op_hflip(fx, &ds, icon_srfc, icon_ctx, NULL);
  awn_effect_op_glow(fx, &ds, icon_srfc, icon_ctx, NULL);
  awn_effect_move_x(fx, &ds, icon_srfc, icon_ctx, NULL);
  
//always last.
  awn_effect_op_3dturn(fx, cr, &ds, icon_srfc, icon_ctx, NULL);

  cairo_set_source_surface(cr, icon_srfc, ds.x1, ds.y1);
  cairo_paint_with_alpha(cr, fx->alpha);

  //------------------------------------------------------------------------
  /* reflection */


  if (fx->y_offset >= 0)
  {
    ds.y1 += ds.current_height + fx->y_offset * 2;
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix,
                      1,
                      0,
                      0,
                      -1,
                      (ds.current_width / 2.0)*(1 - (1)),
                      (ds.current_height / 2.0)*(1 - (-1))
                     );

    cairo_transform(reflect_ctx, &matrix);
    cairo_set_source_surface(reflect_ctx, icon_srfc, 0, 0);
    cairo_paint(reflect_ctx);

    cairo_set_source_surface(cr, reflect_srfc, ds.x1, ds.y1);
    cairo_paint_with_alpha(cr, fx->alpha / 3);
    /* icon depth */
  }

  //------------------------------------------------------------------------


  /* 4px offset for 3D look */
  /*  if (fx->settings && fx->settings->bar_angle > 0)
    {
      cairo_save (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr, 1, 1, 1, 0);
      cairo_rectangle (cr, 0, ((fx->settings->bar_height * 2) - 4) +
         fx->settings->icon_offset, fx->window_width, 4);
      cairo_fill (cr);
      cairo_restore (cr);
    }*/
  cairo_destroy(reflect_ctx);
  cairo_surface_destroy(reflect_srfc);
}

void
awn_draw_icons(AwnEffects * fx, cairo_t * cr, GdkPixbuf * icon,
               GdkPixbuf * reflect)
{
  /*rather inefficient still...
    eventually this can be handled in awn-applet simple which will allow
    context / surfaces to be preserved unless the actuall icons change....

   */

  cairo_surface_t * icon_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                gdk_pixbuf_get_width(icon),
                                gdk_pixbuf_get_height(icon));

  cairo_t * icon_context = cairo_create(icon_srfc);

  gdk_cairo_set_source_pixbuf(icon_context, icon, 0, 0);

  cairo_paint(icon_context);
  awn_draw_icons_cairo(fx, cr, icon_context, NULL);
  cairo_surface_destroy(icon_srfc);
  cairo_destroy(icon_context);
}

void
awn_draw_foreground(AwnEffects * fx, cairo_t * cr)
{
  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    apply_spotlight(fx, cr);
  }

}

void
awn_draw_set_icon_size(AwnEffects * fx, const gint width, const gint height)
{
  fx->icon_width = width;
  fx->icon_height = height;
}

void
awn_draw_set_window_size(AwnEffects * fx, const gint width,
                         const gint height)
{
  fx->window_width = width;
  fx->window_height = height;
}

static gdouble
calc_curve_position(gdouble cx, gdouble a, gdouble b)  // pos, width, height
{
  return a <= 0 ? 0 : sin(cx / a * M_PI) * b;
}
