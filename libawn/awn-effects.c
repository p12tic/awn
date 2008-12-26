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
#include "awn-effects-ops-new.h"

#include "awn-config-client.h"
#include "awn-config-bridge.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <cairo/cairo-xlib.h>

#include "anims/awn-effects-shared.h"
#include "anims/awn-effect-spotlight.h"
#include "anims/awn-effect-bounce.h"
#include "anims/awn-effect-glow.h"
#include "anims/awn-effect-zoom.h"
#include "anims/awn-effect-fade.h"
#include "anims/awn-effect-squish.h"
#include "anims/awn-effect-turn.h"
#include "anims/awn-effect-spotlight3d.h"
#include "anims/awn-effect-desaturate.h"

G_DEFINE_TYPE(AwnEffects, awn_effects, G_TYPE_OBJECT);

#define AWN_EFFECTS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_EFFECTS, \
  AwnEffectsPrivate))

#ifndef M_PI
#define  M_PI 3.14159265358979323846
#endif
#define  RADIANS_PER_DEGREE  0.0174532925

// FIXME: add property for fps
#define AWN_FRAME_RATE(fx) (40)
#define AWN_ANIMATIONS_PER_BUNDLE 5

typedef gboolean (*_AwnAnimation)(AwnEffectsAnimation*);

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
  PROP_PROGRESS,
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

