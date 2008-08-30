/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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
#include <cairo/cairo-xlib.h>

#include "awn-effect-spotlight.h"
#include "awn-effect-bounce.h"
#include "awn-effect-glow.h"
#include "awn-effect-zoom.h"
#include "awn-effect-fade.h"
#include "awn-effect-squish.h"
#include "awn-effect-turn.h"
#include "awn-effect-spotlight3d.h"
#include "awn-effect-desaturate.h"

#ifndef M_PI
#define  M_PI 3.14159265358979323846
#endif
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
static const GSourceFunc FINALIZE_EFFECTS[] =
{
  NULL,
  (GSourceFunc) bounce_effect_finalize,
  (GSourceFunc) fading_effect_finalize,
  (GSourceFunc) spotlight_effect_finalize,
  (GSourceFunc) zoom_effect_finalize,
  (GSourceFunc) bounce_squish_effect_finalize,
  (GSourceFunc) turn_effect_finalize,
  (GSourceFunc) spotlight3D_effect_finalize,
  (GSourceFunc) glow_effect_finalize
};

//The default ops list that the existing effects expect.
static const AwnEffectsOp OP_LIST[] =
{
  {awn_effect_op_saturate, NULL},
  {awn_effect_op_hflip, NULL},
  {awn_effect_op_glow, NULL},
  {awn_effect_move_x, NULL},
  {awn_effect_op_3dturn, NULL},
  {(AwnEffectsOpfn)NULL, NULL}
};

// effect functions

/**
 * awn_effects_init:
 * @fx: Pointer to #AwnEffects structure.
 * @obj: Object which will be passed to all callback functions, this object is
 * also passed to gtk_widget_queue_draw() during the animation.
 *
 * Initializes #AwnEffects structure.
 */
static void awn_effects_init(AwnEffects * fx, GtkWidget * widget);

static gboolean awn_effects_enter_event(GtkWidget * widget,
                                   GdkEventCrossing * event, gpointer data);
static gboolean awn_effects_leave_event(GtkWidget * widget,
                                   GdkEventCrossing * event, gpointer data);

static gdouble calc_curve_position(gdouble cx, gdouble a, gdouble b);

static gpointer _awn_effects_copy(gpointer boxed)
{
  // FIXME: should we really make a copy?
  return boxed;
}

static void _awn_effects_free(gpointer boxed)
{
  if (boxed)
    awn_effects_free(AWN_EFFECTS(boxed));
}

void awn_effects_free(AwnEffects *fx)
{
  awn_effects_finalize(fx);
  g_free(fx->op_list);
  g_free(fx);
}

GType awn_effects_get_type(void)
{
  static GType type = 0;

  if (type == 0) {
    type = g_boxed_type_register_static("AwnEffects",
                                        _awn_effects_copy,
                                        _awn_effects_free);
  }

  return type;
}

AwnEffects* awn_effects_new()
{
  AwnEffects *fx = g_new(AwnEffects, 1);
  awn_effects_init(fx, NULL);
  return fx;
}

AwnEffects* awn_effects_new_for_widget(GtkWidget * widget)
{
  AwnEffects *fx = g_new(AwnEffects, 1);
  awn_effects_init(fx, widget);
  return fx;
}

void
awn_effects_init(AwnEffects * fx, GtkWidget * widget)
{
  fx->self = widget;
  fx->focus_window = NULL;
  fx->settings = awn_get_settings();
  fx->title = NULL;
  fx->get_title = NULL;
  fx->effect_queue = NULL;

  fx->icon_width = 48;
  fx->icon_height = 48;
  fx->window_width = 0;
  fx->window_height = 0;

  /* EFFECT VARIABLES */
  fx->effect_lock = FALSE;
  fx->current_effect = AWN_EFFECT_NONE;
  fx->direction = AWN_EFFECT_DIR_NONE;
  fx->count = 0;

  fx->x_offset = 0;
  fx->y_offset = 0;
  fx->curve_offset = 0;

  fx->delta_width = 0;
  fx->delta_height = 0;

  //fx->clip_region = ; // no need to init, there's a boolean guarding it

  fx->rotate_degrees = 0.0;
  fx->alpha = 1.0;
  fx->spotlight_alpha = 0.0;
  fx->saturation = 1.0;
  fx->glow_amount = 0.0;

  fx->icon_depth = 0;
  fx->icon_depth_direction = 0;

  fx->hover = FALSE;
  fx->clip = FALSE;
  fx->flip = FALSE;
  fx->spotlight = FALSE;
  fx->do_reflections = TRUE;
  fx->do_offset_cut = TRUE;

  fx->enter_notify = 0;
  fx->leave_notify = 0;
  fx->timer_id = 0;

  fx->icon_ctx = NULL;
  fx->reflect_ctx = NULL;

  fx->op_list = g_malloc(sizeof(OP_LIST));
  memcpy(fx->op_list, OP_LIST, sizeof(OP_LIST));
}

