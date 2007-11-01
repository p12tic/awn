/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
*/

/*! \file awn-effects.h */

#ifndef __AWN_EFFECTS_H__
#define __AWN_EFFECTS_H__

#include <gtk/gtk.h>

#include "awn-defines.h"
#include "awn-gconf.h"
#include "awn-title.h"

G_BEGIN_DECLS

typedef enum {
	AWN_EFFECT_NONE,
	AWN_EFFECT_OPENING,
	AWN_EFFECT_LAUNCHING,
	AWN_EFFECT_HOVER,
	AWN_EFFECT_ATTENTION,
	AWN_EFFECT_CLOSING,
	AWN_EFFECT_DESATURATE
} AwnEffect;

typedef enum {
	AWN_EFFECT_DIR_NONE,
	AWN_EFFECT_DIR_STOP,
	AWN_EFFECT_DIR_DOWN,
	AWN_EFFECT_DIR_UP,
	AWN_EFFECT_DIR_LEFT,
	AWN_EFFECT_DIR_RIGHT,
	AWN_EFFECT_SQUISH_DOWN,
	AWN_EFFECT_SQUISH_DOWN2,
	AWN_EFFECT_SQUISH_UP,
	AWN_EFFECT_SQUISH_UP2,
	AWN_EFFECT_TURN_1,
	AWN_EFFECT_TURN_2,
	AWN_EFFECT_TURN_3,
	AWN_EFFECT_TURN_4,
	AWN_EFFECT_SPOTLIGHT_ON,
	AWN_EFFECT_SPOTLIGHT_TREMBLE_UP,
	AWN_EFFECT_SPOTLIGHT_TREMBLE_DOWN,
	AWN_EFFECT_SPOTLIGHT_OFF
} AwnEffectSequence;

typedef const gchar* (*AwnTitleCallback)(GObject*);
typedef void (*AwnEventNotify)(GObject*);

typedef struct _AwnEffects AwnEffects;

struct _AwnEffects
{
	GObject *self;
	GtkWidget *focus_window;
	AwnSettings *settings;
	AwnTitle *title;
	AwnTitleCallback get_title;
	GList *effect_queue;

	gint icon_width, icon_height;
	gint window_width, window_height;

	/* EFFECT VARIABLES */
	gboolean effect_lock;
	AwnEffect current_effect;
	AwnEffectSequence direction;
	gint count;

	gdouble x_offset;
	gdouble y_offset;

	gint delta_width;
	gint delta_height;

	GtkAllocation clip_region;

	gdouble rotate_degrees;
	gfloat alpha;
	gfloat spotlight_alpha;
	gfloat saturation;
	gfloat glow_amount;

	gint icon_depth;
	gint icon_depth_direction;

	/* State variables */
	gboolean hover;
	gboolean clip;
	gboolean flip;
	gboolean spotlight;

	guint enter_notify;
	guint leave_notify;
	guint timer_id;

	/* padding so we dont break ABI compability every time */
	void *pad1;
	void *pad2;
	void *pad3;
	void *pad4;
};

//! Initializes AwnEffects structure.
/*!
 * \param obj Object which will be passed to all callback functions, this object is also passed to gtk_widget_queue_draw() during the animation.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_effects_init(GObject *obj, AwnEffects *fx);

//! Finalizes AwnEffects usage and frees internally allocated memory. (also calls awn_unregister_effects())
/*!
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_effects_finalize(AwnEffects *fx);

//! Registers enter-notify and leave-notify events for managed window.
/*!
 * \param obj Managed window to which the effects will apply.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_register_effects (GObject *obj, AwnEffects *fx);

//! Unregisters events for managed window.
/*!
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_unregister_effects (AwnEffects *fx);

//! Start a single effect. The effect will loop until awn_effect_stop
//! is called.
/*!
 * \param effect Effect to schedule.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_effect_start(AwnEffects *fx, const AwnEffect effect);

//! Stop a single effect.
/*!
 * \param effect Effect to stop.
 * \param fx Pointer to AwnEffects structure.
 */

void
awn_effect_stop(AwnEffects *fx, const AwnEffect effect);

//! Makes AwnTitle appear on event-notify.
/*!
 * \param fx Pointer to AwnEffects structure.
 * \param title Pointer to AwnTitle instance.
 * \param title_func Pointer to function which returns desired title text.
 */
void
awn_effects_set_title(AwnEffects *fx, AwnTitle *title, AwnTitleCallback title_func);

//! Extended effect start, which provides callbacks for animation start, end and possibility to specify maximum number of loops.
/*!
 * \param fx Pointer to AwnEffects structure.
 * \param effect Effect to schedule.
 * \param start Function which will be called when animation starts.
 * \param stop Function which will be called when animation finishes.
 * \param max_loops Number of maximum animation loops (0 for unlimited).
 */
void
awn_effect_start_ex(AwnEffects *fx, const AwnEffect effect, AwnEventNotify start, AwnEventNotify stop, gint max_loops);

void awn_draw_background(AwnEffects*, cairo_t*);
void awn_draw_icons(AwnEffects*, cairo_t*, GdkPixbuf*, GdkPixbuf*);
void awn_draw_foreground(AwnEffects*, cairo_t*);
void awn_draw_set_window_size(AwnEffects*, const gint, const gint);
void awn_draw_set_icon_size(AwnEffects*, const gint, const gint);

G_END_DECLS

#endif
