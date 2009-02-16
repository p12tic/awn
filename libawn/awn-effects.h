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

#ifndef _LIBAWN_AWN_EFFECTS_H
#define _LIBAWN_AWN_EFFECTS_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-defines.h"
#include "awn-config-client.h"

G_BEGIN_DECLS

typedef enum
{
  AWN_EFFECT_NONE = 0,
  AWN_EFFECT_OPENING = 1,
  AWN_EFFECT_CLOSING = 2,
  AWN_EFFECT_HOVER = 3,
  AWN_EFFECT_LAUNCHING = 4,
  AWN_EFFECT_ATTENTION = 5,
  AWN_EFFECT_DESATURATE
} AwnEffect;

/* GObject stuff */
#define AWN_TYPE_EFFECTS awn_effects_get_type()

#define AWN_EFFECTS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_EFFECTS, AwnEffects))

#define AWN_EFFECTS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_EFFECTS, AwnEffectsClass))

#define AWN_IS_EFFECTS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_EFFECTS))

#define AWN_IS_EFFECTS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_EFFECTS))

#define AWN_EFFECTS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_EFFECTS, AwnEffectsClass))

/**
 * AwnEffects:
 *
 * Class containing all necessary variables for effects state for particular widget.
 */
typedef struct _AwnEffects AwnEffects;
typedef struct _AwnEffectsClass AwnEffectsClass;
typedef struct _AwnEffectsPrivate AwnEffectsPrivate;

typedef gboolean(* AwnEffectsOpfn )(AwnEffects * fx,
                                     GtkAllocation * alloc,
                                     gpointer user_data);

typedef struct
{
  AwnEffectsOpfn      fn;
  gpointer            data;
} AwnEffectsOp;

struct _AwnEffects
{
  GObject parent;

  /* Properties */
  GtkWidget *widget; /* FIXME: add as property */
  gboolean no_clear;
  gboolean indirect_paint;
  gint orientation;
  guint set_effects;
  gint icon_offset;
  gint refl_offset;
  gfloat icon_alpha;
  gfloat refl_alpha;
  gboolean do_reflection;
  gboolean make_shadow;
  gboolean is_active;
  gboolean show_arrow;
  gchar *label;
  gfloat progress;
  gint border_clip;
  GQuark spotlight_icon;
  GQuark custom_active_icon;
  GQuark custom_arrow_icon;
  /* properties end */

  cairo_t * window_ctx;
  cairo_t * virtual_ctx;

  AwnEffectsPrivate *priv;
};

struct _AwnEffectsClass {
  GObjectClass parent_class;

  void (*animation_start) (AwnEffects *fx, AwnEffect effect);
  void (*animation_end) (AwnEffects *fx, AwnEffect effect);

  GPtrArray *animations;
  GData     *custom_icons;
};

GType awn_effects_get_type(void);

/**
 * awn_effects_new_for_widget:
 * @widget: Managed widget, computing window width and height is based on it and
 * it is also passed to gtk_widget_queue_draw() during the animations.
 *
 * Creates new #AwnEffects instance.
 */
AwnEffects* awn_effects_new_for_widget(GtkWidget * widget);

/**
 * awn_effects_start:
 * @fx: Pointer to #AwnEffects instance.
 * @effect: #AwnEffect to schedule.
 *
 * Start a single effect. The effect will loop until awn_effect_stop()
 * is called.
 */
void awn_effects_start(AwnEffects * fx, const AwnEffect effect);

/**
 * awn_effects_stop:
 * @fx: Pointer to #AwnEffects instance.
 * @effect: #AwnEffect to stop.
 *
 * Stop a single effect.
 */
void awn_effects_stop(AwnEffects * fx, const AwnEffect effect);

/**
 * awn_effects_start_ex:
 * @fx: Pointer to #AwnEffects instance.
 * @effect: Effect to schedule.
 * @max_loops: Number of maximum animation loops (0 for unlimited).
 * @signal_start: Determines whether the animation should emit "animation-start"
 *   signal when it starts.
 * @signal_stop: Determines whether the animation should emit "animation-end"
 *   signal when it finishes.
 *
 * Extended effect start, which provides a way to specify maximum number of loops
 * and possibility to emit signals for animation start & end.
 */
void
awn_effects_start_ex(AwnEffects * fx, const AwnEffect effect, gint max_loops,
                     gboolean signal_start, gboolean signal_end);

/**
 * awn_effects_set_icon_size:
 * @fx: Pointer to #AwnEffects instance.
 * @width: Width of drawn icon.
 * @height: Height of drawn icon.
 * @requestSize: Set to true to call gtk_widget_set_size_request
 *   for the managed widget.
 *
 * Sets up correct offsets in managed window based on dimensions of drawn icon.
 */
void awn_effects_set_icon_size(AwnEffects *fx, gint width, gint height,
                               gboolean requestSize);

/**
 * awn_effects_cairo_create:
 * @fx: Pointer to #AwnEffects instance.
 *
 * Returns cairo context where an icon can be drawn. (the icon should have 
 * dimensions specified by a previous call to #awn_effects_set_icon_size)
 */
cairo_t *awn_effects_cairo_create(AwnEffects *fx);

/**
 * awn_effects_cairo_create_clipped:
 * @fx: Pointer to #AwnEffects instance.
 * @region: A region the drawing will be clipped to.
 *
 * Returns cairo context where an icon can be drawn. (the icon should have
 * dimensions specified by a previous call to #awn_effects_set_icon_size)
 */
cairo_t *awn_effects_cairo_create_clipped(AwnEffects *fx,
                                               GdkRegion *region);

/**
 * awn_effects_cairo_destroy:
 * @fx: Pointer to #AwnEffects instance.
 *
 * Finish drawing of the icon and run all post-ops.
 */
void awn_effects_cairo_destroy(AwnEffects *fx);

/* Move this somewhere else eventually, these are used only internally */
void awn_effects_redraw(AwnEffects *fx);
void awn_effects_main_effect_loop(AwnEffects * fx);
void awn_effects_emit_anim_start(AwnEffects *fx, AwnEffect effect);
void awn_effects_emit_anim_end(AwnEffects *fx, AwnEffect effect);

G_END_DECLS

#endif