static void
awn_effects_dispose_queue(AwnEffects * fx)
{
  if (fx->timer_id)
  {
    GSource *s = g_main_context_find_source_by_id(NULL, fx->timer_id);

    if (s)
    {
      g_source_destroy(s);
    }
  }

  GList *queue = fx->effect_queue;

  while (queue)
  {
    g_free(queue->data);
    queue->data = NULL;
    queue = g_list_next(queue);
  }

  if (fx->effect_queue) g_list_free(fx->effect_queue);

  fx->effect_queue = NULL;
}

void
awn_effects_finalize(AwnEffects * fx)
{
  awn_effects_unregister(fx);
  awn_effects_dispose_queue(fx);
  

  if (  fx->icon_ctx)
  {
    cairo_surface_destroy( cairo_get_target(fx->icon_ctx));    
    cairo_destroy(  fx->icon_ctx);
    fx->icon_ctx=NULL;
  }

  if (   fx->reflect_ctx )
  {
    cairo_surface_destroy( cairo_get_target(fx->reflect_ctx));        
    cairo_destroy(   fx->reflect_ctx );
    fx->reflect_ctx=NULL;
  }
  
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
awn_effects_enter_event(GtkWidget * widget, GdkEventCrossing * event,
                   gpointer data)
{

  AwnEffects *fx = (AwnEffects *) data;

  if (fx->settings)
  {
    fx->settings->hiding = FALSE;
  }

  if (fx->title && fx->get_title)
  {
    awn_title_show(fx->title, fx->focus_window, fx->get_title(fx->self));
  }

  fx->hover = TRUE;

  awn_effects_start(fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gboolean
awn_effects_leave_event(GtkWidget * widget, GdkEventCrossing * event,
                   gpointer data)
{
  AwnEffects *fx = (AwnEffects *) data;

  if (fx->title)
  {
    awn_title_hide(fx->title, fx->focus_window);
  }

  fx->hover = FALSE;

  awn_effects_stop(fx, AWN_EFFECT_HOVER);
  return FALSE;
}

static gint
awn_effects_sort(gconstpointer a, gconstpointer b)
{
  const AwnEffectsPrivate *data1 = (AwnEffectsPrivate *) a;
  const AwnEffectsPrivate *data2 = (AwnEffectsPrivate *) b;
  return (gint)(data1->priority - data2->priority);
}

static AwnEffectPriority
awn_effects_get_priority(const AwnEffect effect)
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

void awn_effects_reflection_off(AwnEffects * fx)
{
  fx->do_reflections = FALSE;  
}
void awn_effects_reflection_on(AwnEffects * fx)
{
  fx->do_reflections = TRUE; 
}
void awn_effects_set_offset_cut(AwnEffects * fx, gboolean cut)
{
  fx->do_offset_cut = cut;
}


void
awn_effects_start(AwnEffects * fx, const AwnEffect effect)
{
  awn_effects_start_ex(fx, effect, NULL, NULL, 0);
}

void
awn_effects_start_ex(AwnEffects * fx, const AwnEffect effect,
                    AwnEventNotify start, AwnEventNotify stop,
                    gint max_loops)
{
  if (effect == AWN_EFFECT_NONE || fx->self == NULL)
  {
    return;
  }

  AwnEffectsPrivate *queue_item;

  GList *queue = fx->effect_queue;

  // dont start the effect if already in queue

  while (queue)
  {
    queue_item = queue->data;

    if (queue_item->this_effect == effect)
    {
      return;
    }

    queue = g_list_next(queue);
  }

  AwnEffectsPrivate *priv = g_new(AwnEffectsPrivate, 1);

  priv->effects = fx;
  priv->this_effect = effect;
  priv->priority = awn_effects_get_priority(effect);
  priv->max_loops = max_loops;
  priv->start = start;
  priv->stop = stop;

  fx->effect_queue = g_list_insert_sorted(fx->effect_queue, priv,
                                          awn_effects_sort);
  awn_effects_main_effect_loop(fx);
}

void
awn_effects_stop(AwnEffects * fx, const AwnEffect effect)
{
  if (effect == AWN_EFFECT_NONE)
  {
    return;
  }

  AwnEffectsPrivate *queue_item;

  GList *queue = fx->effect_queue;

  // remove the effect if in queue

  while (queue)
  {
    queue_item = queue->data;

    if (queue_item->this_effect == effect)
    {
      break;
    }

    queue = g_list_next(queue);
  }

  if (queue)
  {
    gboolean dispose = queue_item->this_effect != fx->current_effect;
    fx->effect_queue = g_list_remove(fx->effect_queue, queue_item);

    if (dispose)
    {
      g_free(queue_item);
    }
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
//      g_assert(FALSE);    //let's find out when this is used.  I believe it is somewhere...
      animation = (GSourceFunc) desaturate_effect;
      break;

    default:
      break;
  }

  return animation;
}

void
awn_effects_main_effect_loop(AwnEffects * fx)
{
  if (fx->current_effect != AWN_EFFECT_NONE || fx->effect_queue == NULL)
  {
    return;
  }

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
      {
        effect = -1;
      }

      // spotlight initialization
      if (effect == EFFECT_SPOTLIGHT || effect == EFFECT_TURN_3D_SPOTLIGHT)
      {
        spotlight_init();
      }

      effects[i] = effect + 1;
    }
  }

  animation = get_animation(topEffect, effects);

  if (animation)
  {
    fx->timer_id = g_timeout_add(1000 / fx->settings->frame_rate,
                                 animation, topEffect);
    fx->current_effect = topEffect->this_effect;
    fx->effect_lock = FALSE;
  }
  else
  {
    if (topEffect->start)
    {
      topEffect->start(fx->self);
    }

    if (topEffect->stop)
    {
      topEffect->stop(fx->self);
    }

    // dispose AwnEffectsPrivate
    awn_effects_stop(fx, topEffect->this_effect);
  }
}

void
awn_effects_register(AwnEffects * fx, GtkWidget * obj)
{
  if (fx->focus_window)
  {
    awn_effects_unregister(fx);
  }

  fx->focus_window = obj;

  fx->enter_notify = g_signal_connect(G_OBJECT(obj), "enter-notify-event",
                                      G_CALLBACK(awn_effects_enter_event), fx);
  fx->leave_notify = g_signal_connect(G_OBJECT(obj), "leave-notify-event",
                                      G_CALLBACK(awn_effects_leave_event), fx);
}

void
awn_effects_unregister(AwnEffects * fx)
{
  if (fx->enter_notify)
  {
    g_signal_handler_disconnect(G_OBJECT(fx->focus_window), fx->enter_notify);
    fx->enter_notify = 0;
  }

  if (fx->leave_notify)
  {
    g_signal_handler_disconnect(G_OBJECT(fx->focus_window), fx->leave_notify);
    fx->leave_notify = 0;
  }

  fx->focus_window = NULL;
}

void
apply_spotlight(AwnEffects * fx, cairo_t * cr)
{
  static cairo_t * unscaled_spot_ctx = NULL;
  static gint scaled_width = -1;
  static gint scaled_height = -1;
  static cairo_t * spot_ctx = NULL;
  cairo_surface_t * spot_srfc = NULL;
//  gint x1 = 0;
  gint y1 = fx->window_height - fx->icon_height;

  if (fx->settings)
    y1 = fx->window_height - fx->settings->icon_offset - fx->icon_height;

  if (!unscaled_spot_ctx)
  {
    cairo_surface_t * srfc = cairo_surface_create_similar(cairo_get_target(cr),
                             CAIRO_CONTENT_COLOR_ALPHA,
                             gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF),
                             gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF));
    unscaled_spot_ctx = cairo_create(srfc);
    gdk_cairo_set_source_pixbuf(unscaled_spot_ctx, SPOTLIGHT_PIXBUF, 0, 0);
    cairo_paint(unscaled_spot_ctx);
  }
 // printf("window width = %d\n",fx->window_width);
  if ((scaled_width != fx->window_width) ||
      (scaled_height != fx->icon_height*5/4))
  {
  //  printf("rescale spot \n");
    if (spot_ctx)
    {
      cairo_surface_destroy(spot_srfc);
      cairo_destroy(spot_ctx);
    }

    scaled_width = fx->window_width;

    scaled_height = fx->icon_height*5/4;
    spot_srfc = cairo_surface_create_similar(cairo_get_target(cr),
                CAIRO_CONTENT_COLOR_ALPHA,
                scaled_width,
                scaled_height);
    spot_ctx = cairo_create(spot_srfc);
    cairo_save(spot_ctx);
    cairo_scale(spot_ctx,
                fx->window_width / (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF),
                fx->icon_height*5/4 / (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF)
                );

    cairo_set_source_surface(spot_ctx, cairo_get_target(unscaled_spot_ctx),
                             0.0, 0.0);
    cairo_paint(spot_ctx);
    cairo_scale(spot_ctx,
                (double) gdk_pixbuf_get_width(SPOTLIGHT_PIXBUF) / fx->window_width,
                (double) gdk_pixbuf_get_height(SPOTLIGHT_PIXBUF) * 5/4 / fx->icon_height
               );
    cairo_restore(spot_ctx);
  }

  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    y1 = y1 + fx->icon_height / 12;
    cairo_save(cr);
    cairo_set_source_surface(cr, cairo_get_target(spot_ctx), 0, y1);
