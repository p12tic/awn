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
#include "awn-effects-ops-new.h"

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

// FIXME: remove this include, it should be only used in awn-effects-ops.c
#include "awn-effects-ops-helpers.h"

#include "awn-config-client.h"
#include "awn-config-bridge.h"

G_DEFINE_TYPE(AwnEffects, awn_effects, G_TYPE_OBJECT);

#ifndef M_PI
#define  M_PI 3.14159265358979323846
#endif
#define  RADIANS_PER_DEGREE  0.0174532925

// FIXME: add property for fps
#define AWN_FRAME_RATE(fx) (40)
#define AWN_ANIMATIONS_PER_BUNDLE 5

typedef gboolean (*_AwnAnimation)(AwnEffectsPrivate*);

/* FORWARD DECLARATIONS */

extern GdkPixbuf *SPOTLIGHT_PIXBUF;

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_CURRENT_EFFECTS,
  PROP_ICON_OFFSET,
  PROP_ICON_ALPHA,
  PROP_REFLECTION_OFFSET,
  PROP_REFLECTION_ALPHA,
  PROP_REFLECTION_VISIBLE,
  PROP_MAKE_SHADOW,
  PROP_LABEL,
  PROP_IS_ACTIVE,
  PROP_BORDER_CLIP
};

#define EFFECT_BOUNCE 0
#define EFFECT_FADE 1
#define EFFECT_SPOTLIGHT 2
#define EFFECT_ZOOM 3
#define EFFECT_SQUISH 4
#define EFFECT_TURN_3D 5
#define EFFECT_TURN_3D_SPOTLIGHT 6
#define EFFECT_GLOW 7

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

static void
awn_effects_finalize(GObject *object)
{
  AwnEffects * fx = AWN_EFFECTS(object);
  // destroy animation timer
  if (fx->timer_id)
  {
    GSource *s = g_main_context_find_source_by_id(NULL, fx->timer_id);
    if (s) g_source_destroy(s);
  }

  // free effect queue and associated AwnEffectsPriv
  GList *queue = fx->effect_queue;
  while (queue)
  {
    g_free(queue->data);
    queue->data = NULL;
    queue = g_list_next(queue);
  }

  if (fx->effect_queue) g_list_free(fx->effect_queue);
  fx->effect_queue = NULL;

  fx->self = NULL;

  if (fx->label) {
    g_free(fx->label);
    fx->label = NULL;
  }

  // FIXME: can be removed, new effects API doesn't save contexts
  if (fx->icon_ctx)
  {
    cairo_surface_destroy(cairo_get_target(fx->icon_ctx));
    cairo_destroy( fx->icon_ctx);
    fx->icon_ctx = NULL;
  }

  if (fx->reflect_ctx)
  {
    cairo_surface_destroy(cairo_get_target(fx->reflect_ctx));
    cairo_destroy( fx->reflect_ctx );
    fx->reflect_ctx = NULL;
  }

  if (fx->client)
  {
    awn_config_client_free (fx->client);
    fx->client = NULL;
  }

  g_free(fx->op_list);
  fx->op_list = NULL;
}