static void
awn_effects_finalize(GObject *object)
{
  AwnEffects * fx = AWN_EFFECTS(object);
  // destroy animation timer
  if (fx->priv->timer_id)
  {
    GSource *s = g_main_context_find_source_by_id(NULL, fx->priv->timer_id);
    if (s) g_source_destroy(s);
  }

  // free effect queue and associated AwnEffectsPriv
  GList *queue = fx->priv->effect_queue;
  while (queue)
  {
    g_free(queue->data);
    queue->data = NULL;
    queue = g_list_next(queue);
  }

  if (fx->priv->effect_queue)
    g_list_free(fx->priv->effect_queue);
  fx->priv->effect_queue = NULL;

  fx->priv->self = NULL;

  if (fx->label)
    g_free(fx->label);
  fx->label = NULL;

  G_OBJECT_CLASS (awn_effects_parent_class)->finalize(object);
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
      g_value_set_string(value, fx->label);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean(value, fx->is_active);
      break;
    case PROP_PROGRESS:
      g_value_set_float(value, fx->progress);
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
      // make sure we set correct orient
      switch (g_value_get_int(value)) {
        case AWN_EFFECT_ORIENT_TOP:
        case AWN_EFFECT_ORIENT_RIGHT:
        case AWN_EFFECT_ORIENT_BOTTOM:
        case AWN_EFFECT_ORIENT_LEFT:
          fx->orientation = g_value_get_int(value);
          break;
        default:
          fx->orientation = AWN_EFFECT_ORIENT_BOTTOM;
          break;
      }
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
    case PROP_PROGRESS:
      fx->progress = g_value_get_float(value);
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
  if (fx->priv->self && GTK_WIDGET_DRAWABLE(fx->priv->self)) {
    gtk_widget_queue_draw(fx->priv->self);
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
  // FIXME: move this to AwnIcon, AwnEffects are more flexible without it
  AwnConfigClient *client;
  AwnConfigBridge *bridge = awn_config_bridge_get_default ();

  client = awn_config_client_new ();

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
                          "effects", "show_shadows",
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

  g_type_class_add_private (obj_class, sizeof (AwnEffectsPrivate));

  klass->animations = g_ptr_array_sized_new(AWN_ANIMATIONS_PER_BUNDLE * 8); // 5 animations per bundle, 8 effect bundles

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
    // keep in sync with AwnOrientation
    g_param_spec_int("orientation",
                     "Orientation",
                     "Icon orientation",
                     0, 3, AWN_EFFECT_ORIENT_BOTTOM,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_CURRENT_EFFECTS,
    g_param_spec_uint("effects",
                      "Current effects",
                      "Active effects set for this instance",
                      0, G_MAXUINT, 0, // set to classic (bouncing)
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
                        "Extra label drawn on top of the icon",
                        NULL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property(
    obj_class, PROP_PROGRESS,
    g_param_spec_float("progress",
                       "Progress",
                       "Value displayed on extra progress pie"
                       " drawn on the icon",
                       0.0, 1.0, 1.0,
                       G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
awn_effects_init(AwnEffects * fx)
{
  fx->priv = AWN_EFFECTS_GET_PRIVATE(fx);
  // the entire structure is zeroed in allocation, define only non-zero vars
  //  which are not properties

  fx->priv->icon_width = 48;
  fx->priv->icon_height = 48;

  fx->priv->alpha = 1.0;
  fx->priv->saturation = 1.0;
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
  fx->priv->self = widget;

  return fx;
}

// we don't really need to expose this enum
typedef enum
{
  AWN_EFFECT_PRIORITY_HIGHEST,
  AWN_EFFECT_PRIORITY_HIGH,
  AWN_EFFECT_PRIORITY_ABOVE_NORMAL,
  AWN_EFFECT_PRIORITY_NORMAL,
  AWN_EFFECT_PRIORITY_BELOW_NORMAL,
  AWN_EFFECT_PRIORITY_LOW,
  AWN_EFFECT_PRIORITY_LOWEST
} AwnEffectPriority;

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

static gint
awn_effects_sort(gconstpointer a, gconstpointer b)
{
  const AwnEffectsAnimation *data1 = (AwnEffectsAnimation *) a;
  const AwnEffectsAnimation *data2 = (AwnEffectsAnimation *) b;
  return (gint)(awn_effects_get_priority(data1->this_effect) - 
    awn_effects_get_priority(data2->this_effect));
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
  if (effect == AWN_EFFECT_NONE || fx->priv->self == NULL)
  {
    return;
  }

  AwnEffectsAnimation *queue_item;

  GList *queue = fx->priv->effect_queue;

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

  AwnEffectsAnimation *anim = g_new(AwnEffectsAnimation, 1);

  anim->effects = fx;
  anim->this_effect = effect;
  anim->max_loops = max_loops;
  anim->start = start;
  anim->stop = stop;

  fx->priv->effect_queue = g_list_insert_sorted(fx->priv->effect_queue, anim,
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

  AwnEffectsAnimation *queue_item;

  GList *queue = fx->priv->effect_queue;

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
    gboolean dispose = queue_item->this_effect != fx->priv->current_effect;
    fx->priv->effect_queue = g_list_remove(fx->priv->effect_queue, queue_item);

    if (dispose)
    {
      g_free(queue_item);
    } 
    else if (fx->priv->sleeping_func)
    {
      // wake up sleeping effect
      fx->priv->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
                                         fx->priv->sleeping_func, queue_item);
      fx->priv->sleeping_func = NULL;
    }
  }
}

static gpointer get_animation(AwnEffectsAnimation *topEffect, guint fxNum)
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
  if (fx->priv->current_effect != AWN_EFFECT_NONE
      || fx->priv->effect_queue == NULL)
  {
    if (fx->priv->sleeping_func && fx->priv->effect_queue)
    {
      // check if sleeping effect is still on top
      AwnEffectsAnimation *queue_item = fx->priv->effect_queue->data;
      if (fx->priv->current_effect == queue_item->this_effect) return;

      // sleeping effect is not on top -> wake & play it

      // find correct effectsPrivate starting with the second item
      GList *queue = g_list_next(fx->priv->effect_queue);
      while (queue)
      {
        queue_item = queue->data;

        if (queue_item->this_effect == fx->priv->current_effect) break;

        queue = g_list_next(queue);
      }

      g_return_if_fail(queue_item);

      fx->priv->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
                                         fx->priv->sleeping_func, queue_item);
      fx->priv->sleeping_func = NULL;
    }
    return;
  }

  AwnEffectsAnimation *topEffect =

    (AwnEffectsAnimation *)(fx->priv->effect_queue->data);

  /* FIXME: simplifing the index to (topEffect->thisEffect-1) changed
   *  the gconf key a bit -> update awn-manager's custom effect setting
   *  to reflect this change.
   */

  gint i = topEffect->this_effect - 1;
  guint effect = fx->set_effects & (0xF << (i * 4));
  effect >>= i * 4;

  // FIXME: do something with this init stuff (include in class' GPtrArray?)
  
  // spotlight initialization
  if (effect == EFFECT_SPOTLIGHT || effect == EFFECT_TURN_3D_SPOTLIGHT)
  {
    spotlight_init();
  }

  GSourceFunc animation = (GSourceFunc) get_animation(topEffect, effect);

  if (animation)
  {
    fx->priv->timer_id = g_timeout_add(AWN_FRAME_RATE(fx),
                                       animation, topEffect);
    fx->priv->current_effect = topEffect->this_effect;
    fx->priv->effect_lock = FALSE;
  }
  else
  {
    if (topEffect->start)
    {
      topEffect->start(fx->priv->self);
    }

    if (topEffect->stop)
    {
      topEffect->stop(fx->priv->self);
    }

    // dispose AwnEffectsAnimation
    awn_effects_stop(fx, topEffect->this_effect);
  }
}

void
awn_effects_draw_set_icon_size(AwnEffects * fx, const gint width, const gint height, gboolean requestSize)
{
  fx->priv->icon_width = width;
  fx->priv->icon_height = height;
  if (requestSize && fx->priv->self)
  {
    // this should be only used without AwnIcon,
    // AwnIcon handles size_requests well enough
    switch (fx->orientation) {
      case AWN_EFFECT_ORIENT_TOP:
      case AWN_EFFECT_ORIENT_BOTTOM:
        if (fx->priv->self->allocation.width < width*6/5)
          gtk_widget_set_size_request(fx->priv->self, width*6/5, height);
        break;
      case AWN_EFFECT_ORIENT_RIGHT:
      case AWN_EFFECT_ORIENT_LEFT:
        if (fx->priv->self->allocation.height < height*6/5)
          gtk_widget_set_size_request(fx->priv->self, width, height*6/5);
        break;
    }
  }
}

cairo_t *awn_effects_draw_cairo_create(AwnEffects *fx)
{
  g_return_val_if_fail(fx->priv->self, NULL);
  cairo_t *cr = gdk_cairo_create(fx->priv->self->window);
  g_return_val_if_fail(cairo_status(cr) == CAIRO_STATUS_SUCCESS, NULL);
  fx->window_ctx = cr;

  cairo_surface_t *targetSurface = cairo_get_target(cr);
  if (cairo_surface_get_type(targetSurface) == CAIRO_SURFACE_TYPE_XLIB) {
    fx->priv->window_width = cairo_xlib_surface_get_width(targetSurface);
    fx->priv->window_height = cairo_xlib_surface_get_height(targetSurface);
  } else {
    g_warning("AwnEffects: Drawing to non-xlib surface, unknown dimensions.");
  }

  // we'll give to user virtual context and later paint everything on real one
  targetSurface = cairo_surface_create_similar(targetSurface,
                                               CAIRO_CONTENT_COLOR_ALPHA,
                                               fx->priv->window_width,
                                               fx->priv->window_height
                                              );
  g_return_val_if_fail(cairo_surface_status(targetSurface) == CAIRO_STATUS_SUCCESS, NULL);
  cr = cairo_create(targetSurface);
  fx->virtual_ctx = cr;

  // FIXME: make GtkAllocation AwnEffects member, so it's accessible in both
  //  pre and post ops (can be then used for optimizations)
  //  Then the param should be also removed from all ops functions.
  GtkAllocation ds;
  ds.width = fx->priv->icon_width;
  ds.height = fx->priv->icon_height;
  ds.x = (fx->priv->window_width - ds.width) / 2;
  ds.y = (fx->priv->window_height - ds.height); // sit on bottom by default

  // put actual transformations here
  // FIXME: put the functions in some kind of list/array
  awn_effects_pre_op_clear(fx, cr, &ds, NULL);
  awn_effects_pre_op_translate(fx, cr, &ds, NULL);
  awn_effects_pre_op_clip(fx, cr, &ds, NULL);
  awn_effects_pre_op_scale(fx, cr, &ds, NULL);
  awn_effects_pre_op_rotate(fx, cr, &ds, NULL);
  awn_effects_pre_op_flip(fx, cr, &ds, NULL);
  awn_effects_pre_op_active (fx, cr, &ds, NULL);

  return cr;
}

cairo_t *awn_effects_draw_get_window_context(AwnEffects *fx)
{
  return fx->window_ctx;
}

void awn_effects_draw_clear_window_context(AwnEffects *fx)
{
  if (fx->window_ctx) {
    awn_effects_pre_op_clear(fx, fx->window_ctx, NULL, NULL);
  }
}

void awn_effects_draw_cairo_destroy(AwnEffects *fx)
{
  cairo_t *cr = fx->virtual_ctx;

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
  awn_effects_post_op_progress(fx, cr, NULL, NULL);
  // TODO: we're missing ops to paint label & active hint

  cairo_set_source_surface(fx->window_ctx, cairo_get_target(cr), 0, 0);
  cairo_paint(fx->window_ctx);

  cairo_surface_destroy(cairo_get_target(cr));
  cairo_destroy(fx->window_ctx);
  cairo_destroy(fx->virtual_ctx);

  fx->window_ctx = NULL;
  fx->virtual_ctx = NULL;
}