//    cairo_rectangle(cr,0,0,fx->window_width,fx->window_height*5/4);
    cairo_paint_with_alpha(cr, fx->spotlight_alpha);
    cairo_restore(cr);
  }
}


void
awn_effects_draw_background(AwnEffects * fx, cairo_t * cr)
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

void
darken_surface(cairo_surface_t *src)
{
  int width, height, row_stride;
  guchar *pixsrc, *target_pixels;
  cairo_surface_t *temp_srfc;
  cairo_t *temp_ctx;
  
  temp_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         cairo_xlib_surface_get_width(src),
                                         cairo_xlib_surface_get_height(src)
                                        );
  temp_ctx = cairo_create(temp_srfc);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_set_source_surface(temp_ctx, src, 0, 0);
  cairo_paint(temp_ctx);
  
  width = cairo_image_surface_get_width(temp_srfc);
  height = cairo_image_surface_get_height(temp_srfc);
  row_stride = cairo_image_surface_get_stride(temp_srfc);
  target_pixels = cairo_image_surface_get_data(temp_srfc);

  // darken
  int i, j;
  for (i = 0; i < height; i++) {
    pixsrc = target_pixels + i * row_stride;
    for (j = 0; j < width; j++) {
      *pixsrc = 0;
      pixsrc++;
      *pixsrc = 0;
      pixsrc++;
      *pixsrc = 0;
      pixsrc++;
      // alpha
      pixsrc++;
    }
  }

  // --
  
  cairo_destroy(temp_ctx);

  temp_ctx = cairo_create(src);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);
  g_assert( cairo_get_operator(temp_ctx) == CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(temp_ctx, temp_srfc, 0, 0);
  cairo_paint(temp_ctx);
  cairo_surface_destroy(temp_srfc);
  cairo_destroy(temp_ctx);
}
/*
void
blur_surface(cairo_surface_t *src, const int radius)
{
  guchar * pixdest, * target_pixels_dest, * target_pixels, * pixsrc;
  cairo_surface_t * temp_srfc, * temp_srfc_dest;
  cairo_t         * temp_ctx, * temp_ctx_dest;

  g_return_if_fail(src);
  int width = cairo_xlib_surface_get_width(src);
  int height = cairo_xlib_surface_get_height(src);
  
  // the original stuff
  temp_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  temp_ctx = cairo_create(temp_srfc);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_set_source_surface(temp_ctx, src, 0, 0);
  cairo_paint(temp_ctx);
  
  // the stuff we draw to
  temp_srfc_dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  temp_ctx_dest = cairo_create(temp_srfc_dest);
  //---

  int row_stride = cairo_image_surface_get_stride(temp_srfc);
  target_pixels = cairo_image_surface_get_data(temp_srfc);
  target_pixels_dest = cairo_image_surface_get_data(temp_srfc_dest);

  // -- blur ---
  int total_r, total_g, total_b, total_a;
  int x, y, kx, ky;

  for (y = 0; y < height; ++y)
  {
    for (x = 0; x < width; ++x)
    {
      total_r = total_g = total_b = total_a = 0;

      for (ky = -radius; ky <= radius; ++ky) {
        if ((y+ky)>0 && (y+ky)<height) {
          for (kx = -radius; kx <= radius; ++kx) {
            if((x+kx)>0 && (x+kx)<width) {
              pixsrc = (target_pixels + (y+ky) * row_stride);
              pixsrc += (x+kx)*4;
              pixsrc += 3;
              if(*pixsrc > 0) {
                pixsrc -= 3;
                total_r += *pixsrc; pixsrc++;
                total_g += *pixsrc; pixsrc++;
                total_b += *pixsrc; pixsrc++;
                total_a += *pixsrc;
              }
            }
          }
        }
      }
  
      total_r /= pow((radius<<1)|1,2);
      total_g /= pow((radius<<1)|1,2);
      total_b /= pow((radius<<1)|1,2);
      total_a /= pow((radius<<1)|1,2);    

      pixdest = (target_pixels_dest + y * row_stride);
      pixdest += x*4;

      *pixdest = (guchar) total_r; pixdest++;
      *pixdest = (guchar) total_g; pixdest++;
      *pixdest = (guchar) total_b; pixdest++;
      *pixdest = (guchar) total_a;
    }	
  }
  //----------
  
  cairo_set_operator(temp_ctx, CAIRO_OPERATOR_CLEAR);
  cairo_paint(temp_ctx);
  cairo_destroy(temp_ctx);

  temp_ctx = cairo_create(src);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);
  g_assert( cairo_get_operator(temp_ctx) == CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(temp_ctx, temp_srfc_dest, 0, 0);
  cairo_paint(temp_ctx);
  cairo_surface_destroy(temp_srfc);
  cairo_surface_destroy(temp_srfc_dest);
  cairo_destroy(temp_ctx);
  cairo_destroy(temp_ctx_dest);
}
*/
void
blur_surface_shadow(cairo_surface_t *src, const int radius)
{
  guchar * pixdest, * target_pixels_dest, * target_pixels, * pixsrc;
  cairo_surface_t * temp_srfc, * temp_srfc_dest;
  cairo_t         * temp_ctx, * temp_ctx_dest;

  g_return_if_fail(src);
  int width = cairo_xlib_surface_get_width(src);
  int height = cairo_xlib_surface_get_height(src);
  
  // the original stuff
  temp_srfc = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  temp_ctx = cairo_create(temp_srfc);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);  
  cairo_set_source_surface(temp_ctx, src, 0, 0);
  cairo_paint(temp_ctx);
  
  // the stuff we draw to
  temp_srfc_dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  temp_ctx_dest = cairo_create(temp_srfc_dest);
  //---

  int row_stride = cairo_image_surface_get_stride(temp_srfc);
  target_pixels = cairo_image_surface_get_data(temp_srfc);
  target_pixels_dest = cairo_image_surface_get_data(temp_srfc_dest);

  // -- blur ---
  int total_a;
  int x, y, kx, ky;

  for (y = 0; y < height; ++y)
  {
    for (x = 0; x < width; ++x)
    {
			total_a = 0;

      for (ky = -radius; ky <= radius; ++ky) {
        if ((y+ky)>0 && (y+ky)<height) {
          for (kx = -radius; kx <= radius; ++kx) {
            if((x+kx)>0 && (x+kx)<width) {
              pixsrc = (target_pixels + (y+ky) * row_stride) + ((x+kx)*4) + 3;

              total_a += *pixsrc;
            }
          }
        }
      }
  
      total_a /= pow((radius<<1)|1,2);    

      pixdest = (target_pixels_dest + y * row_stride);
      pixdest += x*4;
			pixdest += 3;
      *pixdest = (guchar) total_a;
    }	
  }
  //----------
  
  cairo_set_operator(temp_ctx, CAIRO_OPERATOR_CLEAR);
  cairo_paint(temp_ctx);
  cairo_destroy(temp_ctx);

  temp_ctx = cairo_create(src);
  cairo_set_operator(temp_ctx,CAIRO_OPERATOR_SOURCE);
  g_assert( cairo_get_operator(temp_ctx) == CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(temp_ctx, temp_srfc_dest, 0, 0);
  cairo_paint(temp_ctx);
  cairo_surface_destroy(temp_srfc);
  cairo_surface_destroy(temp_srfc_dest);
  cairo_destroy(temp_ctx);
  cairo_destroy(temp_ctx_dest);
}