static void
awn_effects_get_property (GObject      *object,
                          guint         prop_id,
                          GValue *value,
                          GParamSpec   *pspec)
{
  AwnEffects *fx = AWN_EFFECTS(object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      g_value_set_int(value, fx->orientation);
      break;
    case PROP_CURRENT_EFFECTS:
      g_value_set_uint(value, fx->set_effects);
      break;
    case PROP_ICON_OFFSET:
      g_value_set_int(value, fx->icon_offset);
      break;
    case PROP_ICON_ALPHA:
      g_value_set_float(value, fx->icon_alpha);
      break;
    case PROP_REFLECTION_OFFSET:
      g_value_set_int(value, fx->refl_offset);
      break;
    case PROP_REFLECTION_ALPHA:
      g_value_set_float(value, fx->refl_alpha);
      break;
    case PROP_REFLECTION_VISIBLE:
      g_value_set_boolean(value, fx->do_reflection);
      break;
    case PROP_MAKE_SHADOW:
      g_value_set_boolean(value, fx->make_shadow);
      break;
    case PROP_LABEL:
      g_assert(fx->label);
      g_value_set_string(value, fx->label);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean(value, fx->is_active);
      break;
    case PROP_BORDER_CLIP:
      g_value_set_int(value, fx->border_clip);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_effects_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  AwnEffects *fx = AWN_EFFECTS(object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      fx->orientation = g_value_get_int(value);
      break;
    case PROP_CURRENT_EFFECTS:
      fx->set_effects = g_value_get_uint(value);
      break;
    case PROP_ICON_OFFSET:
      fx->icon_offset = g_value_get_int(value);
      break;
    case PROP_ICON_ALPHA:
      fx->icon_alpha = g_value_get_float(value);
      break;
    case PROP_REFLECTION_OFFSET:
      fx->refl_offset = g_value_get_int(value);
      break;
    case PROP_REFLECTION_ALPHA:
      fx->refl_alpha = g_value_get_float(value);
      break;
    case PROP_REFLECTION_VISIBLE:
      fx->do_reflection = g_value_get_boolean(value);
      break;
    case PROP_MAKE_SHADOW:
      fx->make_shadow = g_value_get_boolean(value);
      break;
    case PROP_LABEL:
      if (fx->label) g_free(fx->label);
      fx->label = g_value_dup_string(value);
      break;
    case PROP_IS_ACTIVE:
      fx->is_active = g_value_get_boolean(value);
      break;
    case PROP_BORDER_CLIP:
      fx->border_clip = g_value_get_int(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_effects_prop_changed(GObject *object, GParamSpec *pspec)
{
  AwnEffects *fx = AWN_EFFECTS(object);

  awn_effects_redraw(fx);
}

void
awn_effects_redraw(AwnEffects *fx)
{
  if (fx->self && GTK_WIDGET_DRAWABLE(fx->self)) {
    gtk_widget_queue_draw(fx->self);
  }
}

void awn_effects_register_effect_bundle(AwnEffectsClass *klass,
                                        _AwnAnimation opening,
                                        _AwnAnimation closing,
                                        _AwnAnimation hover,
                                        _AwnAnimation launching,
                                        _AwnAnimation attention)
{
  // make sure there are exactly AWN_ANIMATIONS_PER_BUNDLE items
  GPtrArray *anims = klass->animations;

  g_ptr_array_add(anims, opening);
  g_ptr_array_add(anims, closing);
  g_ptr_array_add(anims, hover);
  g_ptr_array_add(anims, launching);
  g_ptr_array_add(anims, attention);
}

static void
awn_effects_constructed (GObject *object)
{
  AwnEffects      *effects = AWN_EFFECTS (object);
  AwnConfigClient *client;
  AwnConfigBridge *bridge = awn_config_bridge_get_default ();

  effects->client = awn_config_client_new ();
  client = effects->client;

  awn_config_bridge_bind (bridge, client,
                          "effects", "icon_effect",
                          object, "effects");
  awn_config_bridge_bind (bridge, client,
                          "panel", "offset",
                          object, "icon-offset");
  awn_config_bridge_bind (bridge, client,
                          "effects", "icon_alpha",
                          object, "icon-alpha");
  awn_config_bridge_bind (bridge, client,
                          "effects", "reflection_alpha_multiplier",
                          object, "reflection-alpha");
  awn_config_bridge_bind (bridge, client,
                          "effects", "reflection_offset",
                          object, "reflection-offset");
  awn_config_bridge_bind (bridge, client,
                          "effects", "show_shadow",
                          object, "make-shadow");
}

static void
awn_effects_class_init(AwnEffectsClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class->set_property = awn_effects_set_property;
  obj_class->get_property = awn_effects_get_property;
  obj_class->notify = awn_effects_prop_changed;
  obj_class->finalize = awn_effects_finalize;
  obj_class->constructed = awn_effects_constructed;

  klass->animations = g_ptr_array_sized_new(40); // 5 animations per bundle, 8 effect bundles

  awn_effects_register_effect_bundle(klass,
    bounce_opening_effect,
    fade_out_effect,
    bounce_effect,
    bounce_effect,
    bounce_effect
  );
  awn_effects_register_effect_bundle(klass,
    zoom_opening_effect,
    zoom_closing_effect,
    fading_effect,
    fading_effect,
    fading_effect
  );
  awn_effects_register_effect_bundle(klass,
    spotlight_opening_effect2,
    spotlight_closing_effect,
    spotlight_effect,
    spotlight_half_fade_effect,
    spotlight_half_fade_effect
  );
  awn_effects_register_effect_bundle(klass,
    zoom_opening_effect,
    zoom_closing_effect,
    zoom_effect,
    zoom_attention_effect,
    zoom_attention_effect
  );
  awn_effects_register_effect_bundle(klass,
    bounce_squish_opening_effect,
    bounce_squish_closing_effect,
    bounce_squish_effect,
    bounce_squish_effect,
    bounce_squish_attention_effect
  );
  awn_effects_register_effect_bundle(klass,
    turn_opening_effect,
    turn_closing_effect,
    turn_hover_effect,
    turn_hover_effect,
    turn_hover_effect
  );
  awn_effects_register_effect_bundle(klass,
    spotlight3D_opening_effect,
    spotlight3D_closing_effect,
    spotlight3D_hover_effect,
    spotlight_half_fade_effect,
    spotlight3D_hover_effect
  );
  awn_effects_register_effect_bundle(klass,
    glow_opening_effect,
    glow_closing_effect,
    glow_effect,
    glow_attention_effect,
    glow_attention_effect
  );

  g_object_class_install_property(
    obj_class, PROP_ORIENTATION,
    g_param_spec_int("orientation",
                     "Orientation",
                     "Icon orientation",
                     0, 3, 3, // keep in sync with AwnOrientation
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_CURRENT_EFFECTS,
    g_param_spec_uint("effects",
                      "Current effects",
                      "Active effects set for this instance",
                      0, G_MAXUINT, 4, // set to classic (bouncing)
                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_ICON_OFFSET,
    g_param_spec_int("icon-offset",
                     "Icon offset",
                     "Offset of drawn icon to window border",
                     G_MININT, G_MAXINT, 0,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_ICON_ALPHA,
    g_param_spec_float("icon-alpha",
                       "Icon alpha",
                       "Alpha value of drawn icon",
                       0.0, 1.0, 1.0,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_REFLECTION_OFFSET,
    g_param_spec_int("reflection-offset",
                     "Reflection offset",
                     "Offset of drawn reflection to icon",
                     G_MININT, G_MAXINT, 0,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_REFLECTION_ALPHA,
    g_param_spec_float("reflection-alpha",
                       "Reflection alpha",
                       "Alpha value of drawn reflection",
                       0.0, 1.0, 0.25,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_REFLECTION_VISIBLE,
    g_param_spec_boolean("reflection-visible",
                         "Reflection visibility",
                         "Determines whether reflection is visible",
                         TRUE,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_MAKE_SHADOW,
    g_param_spec_boolean("make-shadow",
                         "Create shadow",
                         "Determines whether shadow is drawn around icon",
                         FALSE,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_IS_ACTIVE,
    g_param_spec_boolean("active",
                         "Active",
                         "Determines whether to draw active hint around icon",
                         FALSE,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_BORDER_CLIP,
    g_param_spec_int("border-clip",
                     "Active",
                     "Clips border of the icon",
                     0, G_MAXINT, 0,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_LABEL,
    g_param_spec_string("label",
                        "Label",
                        "Extra label drawn on top of icon",
                        "",
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
awn_effects_init(AwnEffects * fx)
{
  // the entire structure is zeroed in allocation, define only non-zero vars
  fx->icon_width = 48;
  fx->icon_height = 48;

  fx->alpha = 1.0;
  fx->saturation = 1.0;

  fx->op_list = g_malloc(sizeof(OP_LIST));
  memcpy(fx->op_list, OP_LIST, sizeof(OP_LIST));
}

AwnEffects* awn_effects_new_for_widget(GtkWidget * widget)
{
  g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);

  AwnEffects *fx = g_object_new(AWN_TYPE_EFFECTS, NULL);

  /*
   * we will use weak reference, because we want the widget 
   * to destroy us when it dies
   * though we could also ref the widget and unref it in our dispose
   */
  fx->self = widget;

  return fx;
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
  g_object_set(fx, "reflection-visible", FALSE, NULL);
}
void awn_effects_reflection_on(AwnEffects * fx)
{
  g_object_set(fx, "reflection-visible", TRUE, NULL);
}
void awn_effects_set_reflection_visible(AwnEffects * fx, gboolean value)
{
  g_object_set(fx, "reflection-visible", value, NULL);
}
void awn_effects_set_offset_cut(AwnEffects * fx, gboolean cut)
{
  g_object_set(fx, "border-clip", cut ? 4 : 0, NULL);
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

  // find the effect in queue

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
    // remove the effect from effect_queue and free the struct if the effect
    // is not in the middle of animation currently
    gboolean dispose = queue_item->this_effect != fx->current_effect;
    fx->effect_queue = g_list_remove(fx->effect_queue, queue_item);

    if (dispose)
    {
      g_free(queue_item);
    } else if (fx->sleeping_func) {
      // wake up sleeping effect
      fx->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
                                   fx->sleeping_func, queue_item);
      fx->sleeping_func = NULL;
    }
  }
}

static gpointer get_animation(AwnEffectsPrivate *topEffect, guint fxNum)
{
  switch (topEffect->this_effect)
  {
    case AWN_EFFECT_DESATURATE:
      return desaturate_effect;

    default:
      break;
  }

  GPtrArray *anims = AWN_EFFECTS_GET_CLASS(topEffect->effects)->animations;

  guint increment = topEffect->this_effect - 1;
  if (fxNum*AWN_ANIMATIONS_PER_BUNDLE + increment >= anims->len) {
    return NULL;
  }

  return g_ptr_array_index(anims, fxNum*AWN_ANIMATIONS_PER_BUNDLE + increment);
}

void
awn_effects_main_effect_loop(AwnEffects * fx)
{
  if (fx->current_effect != AWN_EFFECT_NONE || fx->effect_queue == NULL)
  {
    if (fx->sleeping_func && fx->effect_queue)
    {
      // check if sleeping effect is still on top
      AwnEffectsPrivate *queue_item = fx->effect_queue->data;
      if (fx->current_effect == queue_item->this_effect) return;

      // sleeping effect is not on top -> wake & play it

      // find correct effectsPrivate starting with the second item
      GList *queue = g_list_next(fx->effect_queue);
      while (queue)
      {
        queue_item = queue->data;

        if (queue_item->this_effect == fx->current_effect) break;

        queue = g_list_next(queue);
      }

      g_return_if_fail(queue_item);

      fx->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
                                   fx->sleeping_func, queue_item);
      fx->sleeping_func = NULL;
    }
    return;
  }

  AwnEffectsPrivate *topEffect =

    (AwnEffectsPrivate *)(fx->effect_queue->data);

  /* FIXME: simplifing the index to (topEffect->thisEffect-1) changed
   *  the gconf key a bit -> update awn-manager's custom effect setting
   *  to reflect this change.
   */

  gint i = topEffect->this_effect - 1;
  guint effect = fx->set_effects & (0xF << (i * 4));
  effect >>= i * 4;

  effect = fx->set_effects;

  // FIXME: do something with this init stuff (include in class' GPtrArray?)

  // spotlight initialization
  if (effect == EFFECT_SPOTLIGHT || effect == EFFECT_TURN_3D_SPOTLIGHT)
  {
    spotlight_init();
  }

  GSourceFunc animation = (GSourceFunc) get_animation(topEffect, effect);

  if (animation)
  {
    fx->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
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
apply_spotlight(AwnEffects * fx, cairo_t * cr)
{
  static cairo_t * unscaled_spot_ctx = NULL;
  static gint scaled_width = -1;
  static gint scaled_height = -1;
  static cairo_t * spot_ctx = NULL;
  cairo_surface_t * spot_srfc = NULL;

  // FIXME: for different orientation icon_offset should affect X axis
  //gint x1 = 0;
  gint y1 = fx->window_height - fx->icon_height - fx->icon_offset;

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
  // FIXME: get rid of settings
  /*
  if (fx->settings->bar_angle < 0)
  {
    int awn_bar_width = fx->settings->bar_width;
    double awn_curve_curviness = fx->settings->curviness;
    int awn_monitor_width = fx->settings->monitor_width;

    gint curvex = fx->self->allocation.x;

    if (curvex == 0)  // is applet?
    {
      gint curvex1 = 0;
      gdk_window_get_origin(fx->self->window, &curvex1, NULL);
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
  {*/
    fx->curve_offset = 0;
  //}
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
    /*
    cairo_rectangle(cr, 0, fx->window_height - fx->settings->bar_height + (fx->settings->bar_height / 2) - 4, fx->window_width, fx->settings->bar_height);
    cairo_clip(cr);
    */
    x1 = (fx->window_width - width) / 2;
    y1 = fx->window_height - height - fx->icon_offset;

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
  GtkAllocation ds;
  gboolean icon_changed = FALSE;
  gint i;

  icon = cairo_get_target(icon_context);

  if (reflect_context)
  {
    reflect = cairo_get_target(reflect_context);
  }

  fx->icon_width = cairo_xlib_surface_get_width(icon);

  fx->icon_height = cairo_xlib_surface_get_height(icon);
  ds.width = fx->icon_width;
  ds.height = fx->icon_height;
  ds.x = (fx->window_width - ds.width) / 2;
  ds.y = fx->window_height - fx->icon_offset - ds.height - fx->top_offset;

  apply_awn_curves(fx); //hopefully I haven't broken awn-curves.

  ds.y -= fx->curve_offset;
  
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
  if (fx->delta_width <= -ds.width
      || fx->delta_height <= -ds.height)
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
  if (fx->make_shadow)
  {
		make_shadows(fx, cr, ds.x, ds.y, ds.width, ds.height);
  }
	
  //Update our displayed Icon.
  cairo_set_source_surface(cr, cairo_get_target(fx->icon_ctx), ds.x, ds.y);

  cairo_paint_with_alpha(cr, fx->alpha * fx->icon_alpha);

  //------------------------------------------------------------------------
  /* reflection */
  
  if (fx->do_reflection)
  {
    if (fx->top_offset >= 0)
    {
      ds.y += ds.height + fx->top_offset * 2;
      ds.y -= (fx->refl_offset > 30)? 30 : fx->refl_offset;

      if (icon_changed || !reflect)
      {
        cairo_matrix_t matrix;
        cairo_matrix_init(&matrix,
                          1,
                          0,
                          0,
                          -1,
                          (ds.width / 2.0)*(1 - (1)),
                          (ds.height / 2.0)*(1 - (-1))
                         );
        cairo_save(fx->reflect_ctx);
        cairo_transform(fx->reflect_ctx, &matrix);
        cairo_set_source_surface(fx->reflect_ctx, cairo_get_target(fx->icon_ctx), 
                                 0, 0);
        cairo_paint(fx->reflect_ctx);

        cairo_set_operator(cr,CAIRO_OPERATOR_DEST_OVER);
        cairo_set_source_surface(cr, cairo_get_target(fx->reflect_ctx), 
                                 ds.x, ds.y);
        cairo_paint_with_alpha(cr, fx->alpha / 4);
        cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
        cairo_restore(fx->reflect_ctx);
      }
      else
      {
        cairo_set_source_surface(cr, reflect, ds.x, ds.y);
        cairo_paint_with_alpha(cr, fx->alpha * fx->refl_alpha);
      }
    }

    // FIXME: get rid of settings
    /* 4px offset for 3D look for reflection
    if (fx->do_offset_cut)
    {
      cairo_save(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba(cr, 1, 1, 1, 0);
      cairo_rectangle(cr, 0, ((fx->settings->bar_height * 2) - 4) +
                      fx->settings->icon_offset, fx->window_width, 4);
      cairo_fill(cr);
      cairo_restore(cr);
    }
    */
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

  //awn_effects_draw_icons_cairo(fx, cr, icon_context, NULL);

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
awn_effects_draw_set_icon_size(AwnEffects * fx, const gint width, const gint height, gboolean requestSize)
{
  fx->icon_width = width;
  fx->icon_height = height;
  if (requestSize && fx->self->allocation.width != width*5/4)
  {
    // AwnTitle position depends on our height requisition!
    // FIXME: come up with something better other than "height*2+4", it's here
    //  only because AwnAppletSimple currently calculates it like this and we
    //  want to have AwnTitle on the same Y-pos
    // FIXME: add icon_offset to height
    gtk_widget_set_size_request(fx->self, width*5/4, height*2+4);
  }
}

void
awn_effects_draw_set_window_size(AwnEffects * fx, const gint width,
                         const gint height)
{
  fx->window_width = width;
  fx->window_height = height;
}
#if 0
static gdouble
calc_curve_position(gdouble cx, gdouble a, gdouble b)  // pos, width, height
{
  return a <= 0 ? 0 : sin(cx / a * M_PI) * b;
}
#endif

cairo_t *awn_effects_draw_cairo_create(AwnEffects *fx)
{
  // FIXME: we're misusing fx->icon/reflect_ctx, rename those two
  g_return_val_if_fail(fx->self, NULL);
  cairo_t *cr = gdk_cairo_create(fx->self->window);
  g_return_val_if_fail(cairo_status(cr) == CAIRO_STATUS_SUCCESS, NULL);
  fx->icon_ctx = cr;

  cairo_surface_t *targetSurface = cairo_get_target(cr);
  if (cairo_surface_get_type(targetSurface) == CAIRO_SURFACE_TYPE_XLIB) {
    fx->window_width = cairo_xlib_surface_get_width(targetSurface);
    fx->window_height = cairo_xlib_surface_get_height(targetSurface);
  } else {
    g_warning("AwnEffects: Drawing to non-xlib surface, unknown dimensions.");
  }

  // we'll give to user virtual context and later paint everything on real one
  targetSurface = cairo_surface_create_similar(targetSurface,
                                               CAIRO_CONTENT_COLOR_ALPHA,
                                               fx->window_width,
                                               fx->window_height
                                              );
  g_return_val_if_fail(cairo_surface_status(targetSurface) == CAIRO_STATUS_SUCCESS, NULL);
  cr = cairo_create(targetSurface);
  fx->reflect_ctx = cr;

  // FIXME: make GtkAllocation AwnEffects member, so it's accessible in both
  //  pre and post ops (can be then used for optimizations)
  //  Then the param should be also removed from all ops functions.
  GtkAllocation ds;
  ds.width = fx->icon_width;
  ds.height = fx->icon_height;
  ds.x = (fx->window_width - ds.width) / 2;
  ds.y = (fx->window_height - ds.height); // sit on bottom by default

  // put actual transformations here
  // FIXME: put the functions in some kind of list/array
  awn_effects_pre_op_clear(fx, cr, &ds, NULL);
  awn_effects_pre_op_translate(fx, cr, &ds, NULL);
  awn_effects_pre_op_clip(fx, cr, &ds, NULL);
  awn_effects_pre_op_scale(fx, cr, &ds, NULL);
  awn_effects_pre_op_rotate(fx, cr, &ds, NULL);
  awn_effects_pre_op_flip(fx, cr, &ds, NULL);

  return cr;
}

cairo_t *awn_effects_draw_get_window_context(AwnEffects *fx)
{
  return fx->icon_ctx;
}

void awn_effects_draw_clear_window_context(AwnEffects *fx)
{
  if (fx->icon_ctx) {
    awn_effects_pre_op_clear(fx, fx->icon_ctx, NULL, NULL);
  }
}
void awn_effects_draw_cairo_destroy(AwnEffects *fx)
{
  cairo_t *cr = fx->reflect_ctx;

  cairo_reset_clip(cr);
  cairo_identity_matrix(cr);

  // put surface operations here
  // FIXME: put the functions in some kind of list/array
  awn_effects_post_op_clip(fx, cr, NULL, NULL);
  awn_effects_post_op_depth(fx, cr, NULL, NULL);
  awn_effects_post_op_shadow(fx, cr, NULL, NULL);
  awn_effects_post_op_saturate(fx, cr, NULL, NULL);
  awn_effects_post_op_glow(fx, cr, NULL, NULL);
  awn_effects_post_op_alpha(fx, cr, NULL, NULL);
  awn_effects_post_op_reflection(fx, cr, NULL, NULL);
  awn_effects_post_op_spotlight(fx, cr, NULL, NULL);

  cairo_set_source_surface(fx->icon_ctx, cairo_get_target(cr), 0, 0);
  cairo_paint(fx->icon_ctx);

  cairo_surface_destroy(cairo_get_target(cr));
  cairo_destroy(fx->icon_ctx);
  cairo_destroy(fx->reflect_ctx);

  fx->icon_ctx = NULL;
  fx->reflect_ctx = NULL;
}