void
make_shadows(AwnEffects * fx, cairo_t * cr, int x1, int y1, int width, int height)
{
    cairo_surface_t * blur_s;
    cairo_t * blur_c;

    // icon shadow
    blur_s = cairo_surface_create_similar(cairo_get_target(cr),CAIRO_CONTENT_COLOR_ALPHA, fx->icon_width+10, fx->icon_height+10);
    blur_c = cairo_create(blur_s);
	
    cairo_set_operator(blur_c,CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(blur_c, cairo_get_target(fx->icon_ctx), 5, 5);
    cairo_paint(blur_c);

    darken_surface(blur_s);
    blur_surface_shadow(blur_s, 4);
  
    cairo_set_source_surface(cr, blur_s, x1-5, y1-7);	
    cairo_paint_with_alpha(cr, 0.5);

    cairo_surface_destroy(blur_s);
    cairo_destroy(blur_c);


    // scaled shadow
    cairo_rectangle(cr, 0, fx->window_height - fx->settings->bar_height + (fx->settings->bar_height / 2) - 4, fx->window_width, fx->settings->bar_height);
    cairo_clip(cr);
    x1 = (fx->window_width - width) / 2;
    y1 = fx->window_height - height - fx->settings->icon_offset;

    blur_s = cairo_surface_create_similar(cairo_get_target(cr),CAIRO_CONTENT_COLOR_ALPHA, fx->icon_width+10, fx->icon_height+10);
    blur_c = cairo_create(blur_s);
	
    cairo_set_operator(blur_c,CAIRO_OPERATOR_SOURCE);
    cairo_scale(blur_c, 1, 0.5);
    cairo_set_source_surface(blur_c, cairo_get_target(fx->icon_ctx), 5, 5);
    cairo_paint(blur_c);

    darken_surface(blur_s);
    blur_surface_shadow(blur_s, 1);
  
    cairo_set_source_surface(cr, blur_s, x1-5, y1-5+(fx->icon_height/2));	
    cairo_paint_with_alpha(cr, 0.2);

    cairo_reset_clip(cr);
    cairo_surface_destroy(blur_s);
    cairo_destroy(blur_c);
}

/*
  The icon surface must be an xlib surface.  expose() in awn-applet-simple
  converts image surfaces to xlib surfaces if needed.  Direct callers of
  this function will need to handle it themselves.
*/
void
awn_effects_draw_icons_cairo(AwnEffects * fx, cairo_t * cr, cairo_t *  icon_context,
                     cairo_t * reflect_context)
{
  cairo_surface_t * icon;       //Surfaces pulled from args.
  cairo_surface_t * reflect = NULL;
  DrawIconState  ds;
  gboolean icon_changed = FALSE;
  gint i;

  icon = cairo_get_target(icon_context);

  if (reflect_context)
  {
    reflect = cairo_get_target(reflect_context);
  }

  fx->icon_width = cairo_xlib_surface_get_width(icon);

  fx->icon_height = cairo_xlib_surface_get_height(icon);
  ds.current_width = fx->icon_width;
  ds.current_height = fx->icon_height;
  ds.x1 = (fx->window_width - ds.current_width) / 2;
  ds.y1 = (fx->window_height - ds.current_height); // sit on bottom by default

  apply_awn_curves(fx); //hopefully I haven't broken awn-curves.

  if (fx->settings)
  {
    ds.y1 = fx->window_height - fx->settings->icon_offset - ds.current_height -
            fx->y_offset;
  }

  ds.y1 -= fx->curve_offset;
  
	if (fx->clip) 
  {
		gint x = fx->clip_region.x;
		gint y = fx->clip_region.y;
		gint w = fx->clip_region.width;
		gint h = fx->clip_region.height;  
		if ( !(	x >= 0 && x < fx->icon_width &&
			w-x > 0 && w-x <= fx->icon_width &&
			y >= 0 && x < fx->icon_height &&
			h-y > 0 && h-y <= fx->icon_height) )
    {
      return;
    }
  }
  
  // sanity check
  if (fx->delta_width <= -ds.current_width
      || fx->delta_height <= -ds.current_height)
  {
    // we would display blank icon
    return;
  }

  /* scaling */
  /*
   first op. WARNING will take care of the (re)allocate of the context/surfaces
   if needed.

   WARNING make no assumptions about fx->icon_srfc, fx->icon_ctx,
   fx->reflect_srfc, fx->reflect_ctx being correct before this call.
  */
  icon_changed = awn_effect_op_scale_and_clip(fx, &ds, icon, &fx->icon_ctx,
                                       &fx->reflect_ctx) || icon_changed;

  for (i = 0;fx->op_list[i].fn;i++)
  {
    icon_changed = fx->op_list[i].fn(fx, &ds, fx->op_list[i].data)
                                    || icon_changed;
  }

  // shadows
  if (fx->settings && fx->settings->bar_angle > 0 && fx->settings->show_shadows)
  {
		make_shadows(fx, cr, ds.x1, ds.y1, ds.current_width, ds.current_height);
  }
	
  //Update our displayed Icon.
  cairo_set_source_surface(cr, cairo_get_target(fx->icon_ctx), ds.x1, ds.y1);

  cairo_paint_with_alpha(cr, fx->settings->icon_alpha * fx->alpha);

  //------------------------------------------------------------------------
  /* reflection */
  
  if (fx->do_reflections)
  {
    if (fx->y_offset >= 0)
    {
      ds.y1 += ds.current_height + fx->y_offset * 2 - ((fx->settings->reflection_offset > 30)? 30 : fx->settings->reflection_offset);

      if (icon_changed || !reflect)
      {
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix,
                          1,
                          0,
                          0,
                          -1,
                          (ds.current_width / 2.0)*(1 - (1)),
                          (ds.current_height / 2.0)*(1 - (-1))
                         );
        cairo_save(fx->reflect_ctx);
        cairo_transform(fx->reflect_ctx, &matrix);
        cairo_set_source_surface(fx->reflect_ctx, cairo_get_target(fx->icon_ctx), 
                                 0, 0);
        cairo_paint(fx->reflect_ctx);

        cairo_set_operator(cr,CAIRO_OPERATOR_DEST_OVER);
        cairo_set_source_surface(cr, cairo_get_target(fx->reflect_ctx), 
                                 ds.x1, ds.y1);
        cairo_paint_with_alpha(cr, fx->alpha / 4);
        cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
        cairo_restore(fx->reflect_ctx);
      }
      else
      {
        cairo_set_source_surface(cr, reflect, ds.x1, ds.y1);
        cairo_paint_with_alpha(cr, fx->settings->icon_alpha * 
                                    fx->alpha * 
                                    fx->settings->reflection_alpha_mult);
      }
    }
    /* 4px offset for 3D look for reflection*/
    if (fx->do_offset_cut && fx->settings && fx->settings->bar_angle > 0)
    {
      cairo_save(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba(cr, 1, 1, 1, 0);
      cairo_rectangle(cr, 0, ((fx->settings->bar_height * 2) - 4) +
                      fx->settings->icon_offset, fx->window_width, 4);
      cairo_fill(cr);
      cairo_restore(cr);
    }
  }
}

void
awn_effects_draw_icons(AwnEffects * fx, cairo_t * cr, GdkPixbuf * icon,
               GdkPixbuf * reflect)
{
  /*rather inefficient still...
    Only user of this AFAIK is core.
   */
  g_return_if_fail(GDK_IS_PIXBUF(icon));

  cairo_surface_t * icon_srfc = cairo_surface_create_similar(
                                  cairo_get_target(cr),
                                  CAIRO_CONTENT_COLOR_ALPHA,
                                  gdk_pixbuf_get_width(icon),
                                  gdk_pixbuf_get_height(icon));
  cairo_t * icon_context = cairo_create(icon_srfc);
  gdk_cairo_set_source_pixbuf(icon_context, icon, 0, 0);
  cairo_paint(icon_context);

  awn_effects_draw_icons_cairo(fx, cr, icon_context, NULL);

  cairo_surface_destroy(icon_srfc);
  cairo_destroy(icon_context);
}

void
awn_effects_draw_foreground(AwnEffects * fx, cairo_t * cr)
{
  if (fx->spotlight && fx->spotlight_alpha > 0)
  {
    apply_spotlight(fx, cr);
  }

}

void
awn_effects_draw_set_icon_size(AwnEffects * fx, const gint width, const gint height)
{
  fx->icon_width = width;
  fx->icon_height = height;
}

void
awn_effects_draw_set_window_size(AwnEffects * fx, const gint width,
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
